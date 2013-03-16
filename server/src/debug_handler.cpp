/*
 *  A simple FastCGI application example in C++.
 *
 *  $Id: echo-cpp.cpp,v 1.10 2002/02/25 00:46:17 robs Exp $
 *
 *  Copyright (c) 2001  Rob Saccoccio and Chelsea Networks
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
