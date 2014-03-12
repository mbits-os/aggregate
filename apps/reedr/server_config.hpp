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

#ifndef __SERVER_CONFIG_HPP__
#define __SERVER_CONFIG_HPP__

#include <string>
#include <config.hpp>
#include <filesystem.hpp>

#ifdef _WIN32
#	define SEP_S "\\"
#	define APP_PATH ::filesystem::app_directory() / ".." /
#else
#	define SEP_S "/"
#	define APP_PATH ::filesystem::path{"/usr/share/reedr"} /
#endif

using config::base::config_ptr;
using config::base::section_ptr;

namespace config
{
	namespace wrapper
	{
		template <>
		struct get_value<filesystem::path>
		{
			static filesystem::path helper(section& sec, const std::string& name, const filesystem::path& def_val)
			{
				return sec.get_string(name, def_val.string());
			}
		};

		template <>
		struct set_value<filesystem::path>
		{
			static void helper(section& sec, const std::string& name, const filesystem::path& val)
			{
				return sec.set_value(name, val.string());
			}
		};
	}
}

struct ConfigINI
{
	using path = filesystem::path;

	struct server_wrapper : config::wrapper::section
	{
		server_wrapper(const config::base::config_ptr& impl)
		: config::wrapper::section(impl, "Server")
		, address   (*this, "address",    "127.0.0.1:1337")
		, static_web(*this, "static-web", "http://static.reedr.net/")
		, pidfile   (*this, "pid",        APP_PATH "reedr.pid")
		, user      (*this, "user"        )
		, group     (*this, "group"       )
		{
		}

		config::wrapper::setting<std::string> address;
		config::wrapper::setting<std::string> static_web;
		config::wrapper::setting<path>        pidfile;
		config::wrapper::setting<std::string> user;
		config::wrapper::setting<std::string> group;
	};

	struct data_wrapper : config::wrapper::section
	{
		data_wrapper(const config::base::config_ptr& impl)
		: config::wrapper::section(impl, "Data")
		, dir    (*this, "dir",     APP_PATH "data")
		, locales(*this, "locales", "locales")
		, charset(*this, "charset", "locales/charsets.db")
		{
		}
		config::wrapper::setting<path> dir;
		config::wrapper::setting<path> locales;
		config::wrapper::setting<path> charset;
	};

	struct connection_wrapper : config::wrapper::section
	{
		connection_wrapper(const config::base::config_ptr& impl)
		: config::wrapper::section(impl, "Connection")
		, database(*this, "database", "connection.ini")
		, smtp    (*this, "SMTP",     "smtp.ini")
		{
		}
		config::wrapper::setting<path> database;
		config::wrapper::setting<path> smtp;
	};

	struct logs_wrapper : config::wrapper::section
	{
		logs_wrapper(const config::base::config_ptr& impl)
		: config::wrapper::section(impl, "Logs")
		, dir   (*this, "dir",    APP_PATH "logs")
		, debug (*this, "debug",  "reedr.log")
		, access(*this, "access", "access.log")
		{
		}
		config::wrapper::setting<path> dir;
		config::wrapper::setting<path> debug;
		config::wrapper::setting<path> access;
	};

	ConfigINI(const config::base::config_ptr& impl)
		: server(impl)
		, data(impl)
		, connection(impl)
		, logs(impl)
	{
	}

	server_wrapper server;
	data_wrapper data;
	connection_wrapper connection;
	logs_wrapper logs;
};

struct Config
{
	using path = filesystem::path;

	struct Server
	{
		std::string address;
		std::string static_web;
		path        pidfile;
		std::string user;
		std::string group;
	};

	struct Data
	{
		path dir;
		path locales;
		path charset;
	};

	struct Connection
	{
		path database;
		path smtp;
	};

	struct Logs
	{
		path dir;
		path debug;
		path access;
	};

	Server server;
	Data data;
	Connection connection;
	Logs logs;
};


#endif // __SERVER_CONFIG_HPP__
