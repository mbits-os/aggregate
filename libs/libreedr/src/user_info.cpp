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
#include <user_info.hpp>
#include <fast_cgi/application.hpp>
#include <fast_cgi/session.hpp>
#include <dbconn.hpp>
#include <crypt.hpp>
#include <dom.hpp>
#include <http.hpp>
#include <feed_parser.hpp>
#include <uri.hpp>

#define UNREAD_COUNT 10

namespace FastCGI
{
	namespace app
	{
		FastCGI::UserInfoPtr UserInfoFactory::fromId(const db::ConnectionPtr& db, long long id)
		{
			static const char* SQL_READ_USER =
				"SELECT login, root_folder, is_admin, prefs "
				"FROM user "
				"WHERE _id=?";

			auto user = db->prepare(SQL_READ_USER);
			if (user && user->bind(0, id))
			{
				auto c = user->query();
				if (c && c->next())
				{
					std::string login = c->getText(0);
					auto rootFolder = c->getLongLong(1);
					auto isAdmin = c->getInt(2) != 0;
					uint32_t prefs = c->getInt(3);

					return std::make_shared<app::UserInfo>(
						id, login, rootFolder, isAdmin, prefs
						);
				}
			}
			return nullptr;
		}

		FastCGI::UserInfoPtr UserInfoFactory::fromLogin(const db::ConnectionPtr& db, const std::string& login)
		{
			static const char* SQL_READ_USER =
				"SELECT _id, root_folder, is_admin, prefs "
				"FROM user "
				"WHERE login=?";

			auto user = db->prepare(SQL_READ_USER);
			if (user && user->bind(0, login))
			{
				auto c = user->query();
				if (c && c->next())
				{
					auto _id = c->getLongLong(0);
					auto rootFolder = c->getLongLong(1);
					auto isAdmin = c->getInt(2) != 0;
					uint32_t prefs = c->getInt(3);

					return std::make_shared<app::UserInfo>(
						_id, login, rootFolder, isAdmin, prefs
						);
				}
			}
			return nullptr;
		}

		void UserInfo::storeFlags(const db::ConnectionPtr& db)
		{
			db::StatementPtr query = db->prepare("UPDATE user SET prefs=? WHERE _id=?");
			if (query && query->bind(0, (int)m_flags) && query->bind(1, m_userId))
				query->execute();
		}

		long long UserInfo::createFolder(const db::ConnectionPtr& db, const char* name, long long parent)
		{
			return SERR_INTERNAL_ERROR;
		}

		namespace discovery
		{
			enum class FEED {
				SITE_ATOM,
				ATOM,
				CLASS,
				SITE_RSS = CLASS,
				RSS,
			};

			struct Link
			{
				int fetch_result = 0;
				std::string attr_title;
				std::string comment;
				std::string url;
				feed::Feed feed;
				FEED type = FEED::RSS;
			};

			std::string htmlTitle(const dom::XmlDocumentPtr& doc)
			{
				putc('#', stdout); fflush(stdout);
				std::string title;
				auto titles = doc->getElementsByTagName("title");
				if (titles && titles->length())
				{
					auto e = titles->element(titles->length() - 1);
					title = std::trim(e->innerText());
				}

				return title;
			}

			int getFeedSimple(const http::XmlHttpRequestPtr& xhr, const std::string& url, feed::Feed& feed)
			{
				xhr->open(http::HTTP_GET, url, false);
				xhr->send();

				int status = xhr->getStatus() / 100;
				if (status == 4)      return SERR_4xx_ANSWER;
				else if (status == 5) return SERR_5xx_ANSWER;
				else if (status != 2) return SERR_OTHER_ANSWER;

				auto doc = xhr->getResponseXml();
				if (!doc || !feed::parse(doc, feed))
					return SERR_NOT_A_FEED;

				if (xhr->wasRedirected())
					feed.m_self = xhr->getFinalLocation();
				else
					feed.m_self = url;
				feed.m_etag = xhr->getResponseHeader("Etag");
				feed.m_lastModified = xhr->getResponseHeader("Last-Modified");
				return 0;
			}

			void makeURI(std::string& uri, const std::string& server, const filesystem::path& base)
			{
				if (std::tolower(uri.substr(0, 7)) != "http://" &&
					std::tolower(uri.substr(0, 8)) != "https://")
				{
					auto path = filesystem::canonical(uri, base);
					if (path.has_root_name())
						path = path.string().substr(path.root_name().string().length());
					uri = server + path.string();
				}
			}

			bool feedType(const dom::XmlElementPtr& e, FEED& feed)
			{
				if (!e) return false;
				auto rel = e->getAttributeNode("rel");
				if (!rel) return false;

				bool alternate = false;
				bool home = false;
				auto node = std::tolower(rel->nodeValue());
				if (node == "alternate")
					alternate = true;
				else if (node == "alternate home")
					home = alternate = true;

				if (!alternate)
					return false;

				std::string type = e->getAttribute("type");
				std::tolower(type);

				if (type != "text/xml" && type != "application/rss+xml" && type != "application/atom+xml")
					return false;

				feed = (type == "application/atom+xml") ? (home ? FEED::SITE_ATOM : FEED::ATOM) : (home ? FEED::SITE_RSS : FEED::RSS);
				return true;
			}

			std::vector<Link> feedLinks(const dom::XmlDocumentPtr& doc, const Uri& base)
			{
				std::vector<Link> out;
				std::string pageTitle = htmlTitle(doc);

				auto links = doc->getElementsByTagName("link");
				if (!links)
					return out;

				size_t count = links->length();
				for (size_t i = 0; i < count; ++i)
				{
					auto e = links->element(i);
					FEED type;
					if (!feedType(e, type))
						continue;

					Link link;
					link.attr_title = e->getAttribute("title");
					link.url = Uri::canonical(e->getAttribute("href"), base).string();
					link.type = type;

					if (link.attr_title.empty())
						link.attr_title = pageTitle;

					out.push_back(link);
				}

				std::stable_sort(out.begin(), out.end(), [](const Link& lhs, const Link& rhs){
					return lhs.type < rhs.type;
				});
				return out;
			}

			void fixTitles(std::vector<Link>& links)
			{
				size_t count = links.size();
				for (size_t i = 0; i < count; ++i)
				{
					Link& link = links[i];
					if (link.feed.m_feed.m_title.empty()) continue;

					bool atLeastOneChanged = false;
					for (size_t j = i + 1; j < count; ++j)
					{
						Link& check = links[j];
						if (link.feed.m_feed.m_title == check.feed.m_feed.m_title)
						{
							atLeastOneChanged = true;
							if (!check.attr_title.empty())
								check.comment = check.attr_title;
						}
					}

					if (atLeastOneChanged)
					{
						if (!link.attr_title.empty())
							link.comment = link.attr_title;
					}
				}

#if 0
				, lng::Translation& tr
				for (auto&& link : links)
				{
					if (link.feed.empty())
						link.feed = link.title;

					if (link.feed.empty())
						link.feed = "(no title)";
				}
#endif
			}

			std::vector<Link> reduce(const std::vector<Link>& list)
			{
				std::vector<Link> links;

				for (auto&& link : list)
				{
					if (link.fetch_result) continue;

					// if there are two links with the same feed title
					// and the type is different, prefer SITE_ATOM to SITE_RSS
					// and ATOM to RSS
					if (link.feed.m_feed.m_title.empty())
					{
						links.push_back(link);
						continue;
					}

					bool found = false;
					for (auto&& check : links)
					{
						// this will add an RSS link, even if there is already SITE_ATOM with the same title,
						// but will not add SITE_RSS in this situation, nor will it add the RSS link if there
						// already is an ATOM one.
						if (check.feed.m_feed.m_title == link.feed.m_feed.m_title && // same feed title
							check.type < link.type && // and the added link has higher type
							((int)link.type - (int)check.type) == (int)FEED::CLASS) // and the types are exactly class apart
						{
							found = true; // don't add this link
							break;
						}
					}

					if (found) continue;
					links.emplace_back(std::move(link));
				}

				return links;
			}

			int htmlDiscover(const http::XmlHttpRequestPtr& xhr, const std::string& url, feed::Feed& feed, Discoveries& links)
			{
				auto doc = xhr->getResponseXml();
				if (!doc)
					doc = xhr->getResponseHtml();
				if (!doc)
					return SERR_NOT_A_FEED;

				std::string uri = url;
				if (xhr->wasRedirected())
					uri = xhr->getFinalLocation();

				auto discovered = feedLinks(doc, Uri::make_base(uri));
				if (discovered.empty())
					return SERR_DISCOVERY_EMPTY;

				for (auto&& link : discovered)
					link.fetch_result = getFeedSimple(xhr, link.url, link.feed);

				if (discovered.size() == 1 && discovered.front().fetch_result)
					return discovered.front().fetch_result;

				discovered = reduce(discovered);
				fixTitles(discovered);

				if (discovered.empty())
					return SERR_DISCOVERY_EMPTY;

				if (discovered.size() == 1)
				{
					feed = std::move(discovered.front().feed);
					return 0;
				}

				links.reserve(discovered.size());

				for (auto&& link : discovered)
					links.push_back({ link.url, link.feed.m_feed.m_title, link.comment });

				return SERR_DISCOVERY_MULTIPLE;
			}
		}

		int getFeed(const std::string& url, feed::Feed& feed, Discoveries& links, bool strict)
		{
			auto xhr = http::XmlHttpRequest::Create();
			if (!xhr)
				return SERR_INTERNAL_ERROR;

			int ret = discovery::getFeedSimple(xhr, url, feed);
			if (strict || ret != SERR_NOT_A_FEED)
				return ret;

			return discovery::htmlDiscover(xhr, url, feed, links);
		}

		inline static const char* nullifier(const std::string& s)
		{
			return s.empty() ? nullptr : s.c_str();
		}

		inline static bool createEntry(const db::ConnectionPtr& db, long long feed_id, const feed::Entry& entry)
		{
			const char* entryUniqueId = nullifier(entry.m_entryUniqueId);
			const char* title = nullifier(entry.m_entry.m_title);

			if (!entryUniqueId)
				return false;

			if (!title)
				title = "";

			auto del = db->prepare("DELETE FROM entry WHERE guid=?");

			if (!del || !del->bind(0, entryUniqueId))
			{
				FLOG << del->errorMessage();
				return false;
			}

			if (!del->execute())
			{
				FLOG << del->errorMessage();
				return false;
			}

			auto insert = db->prepare("INSERT INTO entry (feed_id, guid, title, url, date, author, authorLink, description, contents) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
			if (!insert)
				return false;

			tyme::time_t date = entry.m_dateTime == -1 ? tyme::now() : entry.m_dateTime;

			if (!insert->bind(0, feed_id)) return false;
			if (!insert->bind(1, entryUniqueId)) return false;
			if (!insert->bind(2, nullifier(entry.m_entry.m_title))) return false;
			if (!insert->bind(3, nullifier(entry.m_entry.m_url))) return false;
			if (!insert->bindTime(4, date)) return false;
			if (!insert->bind(5, nullifier(entry.m_author.m_name))) return false;
			if (!insert->bind(6, nullifier(entry.m_author.m_email))) return false;
			if (!insert->bind(7, nullifier(entry.m_description))) return false;
			if (!insert->bind(8, nullifier(entry.m_content))) return false;

			if (!insert->execute()) return false;

			if (entry.m_categories.empty() && entry.m_enclosures.empty())
				return true;

			auto guid = db->prepare("SELECT _id FROM entry WHERE guid=?");
			if (!guid || !guid->bind(0, entryUniqueId)) return false;

			auto c = guid->query();
			if (!c || !c->next()) return false;
			long long entry_id = c->getLongLong(0);

			insert = db->prepare("INSERT INTO categories(entry_id, cat) VALUES (?, ?)");
			if (!insert)
				return false;
			if (!insert->bind(0, entry_id)) return false;
			auto cat_it = std::find_if(entry.m_categories.begin(), entry.m_categories.end(), [&insert](const std::string& cat) -> bool
			{
				if (!insert->bind(1, cat.c_str())) { FLOG << insert->errorMessage(); return true; }
				if (!insert->execute()) { FLOG << insert->errorMessage(); return true; }
				return false;
			});
			if (cat_it != entry.m_categories.end()) return false;

			insert = db->prepare("INSERT INTO enclosure(entry_id, url, mime, length) VALUES (?, ?, ?, ?)");
			if (!insert)
				return false;
			if (!insert->bind(0, entry_id)) return false;
			auto enc_it = std::find_if(entry.m_enclosures.begin(), entry.m_enclosures.end(), [&insert](const feed::Enclosure& enc) -> bool
			{
				if (!insert->bind(1, enc.m_url.c_str())) { FLOG << insert->errorMessage(); return true; }
				if (!insert->bind(2, enc.m_type.c_str())) { FLOG << insert->errorMessage(); return true; }
				if (!insert->bind(3, (long long)enc.m_size)) { FLOG << insert->errorMessage(); return true; }
				if (!insert->execute()) { FLOG << insert->errorMessage(); return true; }
				return false;
			});
			if (enc_it != entry.m_enclosures.end()) return false;

			return true;
		}

		inline static bool createEntries(const db::ConnectionPtr& db, long long feed_id, const feed::Entries& entries)
		{
			auto it = std::find_if(entries.rbegin(), entries.rend(), [&](const feed::Entry& entry) -> bool
			{
				return !createEntry(db, feed_id, entry);
			});
			return it == entries.rend();
		}

		inline static bool createFeed(const db::ConnectionPtr& db, const feed::Feed& feed)
		{
			tyme::time_t last_updated = tyme::now();
			auto insert = db->prepare("INSERT INTO feed (title, site, feed, last_update, author, authorLink, etag, last_modified) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
			if (!insert)
				return false;

			if (!insert->bind(0, nullifier(feed.m_feed.m_title))) return false;
			if (!insert->bind(1, nullifier(feed.m_feed.m_url))) return false;
			if (!insert->bind(2, nullifier(feed.m_self))) return false;
			if (!insert->bindTime(3, last_updated)) return false;
			if (!insert->bind(4, nullifier(feed.m_author.m_name))) return false;
			if (!insert->bind(5, nullifier(feed.m_author.m_email))) return false;
			if (!insert->bind(6, nullifier(feed.m_etag))) return false;
			if (!insert->bind(7, nullifier(feed.m_lastModified))) return false;

			return insert->execute();
		}

		static inline int alreadySubscribes(const db::ConnectionPtr& db, long long user_id, long long feed_id)
		{
			auto stmt = db->prepare("SELECT count(*) FROM subscription WHERE feed_id=? AND folder_id IN (SELECT _id FROM folder WHERE user_id=?)");
			if (!stmt) { FLOG << db->errorMessage(); return SERR_INTERNAL_ERROR; }
			if (!stmt->bind(0, feed_id)) { FLOG << stmt->errorMessage(); return SERR_INTERNAL_ERROR; }
			if (!stmt->bind(1, user_id)) { FLOG << stmt->errorMessage(); return SERR_INTERNAL_ERROR; }
			auto c = stmt->query();
			if (!c || !c->next()) { FLOG << stmt->errorMessage(); return SERR_INTERNAL_ERROR; }
			return c->getInt(0);
		}

		static inline long long getRootFolder(const db::ConnectionPtr& db, long long user_id)
		{
			auto select = db->prepare("SELECT root_folder FROM user WHERE _id=?");
			if (!select || !select->bind(0, user_id)) return SERR_INTERNAL_ERROR;
			auto c = select->query();
			if (!c || !c->next()) { FLOG << select->errorMessage(); return SERR_INTERNAL_ERROR; }
			return c->getLongLong(0);
		}

		static inline bool doSubscribe(const db::ConnectionPtr& db, long long feed_id, long long folder_id)
		{
			auto max_id = db->prepare("SELECT max(ord) FROM subscription WHERE folder_id=?");
			if (!max_id || !max_id->bind(0, folder_id))
			{
				FLOG << (max_id ? max_id->errorMessage() : db->errorMessage());
				return false;
			}

			long ord = 0;
			auto c = max_id->query();
			if (c && c->next())
			{
				if (!c->isNull(0))
					ord = c->getLong(0) + 1;
			}

			auto subscription = db->prepare("INSERT INTO subscription (feed_id, folder_id, ord) VALUES (?, ?, ?)");
			if (!subscription) { FLOG << subscription->errorMessage(); return false; }
			if (!subscription->bind(0, feed_id)) { FLOG << subscription->errorMessage(); return false; }
			if (!subscription->bind(1, folder_id)) { FLOG << subscription->errorMessage(); return false; }
			if (!subscription->bind(2, ord)) { FLOG << subscription->errorMessage(); return false; }
			if (!subscription->execute()) { FLOG << subscription->errorMessage(); return false; }
			return true;
		}

		static inline bool unreadLatest(const db::ConnectionPtr& db, long long feed_id, long long user_id, int unread_count)
		{
			auto unread = db->prepare("SELECT _id FROM entry WHERE feed_id=? ORDER BY date DESC", 0, unread_count);
			if (!unread || !unread->bind(0, feed_id))
			{
				FLOG << (unread ? unread->errorMessage() : db->errorMessage());
				return false;
			}

			auto state = db->prepare("INSERT INTO state (user_id, type, entry_id) VALUES (?, ?, ?)");
			if (!state)
			{
				FLOG << db->errorMessage();
				return false;
			}
			if (!state->bind(0, user_id)) { FLOG << state->errorMessage(); return false; }
			if (!state->bind(1, ENTRY_UNREAD)) { FLOG << state->errorMessage(); return false; }

			auto c = unread->query();
			if (!c) { FLOG << db->errorMessage(); return false; }
			while (c->next())
			{
				if (!state->bind(2, c->getLongLong(0))) { FLOG << state->errorMessage(); return false; }
				if (!state->execute()) { FLOG << state->errorMessage(); return false; }
			}

			return true;
		}

		long long UserInfo::subscribe(const db::ConnectionPtr& db, const std::string& url, Discoveries& links, bool strict, long long folder)
		{
			int unread_count = UNREAD_COUNT;
			long long feed_id = 0;

			db::Transaction transaction(db);
			if (!transaction.begin()) { FLOG << db->errorMessage(); return SERR_INTERNAL_ERROR; }

			auto feed_query = db->prepare("SELECT _id FROM feed WHERE feed.feed=?");
			if (!feed_query || !feed_query->bind(0, url)) return SERR_INTERNAL_ERROR;

			auto c = feed_query->query();
			if (!c)
				return SERR_INTERNAL_ERROR;

			bool already_subscribed = true;

			if (!c->next())
			{
				feed::Feed feed;
				int result = getFeed(url, feed, links, strict);
				if (result)
					return result;

				unread_count = feed.m_entry.size();
				if (!feed_query->bind(0, feed.m_self))
					return SERR_INTERNAL_ERROR;

				c = feed_query->query();
				if (!c)
					return SERR_INTERNAL_ERROR;

				if (!c->next())
				{
					already_subscribed = false;

					if (!createFeed(db, feed))
						return SERR_INTERNAL_ERROR;
					c = feed_query->query();
					if (!c || !c->next())
						return SERR_INTERNAL_ERROR;
					feed_id = c->getLongLong(0);
					if (!createEntries(db, feed_id, feed.m_entry))
						return SERR_INTERNAL_ERROR;
				}
			}

			if (already_subscribed)
			{
				feed_id = c->getLongLong(0);
				//could it be double subcription?
				int result = alreadySubscribes(db, m_userId, feed_id);
				if (result < 0)
					return result; //an error
				if (result > 0) //already subscribes
					return feed_id;
			}

			if (folder == 0)
			{
				folder = getRootFolder(db, m_userId);
				if (folder < 0)
					return folder;
			}

			if (!doSubscribe(db, feed_id, folder)) return SERR_INTERNAL_ERROR;
			if (!unreadLatest(db, feed_id, m_userId, unread_count)) return SERR_INTERNAL_ERROR;

			if (!transaction.commit()) { FLOG << db->errorMessage(); return SERR_INTERNAL_ERROR; }

			return feed_id;
		}
	}
}
