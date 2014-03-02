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

#include <new.hpp>

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
#include <fast_cgi/application.hpp>
#include <wiki.hpp>
#include "../server/server_config.hpp"

#ifdef WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#	define CONFIG_FILE  APP_PATH "config/reedr.conf"
#else
#	define CONFIG_FILE  "/etc/reedr/reedr.conf"
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
	{
		for (size_t i = 0; i < array_size(commands); ++i)
		{
			if (strcmp(argv[1], commands[i].name) == 0)
				return commands + i;
		}
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
int schema_version(int, char*[], const db::ConnectionPtr&);
int install(int, char*[], const db::ConnectionPtr&);
int downgrade(int, char*[], const db::ConnectionPtr&);
int backup(int, char*[], const db::ConnectionPtr&);
int restore(int, char*[], const db::ConnectionPtr&);
int refresh(int, char*[], const db::ConnectionPtr&); // in refresh.cpp
int fetch(int, char*[], const db::ConnectionPtr&); // in fetch.cpp
int opml_cmd(int, char*[], const db::ConnectionPtr&); // in fetch.cpp
int user(int, char*[], const db::ConnectionPtr&);
int wiki_cmd(int, char*[], const db::ConnectionPtr&);

Command commands[] = {
	Command("status", status),
	Command("schema-version", schema_version),
	Command("install", install),
	Command("downgrade", downgrade),
	Command("backup", backup),
	Command("restore", restore),
	Command("refresh", refresh),
	Command("fetch", fetch, false),
	Command("opml", opml_cmd, false),
	Command("user", user),
	Command("wiki", wiki_cmd, false),
};

bool get_conn_ini(int& argc, char**& argv, filesystem::path& cfg_path)
{
	if (argc >= 3)
	{
		if (!strcmp(argv[1], "-c"))
		{
			cfg_path = argv[2];
			argc -= 2;
			argv += 2;
			return true;
		}
	}

	if (argc >= 2)
	{
		if (!strncmp(argv[1], "-c", 2))
		{
			cfg_path = argv[1] + 2;
			argc -= 1;
			argv += 1;
			return true;
		}
	}

	cfg_path = CONFIG_FILE;
	return false;
}

namespace fs = filesystem;
bool open_cfg(int& argc, char**& argv, filesystem::path& charset, filesystem::path& dbConn, filesystem::path& debug)
{
	filesystem::path cfg;
	bool cfg_needed = get_conn_ini(argc, argv, cfg);

	if (cfg.is_relative())
		cfg = fs::absolute(cfg);

	auto config_file = config::base::file_config(cfg, cfg_needed);
	if (!config_file)
	{
		std::cerr << "Could not open " << cfg << std::endl;
		return false;
	}
	config_file->set_read_only(true);

	ConfigINI config{ config_file };

	cfg = cfg.parent_path();

	dbConn = fs::canonical(fs::path(config.connection.database), cfg);

	auto data = fs::canonical(fs::path(config.data.dir), cfg);
	charset = fs::canonical(fs::path(config.data.charset), data);

	auto logs = fs::canonical(fs::path(config.logs.dir), cfg);
	debug = fs::canonical(fs::path(config.logs.debug), logs);

	return true;
}

int main(int argc, char* argv[])
{
	FastCGI::FLogSource log;

	filesystem::path charset, dbConn, debug;

	if (!open_cfg(argc, argv, charset, dbConn, debug))
		return false;

	log.open(debug);

	db::environment env;
	if (env.failed)
		return 1;

	http::init(charset);

	char prog[] = "dbtool";
	argv[0] = prog;
	Command* command = get_cmmd(commands, argc, argv);
	db::ConnectionPtr conn;

	if (command == nullptr)
		return 1;

	if (command->needsConnection)
	{
		conn = db::Connection::open(dbConn);
		if (conn.get() == nullptr)
		{
			fprintf(stderr, "%s: error connecting to the database\n", argv[0]);
			return 1;
		}
	}

	int ret = command->run(argc - 1, argv + 1, conn);
	if (ret != 0 && conn.get() != nullptr)
		db::model::errorMessage("DB message", conn);
}

int status(int, char*[], const db::ConnectionPtr& db)
{
	if (db != nullptr && db->isStillAlive())
	{
		db::model::Schema schema{ db };
		auto ver = schema.version();
		if (ver == db::model::VERSION::CURRENT)
			printf("status: connection established (schema version: %d, current)\n", ver);
		else
			printf("status: connection established (schema version: %d)\n", ver);

		if (ver < db::model::VERSION::CURRENT)
		{
			printf("\n"
				"warning: current known schema version is %d\n"
				"warning: update by calling\n"
				"warning:    dbtool install\n",
				db::model::VERSION::CURRENT);
		}
		else if (ver > db::model::VERSION::CURRENT)
		{
			printf("\n"
				"error: current known schema version is %d\n"
				"error: using this database is not supported\n",
				db::model::VERSION::CURRENT);
			return 1;
		}

		return 0;
	}
	return 1;
}

int schema_version(int argc, char* argv[], const db::ConnectionPtr& dbConn)
{
	if (argc < 2)
	{
		fprintf(stderr, "schema-version: not enough params\n");
		fprintf(stderr, "user schema-version <version>\n");
		return 1;
	}

	long version = atoi(argv[1]);
	if (version < 0)
		version = 0;

	printf("schema-version: setting version %d\n", version);

	db::model::Schema schema{ dbConn };
	bool success = schema.version(version);

	if (!success || schema.version() != version)
	{
		fprintf(stderr, "schema-version: trying to force schema config\n");

		success = schema.force_schema_config();
		if (!success)
			return 1;

		success = schema.version(version);
		if (!success)
			return 1;
	}

	return 0;
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

int downgrade(int, char*[], const db::ConnectionPtr& dbConn)
{
	if (!db::model::Schema(dbConn).downgrade())
	{
		fprintf(stderr, "downgrade: error downgrading the schema\n");
		return 1;
	}
	printf("downgrade: schema downgraded\n");
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
int user_list(int, char*[], const db::ConnectionPtr&);

Command user_cmmd[] = {
	Command("add", user_add),
	Command("remove", user_remove),
	Command("passwd", user_passwd),
	Command("list", user_list)
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

int user_list(int, char*[], const db::ConnectionPtr& dbConn)
{
	db::model::Users users;
	if (!db::model::Schema{ dbConn }.getUsers(users))
	{
		fprintf(stderr, "user: error listing users\n");
		return 1;
	}

	size_t size1 = 0, size2 = 0;
	for (auto&& user : users)
	{
		auto len = user.login.length();
		if (len > size1) size1 = len;
		len = user.name.length();
		if (len > size2) size2 = len;
	}

	for (auto&& user : users)
	{
		printf("%-*s | %-*s | %s\n", size1, user.login.c_str(), size2, user.name.c_str(), user.email.c_str());
	}

	return 0;
}

int wiki_cmd(int argc, char* argv[], const db::ConnectionPtr&)
{
	if (argc < 2)
	{
		fprintf(stderr, "wiki: not enough params\n");
		fprintf(stderr, "dbtool wiki <path>\n");
		return 1;
	}

	wiki::document_ptr doc;
	auto path = filesystem::canonical(argv[1]);
	if (argc < 3)
		doc = wiki::compile(path);
	else
		doc = wiki::compile(path, filesystem::canonical(argv[2]));

	return doc ? 0 : 1;
}
