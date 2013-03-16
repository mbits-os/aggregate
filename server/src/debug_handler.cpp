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

#include "handlers.h"

#if DEBUG_CGI

#include <stdlib.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
extern char ** environ;
#endif
#include <time.h>

static int count = 0;

#ifdef WIN32
#define SEP ';'
#else
#define SEP ':'
#endif

namespace app
{
	class DebugPageHandler: public Handler
	{
		std::string m_service_url;
	public:

		std::string name() const
		{
			return "DEBUG_CGI";
		}

		static void penv(FastCGI::Response& response, const char * const * envp)
		{
			response << "<table class='env'>\n";
			response << "<thead><tr><th>Name</th><th>Value</th></tr><thead><tbody>\n";
			size_t counter = 0;
			for ( ; *envp; ++envp)
			{
				const char* eq = strchr(*envp, '=');
				response << "<tr";
				if (counter++ % 2)
					response << " class='even'";
				response << "><td>";
				if (eq == *envp) response << "&nbsp;";
				else if (eq == NULL) response << *envp;
				else response << std::string(*envp, eq);
				response << "</td><td>";
				if (eq == NULL) response << "&nbsp;";
				else if (strncmp("PATH=", *envp, 5))
					response << eq + 1;
				else 
				{
					const char* prev = eq + 1;
					const char* c = eq + 1;
					while (*c != 0)
					{
						while (*c != 0 && *c != SEP) ++c;
						response << std::string(prev, c);
						if (*c != 0) response << "<br/>\n";
						if (*c != 0) ++c;
						prev = c;
					}
				}
				response << "</td></tr>\n";
			}
			response << "</table>\n";
		}

		static void handlers(FastCGI::Response& response,
			HandlerMap::const_iterator begin,
			HandlerMap::const_iterator end)
		{
			response << "<table class='handlers'>\n";
			response << "<thead><tr><th>URL</th><th>Action</th><th>File</th></tr><thead><tbody>\n";
			size_t counter = 0;
			for ( ; begin != end; ++begin)
			{
				response << "<tr";
				if (counter++ % 2)
					response << " class='even'";
				response
					<< "><td><nobr><a href='" << begin->first << "'>" << begin->first << "</a></nobr></td>"
					<< "<td><nobr>" << begin->second.ptr->name() << "</nobr></td><td>" << begin->second.file << ":"
					<< begin->second.line << "</td></tr>\n";
			}
			response << "</tbody></table>\n";
		}

		static void requests(FastCGI::Response& response, const FastCGI::Application::ReqList& list)
		{
			response << "<table class='requests'>\n";
			response << "<thead><tr><th>URL</th><th>Remote Addr</th><th>Time (GMT)</th></tr><thead><tbody>\n";

			auto _cur = list.begin(), _end = list.end();
			size_t counter = 0;
			for ( ; _cur != _end; ++_cur)
			{
				tm gmt;
				_gmtime64_s( &gmt, &_cur->now );
				char timebuf[50];
				asctime_s(timebuf, &gmt);

				response << "<tr";
				if (counter++ % 2)
					response << " class='even'";
				response
					<< "><td><nobr><a href='http://" << _cur->server << _cur->resource << "'>" << _cur->resource << "</a></nobr></td>"
					<< "<td><nobr>" << _cur->remote_addr << ":" << _cur->remote_port << "</nobr></td><td>" << timebuf << "</td></tr>\n";
			}
			response << "</tbody></table>\n";
		}

		void visit(FastCGI::Request& request, FastCGI::Response& response)
		{
			response << "<style type='text/css'>\n"
				"body, td, th { font-family: Helvetica, Arial, sans-serif; font-size: 10pt }\n"
				"div#content { width: 650px; margin: 0px auto }\n"
				"th, td { text-align: left; vertical-align: top; padding: 0.2em 0.5em }\n"
				".even td { background: #ddd }\n"
				"th { font-weight: normal; color: white; background: #444; }\n"
				"table { width: auto; max-width: 100% }\n"
				"</style>\n"
				"<title>Debug page</title>\n<div id='content'>\n"
				"<h1>Debug page</h1>\n"
				"<h4>PID: " << request.app().pid() << "</h4>\n"
				"<h4>Request Number: " << request.app().requs().size() << "</h4>\n";

			response << "<h4 class='head'>Request Environment</h4>\n";
			penv(response, request.envp());

			response << "<h4 class='head'>Process/Initial Environment</h4>\n";
			penv(response, environ);

			response << "<h4 class='head'>Page Handlers</h4>\n";
			handlers(response, app::Handlers::begin(), app::Handlers::end());

			response << "<h4 class='head'>Page Requests</h4>\n";
			requests(response, request.app().requs());
		}

	};
}
REGISTER_HANDLER("/debug/", app::DebugPageHandler);
REGISTER_REDIRECT("/debug", "/debug/");

#endif
