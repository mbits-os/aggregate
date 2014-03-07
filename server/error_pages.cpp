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
#include <forms/vertical_renderer.hpp>
#include <fast_cgi.hpp>
#include "pages/reader_form.hpp"
#include <utils.hpp>

namespace FastCGI { namespace app { namespace reader { namespace errors {

	class ErrorPageHandler : public ErrorHandler
	{
	protected:
		virtual const char* pageTitle(int errorCode) { return nullptr; }
		virtual const char* pageTitle(int errorCode, PageTranslation& tr) { return pageTitle(errorCode); }
		std::string getTitle(int errorCode, Request& request, PageTranslation& tr, bool hasStrings)
		{
			std::string title = hasStrings ? tr(lng::LNG_GLOBAL_SITENAME) : "reedr";
			const char* page = hasStrings ? pageTitle(errorCode, tr) : pageTitle(errorCode);
			if (!page) return title;
			title += " &raquo; ";
			title += page;
			return title;
		}

		virtual void header(int errorCode, const SessionPtr& session, Request& request, PageTranslation& tr, bool hasStrings)
		{
			request << "<!DOCTYPE html "
				"PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
				"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n"
				"<html>\r\n"
				"  <head>\r\n"
				"    <title>" << getTitle(errorCode, request, tr, hasStrings) << "</title>\r\n"
				"    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/site.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/topbar.css\");</style>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/jquery-1.9.1.js\"></script>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/topbar.js\"></script>\r\n"
				//"    <style type=\"text/css\">@import url(\"" << static_web << "css/topbar_icons.css\");</style>\r\n"
				"    <meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"/>\r\n";
#if DEBUG_CGI
			request <<
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/fd_icons.css\");</style>\r\n";
#endif
			const char* description = hasStrings ? tr(lng::LNG_GLOBAL_DESCRIPTION) : "stay up to date";
			request <<
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/site-logo-big.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/tabs.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/forms-base.css\");</style>\r\n"
				"    <style type=\"text/css\" media=\"screen and (min-width: 490px)\">@import url(\"" << static_web << "css/forms-wide.css\");</style>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/jquery-1.9.1.js\"></script>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/jquery.pjax.js\"></script>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/ajax_fragment.js\"></script>\r\n"
				"  </head>\r\n"
				"  <body>\r\n"
				"  <div class='site-logo'><div>\r\n"
				"    <div class='logo'><a href='/'><img src='" << static_web << "images/auth_logo.png' /></a></div>\r\n"
				"    <div class='site'><a href='/'>" << description << "</a></div>\r\n"
				"  </div></div>\r\n"
				"\r\n"
				"  <div id=\"form-content\">\r\n";
		}

		virtual void footer(const SessionPtr& session, Request& request, PageTranslation& tr, bool hasStrings)
		{
			request << "\r\n"
				"  </div> <!-- form-content -->\r\n"
				"  </body>\r\n"
				"</html\r\n";
		}

		virtual void contents(int errorCode, const SessionPtr& session, Request& request, PageTranslation& tr, bool hasStrings)
		{
			request <<
				"    <h1>" << errorCode << "</h1>\r\n"
				"    <p>A message</p>\r\n"
				;
		}

		void onError(int errorCode, Request& request) override
		{
			auto session = request.getSession(false);
			PageTranslation tr{};

			bool hasStrings = tr.init(session, request);

			header(errorCode, session, request, tr, hasStrings);
			contents(errorCode, session, request, tr, hasStrings);
			footer(session, request, tr, hasStrings);
		}
	};

	void setErrorHandlers(Application& app)
	{
		app.setErrorHandler(0, std::make_shared<ErrorPageHandler>());
	}

}}}} // FastCGI::app::reader::errors
