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

	class ProfileSettingsPageHandler : public SettingsPageHandler
	{
	public:
		DEBUG_NAME("Settings: Profile");

	protected:
		void prerender(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			if (request.getVariable("close"))
				onPageDone(request);

			auto content = std::make_shared<settings::SectionForm>(settings::PAGE::PROFILE);
			request.setContent(content);

			std::string url = "/auth/change";
			auto here_uri = request.getParam("DOCUMENT_URI");
			if (here_uri)
				url += "?continue=" + url::encode(request.serverUri(here_uri, false));

			auto& signin = content->section(tr(lng::LNG_SETTINGS_PROFILE_SEC_SIGNIN));
			auto& name = content->section(tr(lng::LNG_SETTINGS_PROFILE_SEC_NAME));

			content->buttons().submit("submit", tr(lng::LNG_CMD_UPDATE));
			content->buttons().submit("close", tr(lng::LNG_CMD_CLOSE), ButtonType::Narrow);

			signin.text("login-text", tr(lng::LNG_LOGIN_USERNAME));
			signin.text("pass-word", tr(lng::LNG_LOGIN_PASSWORD), true, tr(lng::LNG_SETTINGS_PROFILE_PASSWORD_MAIL_HINT));
			signin.text("email", tr(lng::LNG_SETTINGS_PROFILE_EMAIL));
			signin.link("pass", url, tr(lng::LNG_SETTINGS_PROFILE_CHANGE_PASSWORD));

			Options opts;
			opts.add("0", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_CUSTOM))
				.add("1", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_ORIGINAL))
				.add("2", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_NAME))
				.add("3", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_FAMILY_NAME))
				.add("4", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_NFN))
				.add("5", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_FNN))
				.add("6", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_NCFN))
				.add("7", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_FNCN));

			name.text("name", tr(lng::LNG_SETTINGS_PROFILE_NAME));
			name.text("family_name", tr(lng::LNG_SETTINGS_PROFILE_FAMILY_NAME));
			name.selection("display_name", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME), opts);
			name.text("custom", std::string());
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/settings/profile", FastCGI::app::reader::ProfileSettingsPageHandler);
