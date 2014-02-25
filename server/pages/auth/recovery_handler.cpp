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
#include "auth.hpp"
#include "auth_forms.hpp"

namespace FastCGI { namespace app { namespace reader {

	class PasswordRecoveryPageHandler: public AuthPageHandler
	{
	public:

		std::string name() const
		{
			return "Password Recovery";
		}

	protected:
		virtual bool restrictedPage() { return false; }

		void prerender(SessionPtr session, Request& request, PageTranslation& tr)
		{
			auto content = std::make_shared<auth::AuthForm>(tr(lng::LNG_RECOVERY_TITLE));
			request.setContent(content);

			auto recoveryId = request.getVariable("id");
			UserInfo info;
			info.m_id = -1;
			if (recoveryId)
			info = UserInfo::fromRecoveryId(request.dbConn(), recoveryId);

			if (info.m_id < 0)
			{
				content->addMessage(tr(lng::LNG_RECOVERY_INVALID));
				content->link("cancel", "/", std::string("&laquo; ") + tr(lng::LNG_CMD_CLOSE));
				return;
			}

			content->hidden("id");

			content->addMessage(tr(lng::LNG_CHNGPASS_MESSAGE));

			auto message = content->control<Message>("message");
			auto email = content->text("password", tr(lng::LNG_CHNGPASS_NEW), true);
			auto password = content->text("restype", tr(lng::LNG_CHNGPASS_RETYPE), true);

			content->submit("submit", tr(lng::LNG_RECOVERY_CMD));
			content->link("cancel", "/", std::string("&laquo; ") + tr(lng::LNG_CMD_CLOSE));
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/auth/recovery", FastCGI::app::reader::PasswordRecoveryPageHandler);
