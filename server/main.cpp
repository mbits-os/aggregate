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
#include <dbconn.hpp>
#include <locale.hpp>
#include <string.h>
#include <http.hpp>
#include "args.hpp"
#include <exception>
#include <stdexcept>
#include <remote/signals.hpp>
#include <remote/pid.hpp>

#define THREAD_COUNT 1

#ifdef _WIN32
#define LOCALE_PATH "..\\locales\\"
#define CHARSET_PATH "..\\locales\\charset.db"
#define LOG_FILE "..\\reedr.log"
#define PID_FILE "..\\reedr.pid"
#endif

#ifdef POSIX
#define LOCALE_PATH "/usr/share/reedr/locales/"
#define LOG_FILE "/usr/share/reedr/reedr.log"
#define CHARSET_PATH "/usr/share/reedr/locales/charset.db"
#define PID_FILE "/usr/share/reedr/reedr.pid"
#endif

REGISTER_REDIRECT("/", "/view/");

class Thread: public FastCGI::Thread
{
	unsigned long m_load;
public:
	Thread(): m_load(0) {}
	Thread(const char* uri):  FastCGI::Thread(uri) {}
	void onRequest(FastCGI::Request& req)
	{
		++m_load;
		FastCGI::app::HandlerPtr handler = FastCGI::app::Handlers::handler(req);
		if (handler.get() != nullptr)
			handler->visit(req);
		else
			req.on404();
	}
	unsigned long getLoad() const
	{
		return m_load;
	}
};

#define RETURN_IF_ERROR(cmd) do { auto ret = (cmd); if (ret) return ret; } while (0)

static int send_command(std::string& cmd, remote::signals& signals)
{
	int pid = -1;
	if (!remote::pid::read(PID_FILE, pid))
	{
		std::cerr << PID_FILE << " not found.\n";
		return 1;
	}

	for (auto& c : cmd)
		c = ::tolower((unsigned char)c);

	signals.signal(cmd.c_str(), pid);
	return 0;
}

class RemoteLogger : public remote::logger
{
	class StreamLogger : public remote::stream_logger
	{
		FastCGI::ApplicationLog m_log;
	public:
		StreamLogger(const char* path, int line)
			: m_log{ path, line }
		{}

		std::ostream& out() override { return m_log.log(); }
	};
public:
	remote::stream_logger_ptr line(const char* path, int line) override
	{
		return std::make_shared<StreamLogger>(path, line);
	}
};

int main (int argc, char* argv[])
{
	FastCGI::FLogSource log(LOG_FILE);
	remote::signals signals{ std::make_shared<RemoteLogger>() };

	Args args;
	RETURN_IF_ERROR(args.read(argc, argv));

	if (args.version)
	{
		std::cout << http::getUserAgent() << std::endl;
		return 0;
	}

	if (!args.command.empty())
		return send_command(args.command, signals);

	try
	{
		FLOG << "Application started";

		remote::pid guard(PID_FILE);

		db::environment env;
		if (env.failed) return 1;

		http::init(CHARSET_PATH);

		FastCGI::Application app;
		RETURN_IF_ERROR(app.init(LOCALE_PATH));

		signals.set("stop", [&app](){ app.shutdown(); });

		if (!args.uri.empty())
		{
			Thread local(args.uri.c_str());
			local.setApplication(app);

			app.addStlSession();

			local.handleRequest();
			return 0;
		}

		if (!app.addThreads<Thread>(THREAD_COUNT))
			return 1;

		app.run();

		FLOG << "Application stopped";
		return 0;
	}
	catch (std::exception& ex)
	{
		std::cout << "error:" << ex.what() << std::endl;
		FLOG << "error: " << ex.what();
		return 1;
	}
	catch (...)
	{
		std::cout << "unknown error." << std::endl;
		FLOG << "unknown error";
		return 1;
	}
}
