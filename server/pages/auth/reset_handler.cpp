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

	class ResetPasswordPageHandler: public AuthPageHandler
	{
	public:

		std::string name() const
		{
			return "Password Reset";
		}

	protected:
		virtual bool restrictedPage() { return false; }

		void prerender(SessionPtr session, Request& request, PageTranslation& tr)
		{
			if (request.getVariable("cancel"))
				request.redirect("/");

			FormPtr content(new (std::nothrow) Form(tr(lng::LNG_RESET_TITLE)));
			request.setContent(content);

			content->hidden("continue");

			content->submit("submit", tr(lng::LNG_CMD_SEND));
			content->submit("cancel", tr(lng::LNG_CMD_CLOSE));

			Section& section = content->section(std::string());
			auto email = section.text("email", tr(lng::LNG_LOGIN_USERNAME), false, tr(lng::LNG_LOGIN_USERNAME_HINT));

			content->bind(request);

			if (!request.getVariable("posted")) return;

			if (email->hasUserData())
			{
				UserInfo info = UserInfo::fromDB(request.dbConn(), email->getData().c_str());
				if (info.m_id != 0)
				{
					request.sendMail("password-reset.txt", info.m_email.c_str());
				}
				request.redirect("/auth/msg_sent", false);
			}
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/auth/reset", FastCGI::app::reader::ResetPasswordPageHandler);
