/*
 * Copyright (C) 2013 Aggregate
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

namespace FastCGI { namespace app { namespace reader {

	class LoginPageHandler: public AuthPageHandler
	{
	public:

		std::string name() const
		{
			return "Login";
		}

	protected:
		virtual bool restrictedPage() { return false; }

		void prerender(SessionPtr session, Request& request, PageTranslation& tr)
		{
			if (request.getVariable("reset") != nullptr)
				request.redirect("/auth/reset");

			FormPtr content(new (std::nothrow) Form(tr(lng::LNG_LOGIN_TITLE)));
			request.setContent(content);

			content->hidden("continue");

			content->submit("submit", tr(lng::LNG_NAV_SIGNIN));
			content->submit("reset", tr(lng::LNG_LOGIN_FORGOT));

			Section& section = content->section(std::string());
			auto message = section.control<Message>("message");
			auto email = section.text("email", tr(lng::LNG_LOGIN_USERNAME), false, tr(lng::LNG_LOGIN_USERNAME_HINT));
			auto password = section.text("password", tr(lng::LNG_LOGIN_PASSWORD), true);
			auto cookie = section.checkbox("long_cookie", tr(lng::LNG_LOGIN_STAY));

			content->bind(request);

			bool set_cookie = cookie->isChecked();

			if (email->hasUserData() || password->hasUserData())
			{
				if (!email->hasUserData() || !password->hasUserData())
					content->setError(tr(lng::LNG_LOGIN_ERROR_ONE_MISSING));
				else
				{
					UserInfo info = UserInfo::fromDB(request.dbConn(), email->getData().c_str());
					if (!info.passwordValid(password->getData().c_str()))
						content->setError(tr(lng::LNG_LOGIN_ERROR_MISMATCHED)); 
					else
					{
						SessionPtr session = request.startSession(set_cookie, email->getData().c_str());
						if (session.get())
							onAuthFinished(request);
						else
							request.on500();
					}
				}
			}
			else if (request.getVariable("posted") != nullptr)
			{
				content->setError(tr(lng::LNG_LOGIN_ERROR_ONE_MISSING));
			}
			else
			{
				Strings data;
				if (session.get())
					data["email"] = session->getEmail();
				data["message"] = tr(lng::LNG_LOGIN_UI_STALE); 
				message->bind(request, data);
				email->bind(request, data);
			}
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/auth/login", FastCGI::app::reader::LoginPageHandler);
