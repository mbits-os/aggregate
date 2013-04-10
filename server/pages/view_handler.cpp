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

namespace FastCGI { namespace app { namespace reader {

	class WebUIPageHandler: public PageHandler
	{
	public:

		std::string name() const
		{
			return "Web UI";
		}

		void headElement(SessionPtr session, Request& request, PageTranslation& tr)
		{
			PageHandler::headElement(session, request, tr);
			request << "    <script type=\"text/javascript\" src=\""STATIC_RESOURCES"/code/jquery-1.9.1.js\"></script>\r\n";
			request << "    <script type=\"text/javascript\" src=\""STATIC_RESOURCES"/code/view.js\"></script>\r\n";
		}

		void render(FastCGI::SessionPtr session, Request& request, PageTranslation& tr)
		{
			request << 
				"              <div id=\"navigation\"></div>\r\n"
				"              <div id=\"listing\"></div>\r\n"
				"              <div id=\"startup\"></div>";
		}

	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/view/", FastCGI::app::reader::WebUIPageHandler);
REGISTER_REDIRECT("/view", "/view/");
