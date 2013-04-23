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

#include <stdio.h>
#include <string.h>

#include <memory>
#include <iostream>
#include <algorithm>
#include <map>

#include <dbconn.hpp>
#include <utils.hpp>
#include "schema.hpp"
#include <http.hpp>

#ifdef WIN32
#include <windows.h>
#define CHARSET_PATH ".\\locales\\charset.db"
#else
#include <termios.h>
#include <unistd.h>
#define CHARSET_PATH "./locales/charset.db"
#endif

struct Command {
	const char* name;
	int (*command)(int, char* [], const db::ConnectionPtr&);
	bool needsConnection;
	Command(const char* name, int (*command)(int, char* [], const db::ConnectionPtr&), bool needsConnection = true)
		: name(name)
		, command(command)
		, needsConnection(needsConnection)
	{}
	int run(int argc, char* argv[], const db::ConnectionPtr& conn)
	{
		return command(argc, argv, conn);
	}
};

template <size_t N>
Command* get_cmmd(Command (&commands)[N], int argc, char* argv[])
{
	if (argc > 1)
		for (size_t i = 0; i < array_size(commands); ++i)
		{
			if (strcmp(argv[1], commands[i].name) == 0)
				return commands + i;
		}

		if (argc > 1)
			fprintf(stderr, "%s: unknown command: %s\n", argv[0], argv[1]);
		else
			fprintf(stderr, "%s: command missing\n", argv[0]);

		fprintf(stderr, "\nKnown commands are:\n");
		for (size_t i = 0; i < array_size(commands); ++i)
			fprintf(stderr, "\t%s\n", commands[i].name);

		return nullptr;
}

int status(int, char*[], const db::ConnectionPtr&);
int install(int, char*[], const db::ConnectionPtr&);
int backup(int, char*[], const db::ConnectionPtr&);
int restore(int, char*[], const db::ConnectionPtr&);
int refresh(int, char*[], const db::ConnectionPtr&); // in refresh.cpp
int fetch(int, char*[], const db::ConnectionPtr&); // in fetch.cpp
int opml_cmd(int, char*[], const db::ConnectionPtr&); // in fetch.cpp
int user(int, char*[], const db::ConnectionPtr&);

Command commands[] = {
	Command("status", status),
	Command("install", install),
	Command("backup", backup),
	Command("restore", restore),
	Command("refresh", refresh),
	Command("fetch", fetch, false),
	Command("opml", opml_cmd, false),
	Command("user", user)
};

int main(int argc, char* argv[])
{
	db::environment env;
	if (env.failed)
		return 1;

	http::init(CHARSET_PATH);

	char prog[] = "dbtool";
	argv[0] = prog;
	Command* command = get_cmmd(commands, argc, argv);
	db::ConnectionPtr conn;

	if (command == nullptr)
		return 1;

	if (command->needsConnection)
	{
		conn = db::Connection::open("conn.ini");
		if (conn.get() == nullptr)
		{
			fprintf(stderr, "%s: error connecting to the database\n", argv[0]);
			return 1;
		}
	}

	int ret = command->run(argc - 1, argv + 1, conn);
	if (ret != 0 && conn.get() != nullptr)
	{
		const char* error = conn->errorMessage();
		if (error && *error)
			fprintf(stderr, "DB message: %s\n", conn->errorMessage());
	}
}

int status(int, char*[], const db::ConnectionPtr& db)
{
	if (db != nullptr && db->isStillAlive())
	{
		printf("status: connection established\n");
		return 0;
	}
	return 1;
}

int install(int, char*[], const db::ConnectionPtr& dbConn)
{
	if (!db::model::Schema(dbConn).install())
	{
		fprintf(stderr, "install: error installing the schema\n");
		return 1;
	}
	printf("install: schema installed\n");
	return 0;
}

int backup(int, char*[], const db::ConnectionPtr&)
{
	printf("dbtool: %s: not implemented\n", __FUNCTION__);
	return 0;
}

int restore(int, char*[], const db::ConnectionPtr&)
{
	printf("dbtool: %s: not implemented\n", __FUNCTION__);
	return 0;
}

int user_add(int, char*[], const db::ConnectionPtr&);
int user_remove(int, char*[], const db::ConnectionPtr&);
int user_passwd(int, char*[], const db::ConnectionPtr&);

Command user_cmmd[] = {
	Command("add", user_add),
	Command("remove", user_remove),
	Command("passwd", user_passwd)
};

int user(int argc, char* argv[], const db::ConnectionPtr& conn)
{
	Command* command = get_cmmd(user_cmmd, argc, argv);

	if (command == nullptr)
		return 1;

	return command->run(argc - 1, argv + 1, conn);
}

int user_add(int argc, char* argv[], const db::ConnectionPtr& dbConn)
{
	if (argc < 4)
	{
		fprintf(stderr, "user: not enough params\n");
		fprintf(stderr, "user add <login> <mail> <name>\n");
		return 1;
	}

	if (!db::model::Schema(dbConn).addUser(argv[1], argv[2], argv[3]))
	{
		fprintf(stderr, "user: error adding new user\n");
		return 1;
	}
	fprintf(stderr, "user: user added\n");
	return 0;
}

int user_remove(int argc, char* argv[], const db::ConnectionPtr& dbConn)
{
	if (argc < 2)
	{
		fprintf(stderr, "user: not enough params\n");
		fprintf(stderr, "user remove <mail>\n");
		return 1;
	}

	char answer[10];
	printf("Remove user %s? [Y/n]: ", argv[1]); fflush(stdout);
	std::cin.getline(answer, sizeof(answer));

	std::transform(answer, answer + 10, answer, ::tolower);

	if (*answer && (strcmp(answer, "y") != 0) && (strcmp(answer, "yes") != 0))
		return 0;

	if (!db::model::Schema(dbConn).removeUser(argv[1]))
	{
		fprintf(stderr, "user: error removing the user\n");
		return 1;
	}
	fprintf(stderr, "user: user removed\n");
	return 0;
}

void stdinEcho(bool enable)
{
#ifdef WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
	DWORD mode;
	GetConsoleMode(hStdin, &mode);

	if( !enable )
		mode &= ~ENABLE_ECHO_INPUT;
	else
		mode |= ENABLE_ECHO_INPUT;

	SetConsoleMode(hStdin, mode );

#else
	struct termios tty;
	tcgetattr(STDIN_FILENO, &tty);
	if( !enable )
		tty.c_lflag &= ~ECHO;
	else
		tty.c_lflag |= ECHO;

	(void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

int user_passwd(int argc, char* argv[], const db::ConnectionPtr& dbConn)
{
	if (argc < 2)
	{
		fprintf(stderr, "user: not enough params\n");
		fprintf(stderr, "user passwd <mail>\n");
		return 1;
	}

	std::string passwd, verify;

	stdinEcho(false);
	printf("New password: "); fflush(stdout);
	std::cin >> passwd;
	printf("\nRepeat new password: "); fflush(stdout);
	std::cin >> verify;
	printf("\n");
	stdinEcho(true);

	if (passwd != verify)
	{
		fprintf(stderr, "user: passwords do not match\n");
		return 1;
	}

	if (!db::model::Schema(dbConn).changePasswd(argv[1], passwd.c_str()))
	{
		fprintf(stderr, "user: error changing password\n");
		return 1;
	}
	fprintf(stderr, "user: password changed\n");
	return 0;
}