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

#ifndef __SERVER_PCH_H__
#define __SERVER_PCH_H__

#include "fcgio.h"
#include <memory>
#include <list>
#include <map>
#include <string>
#include <algorithm>

#include <stdlib.h>

#include "../../translate/site_strings.hpp"

#if !defined(DEBUG_CGI)
#ifdef NDEBUG
#define DEBUG_CGI 0
#else
#define DEBUG_CGI 1
#endif
#endif

namespace std
{
	inline std::ostream& operator << (std::ostream& o, const std::string& str)
	{
		return o << str.c_str();
	}
}

#ifdef __CYGWIN__
namespace std
{
	inline std::string to_string(int i)
	{
		char buffer[64];
		sprintf(buffer, "%d", i);
		return buffer;
	}
}
#endif

#endif //__SERVER_PCH_H__