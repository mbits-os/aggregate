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
#include "crypt.hpp"
#include <utils.h>

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
	SessionPtr Session::fromDB(db::ConnectionPtr db, const char* sessionId)
	{
		/*
		I want:

		query = schema
			.select("user._id AS _id", "user.name AS name", "user.email AS email", "session.set_on AS set_on")
			.leftJoin("session", "user")
			.where("session.has=?");

		query.bind(0, sessionId).run(db);
		if (c.next())
		{
			c[0].as<long long>();
			c["_id"].as<long long>();
		}
		*/
		db::StatementPtr query = db->prepare(
			"SELECT user._id AS _id, user.name AS name, user.email AS email, session.set_on AS set_on "
			"FROM session "
			"LEFT JOIN user ON (user._id = session.user_id) "
			"WHERE session.hash=?"
			);

		if (query.get() && query->bind(0, sessionId))
		{
			db::CursorPtr c = query->query();
			if (c.get() && c->next())
			{
				return SessionPtr(new (std::nothrow) Session(
					c->getLongLong(0),
					c->getText(1),
					c->getText(2),
					sessionId,
					c->getTimestamp(3)));
			}
		}
		return SessionPtr();
	}

	SessionPtr Session::startSession(db::ConnectionPtr db, const char* email)
	{
		tyme::time_t now = tyme::now();
		char seed[20];
		crypt::session_t sessionId;
		crypt::newSalt(seed);
		crypt::session(seed, sessionId);

		db::StatementPtr query = db->prepare(
			"SELECT _id, name "
			"FROM user "
			"WHERE email=?"
			);

		if (query.get() && query->bind(0, sessionId))
		{
			db::CursorPtr c = query->query();
			if (c.get() && c->next())
			{
				long long _id = c->getLongLong(0);
				std::string name = c->getText(1);
				c = db::CursorPtr();
				query = db->prepare(
					"INSERT INTO session (hash, seed, user_id, set_on) VALUES (?, ?, ?, ?)"
					);
				if (query.get() &&
					query->bind(0, sessionId) &&
					query->bind(0, seed) &&
					query->bind(0, _id) &&
					query->bind(0, now)
					)
				{
					if (query->execute())
					{
						return SessionPtr(new (std::nothrow) Session(
							_id,
							name,
							email,
							sessionId,
							now
							));
					}
				}
			}
		}
		return SessionPtr();
	}

	void Session::endSession(db::ConnectionPtr db, const char* sessionId)
	{
		db::StatementPtr query = db->prepare("DELETE FROM session WHERE hash=?");
		if (query.get() && query->bind(0, sessionId))
			query->execute();
	}

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
			info.now = tyme::now();
			m_requs.push_back(info);
		}
#endif
		return ret;
	}

	db::ConnectionPtr Application::dbConn(Request& request)
	{
		// synchronized {

		//restart if needed
		if (!m_dbConn.get())
			m_dbConn = db::Connection::open(INI);
		else if (!m_dbConn->isStillAlive())
			m_dbConn->reconnect();

		if (!m_dbConn.get() || !m_dbConn->isStillAlive())
			request.on500();

		return m_dbConn;

		// synchronized }
	}

	void Application::cleanSessionCache()
	{
		tyme::time_t treshold = tyme::now() - 30 * 60;

		Sessions copy;
		std::for_each(m_sessions.begin(), m_sessions.end(), [&copy](const Sessions::value_type& pair)
		{
			copy.insert(pair);
		});

		m_sessions = copy;
	}

	SessionPtr Application::getSession(Request& request, const std::string& sessionId)
	{
		// synchronized {
		cleanSessionCache();

		SessionPtr out;
		Sessions::iterator _it = m_sessions.find(sessionId);
		if (_it == m_sessions.end() || !_it->second.second.get())
		{
			db::ConnectionPtr db = dbConn(request);
			if (db.get())
				out = Session::fromDB(db, sessionId.c_str());

			// TODO: limits needed, or DoS eminent
			if (out.get())
				m_sessions.insert(std::make_pair(sessionId, std::make_pair(tyme::now(), out)));
		}
		else
		{
			out = _it->second.second;
			_it->second.first = tyme::now(); // ping the session
		}

		return out;
		// synchronized }
	}

	SessionPtr Application::startSession(Request& request, const char* email)
	{
		// synchronized {
		cleanSessionCache();

		SessionPtr out;
		db::ConnectionPtr db = dbConn(request);
		if (db.get())
			out = Session::startSession(db, email);
		// TODO: limits needed, or DoS eminent
		if (out.get())
			m_sessions.insert(std::make_pair(out->getSessionId(), std::make_pair(out->getStartTime(), out)));

		return out;
		// synchronized }
	}

	void Application::endSession(Request& request, const std::string& sessionId)
	{
		db::ConnectionPtr db = dbConn(request);
		if (db.get())
			Session::endSession(db, sessionId.c_str());
		Sessions::iterator _it = m_sessions.find(sessionId);
		if (_it != m_sessions.end())
			m_sessions.erase(_it);
	}

	Request::Request(Application& app)
		: m_app(app)
		, m_headersSent(false)
		, m_streambufCin(app.m_request.in)
		, m_streambufCout(app.m_request.out)
		, m_cerr(app.m_request.err)
		, m_cin(&m_streambufCin)
		, m_cout(&m_streambufCout)
		, cerr(&m_cerr)
		, m_alreadyReadSomething(false)
	{
		unpackCookies();
	}

	Request::~Request()
	{
		readAll();
	}

	void Request::unpackCookies()
	{
		param_t HTTP_COOKIE = getParam("HTTP_COOKIE");
		if (!HTTP_COOKIE || !*HTTP_COOKIE)
			return;

		param_t end = HTTP_COOKIE + strlen(HTTP_COOKIE);
		param_t c = HTTP_COOKIE;
		while (c < end)
		{
			while (isspace((unsigned char)*c) && c < end) ++c;
			param_t name_start = c;
			while (!isspace((unsigned char)*c) && *c != '=' && c < end) ++c;
			std::string name(name_start, c);
			while (isspace((unsigned char)*c) && c < end) ++c;
			if (c >= end || *c != '=') break;
			++c;
			while (isspace((unsigned char)*c) && c < end) ++c;
			if (*c == '"')
			{
				const char* quot_end = nullptr;
				std::string value = url::quot_parse(c + 1, end - c - 1, &quot_end);
				if (quot_end && *quot_end)
					c = quot_end + 1;
				else
					break;
				if (name[0] != '$')
					m_reqCookies[name] = value;
			}
			else
			{
				param_t value_start = c;
				while (!isspace((unsigned char)*c) &&
					*c != ';' && *c != ',' && c < end) ++c;
				if (name[0] != '$')
					m_reqCookies[name] = std::string(value_start, c);
			}
			while (isspace((unsigned char)*c) && c < end) ++c;
			if (c >= end || (*c != ';' && *c != ',')) break;
			++c;
		};
	}

	void Request::readAll()
	{
		m_alreadyReadSomething = true;
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
				*this << "can't parse \"CONTENT_LENGTH=" << getParam("CONTENT_LENGTH") << "\"\n";
				return -1;
			}
			return ret;
		}
		return -1;
	}

	void Request::ensureInputWasRead()
	{
		if (m_alreadyReadSomething)
			return;
		readAll();
	}

	void Request::buildCookieHeader()
	{
		std::string cookies;

		std::string domAndPath = "; Version=1; Domain=";
		domAndPath += getParam("SERVER_NAME");
		domAndPath += "; Path=/; HttpOnly";

		bool first = true;
		ResponseCookies::const_iterator _cookie = m_respCookies.begin(), _cend = m_respCookies.end();
		for (; _cookie != _cend; ++_cookie)
		{
			if (first) first = false;
			else cookies += ", ";
			cookies += url::encode(_cookie->second.m_name) + "=";
			if (url::isToken(_cookie->second.m_value))
				cookies += _cookie->second.m_value;
			else
				cookies += "\"" + url::quot_escape(_cookie->second.m_value) + "\"";
			cookies += domAndPath;

			if (_cookie->second.m_expire != 0)
			{
				char buffer[256];
				tyme::tm_t tm = tyme::gmtime(_cookie->second.m_expire);
				tyme::strftime(buffer, "%a, %d-%b-%Y %H:%M:%S GMT", tm );
				cookies += "; Expires=";
				cookies += buffer;
			}
		}
		if (!cookies.empty())
			m_headers["set-cookie"] = "Set-Cookie: " + cookies;
	}

	void Request::printHeaders()
	{
		if (m_headersSent)
			return;

		m_headersSent = true;
		if (m_headers.find("content-type") == m_headers.end())
			m_headers["content-type"] = "Content-Type: text/html; charset=utf-8";

		buildCookieHeader();

		Headers::const_iterator _cur = m_headers.begin(), _end = m_headers.end();
		for (; _cur != _end; ++_cur)
		{
			*this << _cur->second << "\r\n";
		}
		*this << "\r\n";
	}

	void Request::setHeader(const std::string& name, const std::string& value)
	{
		if (m_headersSent)
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
		if (n == "set-cookie" || n == "set-cookie2")
			return; //not that API, use setcookie

		m_headers[n] = v;
	}

	void Request::setCookie(const std::string& name, const std::string& value, tyme::time_t expire)
	{
		if (m_headersSent)
		{
#if DEBUG_CGI
			*this << "<br/><b>Warning</b>: Cannot set a cookie after sending data to the browser (" << name << ": " << value << ")<br/><br/>";
#endif
			return;
		}
		std::string n(name);
		std::transform(n.begin(), n.end(), n.begin(), ::tolower);
		m_respCookies[n] = Cookie(name, value, expire);
	}

	std::string Request::serverUri(const std::string& resource, bool withQuery)
	{
		fcgi::param_t port = getParam("SERVER_PORT");
		fcgi::param_t server = getParam("SERVER_NAME");
		fcgi::param_t query = withQuery ? getParam("QUERY_STRING") : NULL;
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

	void Request::redirectUrl(const std::string& url)
	{
		setHeader("Location", url);
		*this
			<< "<h1>Redirection</h1>\n"
			<< "<p>The app needs to be <a href='" << url << "'>here</a>.</p>"; 
		die();
	}

	void Request::on404()
	{
		setHeader("Status", "404 Not Found");
		*this
			<< "<tt>404: Oops! (URL: " << getParam("REQUEST_URI") << ")</tt>";
#if DEBUG_CGI
		*this << "<br/>\n<a href='/debug/'>Debug</a>.";
#endif
		die();
	}

	void Request::on500()
	{
		setHeader("Status", "500 Internal Error");
		*this
			<< "<tt>500: Oops! (URL: " << getParam("REQUEST_URI") << ")</tt>";
#if DEBUG_CGI
		*this << "<br/>\n<a href='/debug/'>Debug</a>.";
#endif
		die();
	}

	SessionPtr Request::getSession(bool require)
	{
		SessionPtr out;
		param_t sessionId = getCookie("reader.login");
		if (sessionId && *sessionId)
			out = m_app.getSession(*this, sessionId);
		if (require && out.get() == nullptr)
		{
			std::string cont = serverUri(getParam("REQUEST_URI"), false);
			redirect("/auth/login?continue=" + url::encode(cont));
		}

		//if require and session two weeks old: /auth/login?continue=<this-url>&user=<email>

		return out;
	}

}