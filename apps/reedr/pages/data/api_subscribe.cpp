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
#include <feed_parser.hpp>
#include "api_handler.hpp"
#include "json.hpp"
#include <user_info.hpp>

namespace FastCGI { namespace app { namespace api
{
	struct JsonError
	{
		std::string error;
		const char* url;
	};

	struct SubscribeAnswer
	{
		long long feed = 0;
		long long folder = 0;
		int position = 0;
	};

	struct MultipleAnswers
	{
		Discoveries links;
	};
}}}

namespace json
{
	JSON_RULE(FastCGI::app::api::JsonError)
	{
		JSON_ADD(error);
		JSON_ADD(url);
	}

	JSON_RULE(FastCGI::app::api::SubscribeAnswer)
	{
		JSON_ADD(feed);
		JSON_ADD(folder);
		JSON_ADD(position);
	}

	JSON_RULE(FastCGI::app::Discovery)
	{
		JSON_ADD(href);
		JSON_ADD(title);
		JSON_ADD(comment);
	}

	JSON_RULE(FastCGI::app::api::MultipleAnswers)
	{
		JSON_ADD(links);
	}
};

namespace FastCGI { namespace app { namespace api
{
	template <typename Container, typename Pred>
	void for_each(Request& request, const Container& cont, const std::string& join, Pred pred)
	{
		bool first = true;
		for (auto&& item: cont)
		{
			if (first) first = false;
			else request << join;
			pred(request, cont);
		}
	}

	class Subscribe: public APIOperation
	{
	public:
		void render(SessionPtr session, Request& request, PageTranslation& tr) override
		{
#if 1
			auto user = userInfo(session);

			SubscribeAnswer answer;
			JsonError err;
			MultipleAnswers links;
			param_t url = request.getVariable("url");
			if (!url || !*url) err.error = tr(lng::LNG_URL_MISSING);
			else
			{
				db::ConnectionPtr db = request.dbConn();
				answer.feed = user->subscribe(db, url, links.links, false);
				if (answer.feed < 0)
				{
					switch(answer.feed)
					{
					case SERR_INTERNAL_ERROR:  request.on500(std::string("Could not subscribe to ") + url);
					case SERR_4xx_ANSWER:      err.error = tr(lng::LNG_FEED_FAILED); break;
					case SERR_5xx_ANSWER:      err.error = tr(lng::LNG_FEED_SERVER_FAILED); break;
					case SERR_OTHER_ANSWER:    err.error = tr(lng::LNG_FEED_ERROR); break;
					case SERR_NOT_A_FEED:      err.error = tr(lng::LNG_NOT_A_FEED); break;
					case SERR_DISCOVERY_EMPTY: err.error = tr(lng::LNG_NO_FEEDS_ON_PAGE); break;
					}
				}
			}

			if (err.error.empty() && answer.feed != SERR_DISCOVERY_MULTIPLE)
			{
				db::ConnectionPtr db = request.dbConn();
				auto select = db->prepare("SELECT folder_id, ord FROM subscription WHERE feed_id=? AND folder_id IN (SELECT _id FROM folder WHERE user_id=?)");
				if (!select) request.on500(db->errorMessage());
				if (!select->bind(0, answer.feed)) request.on500(select->errorMessage());
				if (!select->bind(1, user->userId())) request.on500(select->errorMessage());

				auto c = select->query();
				if (!c || !c->next()) request.on500(select->errorMessage());

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

			if (answer.feed == SERR_DISCOVERY_MULTIPLE)
			{
				json::render(request, links);
				request.die();
			}

			json::render(request, answer);
#else
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

		const char** getVariables() const override
		{
			static const char* vars[] = { "url", nullptr };
			return vars;
		}
	};

}}}

REGISTER_OPERATION("subscribe", FastCGI::app::api::Subscribe);
