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
#include <remote/signals.hpp>

namespace remote
{
	namespace posix
	{
		static logger_ptr s_log;

		struct mapping_t
		{
			const char* name;
			int signal;
			signal_t function;
		} mapping[] = {
			{ "stop", SIGTERM }
		};

		class signals : public os::signals
		{
			static mapping_t* find(const char* sig)
			{
				for (auto& m : mapping)
				{
					if (!strcmp(sig, m.name))
						return &m;
				}
				return nullptr;
			}

			static mapping_t* find(int sig)
			{
				for (auto& m : mapping)
				{
					if (sig == m.signal)
						return &m;
				}
				return nullptr;
			}

			static void function(int sig)
			{
				auto map = find(sig);
				if (!map || !map->function)
					return;

				LOG(s_log) << "Signalled " << map->signal << "/" << map->name << "...";
				map->function();
			}

		public:
			bool set(const char* sig, const signal_t& fn) override
			{
				auto* map = find(sig);
				if (!map)
					return false;

				LOG(s_log) << "Setting " << map->signal << "/" << map->name << "...";
				map->function = fn;
				::signal(map->signal, signals::function);
				return true;
			}

			bool signal(const char* sig, int pid) override
			{
				auto* map = find(sig);
				if (!map)
					return false;

				LOG(s_log) << "Sending " << map->signal << "/" << map->name << " to " << pid << "...";
				return !::kill(pid, map->signal);
			}

			void cleanup()
			{
				s_log.reset();
			}
		};
	}

	namespace os
	{
		signals_ptr signals::create(const logger_ptr& log)
		{
			posix::s_log = log;
			return std::make_shared<posix::signals>();
		}
	}
}
