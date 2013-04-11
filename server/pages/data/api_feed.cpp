/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pch.h"
#include <handlers.hpp>
#include <utils.hpp>
#include <http.hpp>
#include <dbconn.hpp>
#include <stdlib.h>
#include "api_handler.hpp"
#include "json.hpp"

#define PAGE_SIZE 40

namespace FastCGI { namespace app { namespace api
{
	struct FeedError
	{
		std::string error;
		long long feed;
		FeedError(): feed(0) {}
	};

	struct FeedAnswer
	{
		long long feed;
		int page;
		int pageLength;
		const char* title;
		const char* site;
		const char* url;
		const char* author;
		const char* authorLink;
		tyme::time_t lastUpdate;
		db::CursorPtr entries;
		FeedAnswer(): feed(0), page(0), pageLength(PAGE_SIZE), lastUpdate(-1) {}
	};

}}}

namespace json
{
	struct Category: CursorJson<Category>
	{
		Category(const db::CursorPtr& c);
		bool entryIsScalar() const { return true; }
	};
	Category::Category(const db::CursorPtr& c): CursorJson<Category>(c)
	{
		JSON_CURSOR_TEXT(value, 0);
	};

	JSON_CURSOR_RULE(Enclosure)
	{
		JSON_CURSOR_TEXT(url, 0);
		JSON_CURSOR_TEXT(mime, 1);
		JSON_CURSOR_LL(length, 2);
	};

	JSON_CURSOR_RULE(FeedEntries)
	{
		JSON_CURSOR_LL(id, 0);
		JSON_CURSOR_TEXT(title, 1);
		JSON_CURSOR_TEXT(url, 2);
		JSON_CURSOR_TEXT(author, 3);
		JSON_CURSOR_TEXT(authorLink, 4);
		JSON_CURSOR_TIME(date, 5);
		JSON_CURSOR_TEXT(description, 6);
		JSON_CURSOR_TEXT(contents, 7);
		JSON_CURSOR_SUBQUERY(categories, Category, "SELECT cat FROM categories WHERE entry_id=?");
		JSON_CURSOR_SUBQUERY(enclosures, Enclosure, "SELECT url, mime, length FROM enclosure WHERE entry_id=?");
	}

	JSON_RULE(FastCGI::app::api::FeedError)
	{
		JSON_ADD(error);
		JSON_ADD(feed);
	}

	JSON_RULE(FastCGI::app::api::FeedAnswer)
	{
		JSON_ADD(feed);
		JSON_ADD(page);
		JSON_ADD(pageLength);
		JSON_ADD(title);
		JSON_ADD(site);
		JSON_ADD(url);
		JSON_ADD(author);
		JSON_ADD(authorLink);
		JSON_ADD_CURSOR(entries, FeedEntries);
	}
};

namespace db
{
	CURSOR_RULE(FastCGI::app::api::FeedAnswer)
	{
		CURSOR_ADD(0, title);
		CURSOR_ADD(1, site);
		CURSOR_ADD(2, url);
		CURSOR_ADD(3, author);
		CURSOR_ADD(4, authorLink);
		CURSOR_TIME(5, lastUpdate);
	}
};

namespace FastCGI { namespace app { namespace api
{
	template <typename Container, typename Pred>
	void for_each(Request& request, const Container& cont, const std::string& join, Pred pred)
	{
		bool first = true;
		auto from = cont.begin(), to = cont.end();
		for (; from != to; ++from)
		{
			if (first) first = false;
			else request << join;
			pred(request, *from);
		}
	}

	class Feed: public APIOperation
	{
	public:
		void render(SessionPtr session, Request& request, PageTranslation& tr)
		{
			db::StatementPtr header;
			db::StatementPtr entries;
			db::CursorPtr headerCursor;

			FeedAnswer answer;
			FeedError err;
			param_t sfeed = request.getVariable("feed");
			param_t spage = request.getVariable("page");

			if (!sfeed) err.error = tr(lng::LNG_FEED_MISSING);
			else
			{
				char* _err = nullptr;
				answer.feed = asctoll(sfeed, &_err);
				if (_err && *_err)
					err.error = tr(lng::LNG_FEED_UNKNOWN);

				err.feed = answer.feed;
			}

			if (spage)
			{
				char* _err = nullptr;
				answer.page = (int)asctoll(spage, &_err);
				if (_err && *_err)
					answer.page = 0;
			}

			if (err.error.empty())
			{
				db::ConnectionPtr db = request.dbConn();
				db::Transaction transaction(db);
				if (!transaction.begin())
					request.on500();

				header = db->prepare("SELECT title, site, feed AS url, author, authorLink, last_update FROM feed WHERE _id=?");
				if (!header || !header->bind(0, answer.feed))
				{
					FLOG << (header ? header->errorMessage() : db->errorMessage());
					request.on500();
				}

				headerCursor = header->query();
				if (!headerCursor || !headerCursor->next())
				{
					FLOG << header->errorMessage();
					err.error = tr(lng::LNG_FEED_UNKNOWN);
				}
				else
				{
					db::get(headerCursor, answer);
					request.onLastModified(answer.lastUpdate);

					entries = db->prepare("SELECT _id, title, url, author, authorLink, date, description, contents FROM entry WHERE feed_id=? ORDER BY date DESC", answer.page * answer.pageLength, (answer.page + 1) * answer.pageLength);
					if (!entries || !entries->bind(0, answer.feed))
					{
						FLOG << (entries ? entries->errorMessage() : db->errorMessage());
						request.on500();
					}
					answer.entries = entries->query();
					if (!answer.entries)
					{
						FLOG << entries->errorMessage();
						err.error = tr(lng::LNG_FEED_UNKNOWN);
					}
				}
			}

			request.setHeader("Content-Type", "application/json; charset=utf-8");

			if (!err.error.empty())
			{
				request.setHeader("Status", "400 Bad Request");
				json::render(request, err);
				request.die();
			}

			json::render(request, answer);
#if 0
			param_t url = request.getVariable("url");
			if (!url) err.error = tr(lng::LNG_URL_MISSING);
			else
			{
				db::ConnectionPtr db = request.dbConn();
				answer.feed = session->subscribe(db, url);
				if (answer.feed < 0)
				{
					switch(answer.feed)
					{
					case SERR_INTERNAL_ERROR: request.on500();
					case SERR_4xx_ANSWER:   err.error = tr(lng::LNG_FEED_FAILED); break;
					case SERR_5xx_ANSWER:   err.error = tr(lng::LNG_FEED_SERVER_FAILED); break;
					case SERR_OTHER_ANSWER: err.error = tr(lng::LNG_FEED_ERROR); break;
					case SERR_NOT_A_FEED:   err.error = tr(lng::LNG_NOT_A_FEED); break;
					}
				}
			}

			if (err.error.empty())
			{
				db::ConnectionPtr db = request.dbConn();
				auto select = db->prepare("SELECT folder_id, ord FROM subscription WHERE feed_id=? AND folder_id IN (SELECT _id FROM folder WHERE user_id=?)");
				if (!select) request.on500();
				if (!select->bind(0, answer.feed)) request.on500();
				if (!select->bind(1, session->getId())) request.on500();

				auto c = select->query();
				if (!c || !c->next()) request.on500();

				answer.folder = c->getLongLong(0);
				answer.position = c->getLong(1);
			}

			request.setHeader("Content-Type", "application/json; charset=utf-8");

			if (!err.error.empty())
			{
				err.url = url;
				request.setHeader("Status", "400 Bad Request");
				json::render(request, err);
				request.die();
			}

			json::render(request, answer);

			auto xhr = http::XmlHttpRequest::Create();
			if (!xhr)
				request.on500();

			request.setHeader("Content-Type", "application/json; charset=utf-8");

			std::string error;
			param_t url = request.getVariable("url");
			if (!url) error = tr(lng::LNG_URL_MISSING);

			dom::XmlDocumentPtr doc;

			if (url)
			{
				xhr->open(http::HTTP_GET, url, false);
				xhr->send();

				int issue = xhr->getStatus() / 100;
				if (issue == 4)      error = tr(lng::LNG_FEED_FAILED);
				else if (issue == 5) error = tr(lng::LNG_FEED_SERVER_FAILED);
				else if (issue != 2) error = tr(lng::LNG_FEED_ERROR);

				if (error.empty())
					doc = xhr->getResponseXml();
			}

			feed::Feed feed;
			if (error.empty() && (!doc || !feed::parse(doc, feed)))
				error = tr(lng::LNG_NOT_A_FEED);

			if (!error.empty())
			{
				request.setHeader("Status", "400 Bad Request");
				request << "{\"error\":" << escape(error); if (url) request << ",\"url\":" << escape(url); request << "}";
				request.die();
			}

			request <<
				"{\"id\":123456789"
				<< ",\"title\":" << escape(feed.m_feed.m_title)
				<< ",\"feed\":" << escape(url)
				<< ",\"site\":" << escape(feed.m_feed.m_url)
				<< ",\"description\":" << escape(feed.m_description)
				<< ",\"categories\":["
				;
			for_each(request, feed.m_categories, ",", [](Request& request, const std::string& cat) { request << escape(cat); });

			request << "],\"entries\":[";
			for_each(request, feed.m_entry, ",", [](Request& request, const feed::Entry& entry)
			{
				request <<
					"{\"id\":123456789"
					<< ",\"title\":" << escape(entry.m_entry.m_title)
					<< ",\"link\":" << escape(entry.m_entry.m_url)
					<< ",\"author\":" << escape(entry.m_author.m_name)
					<< ",\"authorLink\":" << escape("mailto:" + entry.m_author.m_email)
					<< ",\"description\":" << escape(entry.m_description)
					<< ",\"date\":" << entry.m_dateTime
					<< ",\"enclosures\":[";
				for_each(request, entry.m_enclosures, ",", [](Request& request, const feed::Enclosure& enclosure)
				{
					request << "{\"url\":" << escape(enclosure.m_url) << ",\"mime\":" << escape(enclosure.m_type) << ",\"length\":" << enclosure.m_size << "}";
				});
				request
					<< "],\"categories\":[";
				for_each(request, entry.m_categories, ",", [](Request& request, const std::string& cat) { request << escape(cat); });
				request << "],\"content\":" << escape(entry.m_content) << "}";
			});
			request << "]}";
#endif
		}

		const char** getVariables() const
		{
			static const char* vars[] = { "feed", "page", nullptr };
			return vars;
		}
	};

}}}

REGISTER_OPERATION("feed", FastCGI::app::api::Feed);
