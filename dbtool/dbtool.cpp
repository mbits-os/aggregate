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

#include <stdio.h>
#include <string.h>

#include <memory>

#include <dbconn.h>

int status(int, char*[], const db::ConnectionPtr&);
int install(int, char*[], const db::ConnectionPtr&);
int backup(int, char*[], const db::ConnectionPtr&);
int restore(int, char*[], const db::ConnectionPtr&);
int refresh(int, char*[], const db::ConnectionPtr&);

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
} commands[] = {
	Command("status", status),
	Command("install", install),
	Command("backup", backup),
	Command("restore", restore),
	Command("refresh", refresh)
};

template<class T, size_t N> size_t array_size(T (&)[N]){ return N; }

int main(int argc, char* argv[])
{
	db::environment env;
	if (env.failed)
		return 1;

	Command* command = NULL;
	db::ConnectionPtr conn;

	if (argc > 1)
	{
		for (size_t i = 0; i < array_size(commands); ++i)
		{
			if (strcmp(argv[1], commands[i].name) == 0)
			{
				command = commands + i;
				break;
			}
		}
	}

	if (command == NULL)
	{
		if (argc > 1)
			fprintf(stderr, "dbtool: unknown command: %s\n", argv[1]);
		else
			fprintf(stderr, "dbtool: command missing\n", argv[1]);
		fprintf(stderr, "\nKnown commands are:\n");
		for (size_t i = 0; i < array_size(commands); ++i)
			fprintf(stderr, "\t%s\n", commands[i].name);

		return 1;
	}

	if (command->needsConnection)
	{
		conn = db::Connection::open("conn.ini");
		if (conn.get() == NULL)
		{
			fprintf(stderr, "dbtool: error connecting to the database\n");
			return 1;
		}
	}

	return command->run(argc - 1, argv + 1, conn);
}

int status(int, char*[], const db::ConnectionPtr& db)
{
	if (db != NULL && db->isStillAlive())
	{
		printf("dbtool: connection established\n");
		return 0;
	}
	return 1;
}

int install(int, char*[], const db::ConnectionPtr&)
{
	printf("dbtool: %s: not implemented\n", __FUNCTION__);
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

int refresh(int, char*[], const db::ConnectionPtr&)
{
	printf("dbtool: %s: not implemented\n", __FUNCTION__);
	return 0;
}
