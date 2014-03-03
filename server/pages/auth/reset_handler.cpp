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

namespace FastCGI { namespace app { namespace reader {

	class ResetPasswordPageHandler: public AuthPageHandler
	{
	public:
		DEBUG_NAME("PWD Reset");

	protected:
		bool restrictedPage() override { return false; }

		void prerender(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			if (request.getVariable("close"))
				onAuthFinished(request);

			auto content = std::make_shared<auth::AuthForm>(tr(lng::LNG_RESET_TITLE));
			request.setContent(content);

			content->hidden("continue");

			content->buttons().submit("submit", tr(lng::LNG_CMD_SEND));
			content->buttons().submit("close", tr(lng::LNG_CMD_CLOSE), true);

			content->addMessage(tr(lng::LNG_RESET_MESSAGE));
			auto email = content->controls().text("email", tr(lng::LNG_RESET_USERNAME));

			content->bind(request);

			if (!request.getVariable("posted")) return;

			if (email->hasUserData())
			{
				UserInfo info = UserInfo::fromDB(request.dbConn(), email->getData().c_str());
				if (info.m_id != 0)
				{
					auto recoveryId = info.createRecoverySession(request.dbConn());
					if (recoveryId.empty())
					{
						std::string msg = "Could not create recovery session";
						const char* dbMsg = request.dbConn()->errorMessage();
						if (dbMsg && *dbMsg)
						{
							msg += " (DB Message: ";
							msg += dbMsg;
							msg += ")";
						}

						request.on500(msg);
					}

					FastCGI::MailInfo mail{ "password-reset.txt", tr, lng::LNG_MAIL_SUBJECT_PASSWORD_RECOVERY };
					mail.add_to(info.m_name, info.m_email);
					mail.var("display_name", info.m_name);
					mail.var("login", info.m_login);
					mail.var("recovery_url", request.serverUri("/auth/recovery?id=" + url::encode(recoveryId), false));

					request.sendMail(std::string(), mail);
					request.redirect("/auth/msg_sent", false);
				}

				content->setError(tr(lng::LNG_RESET_ERROR_MISMATCHED));
				email->setError();
			}
			else
			{
				content->setError(tr(lng::LNG_RESET_ERROR_MISSING));
				email->setError();
			}
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/auth/reset", FastCGI::app::reader::ResetPasswordPageHandler);
