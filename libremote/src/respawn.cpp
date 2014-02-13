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
#include <remote/respawn.hpp>

#ifdef POSIX
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

using SOCKET = int;
#endif

namespace remote
{
	void error_exit(const char* lpszFunction, const char * file, int line)
	{
		std::ostringstream o;
		o << file << ':' << line << ": " << lpszFunction;

		throw spawn_error(128, o.str());
	}
#define ERR(f) remote::error_exit(f, __FILE__, __LINE__)

	struct SocketAnchor
	{
		SOCKET fd;
		SocketAnchor() = delete;
		SocketAnchor(SOCKET fd) : fd(fd) {}
		~SocketAnchor() { if (fd != -1) os::close(fd); }

		explicit operator bool() const { return fd >= 0; }

		SOCKET release()
		{
			auto tmp = fd;
			fd = -1;
			return tmp;
		}
	};

	void check_if_used(sockaddr *fcgi_addr)
	{
		SocketAnchor fd{ socket(fcgi_addr->sa_family, SOCK_STREAM, 0) };

		if (!fd)
			ERR("socket");

		if (0 == connect(fd.fd, fcgi_addr, sizeof(sockaddr_in)))
			ERR("socket is already used, can't spawn");
	}

	sockaddr* set(sockaddr_in& fcgi_addr_in, const char* addr, unsigned short port)
	{
		fcgi_addr_in.sin_family = AF_INET;
		if (addr != nullptr) {
			fcgi_addr_in.sin_addr.s_addr = inet_addr(addr);
		}
		else {
			fcgi_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
		}
		fcgi_addr_in.sin_port = htons(port);
		return (sockaddr *)&fcgi_addr_in;
	}

	SOCKET open(const char* addr, unsigned short port)
	{
		sockaddr_in fcgi_addr_in;

		sockaddr *fcgi_addr = set(fcgi_addr_in, addr, port);

		check_if_used(fcgi_addr);

		SocketAnchor fd{ socket(fcgi_addr_in.sin_family, SOCK_STREAM, 0) };

		/* reopen socket */
		if (!fd)
			ERR("socket");

		int val = 1;
		if (setsockopt(fd.fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) < 0)
			ERR("setsockopt");

		/* create socket */
		if (-1 == bind(fd.fd, fcgi_addr, sizeof(fcgi_addr_in)))
			ERR("bind failed");

		if (-1 == listen(fd.fd, 1024))
			ERR("listen");

		return fd.release();
	}

	int respawn::fcgi(const logger_ptr& log, const std::string& addr, unsigned int port, const std::vector<std::string>& args)
	{
		try
		{
			os::socklib lib;
			SocketAnchor fd{ open(addr.c_str(), port) };

			return os::fcgi(fd.fd, args);
		}
		catch (spawn_error& err)
		{
			LOG(log) << err.what();

			return err.returnValue();
		}
		return 1;
	}
}