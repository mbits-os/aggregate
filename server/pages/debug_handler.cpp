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
#include "crypt.hpp"

#if DEBUG_CGI

#ifdef _WIN32
#define SEP ';'
#else
#define SEP ':'
extern char ** environ;
#endif

namespace app
{
	class DebugPageHandler: public PageHandler
	{
		std::string m_service_url;
	public:

		std::string name() const
		{
			return "DEBUG_CGI";
		}

		static void penv(FastCGI::Request& request, const char * const * envp)
		{
			request << "<table class='env'>\n";
			request << "<thead><tr><th>Name</th><th>Value</th></tr><thead><tbody>\n";
			size_t counter = 0;
			for ( ; *envp; ++envp)
			{
				const char* eq = strchr(*envp, '=');
				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request << "><td>";
				if (eq == *envp) request << "&nbsp;";
				else if (eq == NULL) request << *envp;
				else request << std::string(*envp, eq);
				request << "</td><td>";
				if (eq == NULL) request << "&nbsp;";
				else if (strncmp("PATH=", *envp, 5))
					request << eq + 1;
				else 
				{
					const char* prev = eq + 1;
					const char* c = eq + 1;
					while (*c != 0)
					{
						while (*c != 0 && *c != SEP) ++c;
						request << std::string(prev, c);
						if (*c != 0) request << "<br/>\n";
						if (*c != 0) ++c;
						prev = c;
					}
				}
				request << "</td></tr>\n";
			}
			request << "</table>\n";
		}

		static void handlers(FastCGI::Request& request,
			HandlerMap::const_iterator begin,
			HandlerMap::const_iterator end)
		{
			request << "<table class='handlers'>\n";
			request << "<thead><tr><th>URL</th><th>Action</th><th>File</th></tr><thead><tbody>\n";
			size_t counter = 0;
			for ( ; begin != end; ++begin)
			{
				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request
					<< "><td><nobr><a href='" << begin->first << "'>" << begin->first << "</a></nobr></td>"
					<< "<td><nobr>" << begin->second.ptr->name() << "</nobr></td><td>" << begin->second.file << ":"
					<< begin->second.line << "</td></tr>\n";
			}
			request << "</tbody></table>\n";
		}

		static void requests(FastCGI::Request& request, const FastCGI::Application::ReqList& list)
		{
			request << "<table class='requests'>\n";
			request << "<thead><tr><th>URL</th><th>Remote Addr</th><th>Time (GMT)</th></tr><thead><tbody>\n";

			auto _cur = list.begin(), _end = list.end();
			size_t counter = 0;
			for ( ; _cur != _end; ++_cur)
			{
				if (_cur->resource == "/debug/?all")
					continue;

				tyme::tm_t gmt = tyme::gmtime(_cur->now);
				char timebuf[100];
				tyme::strftime(timebuf, "%a, %d-%b-%Y %H:%M:%S GMT", gmt );

				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request
					<< "><td><nobr><a href='http://" << _cur->server << _cur->resource << "'>" << _cur->resource << "</a></nobr></td>"
					<< "<td><nobr>" << _cur->remote_addr << ":" << _cur->remote_port << "</nobr></td><td>" << timebuf << "</td></tr>\n";
			}
			request << "</tbody></table>\n";
		}

		static void cookies(FastCGI::Request& request, const std::map<std::string, std::string>& list)
		{
			request << "<table class='cookies'>\n";
			request << "<thead><tr><th>Name</th><th>Value</th></tr><thead><tbody>\n";

			auto _cur = list.begin(), _end = list.end();
			size_t counter = 0;
			for ( ; _cur != _end; ++_cur)
			{
				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request
					<< "><td><nobr>" << _cur->first << "</nobr></td>"
					<< "<td>" << _cur->second << "</td></tr>\n";
			}
			request << "</tbody></table>\n";
		}

		bool restrictedPage() { return false; }
		const char* getPageTitle(PageTranslation& tr) { return "Debug"; }
		void prerender(FastCGI::SessionPtr session, Request& request, PageTranslation& tr)
		{
			crypt::session_t hash;
			crypt::session("reader.login", hash);
			request.setCookie("cookie-test", hash, tyme::now() + 86400*60);
		}

		void render(FastCGI::SessionPtr session, Request& request, PageTranslation& tr)
		{
			const char* QUERY_STRING = request.getParam("QUERY_STRING");
			bool all = (QUERY_STRING != NULL) && (strcmp(QUERY_STRING, "all") == 0);

			request << "<style type='text/css'>\n"
				"body, td, th { font-family: Helvetica, Arial, sans-serif; font-size: 10pt }\n"
				"div#content { width: 650px; margin: 0px auto }\n"
				"th, td { text-align: left; vertical-align: top; padding: 0.2em 0.5em }\n"
				".even td { background: #ddd }\n"
				"th { font-weight: normal; color: white; background: #444; }\n"
				"table { width: auto; max-width: 650px }\n"
				"</style>\n"
				"<title>Debug page</title>\n<div id='content'>\n"
				"<h1>Debug page</h1>\n"
				"<h2>Table of Contents</h2>\n"
				"<ol>\n";
			if (all) request <<
				"<li><a href='#request'>Request Environment</a></li>\n"
				//"<li><a href='#process'>Process/Initial Environment</a></li>\n"
				;
			request <<
				"<li><a href='#handlers'>Page Handlers</a></li>\n"
				"<li><a href='#requests'>Page Requests</a></li>\n"
				"<li><a href='#cookies'>Page Cookies</a></li>\n"
				"<li><a href='#session'>Session</a></li>\n"
				"</ol>\n"
				"<h2>PID: <em>" << request.app().pid() << "</em></h2>\n"
				"<h2>Request Number: <em>" << request.app().requs().size() << "</em></h2>\n";

			if (all) {
				request << "<h2 class='head'><a name='request'></a>Request Environment</h2>\n";
				penv(request, request.envp());
				//request << "<h2 class='head'><a name='process'></a>Process/Initial Environment</h2>\n";
				//penv(request, environ);
			}

			request << "<h2 class='head'><a name='handlers'></a>Page Handlers</h2>\n";
			handlers(request, app::Handlers::begin(), app::Handlers::end());

			request << "<h2 class='head'><a name='requests'></a>Page Requests</h2>\n";
			requests(request, request.app().requs());

			request << "<h2 class='head'><a name='cookies'></a>Page Cookies</h2>\n";
			cookies(request, request.cookieDebugData());

			request << "<h2 class='head'><a name='session'></a>Session</h2>\n";

			if (session.get())
			{
				char time[100];
				tyme::strftime(time, "%a, %d-%b-%Y %H:%M:%S GMT", tyme::gmtime(session->getStartTime()));
				request
					<< "<p>Current user: <a href=\"" << session->getEmail() << "\">" << session->getName() << "<br/>\n"
					<< "The session has started on " << time << ".</p>";
			}
			else
			{
				request << "<p>There is no session in progress currently.</p>\n";
			}
		}

	};
}
REGISTER_HANDLER("/debug/", app::DebugPageHandler);
REGISTER_REDIRECT("/debug", "/debug/");

#endif
