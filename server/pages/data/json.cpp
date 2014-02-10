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
#include "json.hpp"

long long asctoll(const char* ptr, char** end)
{
	if (!ptr) return 0;
	if (!*ptr)
	{
		if (end) *end = (char*)ptr;
		return 0;
	}

	long long out = 0;
	int sign = 1;
	const char* e = ptr + strlen(ptr);
	if (ptr < e && (*ptr == '+' || *ptr == '-'))
	{
		if (*ptr == '-') sign = -1;
		++ptr;
	}
	while (ptr < e)
	{
		if (*ptr >= '0' && *ptr <= '9')
		{
			out *= 10;
			out += *ptr - '0';
		}
		else
		{
			if (end) *end = (char*)ptr;
			break;
		}
		++ptr;
	}

	if (sign == -1)
		out = -out;
	return out;
}

namespace json
{
	std::string escape(const char* in)
	{
		if (!in)
			return "null";

		std::string out;
		size_t len = strlen(in);
		out.reserve(len * 110 / 100);
		out.push_back('"');
		std::for_each(in, in + len, [&out](char c)
		{
			switch(c)
			{
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '/': out += "\\/"; break;
			case '\b': out += "\\b"; break;
			case '\f': out += "\\f"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			default:
				//unicode charcters...
				out.push_back(c);
			}
		});
		out.push_back('"');
		return out;
	}

	std::string escape(const std::string& in)
	{
		if (in.empty())
			return "null";

		std::string out;
		out.reserve(in.length() * 110 / 100);
		out.push_back('"');
		for (auto& c: in)
		{
			switch(c)
			{
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '/': out += "\\/"; break;
			case '\b': out += "\\b"; break;
			case '\f': out += "\\f"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			default:
				//unicode charcters...
				out.push_back(c);
			}
		};
		out.push_back('"');
		return out;
	}
}
