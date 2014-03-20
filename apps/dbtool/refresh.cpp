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

#include <iostream>
#include <map>
#include <vector>

#include <dbconn.hpp>
#include <feed_parser.hpp>
#include <http.hpp>
#include <utils.hpp>

namespace Refresh
{
	inline static const char* nullifier(const std::string& s)
	{
		return s.empty() ? nullptr : s.c_str();
	}

	struct Stats
	{
		unsigned long long knownEntries, newEntries, changedEntries;
		Stats(): knownEntries(0), newEntries(0), changedEntries(0) {}
	};

	std::string encodeWord(const std::string& in)
	{
		static const char hex[] = "0123456789ABCDEF";
		auto it = std::find_if(in.begin(), in.end(), [](char c) { return c < 0; });
		if (it == in.end()) return in;
		std::string out;
		out.reserve(in.size() * 3/2);
		out.append("=?UTF-8?Q?");
		for (auto c : in)
		{
			if (c == ' ')
				out.push_back('_');
			else if (c >= 0 && c != '_' && c != '=')
				out.push_back(c);
			else
			{
				out.push_back('=');
				out.push_back(hex[(((unsigned char)c)   >> 4) & 0x0F]);
				out.push_back(hex[( (unsigned char)c        ) & 0x0F]);
			}
		}
		out.append("?=");
		return out;
	}

	struct Feed
	{
		long long _id;
		std::string title;
		std::string feed;
		std::string etag;
		std::string lastModified;
		tyme::time_t lastUpdated;

		bool update(const db::ConnectionPtr& db, size_t counter, int digits, bool mimeEncodedWord) const
		{
			if (title.empty())
				printf("[%0*u] Fetching %s\n     %*c", digits, (unsigned long)counter, feed.c_str(), digits, ' ');
			else
				printf("[%0*u] Fetching %s <%s>\n     %*c", digits, (unsigned long)counter, 
					mimeEncodedWord ? encodeWord(title).c_str() : title.c_str(), feed.c_str(), digits, ' ');
			fflush(stdout);

			auto xhr = http::XmlHttpRequest::Create();
			if (!xhr)
			{
				fprintf(stderr, "refresh: out of memory\n");
				return false;
			}

			xhr->open(http::HTTP_GET, feed, false);
			if (!etag.empty())
				xhr->setRequestHeader("If-None-Match", etag);
			if (!lastModified.empty())
				xhr->setRequestHeader("If-Modified-Since", lastModified);
			xhr->send();

			auto status = xhr->getStatus();

#if 0
			//<debug>
			printf("%d (%s)\n", status, xhr->getStatusText().c_str());
			auto headers = xhr->getResponseHeaders();
			for (auto&& value: headers)
			{
				bool upcase = true;
				std::string head = value.first;
				std::transform(head.begin(), head.end(), head.begin(), [&upcase](char c) -> char
				{
					if (upcase)
					{
						upcase = false;
						return toupper(c);
					}
					if (c == '-')
						upcase = true;
					return c;
				});
				printf("%s: %s\n", head.c_str(), value.second.c_str());
			};
			printf("\n");
			//</debug>
#endif

			if (status / 100 != 2)
			{
				printf("%d (%s)\n", status, xhr->getStatusText().c_str());
				fflush(stdout);
				return true; // only internal errors should stop the refreshing
			}

			feed::Feed newFeed;
			dom::XmlDocumentPtr doc = xhr->getResponseXml();
			if (!doc || !feed::parse(doc, newFeed))
			{
				printf("Error while parsing the data [%d (%s)]\n", status, xhr->getStatusText().c_str());
				fflush(stdout);
				return true;
			}

			newFeed.m_self = feed;
			newFeed.m_etag = xhr->getResponseHeader("Etag");
			newFeed.m_lastModified = xhr->getResponseHeader("Last-Modified");

			return update(db, _id, newFeed);
		}

		static bool installCatsAndEncl(const db::ConnectionPtr& db, const char* entryUniqueId, const feed::Entry& entry)
		{
			if (entry.m_categories.empty() && entry.m_enclosures.empty())
				return true;

			auto guid = db->prepare("SELECT _id FROM entry WHERE guid=?");
			if (!guid || !guid->bind(0, entryUniqueId)) return false;

			auto c = guid->query();
			if (!c || !c->next()) return false;
			long long entry_id = c->getLongLong(0);

			auto insert = db->prepare("INSERT INTO categories(entry_id, cat) VALUES (?, ?)");
			if (!insert)
				return false;
			if (!insert->bind(0, entry_id)) return false;
			auto cat_it = std::find_if(entry.m_categories.begin(), entry.m_categories.end(), [&insert](const std::string& cat) -> bool
			{
				if (!insert->bind(1, cat.c_str())) return true;
				if (!insert->execute()) return true;
				return false;
			});
			if (cat_it != entry.m_categories.end()) return false;

			insert = db->prepare("INSERT INTO enclosure(entry_id, url, mime, length) VALUES (?, ?, ?, ?)");
			if (!insert)
				return false;
			if (!insert->bind(0, entry_id)) return false;
			auto enc_it = std::find_if(entry.m_enclosures.begin(), entry.m_enclosures.end(), [&insert](const feed::Enclosure& enc) -> bool
			{
				if (!insert->bind(1, enc.m_url.c_str())) return true;
				if (!insert->bind(2, enc.m_type.c_str())) return true;
				if (!insert->bind(3, (long long)enc.m_size)) return true;
				if (!insert->execute()) return true;
				return false;
			});
			if (enc_it != entry.m_enclosures.end()) return false;

			return true;
		}

		static bool createEntry(const db::ConnectionPtr& db, long long feed_id, const char* entryUniqueId, const feed::Entry& entry)
		{
			const char* title = nullifier(entry.m_entry.m_title);

			if (!title)
				title = "";

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

			return installCatsAndEncl(db, entryUniqueId, entry);
		}

		static bool updateEntry(const db::ConnectionPtr& db, long long feed_id, long long entry_id, const feed::Entry& entry)
		{
			const char* title = nullifier(entry.m_entry.m_title);

			if (!title)
				title = "";

			auto update = db->prepare("UPDATE entry SET feed_id=?, title=?, url=?, date=?, author=?, authorLink=?, description=?, contents=? WHERE _id=?");
			if (!update)
				return false;

			tyme::time_t date = entry.m_dateTime == -1 ? tyme::now() : entry.m_dateTime;

			if (!update->bind(0, feed_id)) return false;
			if (!update->bind(1, nullifier(entry.m_entry.m_title))) return false;
			if (!update->bind(2, nullifier(entry.m_entry.m_url))) return false;
			if (!update->bindTime(3, date)) return false;
			if (!update->bind(4, nullifier(entry.m_author.m_name))) return false;
			if (!update->bind(5, nullifier(entry.m_author.m_email))) return false;
			if (!update->bind(6, nullifier(entry.m_description))) return false;
			if (!update->bind(7, nullifier(entry.m_content))) return false;
			if (!update->bind(8, entry_id)) return false;

			if (!update->execute()) return false;

			auto del = db->prepare("DELETE FROM categories WHERE entry_id=?");
			if (!del || !del->bind(0, entry_id) || !del->execute())
			{
				const char* msg = del ? del->errorMessage() : db->errorMessage();
				if (msg && *msg)
					printf("DB error: %s\n", msg);
				return false;
			}

			del = db->prepare("DELETE FROM enclosure WHERE entry_id=?");
			if (!del || !del->bind(0, entry_id) || !del->execute())
			{
				const char* msg = del ? del->errorMessage() : db->errorMessage();
				if (msg && *msg)
					printf("DB error: %s\n", msg);
				return false;
			}

			return installCatsAndEncl(db, entry.m_entryUniqueId.c_str(), entry);
		}

		static bool updateOrCreateEntry(const db::ConnectionPtr& db, long long feed_id, const feed::Entry& entry, Stats& stats);
		static bool updateEntries(const db::ConnectionPtr& db, long long feed_id, const feed::Entries& entries, Stats& stats)
		{
			auto it = std::find_if(entries.rbegin(), entries.rend(), [&](const feed::Entry& entry) { return !updateOrCreateEntry(db, feed_id, entry, stats); });
			return it == entries.rend();
		}

		static bool updateFeed(const db::ConnectionPtr& db, long long feed_id, const feed::Feed& feed)
		{
			tyme::time_t last_updated = tyme::now();
			//UPDATE feed SET title="..." WHERE _id=13;
			auto update = db->prepare("UPDATE feed SET title=?, site=?, last_update=?, author=?, authorLink=?, etag=?, last_modified=? WHERE _id=?");
			if (!update)
				return false;

			if (!update->bind(0, nullifier(feed.m_feed.m_title))) return false;
			if (!update->bind(1, nullifier(feed.m_feed.m_url))) return false;
			if (!update->bindTime(2, last_updated)) return false;
			if (!update->bind(3, nullifier(feed.m_author.m_name))) return false;
			if (!update->bind(4, nullifier(feed.m_author.m_email))) return false;
			if (!update->bind(5, nullifier(feed.m_etag))) return false;
			if (!update->bind(6, nullifier(feed.m_lastModified))) return false;
			if (!update->bind(7, feed_id)) return false;

			return update->execute();
		}

		static bool update(const db::ConnectionPtr& db, long long feed_id, const feed::Feed& feed)
		{
			db::Transaction transaction(db);
			if (!transaction.begin())
				return false;

			Stats stats;

			if (!updateEntries(db, feed_id, feed.m_entry, stats)) return false;
			if (!updateFeed(db, feed_id, feed)) return false;
			fprintf(stderr, "\n      ");
			printf("entries: %llu/%llu/%llu\n", stats.knownEntries, stats.newEntries, stats.changedEntries);
			fflush(stdout);

			return transaction.commit();
		}
	};

	struct Category
	{
		std::string value;
		inline bool operator < (const Category& oth) const { return value < oth.value; }
	};

	struct Enclosure
	{
		std::string url;
		std::string mime;
		long long length;
		inline bool operator < (const Enclosure& oth) const
		{
			if (url != oth.url) return url < oth.url;
			if (mime != oth.mime) return mime < oth.mime;
			return length < oth.length;
		}
	};

	struct Entry
	{
		long long id;
		std::string title;
		std::string url;
		std::string author;
		std::string authorLink;
		tyme::time_t date;
		std::string description;
		std::string contents;
	};

	struct User
	{
		long long id;
	};
};

namespace db
{
	CURSOR_RULE(Refresh::Feed)
	{
		CURSOR_ADD(0, _id);
		CURSOR_ADD(1, title);
		CURSOR_ADD(2, feed);
		CURSOR_ADD(3, etag);
		CURSOR_ADD(4, lastModified);
		CURSOR_TIME(5, lastUpdated);
	};

	CURSOR_RULE(Refresh::Category)
	{
		CURSOR_ADD(0, value);
	}

	CURSOR_RULE(Refresh::Enclosure)
	{
		CURSOR_ADD(0, url);
		CURSOR_ADD(1, mime);
		CURSOR_ADD(2, length);
	}

	CURSOR_RULE(Refresh::Entry)
	{
		CURSOR_ADD(0, id);
		CURSOR_ADD(1, title);
		CURSOR_ADD(2, url);
		CURSOR_ADD(3, author);
		CURSOR_ADD(4, authorLink);
		CURSOR_TIME(5, date);
		CURSOR_ADD(6, description);
		CURSOR_ADD(7, contents);
		//JSON_CURSOR_SUBQUERY(enclosures, Enclosure, "");
	}

	CURSOR_RULE(Refresh::User)
	{
		CURSOR_ADD(0, id);
	}
}

namespace Refresh
{
	bool same(const db::ConnectionPtr& db, const feed::Entry& entry, const Entry& _entry)
	{
		if (entry.m_entry.m_title != _entry.title) return false;
		if (entry.m_entry.m_url != _entry.url) return false;
		//if (entry.m_dateTime != _entry.date && entry.m_dateTime != -1) return false;
		if (entry.m_author.m_name != _entry.author) return false;
		if (entry.m_author.m_email != _entry.authorLink) return false;
		if (entry.m_description != _entry.description) return false;
		if (entry.m_content != _entry.contents) return false;

		//compare categories
		{
			auto stmt = db->prepare("SELECT cat FROM categories WHERE entry_id=?");
			if (!stmt || !stmt->bind(0, _entry.id))
				return false;

			auto c = stmt->query();
			if (!c) return false;

			std::vector<Category> _cats;
			if (!db::get(c, _cats)) return false;

			if (_cats.size() != entry.m_categories.size()) return false;

			std::vector<std::string> cats(entry.m_categories.begin(), entry.m_categories.end());
			std::sort(_cats.begin(), _cats.end());
			std::sort(cats.begin(), cats.end());

			auto left = _cats.begin(), end = _cats.end();
			auto right = cats.begin();
			for (; left != end; ++left, ++right)
			{
				if (left->value != *right)
					return false;
			}
		}

		//compare enclosures
		{
			auto stmt = db->prepare("SELECT url, mime, length FROM enclosure WHERE entry_id=?");
			if (!stmt || !stmt->bind(0, _entry.id))
				return false;

			auto c = stmt->query();
			if (!c) return false;

			std::vector<Enclosure> _enclosures;
			if (!db::get(c, _enclosures)) return false;

			if (_enclosures.size() != entry.m_enclosures.size()) return false;

			std::vector<feed::Enclosure> enclosures(entry.m_enclosures.begin(), entry.m_enclosures.end());
			std::sort(_enclosures.begin(), _enclosures.end());
			std::sort(enclosures.begin(), enclosures.end(), [](const feed::Enclosure& left, const feed::Enclosure& right) -> bool
			{
				if (left.m_url != right.m_url) return left.m_url < right.m_url;
				if (left.m_type != right.m_type) return left.m_type < right.m_type;
				return left.m_size < right.m_size;
			});

			auto left = _enclosures.begin(), end = _enclosures.end();
			auto right = enclosures.begin();
			for (; left != end; ++left, ++right)
			{
				if (left->url != right->m_url) return false;
				if (left->mime != right->m_type) return false;
				if (left->length != right->m_size) return false;
			}
		}

		return true;
	}

	bool markAsUnread(const db::ConnectionPtr& db, long long feed_id, long long entry_id)
	{
		auto stmt = db->prepare("SELECT user_id FROM subscription LEFT JOIN folder ON (folder_id = folder._id) WHERE feed_id=?");
		if (!stmt || !stmt->bind(0, feed_id)) return false;

		auto c = stmt->query();
		std::list<User> users;
		if (!c || !db::get(c, users)) return false;
		c.reset();
		stmt.reset();

		auto del = db->prepare("DELETE FROM state WHERE user_id=? AND entry_id=? AND type=0");
		if (!del || !del->bind(1, entry_id)) return false;
		auto ins = db->prepare("INSERT INTO state (user_id, entry_id, type) VALUES(?, ?, 0)");
		if (!ins || !ins->bind(1, entry_id)) return false;

		auto it = std::find_if(users.begin(), users.end(), [&del, &ins](const User& u) -> bool
		{
			if (!del->bind(0, u.id)) return true;
			if (!ins->bind(0, u.id)) return true;
			if (!del->execute()) return true;
			if (!ins->execute()) return true;
			return false;
		});

		return true;
	}

	bool markAsUnread(const db::ConnectionPtr& db, long long feed_id, const char* entryUniqueId)
	{
		auto guid = db->prepare("SELECT _id FROM entry WHERE guid=?");
		if (!guid || !guid->bind(0, entryUniqueId)) return false;

		auto c = guid->query();
		if (!c || !c->next()) return false;
		long long entry_id = c->getLongLong(0);
		c.reset();
		guid.reset();

		return markAsUnread(db, feed_id, entry_id);
	}

	bool Feed::updateOrCreateEntry(const db::ConnectionPtr& db, long long feed_id, const feed::Entry& entry, Stats& stats)
	{
		const char* entryUniqueId = nullifier(entry.m_entryUniqueId);
		if (!entryUniqueId)
			return false;

		auto stmt = db->prepare("SELECT _id, title, url, author, authorLink, date, description, contents FROM entry WHERE guid=?");
		if (!stmt || !stmt->bind(0, entryUniqueId))
			return false;

		Entry _entry;
		auto c = stmt->query();
		if (c && c->next() && db::get(c, _entry))
		{
			if (same(db, entry, _entry))
			{
				stats.knownEntries++;
				fflush(stdout);
				fprintf(stderr, ".");
				fflush(stderr);
				return true;
			}
			stats.changedEntries++;
			fflush(stdout);
			fprintf(stderr, "!");
			fflush(stderr);
			if (!updateEntry(db, feed_id, _entry.id, entry)) return false;
			markAsUnread(db, feed_id, _entry.id);
			return true;
		}

		stats.newEntries++;
		fflush(stdout);
		fprintf(stderr, "#");
		fflush(stderr);
		if (!createEntry(db, feed_id, entryUniqueId, entry)) return false;
		markAsUnread(db, feed_id, entryUniqueId);
		return true;
	}
}

int refresh(int, char*[], const db::ConnectionPtr& db)
{
	auto stmt = db->prepare("SELECT * FROM feed_update");
	if (!stmt)
		return 1;

	auto c = stmt->query();
	std::list<Refresh::Feed> feeds;
	if (!db::get(c, feeds))
		return 1;

	size_t counter = 0;
	size_t length = feeds.size();

	printf("Refreshing %llu feeds\n\n", (unsigned long long)length);
	fflush(stdout);
	fprintf(stderr, 
		". - known\n"
		"# - new\n"
		"! - changed\n\n");
	fflush(stderr);

	int digits = 0;
	while (length > 0)
	{
		length /= 10;
		digits++;
	}
	if (!digits)
		digits = 1;

	auto it = std::find_if(feeds.begin(), feeds.end(), [&db, &counter, digits](const Refresh::Feed& feed) { return !feed.update(db, ++counter, digits, false); });
	return it == feeds.end() ? 0 : 1;
}
