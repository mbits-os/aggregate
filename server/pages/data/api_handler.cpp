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
#include <handlers.hpp>
#include <utils.hpp>
#include "api_handler.hpp"
#include "json.hpp"

namespace FastCGI { namespace app { namespace api
{
	APIOperationPtr Operations::_handler(Request& request)
	{
		param_t op = request.getVariable("op");
		if (!op)
		{
			request.setHeader("Content-Type", "application/json; charset=utf-8");
			request.setHeader("Status", "400 Operation is missing");
			request << "{\"error\":\"Operation is missing\"}";
			request.die();
		}

		auto _it = m_handlers.find(op);
		if (_it == m_handlers.end()) return nullptr;
#if DEBUG_CGI
		return _it->second.ptr;
#else
		return _it->second;
#endif
	}
}}}

namespace FastCGI { namespace app { namespace reader {

	class ApiPageHandler: public Handler
	{
	public:
		std::string name() const { return "reedr API"; }

		void visit(Request& request)
		{
			api::APIOperationPtr ptr = api::Operations::handler(request);
			if (!ptr)
			{
				request.setHeader("Content-Type", "application/json; charset=utf-8");
				request.setHeader("Status", "404 Operation not recognized");
				request << "{\"error\":\"Operation not recognized\",\"op\":" << json::escape(request.getVariable("op")) << "}";
				request.die();
			}

			SessionPtr session = request.getSession();
			PageTranslation tr;
			if (!tr.init(session, request))
				request.on500();

			ptr->render(session, request, tr);
		}
	};

}}}

REGISTER_HANDLER("/data/api", FastCGI::app::reader::ApiPageHandler);
