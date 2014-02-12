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

struct Args
{
	std::string uri;
	std::string command;
	bool version = false;

	int read(int argc, char* argv[])
	{
		getopt::options opts;

		if (!opts.add_arg("u", uri).add_arg("k", command).add("v", version).read(argc, argv))
		{
			switch (opts.cause())
			{
			default:
				std::cerr << "Internal error (" << opts.cause() << ")";
				break;
			case getopt::NO_ARGUMENT:
				std::cerr << "Argument missing for " << opts.cause_arg();
				break;
			case getopt::UNKNOWN:
				std::cerr << "Argument " << opts.cause_arg() << " is unknown";
				break;
			}

			std::cerr << std::endl;
			return opts.cause();
		};

		return 0;
	}
};


#endif // __ARGS_HPP__
