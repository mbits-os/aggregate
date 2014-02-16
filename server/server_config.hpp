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

using config::base::config_ptr;
using config::base::section_ptr;

struct ProxyConfig : config::base::config
{
	config_ptr m_proxy;

	section_ptr get_section(const std::string& name) override { return m_proxy->get_section(name); }
};

struct Config
{
	struct server_wrapper : config::wrapper::section
	{
		server_wrapper(const config::base::config_ptr& impl)
		: config::wrapper::section(impl, "Server")
		, address   (*this, "address",    "127.0.0.1:1337")
		, static_web(*this, "static-web", "http://static.reedr.net/")
		, pidfile   (*this, "pid",        "../reedr.pid")
		, user      (*this, "user"        )
		, group     (*this, "group"       )
		{
		}

		config::wrapper::setting<std::string> address;
		config::wrapper::setting<std::string> static_web;
		config::wrapper::setting<std::string> pidfile;
		config::wrapper::setting<std::string> user;
		config::wrapper::setting<std::string> group;
	};

	struct locale_wrapper : config::wrapper::section
	{
		locale_wrapper(const config::base::config_ptr& impl)
		: config::wrapper::section(impl, "Locale")
		, dir    (*this, "dir",     "../locales")
		, charset(*this, "charset", "../locales/charsets.db")
		{
		}
		config::wrapper::setting<std::string> dir;
		config::wrapper::setting<std::string> charset;
	};

	struct connection_wrapper : config::wrapper::section
	{
		connection_wrapper(const config::base::config_ptr& impl)
		: config::wrapper::section(impl, "Connection")
		, database(*this, "database", "conn.ini")
		, smtp    (*this, "SMTP",     "smtp.ini")
		{
		}
		config::wrapper::setting<std::string> database;
		config::wrapper::setting<std::string> smtp;
	};

	struct logs_wrapper : config::wrapper::section
	{
		logs_wrapper(const config::base::config_ptr& impl)
		: config::wrapper::section(impl, "Logs")
		, debug(*this,   "debug", "../logs/reedr.log")
		, access(*this, "access", "../logs/access.log")
		{
		}
		config::wrapper::setting<std::string> debug;
		config::wrapper::setting<std::string> access;
	};

	Config(const config::base::config_ptr& impl)
		: server(impl)
		, locale(impl)
		, connection(impl)
		, logs(impl)
	{
	}

	server_wrapper server;
	locale_wrapper locale;
	connection_wrapper connection;
	logs_wrapper logs;
};


#endif // __SERVER_CONFIG_HPP__
