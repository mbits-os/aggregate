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

#include "schema_builder.hpp"

namespace db
{
	namespace model
	{
		SDBuilder::SDBuilder()
		{

			/////////////////////////////////////////////////////////////////////////////
			//
			//          VERSION 1
			//
			/////////////////////////////////////////////////////////////////////////////

			schema_config(sd);

			Field language("lang", FIELD_TYPE::TEXT, att::NONE, std::string(), 2);

			sd.table("user")
				._id()
				.field("login")
				.field("name")
				.field("email")
				.field("passphrase")
				.field("root_folder", "0", FIELD_TYPE::KEY)
				.field("is_admin", "0", FIELD_TYPE::BOOLEAN)
				.add(language) // version 2
				.field("prefs", "0", FIELD_TYPE::INTEGER, att::NOTNULL | att::DEFAULT, 2)
				;

			//hash is built from email and new salt
			//hash goes into cookie, seed is saved for later
			sd.table("session")
				.text_id("hash")
				.field("seed")
				.refer("user")
				.field("set_on", std::string(), FIELD_TYPE::TIME)
				;

			sd.table("folder")
				._id()
				.refer("user")
				.field("name")
				.field("parent", "0", FIELD_TYPE::KEY)
				.field("ord", "0", FIELD_TYPE::INTEGER)
				;

			sd.table("feed")
				._id()
				.field("title")
				.field("site")
				.field("feed")
				.field("last_update", std::string(), FIELD_TYPE::TIME)
				.nullable("author")
				.nullable("authorLink")
				.nullable("etag")
				.nullable("last_modified")
				;

			sd.table("entry")
				._id()
				.refer("feed")
				.field("guid")
				.field("title", "")
				.nullable("url")
				.nullable("date", std::string(), FIELD_TYPE::TIME)
				.nullable("author")
				.nullable("authorLink")
				//only one of those can acutally be null:
				.nullable("description")
				.nullable("contents")
				;

			sd.table("enclosure")
				.refer("entry")
				.field("url")
				.field("mime")
				.field("length", std::string(), FIELD_TYPE::INTEGER)
				;

			sd.table("categories")
				.refer("entry")
				.field("cat")
				;

			sd.table("subscription")
				.refer("feed")
				.refer("folder")
				.field("ord", std::string(), FIELD_TYPE::INTEGER)
				;

			sd.table("state")
				.refer("user")
				.refer("entry")
				.field("type", std::string(), FIELD_TYPE::INTEGER) // 0 - unread; 1 - starred; ?2 - important?
				;

			sd.view("parents",
				"SELECT folder.user_id AS user_id, folder._id AS folder_id, folder.name AS name, parent.name AS parent, folder.ord AS ord "
				"FROM folder "
				"LEFT JOIN folder AS parent ON (parent._id = folder.parent) "
				"ORDER BY parent.name, folder.ord");

			sd.view("state_stats",
				"SELECT state.user_id AS user_id, entry.feed_id AS feed_id, state.type AS type, count(*) AS count "
				"FROM state "
				"LEFT JOIN entry ON (entry._id = state.entry_id) "
				"GROUP BY state.type, entry.feed_id, state.user_id");

			sd.view("ordered_stats",
				"SELECT state_stats.user_id AS user_id, subscription.folder_id AS folder_id, subscription.ord AS ord, feed._id AS feed_id, feed.title AS feed, feed.site AS feed_url, type, count "
				"FROM subscription "
				"LEFT JOIN folder ON (subscription.folder_id = folder._id) "
				"LEFT JOIN feed ON (subscription.feed_id = feed._id) "
				"LEFT JOIN state_stats ON (state_stats.feed_id = feed._id AND state_stats.user_id = folder.user_id) "
				"ORDER BY folder_id, type, ord");

			sd.view("trending",
				"SELECT feed_id, count(*) AS popularity "
				"FROM subscription "
				"GROUP BY feed_id");

			sd.view("feed_update",
				"SELECT _id, title, feed, etag, last_modified, last_update, popularity "
				"FROM feed "
				"LEFT JOIN trending ON (trending.feed_id = feed._id) "
				"ORDER BY popularity DESC, last_update ASC");

			/////////////////////////////////////////////////////////////////////////////
			//
			//          VERSION 2
			//
			/////////////////////////////////////////////////////////////////////////////

			Field _id{ "_id", FIELD_TYPE::TEXT_KEY, att::NOTNULL | att::KEY };
			_id.length(150);
			sd.table("recovery", 2)
				.add(_id)
				.refer("user")
				.field("started", std::string(), FIELD_TYPE::TIME)
				;
		}

		void SDBuilder::schema_config(SchemaDefinition& sd)
		{
			sd.table("schema_config")
				.text_id("name")
				.field("value", std::string(), FIELD_TYPE::BLOB, att::NONE)
				;
		}
	}
}