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
#include "json.hpp"

namespace json
{
	struct Subscriptions
	{
		db::CursorPtr folders;
		db::CursorPtr feeds;
	};

	JSON_CURSOR_RULE(Folders)
	{
		JSON_CURSOR_LL(id, 0);
		JSON_CURSOR_TEXT(title, 1);
		JSON_CURSOR_TEXT(parent, 2);
	}

	JSON_CURSOR_RULE(Feeds)
	{
		JSON_CURSOR_LL(id, 0);
		JSON_CURSOR_TEXT(title, 1);
		JSON_CURSOR_LL(parent, 2);
		JSON_CURSOR_LL(unread, 3);
	}

	JSON_RULE(Subscriptions)
	{
		JSON_ADD_CURSOR(folders, Folders);
		JSON_ADD_CURSOR(feeds, Feeds);
	}
};

namespace FastCGI { namespace app { namespace reader {

	class SubscriptionPageHandler: public Handler
	{
		std::string m_service_url;
	public:

		std::string name() const
		{
			return "JSON::Feeds";
		}

		void visit(FastCGI::Request& request)
		{
			SessionPtr session = request.getSession();
			db::ConnectionPtr db = request.dbConn();

			auto folders = db->prepare("SELECT folder_id, name, parent FROM parents WHERE user_id=?");
			if (!folders || !folders->bind(0, session->getId()))
			{
				FLOG << (folders ? folders->errorMessage() : db->errorMessage());
				request.on500();
			}

			auto feeds = db->prepare("SELECT feed_id, feed, folder_id, count FROM ordered_stats WHERE user_id=? AND type=0");
			if (!feeds || !feeds->bind(0, session->getId()))
			{
				FLOG << (feeds ? feeds->errorMessage() : db->errorMessage());
				request.on500();
			}

			json::Subscriptions subs;
			subs.folders = folders->query();
			if (!subs.folders)
			{
				FLOG << folders->errorMessage();
				request.on500();
			}

			subs.feeds = feeds->query();
			if (!subs.feeds)
			{
				FLOG << feeds->errorMessage();
				request.on500();
			}

			request.setHeader("Content-Type", "application/json; charset=utf-8");
			json::render(request, subs);
		}

	};
}}}  // FastCGI::app::reader

REGISTER_HANDLER("/data/subscription.json", FastCGI::app::reader::SubscriptionPageHandler);
