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
#include "server_config.hpp"
#include <exception>
#include <stdexcept>
#include <remote/signals.hpp>
#include <remote/pid.hpp>
#include <filesystem.hpp>

namespace fs = filesystem;

#define THREAD_COUNT 1

#ifdef _WIN32
#	define SEP_S "\\"
#	define APP_PATH "..\\"
#else
#	define SEP_S "/"
#	define APP_PATH "/usr/share/reedr/"
#endif

#define CONFIG_FILE  APP_PATH "confs" SEP_S "reedr.ini"

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

struct Main
{
	FastCGI::FLogSource log;
	remote::signals signals;
	Args args;
	std::shared_ptr<ProxyConfig> config_file;
	Config config{ config_file };

	Main()
		: signals{ std::make_shared<RemoteLogger>() }
		, config_file{ std::make_shared<ProxyConfig>() }
	{
	}

	int run(int argc, char* argv[])
	{
		RETURN_IF_ERROR(args.read(argc, argv));

		if (args.version)
			return version();

		bool cfg_needed = true;
		if (args.config.empty())
		{
			cfg_needed = false;
			args.config = CONFIG_FILE;
		}

		fs::path cfg{ args.config };
		if (cfg.is_relative())
			cfg = fs::absolute(cfg);

		config_file->m_proxy = config::base::file_config(cfg, cfg_needed);
		if (!config_file->m_proxy)
		{
			FLOG << "Could not open " << args.config;
			std::cerr << "Could not open " << args.config << std::endl;
			return 1;
		}

		log.open(debug_log());

#if 0
		std::cout
			<< "config.server.address: " << config.server.address
			<< "\nconfig.server.static_web: " << config.server.static_web
			<< "\nconfig.server.pidfile: " << config.server.pidfile << " -> " << path(config.server.pidfile)
			<< "\nconfig.connection.database: " << config.connection.database << " -> " << path(config.connection.database)
			<< "\nconfig.connection.smtp: " << config.connection.smtp << " -> " << path(config.connection.smtp)
			<< "\nconfig.locale.dir: " << config.locale.dir << " -> " << path(config.locale.dir)
			<< "\nconfig.locale.charset: " << config.locale.charset << " -> " << path(config.locale.charset)
			<< "\nconfig.logs.access: " << config.logs.access << " -> " << path(config.logs.access)
			<< "\nconfig.logs.debug: " << config.logs.debug << " -> " << path(config.logs.debug)
			<< std::endl;
#endif

		if (!args.command.empty())
			return commands(argc, argv);

		if (!args.uri.empty())
			return run<Debug>();

		return run<FCGI>();
	}

	int version()
	{
		std::cout << http::getUserAgent() << std::endl;
		return 0;
	}

	std::string path(std::string file)
	{
		return fs::canonical(file, fs::path(args.config).parent_path()).native();
	}

	std::string pidfile() { return path(config.server.pidfile); }
	std::string charset() { return path(config.locale.charset); }
	std::string locale() { return path(config.locale.dir); }
	std::string debug_log() { return path(config.logs.debug); }
	std::string access_log() { return path(config.logs.access); }

	int commands(int argc, char* argv[])
	{
		for (auto& c : args.command)
			c = ::tolower((unsigned char)c);

		if (args.command == "start")
			return args.respawn(std::make_shared<RemoteLogger>(), config.server.address, argc, argv);

		int pid = -1;
		if (!remote::pid::read(pidfile(), pid))
		{
			std::cerr << pidfile() << " not found.\n";
			return 1;
		}

		signals.signal(args.command.c_str(), pid);
		return 0;
	}

	template <typename Runtime>
	int run()
	{
		try
		{
			remote::pid guard(pidfile());

			db::environment env;
			if (env.failed) return 1;

			http::init(charset().c_str());

			FastCGI::Application app;
			RETURN_IF_ERROR(app.init(locale().c_str()));

			app.setStaticResources(config.server.static_web);

			signals.set("stop", [&app](){ app.shutdown(); });

			return Runtime::run(*this, app);
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

	struct Debug
	{
		static int run(Main& service, FastCGI::Application& app)
		{
			Thread local(service.args.uri.c_str());
			local.setApplication(app);

			app.addStlSession();

			std::cout << "Enter the input stream and finish with ^Z:" << std::endl;
			local.handleRequest();
			return 0;
		}
	};

	struct FCGI
	{
		static int run(Main& service, FastCGI::Application& app)
		{
			if (!app.addThreads<Thread>(THREAD_COUNT))
				return 1;

			FLOG << "Application started";

			app.run();

			FLOG << "Application stopped";
			return 0;
		}
	};
};

int main (int argc, char* argv[])
{
	Main service;

	return service.run(argc, argv);
}
