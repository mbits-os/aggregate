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

#ifndef __API_HANDLER_H__
#define __API_HANDLER_H__

#include <fast_cgi.hpp>

namespace FastCGI { namespace app { namespace api
{
	struct APIOperation
	{
		virtual void render(SessionPtr session, Request& request, PageTranslation& tr) = 0;
		virtual const char** getVariables() const = 0;
	};
	typedef std::shared_ptr<APIOperation> APIOperationPtr;

#if DEBUG_CGI
	struct APIOperationDbgInfo
	{
		APIOperationPtr ptr;
		const char* file;
		size_t line;
	};

	typedef std::map<std::string, APIOperationDbgInfo> OperationMap;
#else
	typedef std::map<std::string, APIOperationPtr> OperationMap;
#endif

	class Operations
	{
		static Operations& get()
		{
			static Operations instance;
			return instance;
		}

		OperationMap m_handlers;
		APIOperationPtr _handler(Request& request);
		void _register(const std::string& command, const APIOperationPtr& ptr
#if DEBUG_CGI
			, const char* file, size_t line
#endif
			)
		{
#if DEBUG_CGI
			APIOperationDbgInfo info;
			info.ptr = ptr;
			info.file = file;
			info.line = line;
			m_handlers[command] = info;
#else
			m_handlers[command] = ptr;
#endif
		}
	public:
		static APIOperationPtr handler(Request& request)
		{
			return get()._handler(request);
		}

		static void registerRaw(const std::string& resource, APIOperation* rawPtr
#if DEBUG_CGI
			, const char* file, size_t line
#endif
			)
		{
			if (!rawPtr)
				return;

			APIOperationPtr ptr(rawPtr);
			get()._register(resource, ptr
#if DEBUG_CGI
				, file, line
#endif
				);
		}

#if DEBUG_CGI
		static const OperationMap& operations() { return get().m_handlers; }
#endif

	};

	template <typename PageImpl>
	struct OperationRegistrar
	{
		OperationRegistrar(const std::string& resource
#if DEBUG_CGI
			, const char* file, size_t line
#endif
			)
		{
			Operations::registerRaw(resource, new (std::nothrow) PageImpl()
#if DEBUG_CGI
				, file, line
#endif
				);
		}
	};
}}} //FastCGI::app::api

#define REGISTER_OPERATION(command, type) static FastCGI::app::api::OperationRegistrar<type> REGISTRAR_NAME(operation) (command REGISTRAR_ARGS)

#endif