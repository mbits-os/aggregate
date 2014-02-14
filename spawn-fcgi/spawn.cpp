// spawn.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
//#include <ws2tcpip.h>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <getopt.hpp>

namespace std
{
#ifdef UNICODE
	using tstring = std::wstring;
#else
	using tstring = std::string;
#endif
}

namespace getopt
{
#ifdef UNICODE
	using toptions = getopt::woptions;
#else
	using toptions = getopt::options;
#endif
}

namespace process
{
	void error_exit(LPTSTR lpszFunction, const char * file, int line)
	{
		// Retrieve the system error message for the last-error code

		LPVOID lpMsgBuf;
		DWORD dw = GetLastError() || WSAGetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, nullptr);

		// Display the error message and exit the process

		fprintf(stderr, "%s:%d: %S: (%u) %S\n", file, line, lpszFunction, dw, lpMsgBuf);
		fflush(stderr);

		LocalFree(lpMsgBuf);
		throw dw;
	}

	void print(const char* msg)
	{
		if (!msg || !*msg)
			return;

		fwrite(msg, 1, strlen(msg), stdout);
	}
}

#define ERR(s) process::error_exit(_T(s), __FILE__, __LINE__)

namespace net
{
	using sa_family_t = unsigned short int;
	constexpr auto UNIX_PATH_LEN = 108;
	constexpr auto FCGI_LISTENSOCK_FILENO = 0;

	struct sockaddr_un
	{
		sa_family_t	sun_family;              /* address family AF_LOCAL/AF_UNIX */
		char		sun_path[UNIX_PATH_LEN]; /* 108 bytes of socket address     */
	};

	/* ??? Evaluates the actual length of `sockaddr_un' structure. */
	inline size_t len(const sockaddr_un& p)
	{
		return (size_t)(((struct sockaddr_un *) nullptr)->sun_path) + strlen(p.sun_path);
	}

	void win32start()
	{
		WORD  wVersion = MAKEWORD(2, 0);
		WSADATA wsaData;

		if (WSAStartup(wVersion, &wsaData))
			ERR("error starting Windows sockets");
	}

	void check_if_used(sockaddr *fcgi_addr)
	{
		SOCKET fcgi_fd = -1;

		if (-1 == (fcgi_fd = socket(fcgi_addr->sa_family, SOCK_STREAM, 0)))
			ERR("socket");

		if (0 == connect(fcgi_fd, fcgi_addr, sizeof(sockaddr_in)))
			ERR("socket is already used, can't spawn");

		closesocket(fcgi_fd);
	}

	sockaddr* set(sockaddr_in& fcgi_addr_in, LPCSTR addr, unsigned short port)
	{
		fcgi_addr_in.sin_family = AF_INET;
		if (addr != nullptr) {
			fcgi_addr_in.sin_addr.s_addr = inet_addr(addr);
		}
		else {
			fcgi_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
		}
		fcgi_addr_in.sin_port = htons(port);
		return (sockaddr *) &fcgi_addr_in;
	}

	SOCKET open(LPCSTR addr, unsigned short port)
	{
		sockaddr_in fcgi_addr_in;

		sockaddr *fcgi_addr = set(fcgi_addr_in, addr, port);

		check_if_used(fcgi_addr);

		SOCKET fcgi_fd = -1;

		/* reopen socket */
		if (-1 == (fcgi_fd = socket(fcgi_addr_in.sin_family, SOCK_STREAM, 0)))
			ERR("socket");

		int val = 1;
		if (setsockopt(fcgi_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) < 0)
			ERR("setsockopt");

		/* create socket */
		if (-1 == bind(fcgi_fd, fcgi_addr, sizeof(fcgi_addr_in)))
			ERR("bind failed");

		if (-1 == listen(fcgi_fd, 1024))
			ERR("listen");

		return fcgi_fd;
	}

	DWORD children[256];
	int used = 0;

	void fcgi_spawn_connection(LPTSTR app, LPCSTR addr, unsigned short port, int child_count, bool wait = false)
	{
		win32start();

		SOCKET fcgi_fd = open(addr, port);

		if (child_count > 256)
			child_count = 256;

		HANDLE handles[256];
		used = child_count;

		/* server is not up, spawn in  */
		for (int i = 0; i < child_count; i++) {
			PROCESS_INFORMATION pi;
			STARTUPINFO si;

			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			si.dwFlags = STARTF_USESTDHANDLES;

			/* FastCGI expects the socket to be passed in hStdInput and the rest should be INVALID_HANDLE_VALUE */
			si.hStdOutput = /* GetStdHandle(STD_OUTPUT_HANDLE); /*/ INVALID_HANDLE_VALUE;
			si.hStdInput = (HANDLE)fcgi_fd;
			si.hStdError = INVALID_HANDLE_VALUE;

			if (!CreateProcess(nullptr, app, nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &si, &pi))
				ERR("CreateProcess failed");

			handles[i] = pi.hProcess;
			children[i] = pi.dwProcessId;

			ResumeThread(pi.hThread);

			printf("Process %d spawned successfully\n", pi.dwProcessId);
		}

		if (wait && WaitForMultipleObjects(child_count, handles, TRUE, INFINITE) == WAIT_FAILED)
			ERR("WaitForMultipleObjects failed");

		for (int i = 0; i < child_count; i++)
			CloseHandle(handles[i]);

		closesocket(fcgi_fd);
	}

	BOOL WINAPI breakAll(_In_  DWORD dwCtrlType)
	{
		switch (dwCtrlType)
		{
		case CTRL_BREAK_EVENT:
		case CTRL_C_EVENT:
			break;
		default:
			return FALSE;
		}

		for (int i = 0; i < used; ++i)
		{
			GenerateConsoleCtrlEvent(CTRL_C_EVENT, children[i]);
		}

		return TRUE;
	}
}

namespace spawn
{
	void show_version() {
		process::print("spawn-fcgi-win32 - spawns fastcgi processes\n");
	}

	void show_help() {
		process::print(
			"Usage: spawn-fcgi [options] -- \"<fcgiapp> [fcgi app arguments]\"\n"
			"\n"
			"spawn-fcgi-win32 - spawns fastcgi processes\n"
			"\n"
			"Options:\n"
			" -f <fcgiapp> filename of the fcgi-application\n"
			" -a <addr>    bind to ip address\n"
			" -p <port>    bind to tcp-port\n"
			" -C <childs>  numbers of childs to spawn (default 1)\n"
			" -v           show version\n"
			" -h           show this help\n"
			);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	std::tstring app;
	std::tstring dir;
	std::string addr;
	unsigned short port = 0;
	int child_count = 1;
	bool version = false, help = false;

	auto opts = getopt::toptions::read(argc, argv);
	opts
		.add_arg(_T("f"), app).add_arg(_T("d"), dir).add_arg(_T("a"), addr).add_arg(_T("p"), port).add_arg(_T("C"), child_count)
		.add(_T("v"), version).add(_T("h"), help);

	if (!opts.act())
	{
		auto arg = getopt::conv_impl<TCHAR, std::string>::convert(opts.cause_arg().c_str());
		switch (opts.cause())
		{
		default: process::print("spawn - Internal error\n"); break;
		case getopt::NO_ARGUMENT:
			process::print("spawn - Argument missing for ");
			process::print(arg.c_str());
			process::print("\n");
			spawn::show_help();
			break;
		case getopt::UNKNOWN:
			process::print("spawn - Argument ");
			process::print(arg.c_str());
			process::print(" is unknown\n\n");
			spawn::show_help();
			break;
		}
		return opts.cause();
	}

	if (opts.empty())
	{
		spawn::show_help();
		return getopt::MISSING;
	}

	if (help)
	{
		spawn::show_help();
		return 0;
	}

	if (version)
	{
		spawn::show_version();
		return 0;
	}

	auto&& args = opts.free_args();
	if (!args.empty())
		app = args.front();

	if (!dir.empty())
		SetCurrentDirectory(dir.c_str());

	SetConsoleCtrlHandler(net::breakAll, TRUE);

	try
	{
		net::fcgi_spawn_connection(&app[0], addr.c_str(), port, child_count);
		return 0;
	}
	catch(DWORD ret)
	{
		return ret;
	}
}
