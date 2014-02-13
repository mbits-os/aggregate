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

#ifndef __LIBREMOTE_LOGGER_HPP__
#define __LIBREMOTE_LOGGER_HPP__

#include <memory>
#include <stdarg.h>

namespace remote
{
	struct stream_logger
	{
		virtual ~stream_logger() {}
		virtual std::ostream& out() = 0;
	};

	using stream_logger_ptr = std::shared_ptr<stream_logger>;

	struct logger
	{
		virtual ~logger() {}
		virtual stream_logger_ptr line(const char* path, int line) = 0;
	};

	using logger_ptr = std::shared_ptr<logger>;

	class line_logger
	{
		stream_logger_ptr m_line;
	public:
		line_logger(const logger_ptr& logger, const char* path, int line) : m_line(logger->line(path, line))
		{
		}

		template <typename T>
		line_logger& operator << (const T& t)
		{
			m_line->out() << t;
			return *this;
		}
	};
}

#define LOG(logger) remote::line_logger{ logger, __FILE__, __LINE__ }

#endif // __LIBREMOTE_LOGGER_HPP__
