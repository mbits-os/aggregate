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
#include <iomanip>
#include "pages.hpp"

#if DEBUG_CGI

#ifdef _WIN32
#define SEP ';'
#else
#define SEP ':'
extern char ** environ;
#endif

long long asctoll(const char* ptr, char** end);

namespace FastCGI { namespace app { namespace reader {

	Request& operator << (Request& request, FrozenState::duration_t dur)
	{
		auto second = FrozenState::duration_t::period::den;
		auto ticks = dur.count();

		auto seconds = ticks / second;

		if (seconds > 59)
		{
			ticks -= (seconds / 60) * 60 * second;

			return request << seconds / 60 << '.' << ticks / second << '.' << std::setw(3) << std::setfill('0') << (ticks * 1000 / second) % 1000 << 's';
		}

		const char* freq = "s";
		if (ticks * 10 / second < 9)
		{
			freq = "ms";
			ticks *= 1000;
			if (ticks * 10 / second < 9)
			{
				freq = "&micro;s";
				ticks *= 1000;
				if (ticks * 10 / second < 9)
				{
					freq = "ns";
					ticks *= 1000;
				}
			}
		}

		return request << ticks / second << '.' << std::setw(3) << std::setfill('0') << (ticks * 1000 / second) % 1000 << freq;
	}

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
				delete [] m_buffer;
			}
			bool alloc(long long size)
			{
				free(m_buffer);
				m_buffer = new (std::nothrow) char[(size_t)size];
				m_contentSize = m_buffer ? size : 0;
				m_read = 0;
				return m_buffer != nullptr;
			}
		};

		DEBUG_NAME("DEBUG_CGI");

		static void penv(FastCGI::Request& request, const char * const * envp)
		{
			request << "<table class='env'>\r\n";
			request << "<thead><tr><th>Name</th><th>Value</th></tr><thead><tbody>\r\n";
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
				else request << url::htmlQuotes(*envp, eq - *envp);
				request << "</td><td>";
				if (eq == nullptr) request << "&nbsp;";
				else if (strncmp("PATH=", *envp, 5))
				{
					++eq;
					if (*eq)
						request << url::htmlQuotes(eq);
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
						request << url::htmlQuotes(prev, c - prev);
						if (*c != 0) request << "<br/>\r\n";
						if (*c != 0) ++c;
						prev = c;
					}
				}
				request << "</td></tr>\r\n";
			}
			request << "</table>\r\n";
		}

		static void handlers(FastCGI::Request& request, const HandlerMap& map)
		{
			request << "<table class='handlers'>\r\n";
			request << "<thead><tr><th>URL</th><th>Action</th><th>File</th></tr><thead><tbody>\r\n";
			size_t counter = 0;
			for (auto&& item : map)
			{
				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request
					<< "><td><nobr><a href='" << item.first << "'>" << item.first << "</a></nobr></td>"
					<< "<td><nobr>" << item.second.ptr->name() << "</nobr></td><td>" << item.second.file << ":"
					<< item.second.line << "</td></tr>\r\n";
			}
			request << "</tbody></table>\r\n";
		}

		static void handlers(FastCGI::Request& request, const api::OperationMap& map)
		{
			request << "<table class='operations'>\r\n";
			request << "<thead><tr><th>Name</th><th>Query</th><th>File</th></tr><thead><tbody>\r\n";
			size_t counter = 0;
			for (auto&& item : map)
			{
				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				const char* ellipsis = "=<span style='color:silver'>\xE2\x80\xA6</span>";
				const char* join = "&amp;";
				request
					<< "><td><nobr>" << item.first << "</a></nobr></td>"
					<< "<td><nobr>";

				bool first = true;
				const char** vars = item.second.ptr->getVariables();
				while (*vars)
				{
					if (first) first = false;
					else request << join;
					request << *vars << "=<span style='color:silver;font-style:italic'>" << std::toupper(*vars) << "</span>";
					++vars;
				}

				request
					<< "</nobr></td><td>" << item.second.file << ":"
					<< item.second.line << "</td></tr>\r\n";
			}
			request << "</tbody></table>\r\n";
		}

		static void requests(FastCGI::Request& request, const FastCGI::Application::ReqList& list)
		{
			request << "<table class='requests'>\r\n";
			request << "<thead><tr><th>URL</th><th>Remote Addr</th><th>Time (GMT)</th><th>Frozen</th></tr><thead><tbody>\r\n";

			size_t counter = 0;
			for (auto&& item : list)
			{
				if (item.resource.substr(0, 7) == "/debug/")
					continue;

				tyme::tm_t gmt = tyme::gmtime(item.now);
				char timebuf[100];
				tyme::strftime(timebuf, "%a, %d-%b-%Y %H:%M:%S GMT", gmt );

				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";

				auto short_res = item.resource;
				if (short_res.length() > 26)
					short_res = short_res.substr(0, 24) + "&#8230;";

				request
					<< "><td><nobr><a href='http://" << item.server << item.resource << "' title='" << item.resource << "'>" << short_res << "</a></nobr></td>"
					<< "<td><nobr>" << item.remote_addr << ":" << item.remote_port << "</nobr></td><td>" << timebuf << "</td><td>";

				if (item.icicle.empty())
					request << "<em style=\"color: silver\">empty</em>";
				else
				{
					FrozenState::duration_t dur;
					auto frozen = request.app().frozen(item.icicle);
					if (frozen)
						request << "<a href='/debug/?frozen=" + url::encode(item.icicle) + "' title='Took " << frozen->duration() << "'>Icicle</a>";
					else
						request << "<a href='/debug/?frozen=" + url::encode(item.icicle) + "' title='Took -.---ms'>Icicle</a>";
				}
				request
					<< "</td></tr>\r\n";
			}
			request << "</tbody></table>\r\n";
		}

		static void threads(FastCGI::Request& request, const std::list<FastCGI::ThreadPtr>& threadList)
		{
			request << "<table class='threads'>\r\n";
			request << "<thead><tr><th>Thread ID</th><th>Requests</th><thead><tbody>\r\n";
			size_t counter = 0;
			for (auto&& thread : threadList)
			{
				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request
					<< "><td><nobr>0x" << std::hex << thread->threadId() << std::dec << "</nobr></td>"
					<< "<td>" << thread->getLoad() << "</td></tr>\r\n";
			};
			request << "</tbody></table>\r\n";
		}

		static void cookies(FastCGI::Request& request, const std::map<std::string, std::string>& list)
		{
			request << "<table class='cookies'>\r\n";
			request << "<thead><tr><th>Name</th><th>Value</th></tr><thead><tbody>\r\n";

			size_t counter = 0;
			for (auto&& pair : list)
			{
				std::string value;
				if (pair.second.empty())
					value = "<em style=\"color: silver\">empty</em>";
				else
					value = url::htmlQuotes(pair.second);

				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request
					<< "><td><nobr>" << url::htmlQuotes(pair.first) << "</nobr></td>"
					<< "<td>" << value << "</td></tr>\r\n";
			}
			request << "</tbody></table>\r\n";
		}

		static void string_pair_list(FastCGI::Request& request, const std::vector<std::pair<std::string, std::string>>& list)
		{
			request << "<table class='cookies'>\r\n";
			request << "<thead><tr><th>Name</th><th>Value</th></tr><thead><tbody>\r\n";

			size_t counter = 0;
			for (auto&& pair : list)
			{
				std::string value;
				if (pair.second.empty())
					value = "<em style=\"color: silver\">empty</em>";
				else
					value = url::htmlQuotes(pair.second);

				request << "<tr";
				if (counter++ % 2)
					request << " class='even'";
				request
					<< "><td><nobr>" << url::htmlQuotes(pair.first) << "</nobr></td>"
					<< "<td>" << value << "</td></tr>\r\n";
			}
			request << "</tbody></table>\r\n";
		}

		bool restrictedPage() override { return false; }
		const char* getPageTitle(Request&, PageTranslation&) override { return "Debug"; }

		void prerender(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			auto force500 = request.getVariable("500");
			if (force500)
				request.on500("Debug forced to respond with 500");

			param_t loop = request.getVariable("loop");
			if (loop)
			{
				if (!*loop)
					request.redirect("/debug/?loop", false);
				long long nloop = asctoll(loop, nullptr);
				if (nloop > 0)
				{
					char buffer[100];
					sprintf(buffer, "/debug/?loop=%lld", (nloop - 1));
					request.redirect(buffer, false);
				}
			}

			long long content_size = request.calcStreamSize();
			if (content_size > -1)
			{
				auto state = std::make_shared<DebugRequestState>();
				if (state)
				{
					if (state->alloc(content_size))
						state->m_read = request.read(state->m_buffer, state->m_contentSize);

					request.setRequestState(state);
				}
			}
		}

		void header(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			request << "<!DOCTYPE html "
				"PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
				"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n"
				"<html>\r\n"
				"  <head>\r\n"
				"    <title>" << getTitle(request, tr) << "</title>\r\n"
				"    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\r\n"
				"    <meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"/>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/site.css\");</style>\r\n"
#if DEBUG_CGI
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/fd_icons.css\");</style>\r\n"
#endif
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/site-logo-big.css\");</style>\r\n"
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/debug.css\");</style>\r\n"
				"    <style type=\"text/css\" media=\"screen and (min-width: 700px)\">@import url(\"" << static_web << "css/debug-wide.css\");</style>\r\n"
				"  </head>\r\n"
				"  <body>\r\n"
				"    <div class='site-logo'>\r\n"
				"      <div>\r\n"
				"        <div class='logo'><a href='/'><img src='" << static_web << "images/auth_logo.png' alt='" << tr(lng::LNG_GLOBAL_PRODUCT) << "'/></a></div>\r\n"
				"        <div class='site'><a href='/'>" << tr(lng::LNG_GLOBAL_DESCRIPTION) << "</a></div>\r\n"
				"      </div>\r\n"
				"    </div>\r\n"
				"    <div id='content' class='form'>\r\n";
		}

		void footer(const SessionPtr& session, Request& request, PageTranslation& tr) override
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
			request <<
				"    </div> <!-- #content -->\r\n"
				"  </body>\r\n"
				"</html>\r\n";
		}

		void render(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			auto icicle = request.getVariable("frozen");
			FrozenStatePtr ptr;
			if (icicle)
				ptr = request.app().frozen(url::decode(icicle));

			if (ptr)
				render_icicle(session, request, tr, ptr);
			else
				render_basic(session, request, tr);
		}

		void render_basic(const SessionPtr& session, Request& request, PageTranslation& tr)
		{
			bool all = request.getVariable("all") != nullptr;
			auto frozen = request.app().frozen(request.getIcicle());

			request << 
				"<h1>Debug page</h1>\r\n"
				"<h2>Table of Contents</h2>\r\n"
				"<ol>\r\n";
			if (all) request <<
				"<li><a href='#request'>Environment</a></li>\r\n"
				;
			if (all && frozen) request <<
				"<li><a href='#response'>Response</a></li>\r\n"
				;
			request <<
				"<li><a href='#handlers'>Page Handlers</a></li>\r\n"
				"<li><a href='#operations'>Operation Handlers</a></li>\r\n"
				"<li><a href='#requests'>Requests</a></li>\r\n"
				"<li><a href='#threads'>Thread Load</a></li>\r\n"
				"<li><a href='#variables'>Variables</a></li>\r\n"
				"<li><a href='#cookies'>Cookies</a></li>\r\n"
				"<li><a href='#session'>Session</a></li>\r\n"
				"</ol>\r\n"
				"<h2>PID: <em>" << request.app().pid() << "</em></h2>\r\n"
				"<h2>Thread: <em>" << mt::Thread::currentId() << "</em></h2>\r\n"
				"<h2>Request Number: <em>" << request.app().requs().size() << "</em></h2>\r\n";
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
				request << "</pre>\r\n";
			}

			if (all)
			{
				request << "<h2 class='head'><a name='request'></a>Environment</h2>\r\n";
				penv(request, request.envp());

				if (frozen)
				{
					request << "<h2 class='head'><a name='response'></a>Response</h2>\r\n";
					string_pair_list(request, frozen->response());
				}
			}

			request << "<h2 class='head'><a name='handlers'></a>Page Handlers</h2>\r\n";
			handlers(request, app::Handlers::handlers());

			request << "<h2 class='head'><a name='operations'></a>Operation Handlers</h2>\r\n";
			handlers(request, api::Operations::operations());

			request << "<h2 class='head'><a name='requests'></a>Requests</h2>\r\n";
			requests(request, request.app().requs());

			request << "<h2 class='threads'><a name='threads'></a>Thread Load</h2>\r\n";
			threads(request, request.app().getThreads());

			request << "<h2 class='variables'><a name='variables'></a>Variables</h2>\r\n";
			cookies(request, request.varDebugData());

			request << "<h2 class='head'><a name='cookies'></a>Cookies</h2>\r\n";
			cookies(request, request.cookieDebugData());

			request << "<h2 class='head'><a name='session'></a>Session</h2>\r\n";

			if (session.get())
			{
				char time[100];
				tyme::strftime(time, "%a, %d-%b-%Y %H:%M:%S GMT", tyme::gmtime(session->getStartTime()));
				request
					<< "<p>Current user: <a href=\"mailto:" << session->getEmail() << "\">" << session->getLogin() << " (" << session->getName() << ")</a><br/>\r\n"
					<< "The session has started on " << time << ".</p>";
			}
			else
			{
				request << "<p>There is no session in progress currently.</p>\r\n";
			}
		}

		void render_icicle(const SessionPtr& session, Request& request, PageTranslation& tr, const FrozenStatePtr& frozen)
		{
			auto item_now = frozen->now();
			auto item_server = frozen->server();
			auto item_resource = frozen->resource();
			auto item_remote_addr = frozen->remote_addr();
			auto item_remote_port = frozen->remote_port();
			auto item_duration = frozen->duration();

			tyme::tm_t gmt = tyme::gmtime(item_now);
			char timebuf[100];
			tyme::strftime(timebuf, "%a, %d-%b-%Y %H:%M:%S GMT", gmt);

			request << "<div id='content'>\r\n"
				"<h1>Frozen debug page</h1>\r\n"
				"<h2>Table of Contents</h2>\r\n"
				"<ol>\r\n"
				"<li><a href='#request'>Environment</a></li>\r\n"
				"<li><a href='#response'>Response</a></li>\r\n"
				"<li><a href='#variables'>Variables</a></li>\r\n"
				"<li><a href='#cookies'>Cookies</a></li>\r\n"
				"<li><a href='#session'>Session</a></li>\r\n"
				"</ol>\r\n"
				"<h2>Resource:</h2>\r\n"
				"<dl>\n<dt>URI:</dt><dd><a href='http://" << item_server << item_resource << "'>" << item_resource << "</a></dd>\n<dt>Remote end:</dt><dd>" << item_remote_addr << ":" << item_remote_port << "</dd><dd>" << timebuf << "</dd>\r\n"
				"<dt>Rendered in:</dt><dd>" << item_duration << "</dd>\n</dl>\r\n";
#if 0
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
				request << "</pre>\r\n";
			}
#endif

			request << "<h2 class='head'><a name='request'></a>Environment</h2>\r\n";
			cookies(request, frozen->environment());

			request << "<h2 class='head'><a name='response'></a>Response</h2>\r\n";
			string_pair_list(request, frozen->response());

			request << "<h2 class='variables'><a name='variables'></a>Variables</h2>\r\n";
			cookies(request, frozen->get());

			request << "<h2 class='head'><a name='cookies'></a>Cookies</h2>\r\n";
			cookies(request, frozen->cookies());

			request << "<h2 class='head'><a name='session'></a>Session</h2>\r\n";

			auto id = frozen->session_user();
			if (!id.empty())
			{
				request
					<< "<p>User logged in: " << id << "<br/>\r\n";
			}
			else
			{
				request << "<p>There was no session in progress at that time.</p>\r\n";
			}
		}
	};
}}} // FastCGI::app::reader

REGISTER_HANDLER("/debug/", FastCGI::app::reader::DebugPageHandler);
REGISTER_REDIRECT("/debug", "/debug/");

#endif
