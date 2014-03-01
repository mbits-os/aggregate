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
#include <sys/wait.h>

namespace remote
{
	namespace os
	{
		socklib::socklib()
		{}
		socklib::~socklib()
		{}

		void close(int socket)
		{
			::close(socket);
		}

		void exec(int stdIn, const std::vector<std::string>& args)
		{
			size_t size = sizeof(char*) * (args.size() + 1);
			for (auto&& a : args)
				size += a.length() + 1;

			char* strings = new (std::nothrow) char[size];
			if (!strings)
				return;

			char** argv = (char**)strings;
			strings += sizeof(char*) * (args.size() + 1);

			char** curr = argv;
			for (auto&& a : args)
			{
				auto len = a.length();
				*curr++ = strings;
				memcpy(strings, a.c_str(), len);
				strings[len] = 0;
				strings += len + 1;
			}
			*curr = nullptr;

			if (stdIn != STDIN_FILENO)
			{
				close(STDIN_FILENO);
				dup2(stdIn, STDIN_FILENO);
				close(stdIn);
			}

			auto fd = open("/dev/null", O_RDWR);
			if (fd >= 0)
			{
				if (fd != STDERR_FILENO)
					dup2(fd, STDERR_FILENO);

				if (fd != STDOUT_FILENO)
					dup2(fd, STDOUT_FILENO);

				if (fd != STDOUT_FILENO && fd != STDERR_FILENO)
					close(fd);
			}

			//spawn-fcgi closes all sockets between STDERR and fd
			for (int i = 3; i < fd; i++)
			{
				if (i != STDIN_FILENO)
					close(i);
			}

			execv(argv[0], argv);
		}

		int fcgi(int stdIn, const std::vector<std::string>& args)
		{
			pid_t child = fork();
			if (child == 0)
				exec(stdIn, args);

			int status = -1;
			int ret = waitpid(child, &status, WNOHANG);
			if (!ret)
				printf("Process %d spawned successfully\n", child);

			return ret;
		}

	}
}
