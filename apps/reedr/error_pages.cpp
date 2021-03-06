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

        struct String
        {
                lng::LNG lngId;
                const char* fallback;
        };

	struct FallbackTranslation
	{
		PageTranslation tr;
		bool hasStrings;
	public:
		void init(const SessionPtr& session, Request& request)
		{
			hasStrings = tr.init(session, request);
		}

		const char* operator()(const String& s)
		{
			return hasStrings ? tr(s.lngId) : s.fallback;
		}

		const char* operator()(lng::LNG id, const char* fallback)
		{
                        return hasStrings ? tr(id) : fallback;
		}
	};

	class ErrorPageHandler : public ErrorHandler
	{
	protected:
		virtual const char* pageTitle(int errorCode, FallbackTranslation& tr) { return nullptr; }
		std::string getTitle(int errorCode, Request& request, FallbackTranslation& tr)
		{
			std::string title = tr(lng::LNG_GLOBAL_SITENAME, "reedr");
			const char* page = pageTitle(errorCode, tr);
			if (!page) return title;
			title += " &raquo; ";
			title += page;
			return title;
		}

		virtual void header(int errorCode, const SessionPtr& session, Request& request, FallbackTranslation& tr)
		{
			request << "<!DOCTYPE html "
				"PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
				"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n"
				"<html>\r\n"
				"  <head>\r\n"
				"    <title>" << getTitle(errorCode, request, tr) << "</title>\r\n"
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
			const char* description = tr(lng::LNG_GLOBAL_DESCRIPTION, "stay up to date");
			request <<
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/site-logo-big.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/tabs.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/forms-base.css\");</style>\r\n"
				"    <style type=\"text/css\" media=\"screen and (min-width: 490px)\">@import url(\"" << static_web << "css/forms-wide.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/error_page.css\");</style>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/jquery-1.9.1.js\"></script>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/jquery.pjax.js\"></script>\r\n"
				"    <script type=\"text/javascript\" src=\"" << static_web << "code/ajax_fragment.js\"></script>\r\n"
				"  </head>\r\n"
				"  <body>\r\n"
				"  <div class='site-logo'><div>\r\n"
				"    <div class='logo'><a href='/'><img src='" << static_web << "images/site-logo-big.svg' /></a></div>\r\n"
				"    <div class='site'><a href='/'>" << description << "</a></div>\r\n"
				"  </div></div>\r\n"
				"\r\n"
				"  <div id=\"form-content\" class=\"form\">\r\n";
		}

		virtual void footer(const SessionPtr& session, Request& request, FallbackTranslation& tr)
		{
#if DEBUG_CGI
			request <<
				"\r\n"
				"    <div class=\"debug-icons\">\r\n"
				"      <div>\r\n"
				"        <a class=\"debug-icon\" href=\"/debug/\"><span>[D]</span></a>\r\n";
			std::string icicle = request.getIcicle();
			if (!icicle.empty())
				request << "        <a class=\"frozen-icon\" href=\"/debug/?frozen=" << url::encode(icicle) << "\"><span>[F]</span></a>\r\n";
			request <<
				"      </div>\r\n"
				"    </div>\r\n";
#endif
			request << "\r\n"
				"  </div> <!-- form-content -->\r\n"
				"  </body>\r\n"
				"</html>\r\n";
		}

		virtual void contents(int errorCode, const SessionPtr& session, Request& request, FallbackTranslation& tr)
		{
			request <<
				"    <h1>" << errorCode << "</h1>\r\n"
				"    <p>A message</p>\r\n"
				;
		}

		void onError(int errorCode, Request& request) override
		{
			auto session = request.getSession(false);
			FallbackTranslation tr{};

			tr.init(session, request);

			header(errorCode, session, request, tr);
			contents(errorCode, session, request, tr);
			footer(session, request, tr);
		}
	};

	struct ErrorInfo
	{
		String title;
		String oops;
		String info;
	};

	static inline void error_contents(Request& request, FallbackTranslation& tr, const ErrorInfo& nfo)
	{
		request << "    <h1>" << tr(nfo.title) << "</h1>\r\n"
			"    <section id='message'>\r\n"
			"      <h2>" << tr(nfo.oops) << "</h2>\r\n"
			"      <p>" << tr(nfo.info) << "</p>\r\n";
	}

	static inline void error_contents_end(Request& request, FallbackTranslation& tr)
	{
		request << "    </section> <!-- #message -->\r\n";
	}

	static inline void error_4xx_consolation(Request& request, FallbackTranslation& tr)
	{
		request << "      <p>" << tr(lng::LNG_ERROR_404_CONSOLATION, "Here are some things you might be interested in:") << "</p>\r\n"
			"      <ul>\r\n"
			"        <li><a href='/view/'>" << tr(lng::LNG_VIEW_HOME, "Home") << "</a></li>\r\n"
			"        <li><a href='/settings/general'>" << tr(lng::LNG_SETTINGS_TITLE, "Settings") << "</a></li>\r\n"
			"        <li><a href='/settings/profile'>" << tr(lng::LNG_SETTINGS_PROFILE, "Profile") << "</a></li>\r\n"
			"      </ul>\r\n";
	}

	class Error404 : public ErrorPageHandler
	{
	public:
		void contents(int errorCode, const SessionPtr& session, Request& request, FallbackTranslation& tr) override
		{
			error_contents(request, tr, {
				{ lng::LNG_ERROR_404_TITLE, "404<br/>FILE NOT FOUND" },
				{ lng::LNG_ERROR_404_OOPS,  "Well, that was not supposed to happen." },
				{ lng::LNG_ERROR_404_INFO,  "This link has nothing to show, maybe you mistyped the link or followed a bad one." }
			});
			error_4xx_consolation(request, tr);
			error_contents_end(request, tr);
		}
	};

	class Error400 : public ErrorPageHandler
	{
	public:
		void contents(int errorCode, const SessionPtr& session, Request& request, FallbackTranslation& tr) override
		{
			error_contents(request, tr, {
				{ lng::LNG_ERROR_400_TITLE, "400<br/>BAD REQUEST" },
				{ lng::LNG_ERROR_400_OOPS,  "Hmm, this request looks mighty strange." },
				{ lng::LNG_ERROR_400_INFO,  "The request your browser made was not understood." }
			});
			error_4xx_consolation(request, tr);
			error_contents_end(request, tr);
		}
	};

	class Error500 : public ErrorPageHandler
	{
	public:
		void contents(int errorCode, const SessionPtr& session, Request& request, FallbackTranslation& tr) override
		{
			error_contents(request, tr, {
				{ lng::LNG_ERROR_500_TITLE, "500<br/>INTERNAL SERVER ERROR" },
				{ lng::LNG_ERROR_500_OOPS,  "Oops, something strange and unusal has happend." },
				{ lng::LNG_ERROR_500_INFO,  "During our work to fullfill your request some error occured. The administrators of the site has been informed." }
			});

			request << "  <p>" << tr(lng::LNG_ERROR_500_CONSOLATION, "We are sorry, please come back soon. Or even <a href='/'>now</a>.") << "</p>\r\n";
			error_contents_end(request, tr);
		}
	};

	void setErrorHandlers(Application& app)
	{
		app.setErrorHandler(0, std::make_shared<ErrorPageHandler>());
		app.setErrorHandler(400, std::make_shared<Error400>());
		app.setErrorHandler(404, std::make_shared<Error404>());
		app.setErrorHandler(500, std::make_shared<Error500>());
	}

}}}} // FastCGI::app::reader::errors
