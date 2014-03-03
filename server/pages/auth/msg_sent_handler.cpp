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

	class MessageSentPageHandler: public AuthPageHandler
	{
	public:
		DEBUG_NAME("SMTP Landing");

	protected:
		bool restrictedPage() override { return false; }

		void prerender(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			if (request.getVariable("close"))
				onAuthFinished(request);

			auto content = std::make_shared<auth::AuthForm>(tr(lng::LNG_MSG_SENT_TITLE));
			request.setContent(content);

			content->addMessage(tr(lng::LNG_MSG_SENT_MESSAGE));
			content->buttons().submit("close", tr(lng::LNG_CMD_CLOSE), true);
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/auth/msg_sent", FastCGI::app::reader::MessageSentPageHandler);
