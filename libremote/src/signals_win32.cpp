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
	namespace win32
	{
		namespace {
			auto handle_deleter = [](HANDLE& obj)
			{
				if (obj)
					CloseHandle(obj);
				obj = nullptr;
			};
		}

		using unique_handle = std::unique_ptr<void, decltype(handle_deleter)>;
		inline unique_handle make_handle(HANDLE obj) { return unique_handle(obj, handle_deleter); }

		enum class EEventResult
		{
			OK,
			EXISTED,
			FAILED
		};

		class Event
		{
			unique_handle m_handle;
			signal_t m_function;
		public:
			Event() = default;
			Event(const Event&) = delete;
			Event& operator = (const Event&) = delete;
			Event(Event&& oth)
				: m_handle{ std::move(oth.m_handle) }
				, m_function{ std::move(oth.m_function) }
			{
			}
			Event& operator = (Event&& oth)
			{
				m_handle = std::move(oth.m_handle);
				m_function = std::move(oth.m_function);
				return *this;
			}

			static std::string make_name(const char* sig, int pid)
			{
				std::ostringstream o;
				o << "Global\\signal_" << (unsigned int)pid << '_' << sig;
				return o.str();
			}

			EEventResult create(const logger_ptr& log, const char* name)
			{
				HANDLE hEvent = CreateEventA(nullptr, TRUE, FALSE, name);
				DWORD ret = GetLastError();
				m_handle = make_handle(hEvent);

				if (!hEvent)
				{
					if (name)
						LOG(log) << "Creation of named event \"" << name << "\" failed with 0x" << std::hex << std::setw(8) << std::setfill('0') << ret;
					else
						LOG(log) << "Creation of unnamed event failed with 0x" << std::hex << std::setw(8) << std::setfill('0') << ret;
					return EEventResult::FAILED;
				}
				if (ret == ERROR_ALREADY_EXISTS)
					return EEventResult::EXISTED;
				return EEventResult::OK;
			}

			void set(const signal_t& fn)
			{
				m_function = fn;
			}

			bool signal()
			{
				return SetEvent(m_handle.get()) != FALSE;
			}

			signal_t call() const { return m_function; }
			HANDLE get() const { return m_handle.get(); }

			bool operator == (HANDLE h) const
			{
				return m_handle.get() == h;
			}
		};

		class signals : public os::signals
		{
			using map_t = std::map<std::string, Event>;
			using method_t = void (signals::*)();
			using methods_t = std::vector<signal_t>;
			using handles_t = std::vector<HANDLE>;

			std::mutex  m_mutex;
			map_t       m_events;
			handles_t   m_waitable;
			methods_t   m_ops;
			std::thread m_thread;
			std::string m_op_event;
			bool        m_shouldStop = false;
			logger_ptr  m_log;

			void op(method_t m)
			{
				op([this, m](){ (this->*m)(); });
			}

			void op(signal_t s)
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_ops.push_back(s);

				auto it = m_events.find(m_op_event);
				if (it != m_events.end())
					it->second.signal();
			}

			void next_op()
			{
				methods_t copy;
				{
					std::lock_guard<std::mutex> guard(m_mutex);
					copy.swap(m_ops);
				}

				for (auto&& op : copy)
					op();
			}

			void update_list()
			{
				std::lock_guard<std::mutex> guard(m_mutex);

				m_waitable.clear();

				size_t size = m_events.size();
				m_waitable.resize(size);
				if (m_waitable.size() != size)
				{
					m_waitable.clear();
					return;
				}

				std::transform(m_events.begin(), m_events.end(), m_waitable.begin(), [](auto&& pair) { return pair.second.get(); });
			}

			std::string __set(const char* name, const signal_t& fn)
			{
				std::lock_guard<std::mutex> guard(m_mutex);

				if (name && *name)
				{
					auto it = m_events.find(name);

					if (it != m_events.end())
					{
						it->second.set(fn);
						return it->first;
					}
				}

				Event ev;
				if (ev.create(m_log, name) == EEventResult::FAILED)
					return std::string();
				ev.set(fn);

				std::string _name;

				if (name && *name)
					_name = name;
				else
				{
					std::ostringstream o;
					o << "\\Unnamed#" << (ULONG_PTR)ev.get();
					_name = o.str();
				}

				m_events[_name] = std::move(ev);

				return _name;
			}

			void start()
			{
				if (m_thread.joinable())
					m_thread.detach();
				m_thread = std::thread([this]() { run(); });
			}

			void run()
			{
				while (!m_shouldStop)
				{
					handles_t copy;
					{
						std::lock_guard<std::mutex> guard(m_mutex);
						copy = m_waitable;
					}
					auto ret = WaitForMultipleObjects(copy.size(), copy.data(), FALSE, INFINITE);
					if (ret == WAIT_FAILED || ret == WAIT_ABANDONED_0)
						return;

					signal_t sig;
					if (ret < copy.size())
					{
						std::lock_guard<std::mutex> guard(m_mutex);
						auto handle = copy[ret];
						for (auto&& pair : m_events)
						{
							if (pair.second == handle)
							{
								sig = pair.second.call();
								break;
							}
						}
						ResetEvent(handle);
					}

					if (sig)
						sig();

				}
			}

			void stop()
			{
				op(&signals::stop_thread);
				if (m_thread.joinable())
					m_thread.join();
			}

			void stop_thread()
			{
				m_shouldStop = true;
			}
		public:
			signals(const logger_ptr& log) : m_log(log)
			{
				m_op_event = __set(nullptr, [this](){ next_op(); });
				if (m_op_event.empty())
					throw std::runtime_error("Signals did not started");

				update_list();
				start();
			}
			~signals()
			{
				stop();
			}

			bool set(const char* sig, const signal_t& fn) override
			{
				if (__set(Event::make_name(sig, _getpid()).c_str(), fn).empty())
					return false;

				op(&signals::update_list);

				return true;
			}

			bool signal(const char* sig, int pid) override
			{
				Event ev;
				if (ev.create(m_log, Event::make_name(sig, pid).c_str()) != EEventResult::FAILED)
					return ev.signal();

				return false;
			}
		};

	}

	namespace os
	{
		signals_ptr signals::create(const logger_ptr& log)
		{
			return std::make_shared<win32::signals>(std::forward<const logger_ptr&>(log));
		}
	}
}
