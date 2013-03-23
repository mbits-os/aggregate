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
#include "handlers.h"

namespace app
{
	class LoginPageHandler: public PageHandler
	{
	public:

		std::string name() const
		{
			return "Login";
		}

	protected:
		virtual bool restrictedPage() { return false; }
		const char* getPageTitle(PageTranslation& tr) { return tr(lng::LNG_LOGIN_TITLE); }
		void render(FastCGI::SessionPtr session, FastCGI::Request& request, PageTranslation& tr)
		{
			fcgi::param_t QUERY_STRING = request.getParam("QUERY_STRING");
			if (!QUERY_STRING || !*QUERY_STRING)
			{
				request << 
					"łotewah";
				request.die();
			}
			request << "<h1>Reading...</h1>";
			request << "<p>Done</p>";
		}

	};
}

REGISTER_HANDLER("/auth/login", app::LoginPageHandler);
