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
#include "api_handler.hpp"
#include "json.hpp"

namespace json
{
	struct Subscriptions
	{
		db::CursorPtr folders;
		db::CursorPtr feeds;
		db::CursorPtr unread;
		db::CursorPtr starred;
	};

	JSON_CURSOR_RULE(Folders)
	{
		JSON_CURSOR_LL(id, 0);
		JSON_CURSOR_TEXT(title, 1);
		JSON_CURSOR_LL(parent, 2);
	}

	JSON_CURSOR_RULE(Feeds)
	{
		JSON_CURSOR_LL(id, 0);
		JSON_CURSOR_TEXT(title, 1);
		JSON_CURSOR_TEXT(url, 2);
		JSON_CURSOR_LL(parent, 3);
		JSON_CURSOR_LL(unread, 4);
	}

	struct State: CursorJson<State>
	{
		State(const db::CursorPtr& c);
		bool entryIsScalar() const { return true; }
	};
	State::State(const db::CursorPtr& c): CursorJson<State>(c)
	{
		JSON_CURSOR_LL(value, 0);
	};

	JSON_RULE(Subscriptions)
	{
		JSON_ADD_CURSOR(folders, Folders);
		JSON_ADD_CURSOR(feeds, Feeds);
		JSON_ADD_CURSOR(unread, State);
		JSON_ADD_CURSOR(starred, State);
	}
};

#define SELECT(stmt, QUERY) \
	auto stmt = db->prepare(QUERY); \
	if (!stmt || !stmt->bind(0, session->getId())) \
	{ \
		FLOG << (stmt ? stmt->errorMessage() : db->errorMessage()); \
		request.on500(); \
	} \
	\
	subs.stmt = stmt->query(); \
	if (!subs.stmt) \
	{ \
		FLOG << stmt->errorMessage(); \
		request.on500(); \
	}

namespace FastCGI { namespace app { namespace api {

	class Subscription: public APIOperation
	{
	public:
		void render(SessionPtr session, Request& request, PageTranslation& tr)
		{
			db::ConnectionPtr db = request.dbConn();
			json::Subscriptions subs;

			SELECT(folders, "SELECT _id, name, parent FROM folder WHERE user_id=?");
			SELECT(feeds,   "SELECT feed_id, feed, feed_url, folder_id, count FROM ordered_stats WHERE user_id=? AND type=0");
			SELECT(unread,  "SELECT entry_id FROM state WHERE user_id=? AND type=0 ORDER BY entry_id ASC");
			SELECT(starred, "SELECT entry_id FROM state WHERE user_id=? AND type=1 ORDER BY entry_id ASC");

			request.setHeader("Content-Type", "application/json; charset=utf-8");
			json::render(request, subs);
		}

		const char** getVariables() const
		{
			static const char* vars[] = { nullptr };
			return vars;
		}
	};
}}}  // FastCGI::app::api

REGISTER_OPERATION("subscription", FastCGI::app::api::Subscription);
