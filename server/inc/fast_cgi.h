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
#include "fcgi_config.h"  // HAVE_IOSTREAM_WITHASSIGN_STREAMBUF

namespace FastCGI {
	class Request;

	class Application {
		friend class Request;

		long m_pid;
		FCGX_Request m_request;
		std::streambuf * m_cin_streambuf;
		std::streambuf * m_cout_streambuf;
		std::streambuf * m_cerr_streambuf;
	public:
		Application();
		~Application();
		int init();
		int pid() const { return m_pid; }
		bool accept();
	};

	class Request {
		FCGX_Request& m_request;
        fcgi_streambuf m_cin_fcgi_streambuf;
        fcgi_streambuf m_cout_fcgi_streambuf;
        fcgi_streambuf m_cerr_fcgi_streambuf;

		char * m_content;
        std::streamsize m_content_size;
	public:
		Request(Application& app);
		~Request();
   		const char **envp() const { return (const char **)m_request.envp; }
		std::streamsize readContents(bool save);
		std::streamsize size() const { return m_content_size; }
		const char* contents() const { return m_content; }

		static const std::streamsize stdin_max;
	};
}

#endif //__SERVER_FAST_CFGI_H__