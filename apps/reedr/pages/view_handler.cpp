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
#include "pages.hpp"

namespace FastCGI { namespace app { namespace reader {

	class WebUIPageHandler: public PageHandler
	{
	public:

		DEBUG_NAME("Web UI");

		void headElement(const SessionPtr& session, Request& request, PageTranslation& tr)
		{
			std::string culture = tr(lng::CULTURE);
			std::transform(culture.begin(), culture.end(), culture.begin(), [](char c) { return c == '-' ? '_' : c; });

			PageHandler::headElement(session, request, tr);
			request << "    <script type=\"text/javascript\" src=\"" << static_web << "code/lang/client-" << culture << ".js\"></script>\r\n";
			request << "    <script type=\"text/javascript\" src=\"" << static_web << "code/navigation.js\"></script>\r\n";
			request << "    <script type=\"text/javascript\" src=\"" << static_web << "code/listing.js\"></script>\r\n";
			request << "    <script type=\"text/javascript\" src=\"" << static_web << "code/view.js\"></script>\r\n";
		}

		void render(const SessionPtr& session, Request& request, PageTranslation& tr)
		{
			request << 
				"<div id=\"navigation\">"
					"<ul>"
						"<li id=\"home\" class=\"section-link\"></li>"
						"<li id=\"all-items\" class=\"section-link\"></li>"
						"<li id=\"subscriptions\" class=\"section-link\"></li>"
					"</ul>"
				"</div>"
				"<div id=\"listing\"></div>"
				"<div id=\"startup\"></div>";
		}

	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/view/", FastCGI::app::reader::WebUIPageHandler);
REGISTER_REDIRECT("/view", "/view/");
