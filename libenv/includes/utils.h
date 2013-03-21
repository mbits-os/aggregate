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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>

template<class T, size_t N> size_t array_size(T (&)[N]){ return N; }

namespace url
{
	bool isToken(const unsigned char* in, size_t in_len);
	inline static bool isToken(const std::string& s)
	{
		return isToken((const unsigned char*)s.c_str(), s.size());
	}

	std::string encode(const unsigned char* in, size_t in_len);
	inline static std::string encode(const std::string& s)
	{
		return encode((const unsigned char*)s.c_str(), s.size());
	}

	std::string decode(const unsigned char* in, size_t in_len);
	inline static std::string decode(const std::string& s)
	{
		return decode((const unsigned char*)s.c_str(), s.size());
	}
}

#endif //__UTILS_H__
