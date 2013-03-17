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
#include "fast_cgi.h"

#ifdef _WIN32
#define INI "..\\conn.ini"
#endif

#ifdef POSIX
#define INI "../conn.ini"
#endif

#ifndef INI
#error There is no path to conn.ini defined...
#endif

namespace FastCGI
{
	Application::Application()
	{
		m_pid = _getpid();
	}

	Application::~Application()
	{
	}

	int Application::init()
	{
		int ret = FCGX_Init();
		if (ret != 0)
			return ret;

		FCGX_InitRequest(&m_request, 0, 0);

		return 0;
	}

	bool Application::accept()
	{
		bool ret = FCGX_Accept_r(&m_request) == 0;
#if DEBUG_CGI
		if (ret)
		{
			ReqInfo info;
			info.resource    = FCGX_GetParam("REQUEST_URI", m_request.envp);
			info.server      = FCGX_GetParam("SERVER_NAME", m_request.envp);
			info.remote_addr = FCGX_GetParam("REMOTE_ADDR", m_request.envp);
			info.remote_port = FCGX_GetParam("REMOTE_PORT", m_request.envp);
			time(&info.now);
			m_requs.push_back(info);
		}
#endif
		return ret;
	}

	db::ConnectionPtr Application::dbConn(Response& response)
	{
		//restart if needed
		if (!m_dbConn.get())
			m_dbConn = db::Connection::open(INI);
		else if (!m_dbConn->isStillAlive())
			m_dbConn->reconnect();

		if (!m_dbConn.get() || !m_dbConn->isStillAlive())
			response.on500();

		return m_dbConn;
	}

	Request::Request(Application& app)
		: m_resp(*this, app)
		, m_app(app)
        , m_streambuf(app.m_request.in)
		, m_cin(&m_streambuf)
		, m_read_something(false)
	{
	}

	Request::~Request()
	{
		readAll();
	}

	void Request::readAll()
	{
		m_read_something = true;
		// ignore() doesn't set the eof bit in some versions of glibc++
		// so use gcount() instead of eof()...
		do m_cin.ignore(1024); while (m_cin.gcount() == 1024);
	}

	long long Request::calcStreamSize()
	{
		param_t sizestr = getParam("CONTENT_LENGTH");
		if (sizestr)
		{
			char* ptr;
			long long ret = strtol(sizestr, &ptr, 10);
			if (*ptr)
			{
				m_resp << "can't parse \"CONTENT_LENGTH=" << getParam("CONTENT_LENGTH") << "\"\n";
				return -1;
			}
			return ret;
		}
		return -1;
	}

	Response::Response(Request& req, Application& app)
		: m_req(req)
		, m_app(app)
		, m_headers_sent(false)
        , m_streambuf(app.m_request.out)
        , m_cerr(app.m_request.err)
		, m_cout(&m_streambuf)
		, cerr(&m_cerr)
	{
	}

	Response::~Response()
	{
	}

	void Response::ensureInputWasRead()
	{
		if (m_req.m_read_something)
			return;
		m_req.readAll();
	}

	void Response::printHeaders()
	{
		if (m_headers_sent)
			return;

		m_headers_sent = true;
		if (m_headers.find("content-type") == m_headers.end())
			m_headers["content-type"] = "Content-Type: text/html; charset=utf-8";

		Headers::const_iterator _cur = m_headers.begin(), _end = m_headers.end();
		for (; _cur != _end; ++_cur)
		{
			*this << _cur->second << "\r\n";
		}
		*this << "\r\n";
	}

	void Response::header(const std::string& name, const std::string& value)
	{
		if (m_headers_sent)
		{
#if DEBUG_CGI
			*this << "<br/><b>Warning</b>: Cannot set header after sending data to the browser (" << name << ": " << value << ")<br/><br/>";
#endif
			return;
		}
		std::string v(name);
		v += ": ";
		v += value;
		std::string n(name);
		std::transform(n.begin(), n.end(), n.begin(), ::tolower);
		m_headers[n] = v;
	}

	std::string Response::server_uri(const std::string& resource, bool with_query)
	{
		fcgi::param_t port = m_req.getParam("SERVER_PORT");
		fcgi::param_t server = m_req.getParam("SERVER_NAME");
		fcgi::param_t query = with_query ? m_req.getParam("QUERY_STRING") : NULL;
		std::string url;

		if (server != NULL)
		{
			const char* proto = "http";
			if (port != NULL && strcmp(port, "443") == 0)
				proto = "https";
			url = proto;
			url += "://";
			url += server;
			if (port != NULL && strcmp(port, "443") != 0 && strcmp(port, "80") != 0)
			{
				url += ":";
				url += port;
			}
		}

		url += resource;

		if (query != NULL && *query != 0)
		{
			url += "?";
			url += query;
		}
		return url;
	}

	void Response::redirect_url(const std::string& url)
	{
		header("Location", url);
		*this
			<< "<h1>Redirection</h1>\n"
			<< "<p>The app needs to be <a href='" << url << "'>here</a>.</p>"; 
		die();
	}

	void Response::on404()
	{
		header("Status", "404 Not Found");
		*this
			<< "<tt>404: Oops! (URL: " << m_req.getParam("REQUEST_URI") << ")</tt>";
#if DEBUG_CGI
		*this << "<br/>\n<a href='/debug/'>Debug</a>.";
#endif
		die();
	}

	void Response::on500()
	{
		header("Status", "500 Internal Error");
		*this
			<< "<tt>500: Oops! (URL: " << m_req.getParam("REQUEST_URI") << ")</tt>";
#if DEBUG_CGI
		*this << "<br/>\n<a href='/debug/'>Debug</a>.";
#endif
		die();
	}

}