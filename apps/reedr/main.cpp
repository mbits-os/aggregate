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
#include <mail.hpp>
#include "args.hpp"
#include "server_config.hpp"
#include <exception>
#include <stdexcept>
#include <remote/signals.hpp>
#include <remote/pid.hpp>
#include <remote/identity.hpp>
#include <user_info.hpp>

namespace fs = filesystem;

#define THREAD_COUNT 1

#ifdef _WIN32
#	define CONFIG_FILE  APP_PATH "config/reedr.conf"
#else
#	define CONFIG_FILE  "/etc/reedr/reedr.conf"
#endif

//#define CONFIG_DBG

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

namespace FastCGI { namespace app { namespace reader { namespace errors {
	void setErrorHandlers(Application& app);
}}}}

struct Main
{
	FastCGI::FLogSource log;
	remote::signals signals;
	Args args;
	fs::path cfg_dir;
	Config config;

	Main()
		: signals{ std::make_shared<RemoteLogger>() }
	{
	}

	bool read_config(bool reloading)
	{
		bool cfg_needed = true;

		auto prev_log = config.logs.debug;

		fs::path cfg{ args.config };
		if (cfg.empty())
		{
			cfg_needed = false;
			cfg = CONFIG_FILE;
		}

		if (cfg.is_relative())
			cfg = fs::absolute(cfg);

		cfg_dir = cfg.parent_path();

		auto config_file = config::base::file_config(cfg, cfg_needed);
		if (!config_file)
		{
			FLOG << "Could not open " << args.config;
			std::cerr << "Could not open " << args.config << std::endl;
			return false;
		}
		ConfigINI config_ini{ config_file };
		config_file->set_read_only(true);


#ifdef CONFIG_DBG
		std::cout
			<< "\nconfig.server.address: " << config_ini.server.address
			<< "\nconfig.server.static_web: " << config_ini.server.static_web
			<< "\nconfig.server.user: " << config_ini.server.user
			<< "\nconfig.server.group: " << config_ini.server.group
			<< "\nconfig.server.pidfile: " << config_ini.server.pidfile << " -> " << canonical(config_ini.server.pidfile)
			<< "\nconfig.connection.database: " << config_ini.connection.database << " -> " << canonical(config_ini.connection.database)
			<< "\nconfig.connection.smtp: " << config_ini.connection.smtp << " -> " << canonical(config_ini.connection.smtp)
			<< "\nconfig.data.dir: " << config_ini.data.dir << " -> " << canonical(config_ini.data.dir)
			<< "\nconfig.data.locales: " << config_ini.data.locales
			<< "\nconfig.data.charset: " << config_ini.data.charset
			<< "\nconfig.logs.dir: " << config_ini.logs.dir << " -> " << canonical(config_ini.logs.dir)
			<< "\nconfig.logs.access: " << config_ini.logs.access
			<< "\nconfig.logs.debug: " << config_ini.logs.debug
			<< std::endl;
#endif

		config.server.address = config_ini.server.address;
		config.server.static_web = config_ini.server.static_web;
		config.server.pidfile = canonical(config_ini.server.pidfile);
		config.server.user = config_ini.server.user;
		config.server.group = config_ini.server.group;

		config.connection.database = canonical(config_ini.connection.database);
		config.connection.smtp = canonical(config_ini.connection.smtp);

		config.data.dir = canonical(config_ini.data.dir);
		config.data.locales = fs::canonical(config_ini.data.locales, config.data.dir);
		config.data.charset = fs::canonical(config_ini.data.charset, config.data.dir);

		config.logs.dir = canonical(config_ini.logs.dir);
		config.logs.access = fs::canonical(config_ini.logs.access, config.logs.dir);
		config.logs.debug = fs::canonical(config_ini.logs.debug, config.logs.dir);

#ifdef CONFIG_DBG
		std::cout
			<< "\nconfig.data.locales: " << config.data.locales
			<< "\nconfig.data.charset: " << config.data.charset
			<< "\nconfig.logs.access: " << config.logs.access
			<< "\nconfig.logs.debug: " << config.logs.debug
			<< std::endl;
#endif

		if (reloading)
			FLOG << "Reloading the config";

		if (reloading && prev_log != config.logs.debug)
			FLOG << "Moving to " << config.logs.debug;

		if (!log.open(config.logs.debug))
			std::cerr << "Could not open " << config.logs.debug << std::endl;

		if (reloading && prev_log != config.logs.debug)
			FLOG << "Moved from " << prev_log;

		return true;
	}

	void reload(FastCGI::Application& app)
	{
		if (!read_config(true))
			return;

		http::reload(config.data.charset);
		mail::PostOffice::reload(config.connection.smtp);

		app.setStaticResources(config.server.static_web);
		app.setDataDir(config.data.dir);
		app.setDBConn(config.connection.database);
		app.setSMTPConn(config.connection.smtp);
		app.setAccessLog(config.logs.access);

		app.reload(config.data.locales);
	}

	int run(int argc, char* argv[])
	{
		RETURN_IF_ERROR(args.read(argc, argv));

		if (args.version)
			return version();

#ifdef CONFIG_DBG
		fs::path cfg{ args.config };
		if (cfg.empty())
			cfg = CONFIG_FILE;

		if (cfg.is_relative())
			cfg = fs::absolute(cfg);

		std::cout << "Config is: " << fs::canonical(cfg).native() << std::endl;
#endif

		if (!read_config(false))
			return 1;

		if (!args.command.empty())
			return commands(argc, argv);

		return run<FCGI>();
	}

	int version()
	{
		std::cout << http::getUserAgent() << std::endl;
		return 0;
	}

	fs::path canonical(const fs::path& file)
	{
		return fs::canonical(file, cfg_dir);
	}

	int commands(int argc, char* argv[])
	{
		for (auto& c : args.command)
			c = ::tolower((unsigned char)c);

		if (args.command == "start")
			return args.respawn(std::make_shared<RemoteLogger>(), config.server.address, argc, argv);

		int pid = -1;
		if (!remote::pid::read(config.server.pidfile.native(), pid))
		{
			std::cerr << config.server.pidfile << " not found.\n";
			return 1;
		}

		signals.signal(args.command.c_str(), pid);
		return 0;
	}

	bool impersonate()
	{
		std::string name = config.server.user;
		if (name.empty())
			return true;

		std::string group = config.server.group;

		switch (remote::change_identity(name.c_str(), group.c_str()))
		{
		case remote::identity::ok:
			return true;
		case remote::identity::no_access:
			FLOG << "Could not impersonate " << name << "/" << group;
			break;
		case remote::identity::name_unknown:
			FLOG << "User " << name << " unknown";
			break;
		case remote::identity::group_unknown:
			FLOG << "Group " << group << " unknown";
			break;
		case remote::identity::oom:
			FLOG << "Out of memory when trying to impersonate " << name << "/" << group;
			break;
		}

		return false;
	}

	struct banner
	{
		banner()
		{
			FLOG << "Application started";
		}
		~banner()
		{
			FLOG << "Application stopped";
		}
	};

	template <typename Runtime>
	int run()
	{
		try
		{
			remote::pid guard(config.server.pidfile.native());

			if (!impersonate())
				return 1;

			db::environment env;
			if (env.failed) return 1;

			banner b{};

			http::init(config.data.charset);
			mail::PostOffice::init(config.connection.smtp);

			FastCGI::Application app;
			RETURN_IF_ERROR(app.init(config.data.locales, std::make_shared<FastCGI::app::UserInfoFactory>()));

			FastCGI::app::reader::errors::setErrorHandlers(app);

			app.setStaticResources(config.server.static_web);
			app.setDataDir(config.data.dir);
			app.setDBConn(config.connection.database);
			app.setSMTPConn(config.connection.smtp);
			app.setAccessLog(config.logs.access);

			signals.set("stop", [&app](){ app.shutdown(); });
			signals.set("reload", [this, &app](){ reload(app); });

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

	struct FCGI
	{
		static int run(Main& service, FastCGI::Application& app)
		{
			if (!app.addThreads<Thread>(THREAD_COUNT))
				return 1;

			app.run();

			return 0;
		}
	};
};

int main (int argc, char* argv[])
{
	Main service;

	return service.run(argc, argv);
}
