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

	class ChangePasswordPageHandler: public AuthPageHandler
	{
	public:

		std::string name() const
		{
			return "PWD Change";
		}

	protected:
		virtual bool restrictedPage() { return true; }

		void prerender(SessionPtr session, Request& request, PageTranslation& tr)
		{
			if (request.getVariable("cancel"))
				onAuthFinished(request);

			auto content = std::make_shared<auth::AuthForm>(tr(lng::LNG_CHNGPASS_TITLE));
			request.setContent(content);

			content->addMessage(tr(lng::LNG_CHNGPASS_MESSAGE));
			content->hidden("continue");

			auto message = content->control<Message>("message");
			auto password = content->text("pasword", tr(lng::LNG_CHNGPASS_CURRENT), true);
			auto new_pass = content->text("new-pass", tr(lng::LNG_CHNGPASS_NEW), true);
			auto retype = content->text("restype", tr(lng::LNG_CHNGPASS_RETYPE), true);

			content->submit("submit", tr(lng::LNG_CMD_UPDATE));
			content->submit("cancel", tr(lng::LNG_CMD_CLOSE));

			content->bind(request);

			if (!request.getVariable("posted")) return;

			UserInfo user = UserInfo::fromSession(request.dbConn(), session);

			if (!password->hasUserData() || !user.passwordValid(password->getData().c_str()))
			{
				content->setError(tr(lng::LNG_CHNGPASS_ERROR_CURRENT));
				password->setError();
			}
			else if (!new_pass->hasUserData() || !retype->hasUserData())
			{
				content->setError(tr(lng::LNG_CHNGPASS_ERROR_ONE_MISSING));
				if (!new_pass->hasUserData())
					new_pass->setError();
				if (!retype->hasUserData())
					retype->setError();
			}
			else
			{
				if (new_pass->getData() != retype->getData())
				{
					content->setError(tr(lng::LNG_CHNGPASS_ERROR_MISMATCHED));
					new_pass->setError();
					retype->setError();
				}
				else
				{
					if (!user.changePasswd(request.dbConn(), new_pass->getData().c_str()))
						request.on500("Could not change the email");

					if (session)
						onAuthFinished(request);
					else
						request.on500("Session could not be started");
				}
			}
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/auth/change", FastCGI::app::reader::ChangePasswordPageHandler);
