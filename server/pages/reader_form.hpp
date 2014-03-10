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

#ifndef __READER_FORMS_H__
#define __READER_FORMS_H__

#include <fast_cgi.hpp>
#include <handlers.hpp>
#include <forms.hpp>
#include <crypt.hpp>
#include <map>
#include <forms/vertical_renderer.hpp>
#include "pages.hpp"

namespace FastCGI { namespace app { namespace reader {

	class ReaderFormPageHandler : public PageHandler
	{
	protected:
		void onPageDone(Request& request)
		{
			param_t _continue = request.getVariable("continue");
			if (_continue != nullptr)
				request.redirectUrl(_continue);
			request.redirect("/view/", false); // reducing the number of 304s by one...
		}

		void header(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			request << "<!DOCTYPE html "
				"PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
				"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n"
				"<html>\r\n"
				"  <head>\r\n";
			headElement(session, request, tr);
			request <<
				"    <meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"/>\r\n";
#if DEBUG_CGI
			request <<
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/fd_icons.css\");</style>\r\n";
#endif
			request <<
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/site-logo-big.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/tabs.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/forms-base.css\");</style>\r\n"
				"    <style type=\"text/css\" media=\"screen and (min-width: 490px)\">@import url(\"" << static_web << "css/forms-wide.css\");</style>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/jquery-1.9.1.js\"></script>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/jquery.pjax.js\"></script>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/ajax_fragment.js\"></script>\r\n"
				"  </head>\r\n"
				"  <body>\r\n";
			bodyStart(session, request, tr);
		}

		void bodyStart(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			request <<
				"  <div class='site-logo'><div>\r\n"
				"    <div class='logo'><a href='/'><img src='" << static_web << "images/auth_logo.png' /></a></div>\r\n"
				"    <div class='site'><a href='/'>" << tr(lng::LNG_GLOBAL_DESCRIPTION) << "</a></div>\r\n"
				"  </div></div>\r\n"
				"\r\n"
				"  <div id=\"form-content\">\r\n";
		}

		void bodyEnd(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			request << "\r\n"
				"  </div> <!-- form-content -->\r\n";
		}
	};

}}} // FastCGI::app::reader

#endif // __READER_FORMS_H__
