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

#ifndef __SERVER_FAST_CFGI_H__
#define __SERVER_FAST_CFGI_H__

#include "fcgio.h"
#include <list>
#include <map>
#include <string>

#if !defined(DEBUG_HANDLERS)
#ifdef NDEBUG
#define DEBUG_CGI 0
#else
#define DEBUG_CGI 1
#endif
#endif

namespace std
{
	inline std::ostream& operator << (std::ostream& o, const std::string& str)
	{
		return o << str.c_str();
	}
}

namespace FastCGI
{
	class Request;
	class Response;

	class Application
	{
		friend class Request;
		friend class Response;

		long m_pid;
		FCGX_Request m_request;
	public:
		Application();
		~Application();
		int init();
		int pid() const { return m_pid; }
		bool accept();

#if DEBUG_CGI
		struct ReqInfo
		{
			std::string resource;
			std::string server;
			std::string remote_addr;
			std::string remote_port;
			time_t now;
		};
		typedef std::list<ReqInfo> ReqList;
		const ReqList& requs() const { return m_requs; }
	private:
		ReqList m_requs;
#endif
	};

	typedef const char* param_t;

	class FinishResponse {};

	class Response
	{
		typedef std::map<std::string, std::string> Headers;

		Request& m_req;
		Application& m_app;
		bool m_headers_sent;
		fcgi_streambuf m_streambuf;
		fcgi_streambuf m_cerr;
		Headers m_headers;
		std::ostream m_cout;
		void ensureInputWasRead();
		void printHeaders();
	public:
		std::ostream cerr;

		Response(Request& req, Application& app);
		~Response();
		const Application& app() const { return m_app; }
		const Request& req() const { return m_req; }

		void header(const std::string& name, const std::string& value);

		void die() { throw FinishResponse(); }

		std::string server_uri(const std::string& resource, bool with_query = true);
		void redirect_url(const std::string& url);
		void redirect(const std::string& resource, bool with_query = true)
		{
			redirect_url(server_uri(resource, with_query));
		}

		void on404();

		template <typename T>
		Response& operator << (const T& obj)
		{
			ensureInputWasRead();
			printHeaders();
			m_cout << obj;
			return *this;
		}
	};

	class Request
	{
		friend class Response;
		Response m_resp;
		Application& m_app;
		fcgi_streambuf m_streambuf;

		std::istream m_cin;
		mutable bool m_read_something;

		void readAll();
	public:
		Request(Application& app);
		~Request();
   		const char * const* envp() const { return m_app.m_request.envp; }
		const Application& app() const { return m_app; }
		long long calcStreamSize();
		param_t getParam(const char* name) const { return FCGX_GetParam(name, m_app.m_request.envp); }
		const Response& resp() const { return m_resp; }
		Response& resp() { return m_resp; }

		template <typename T>
		const Request& operator >> (T& obj) const
		{
			m_read_something = true;
			m_cin >> obj;
			return *this;
		}

		std::streamsize read(void* ptr, std::streamsize length)
		{
			m_read_something = true;
			m_cin.read((char*)ptr, length);
			return m_cin.gcount();
		}

		template<std::streamsize length>
		std::streamsize read(char* (&ptr)[length])
		{
			m_read_something = true;
			m_cin.read(ptr, length);
			return m_cin.gcount();
		}
	};
}

namespace fcgi = FastCGI;

#endif //__SERVER_FAST_CFGI_H__