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

#ifndef __ARGS_HPP__
#define __ARGS_HPP__

#include <getopt.hpp>
#include <cctype>
#include <iostream>
#include <remote/respawn.hpp>

struct Args
{
	std::string command;
	std::string config;

	bool version = false;

	template <typename Action>
	int args(getopt::options::active_options<Action> action)
	{
		if (!action.add_arg("k", command).add("v", version).add_arg("c", config).act())
		{
			switch (action.cause())
			{
			default:
				std::cerr << "Internal error (" << action.cause() << ")";
				break;
			case getopt::NO_ARGUMENT:
				std::cerr << "Argument missing for " << action.cause_arg();
				break;
			case getopt::UNKNOWN:
				std::cerr << "Argument " << action.cause_arg() << " is unknown";
				break;
			}

			std::cerr << std::endl;
			return action.cause();
		};

		return 0;
	}

	int read(int argc, char* argv[])
	{
		return args(getopt::options::read(argc, argv));
	}

	int respawn(const remote::logger_ptr& logger, const std::string& address, int argc, char* argv[])
	{
		std::vector<std::string> moved{ argv[0] };
		int ret = args(getopt::options::sieve(argc, argv, moved, "kv"));
		if (ret)
			return ret;
		return remote::respawn::fcgi(logger, address, moved);
	}
};


#endif // __ARGS_HPP__
