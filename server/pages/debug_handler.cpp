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
#include <handlers.hpp>
#include <crypt.hpp>
#include "data/api_handler.hpp"

#if DEBUG_CGI

#ifdef _WIN32
#define SEP ';'
#else
#define SEP ':'
extern char ** environ;
#endif

namespace FastCGI { namespace app { namespace reader {

	class DebugPageHandler: public PageHandler
	{
		std::string m_service_url;
	public:
		struct DebugRequestState: FastCGI::RequestState
		{
			long long m_contentSize;
			long long m_read;
			char* m_buffer;
			DebugRequestState()
				: m_contentSize(0)
				, m_read(0)
				, m_buffer(nullptr)
			{}
			~DebugRequestState()
			{
				free(m_buffer);
			}
			bool alloc(long long size)
			{
				free(m_buffer);
				m_buffer = (char*)malloc((size_t)size);
				m_contentSize = m_buffer ? size : 0;
				m_read = 0;
				return m_buffer != nullptr;
			}
		};

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
				else if (eq == nullptr) request << *envp;
				else request << std::string(*envp, eq);
				request << "</td><td>";
				if (eq == nullptr) request << "&nbsp;";
				else if (strncmp("PATH=", *envp, 5))
				{
					++eq;
					if (*eq)
						request << eq;
					else
						request << "<em style=\"color: silver\">empty</em>";
				}
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

		static void handlers(FastCGI::Request& request,
			api::OperationMap::const_iterator begin,
			api::OperationMap::const_iterator end)
		{
			request << "<table class='operations'>\n";
			request << "<thead><tr><th>Name</th><th>Query</th><th>File</th></tr><thead><tbody>\n";
			size_t counter = 0;
			for ( ; begin != end; ++begin)
			{
				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				const char* ellipsis = "=<span style='color:silver'>\xE2\x80\xA6</span>";
				const char* join = "&amp;";
				request
					<< "><td><nobr>" << begin->first << "</a></nobr></td>"
					<< "<td><nobr>";

				bool first = true;
				const char** vars = begin->second.ptr->getVariables();
				while (*vars)
				{
					if (first) first = false;
					else request << join;
					request << *vars << "=<span style='color:silver;font-style:italic'>" << std::toupper(*vars) << "</span>";
					++vars;
				}

				request
					<< "</nobr></td><td>" << begin->second.file << ":"
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

		static void threads(FastCGI::Request& request, const std::list<FastCGI::ThreadPtr>& threadList)
		{
			request << "<table class='threads'>\n";
			request << "<thead><tr><th>Thread ID</th><th>Requests</th><thead><tbody>\n";
			size_t counter = 0;
			std::for_each(threadList.begin(), threadList.end(), [&request, &counter](FastCGI::ThreadPtr thread)
			{
				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request
					<< "><td><nobr>0x" << std::hex << thread->threadId() << std::dec << "</nobr></td>"
					<< "<td>" << thread->getLoad() << "</td></tr>\n";
			});
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
		const char* getPageTitle(Request&, PageTranslation& tr) { return "Debug"; }
		void prerender(FastCGI::SessionPtr session, Request& request, PageTranslation& tr)
		{
			long long content_size = request.calcStreamSize();
			if (content_size > -1)
			{
				DebugRequestState* state = new (std::nothrow) DebugRequestState();
				if (state)
				{
					if (state->alloc(content_size))
						state->m_read = request.read(state->m_buffer, state->m_contentSize);

					request.setRequestState(FastCGI::RequestStatePtr(state));
				}
			}
		}

		void render(FastCGI::SessionPtr session, Request& request, PageTranslation& tr)
		{
			bool all = request.getVariable("all") != nullptr;

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
				"<li><a href='#request'>Environment</a></li>\n"
				;
			request <<
				"<li><a href='#handlers'>Page Handlers</a></li>\n"
				"<li><a href='#operations'>Operation Handlers</a></li>\n"
				"<li><a href='#requests'>Requests</a></li>\n"
				"<li><a href='#threads'>Thread Load</a></li>\n"
				"<li><a href='#variables'>Variables</a></li>\n"
				"<li><a href='#cookies'>Cookies</a></li>\n"
				"<li><a href='#session'>Session</a></li>\n"
				"</ol>\n"
				"<h2>PID: <em>" << request.app().pid() << "</em></h2>\n"
				"<h2>Thread: <em>" << mt::Thread::currentId() << "</em></h2>\n"
				"<h2>Request Number: <em>" << request.app().requs().size() << "</em></h2>\n";
			FastCGI::RequestStatePtr statePtr = request.getRequestState();
			DebugRequestState* state = static_cast<DebugRequestState*>(statePtr.get());
			if (state && state->m_contentSize > 0)
			{
				request << "<h2>Data: <em>" << state->m_contentSize;
				if (state->m_contentSize != state->m_read)
					request << " (" << state->m_contentSize - state->m_read << " read)";
				request << "</em></h2>\n<pre>";
				for (long long i = 0; i < state->m_read; i++)
					request << state->m_buffer[i];
				request << "</pre>\n";
			}

			if (all) {
				request << "<h2 class='head'><a name='request'></a>Environment</h2>\n";
				penv(request, request.envp());
			}

			request << "<h2 class='head'><a name='handlers'></a>Page Handlers</h2>\n";
			handlers(request, app::Handlers::begin(), app::Handlers::end());

			request << "<h2 class='head'><a name='operations'></a>Operation Handlers</h2>\n";
			handlers(request, api::Operations::begin(), api::Operations::end());

			request << "<h2 class='head'><a name='requests'></a>Requests</h2>\n";
			requests(request, request.app().requs());

			request << "<h2 class='threads'><a name='threads'></a>Thread Load</h2>\n";
			threads(request, request.app().getThreads());

			request << "<h2 class='variables'><a name='variables'></a>Variables</h2>\n";
			cookies(request, request.varDebugData());

			request << "<h2 class='head'><a name='cookies'></a>Cookies</h2>\n";
			cookies(request, request.cookieDebugData());

			request << "<h2 class='head'><a name='session'></a>Session</h2>\n";

			if (session.get())
			{
				char time[100];
				tyme::strftime(time, "%a, %d-%b-%Y %H:%M:%S GMT", tyme::gmtime(session->getStartTime()));
				request
					<< "<p>Current user: <a href=\"mailto:" << session->getEmail() << "\">" << session->getLogin() << " (" << session->getName() << ")</a><br/>\n"
					<< "The session has started on " << time << ".</p>";
			}
			else
			{
				request << "<p>There is no session in progress currently.</p>\n";
			}
		}

	};
}}} // FastCGI::app::reader

REGISTER_HANDLER("/debug/", FastCGI::app::reader::DebugPageHandler);
REGISTER_REDIRECT("/debug", "/debug/");

#endif
