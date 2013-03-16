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

#include "fast_cgi.h"
#include <stdlib.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
extern char ** environ;
#endif

namespace FastCGI {
	namespace {
		enum {
			STDIN_MAX = 0x100000 ///< Maximum number of bytes allowed to be read from stdin (1 MiB)
		};
	}
	Application::Application()
	{
		m_pid = _getpid();
		m_cin_streambuf  = std::cin.rdbuf();
		m_cout_streambuf = std::cout.rdbuf();
		m_cerr_streambuf = std::cerr.rdbuf();
	}

	Application::~Application()
	{
		std::cin.rdbuf(m_cin_streambuf);
		std::cout.rdbuf(m_cout_streambuf);
		std::cerr.rdbuf(m_cerr_streambuf);
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
		return FCGX_Accept_r(&m_request) == 0;
	}

	const std::streamsize Request::stdin_max = STDIN_MAX;
	Request::Request(Application& app)
		: m_request(app.m_request)
        , m_cin_fcgi_streambuf(app.m_request.in)
        , m_cout_fcgi_streambuf(app.m_request.out)
        , m_cerr_fcgi_streambuf(app.m_request.err)
		, m_content(NULL)
		, m_content_size(0)
	{
        std::cin.rdbuf(&m_cin_fcgi_streambuf);
        std::cout.rdbuf(&m_cout_fcgi_streambuf);
        std::cerr.rdbuf(&m_cerr_fcgi_streambuf);
	}

	Request::~Request()
	{
		delete [] m_content;
	}

	std::streamsize Request::readContents(bool save)
	{
		if (m_content_size != 0)
			return m_content_size;

		char * sizestr = FCGX_GetParam("CONTENT_LENGTH", m_request.envp);
		m_content_size = STDIN_MAX;

		if (sizestr)
		{
			m_content_size = strtol(sizestr, &sizestr, 10);
			if (*sizestr)
			{
				std::cerr << "can't parse \"CONTENT_LENGTH="
					<< FCGX_GetParam("CONTENT_LENGTH", m_request.envp)
					<< "\"\n";
				m_content_size = STDIN_MAX;
			}

			// *always* put a cap on the amount of data that will be read
			if (m_content_size > STDIN_MAX) m_content_size = STDIN_MAX;

			if (save)
			{
				m_content = new char[(size_t)m_content_size];

				std::cin.read(m_content, m_content_size);
				m_content_size = std::cin.gcount();
			}
		}
		else
		{
			// *never* read stdin when CONTENT_LENGTH is missing or unparsable
			m_content = NULL;
			m_content_size = 0;
		}

		// Chew up any remaining stdin - this shouldn't be necessary
		// but is because mod_fastcgi doesn't handle it correctly.

		// ignore() doesn't set the eof bit in some versions of glibc++
		// so use gcount() instead of eof()...
		do std::cin.ignore(1024); while (std::cin.gcount() == 1024);

		return m_content_size;
	}
}