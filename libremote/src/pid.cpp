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
#include <remote/pid.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name
#endif // _MSC_VER

namespace remote
{
	namespace { enum { SIZE = 40 }; }

	void strrev(char *str)
	{
		int i;
		int j;
		unsigned char a;
		unsigned len = strlen(str);

		for (i = 0, j = len - 1; i < j; i++, j--)
		{
			a = str[i];
			str[i] = str[j];
			str[j] = a;
		}
	}

	int itoa(int num, char* str, int len, int base = 10)
	{
		int sum = num;
		int i = 0;
		int digit;

		if (len == 0)
			return -1;

		do
		{
			digit = sum % base;

			if (digit < 0xA)
				str[i++] = '0' + digit;
			else
				str[i++] = 'A' + digit - 0xA;

			sum /= base;

		} while (sum && (i < (len - 1)));

		if (i == (len - 1) && sum)
			return -1;

		str[i] = '\0';
		strrev(str);

		return 0;
	}

	struct FD
	{
		int fd;
		FD(int fd) : fd(fd) {}
		~FD() { if (fd != -1) close(fd); }

		explicit operator bool() const { return fd >= 0; }
		size_t read(void* ptr, size_t len) const { return ::read(fd, ptr, len); }
		size_t write(const void* ptr, size_t len) const { return ::write(fd, ptr, len); }
	};

	pid::pid(const std::string& path) : m_path{ path }
	{
		FD fd{ open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0640) };
		if (!fd)
		{
			int err = errno;
			switch (err)
			{
			case EEXIST:
				throw std::runtime_error("PID file already exists.");
			}
			throw std::runtime_error("Cannot open PID file.");
		}

		char buffer[SIZE];
		itoa(_getpid(), buffer, SIZE);

		fd.write(buffer, strlen(buffer));
	}

	pid::~pid()
	{
		std::remove(m_path.c_str());
	}

	bool pid::read(const std::string& path, int& _pid)
	{
		FD fd{ open(path.c_str(), O_RDONLY) };
		if (!fd)
		{
			int err = errno;
			return false;
		}

		struct stat st;
		if (stat(path.c_str(), &st))
			return false;

		auto size = st.st_size;
		if (size > SIZE)
			return false;

		char buffer[SIZE];
		auto ret = fd.read(buffer, size);

		_pid = 0;
		for (size_t i = 0; i < ret; ++i)
		{
			switch (buffer[i])
			{
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				_pid *= 10;
				_pid += buffer[i] - '0';
				break;
			default:
				return false;
			}
		}

		return true;
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER
