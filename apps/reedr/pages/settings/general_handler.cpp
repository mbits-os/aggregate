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
#include "settings.hpp"

namespace FastCGI { namespace app { namespace reader {

	class GeneralSettingsPageHandler : public SettingsPageHandler
	{
	public:
		DEBUG_NAME("Settings: General");

	protected:
		void prerender(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			if (request.getVariable("close"))
				onPageDone(request);

			if (request.getVariable("posted") && request.getVariable("lang"))
			{
				std::string newLang = request.getVariable("lang");
				auto profile = session->profile();
				if (newLang != profile->preferredLanguage())
				{
					profile->preferredLanguage(newLang);

					// empty preferred language means "only look into Accept-Languages, when there is nothing loaded"
					// so we must behave as if nothing was loaded...
					if (newLang.empty())
						session->setTranslation(nullptr);

					tr.init(session, request);
				}
			}

			auto content = std::make_shared<settings::SimpleForm>(settings::PAGE::GENERAL);
			request.setContent(content);

			Options lang_opts;
			lang_opts.add("", tr(lng::LNG_SETTINGS_GENERAL_DEFAULT));

			auto langs = request.app().knownLanguages();
			for (auto&& lang : langs)
				lang_opts.add(lang.m_lang, lang.m_name);

			auto& controls = content->controls();

			controls.selection("lang", tr(lng::LNG_SETTINGS_GENERAL_LANGUAGES), lang_opts);

			auto feeds = controls.radio_group("feed", tr(lng::LNG_SETTINGS_GENERAL_FEED_TITLE));
			feeds->radio("all", tr(lng::LNG_SETTINGS_GENERAL_FEED_ALL));
			feeds->radio("unread", tr(lng::LNG_SETTINGS_GENERAL_FEED_UNREAD));

			auto posts = controls.radio_group("posts", tr(lng::LNG_SETTINGS_GENERAL_POSTS_TITLE));
			posts->radio("list", tr(lng::LNG_SETTINGS_GENERAL_POSTS_LIST));
			posts->radio("exp", tr(lng::LNG_SETTINGS_GENERAL_POSTS_EXPANDED));

			auto sort = controls.radio_group("sort", tr(lng::LNG_SETTINGS_GENERAL_SORT_TITLE));
			sort->radio("newest", tr(lng::LNG_SETTINGS_GENERAL_SORT_NEWEST));
			sort->radio("oldest", tr(lng::LNG_SETTINGS_GENERAL_SORT_OLDEST));

			content->buttons().submit("submit", tr(lng::LNG_CMD_UPDATE));
			content->buttons().submit("close", tr(lng::LNG_CMD_CLOSE), ButtonType::Narrow);

			Strings data;

			auto userInfo = FastCGI::userInfo(session);
			auto profile = session->profile();

			data["lang"]  = profile->preferredLanguage();
			data["feed"]  = userInfo->viewOnlyUnread()  ? "unread" : "all";
			data["posts"] = userInfo->viewOnlyTitles()  ? "list"   : "exp";
			data["sort"]  = userInfo->viewOldestFirst() ? "oldest" : "newest";

			content->bind(request, data);

			if (!request.getVariable("posted"))
				return;

			if (feeds->hasUserData())
				userInfo->setViewOnlyUnread(feeds->getData() == "unread");
			if (posts->hasUserData())
				userInfo->setViewOnlyTitles(posts->getData() == "list");
			if (sort->hasUserData())
				userInfo->setViewOldestFirst(sort->getData() == "oldest");

			profile->storeLanguage(request.dbConn());
			userInfo->storeFlags(request.dbConn());
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/settings/general", FastCGI::app::reader::GeneralSettingsPageHandler);
