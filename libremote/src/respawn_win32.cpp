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
#include <cctype>

namespace remote
{
	namespace os
	{
		void error_exit(LPSTR lpszFunction, const char * file, int line)
		{
			// Retrieve the system error message for the last-error code

			LPVOID lpMsgBuf;
			DWORD dw = GetLastError() || WSAGetLastError();

			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				dw,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&lpMsgBuf,
				0, nullptr);

			// Display the error message and exit the process
			std::ostringstream o;
			o << file << ':' << line << ": " << lpszFunction << ": (" << dw << ") " << (LPSTR)lpMsgBuf;

			LocalFree(lpMsgBuf);

			throw spawn_error(dw, o.str());
		}
#define ERR(f) os::error_exit(f, __FILE__, __LINE__)

		socklib::socklib()
		{
			WORD  wVersion = MAKEWORD(2, 0);
			WSADATA wsaData;

			if (WSAStartup(wVersion, &wsaData))
				ERR("error starting Windows sockets");
		}
		socklib::~socklib()
		{}

		void close(int socket)
		{
			closesocket(socket);
		}

		std::string shellescape(const std::string& arg)
		{
			std::string out;
			out.reserve(arg.length() * 11 / 10);

			for (auto&& c : arg)
			{
				if (c == '"')
					out.append("\"\"");
				else
					out.push_back(c);
			}

			return out;
		}

		int fcgi(int stdIn, const std::vector<std::string>& args)
		{
			PROCESS_INFORMATION pi;
			STARTUPINFOA si;

			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			si.dwFlags = STARTF_USESTDHANDLES;

			/* FastCGI expects the socket to be passed in hStdInput and the rest should be INVALID_HANDLE_VALUE */
			si.hStdOutput = /* GetStdHandle(STD_OUTPUT_HANDLE); /*/ INVALID_HANDLE_VALUE;
			si.hStdInput = (HANDLE)stdIn;
			si.hStdError = INVALID_HANDLE_VALUE;

			std::ostringstream o;
			bool first = true;
			for (auto&& arg : args)
			{
				if (first) first = false;
				else o << ' ';

				bool hasSpace = false;
				for (auto&& c : arg)
				{
					if (std::isspace((unsigned char)c))
					{
						hasSpace = true;
						break;
					}
				}

				if (hasSpace || arg.empty())
					o << '"' << shellescape(arg) << '"';
				else
					o << shellescape(arg);
			}

			std::string cmdLine = o.str().c_str();

			char appname[2048] = "";
			GetModuleFileNameA(nullptr, appname, sizeof(appname));
			appname[sizeof(appname)-1] = 0;

			if (!CreateProcessA(appname, &cmdLine[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &si, &pi))
			{
				ERR("CreateProcess failed");
			}

			// You can break here to attach to the spawned process
			ResumeThread(pi.hThread);

			printf("Process %d spawned successfully\n", pi.dwProcessId);

			CloseHandle(pi.hProcess);
		}
	}
}
