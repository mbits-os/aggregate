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
#include "settings.hpp"
#include <utils.hpp>

namespace FastCGI { namespace app { namespace reader { namespace settings {

	static struct PageInfo
	{
		lng::LNG title;
		const char* url;
		bool admin;
		PageInfo(lng::LNG title, const char* url, bool admin = false): title(title), url(url), admin(admin) {}
	}
	tabs[] =
	{
		{ lng::LNG_SETTINGS_GENERAL, "/settings/general" }, // PAGE::GENERAL
		{ lng::LNG_SETTINGS_PROFILE, "/settings/profile" }, // PAGE::PROFILE
		{ lng::LNG_SETTINGS_FOLDERS, "/settings/folders" }, // PAGE::FOLDERS
		{ lng::LNG_SETTINGS_FEEDS,   "/settings/feeds"   }, // PAGE::FEEDS
		{ lng::LNG_SETTINGS_ADMIN,   "/settings/admin",  true }  // PAGE::FEEDS
	};

	lng::LNG page_title(PAGE page_type) { return tabs[(int)page_type].title; }

	void render_tabs(const SessionPtr& session, Request& request, PageTranslation& tr, PAGE here)
	{
		xhr_section relative{ request, "pages-container" };
		xhr_section absolute{ request, "pages" };

		request <<
			"<div class='tabs' id='tabs'>\r\n"
			"<ul>\r\n";

		bool isAdmin = session && session->isAdmin();

		int i = 0;
		for (auto&& tab : tabs)
		{
			if (tab.admin && !isAdmin)
			{
				++i;
				continue;
			}

			if ((PAGE)i == here)
				request << "<li class='selected'>" << tr(tab.title) << "</li>\r\n";
			else
				request << "<li><a href='" << tab.url << "' " ATTR_X_AJAX_FRAGMENT "='fragment'>" << tr(tab.title) << "</a></li>\r\n"; // data-pjax='#fragment'
			++i;
			//if (tab.)
		}

		request << "</ul>\r\n"
			"</ul>\r\n"
			"</div>\r\n";
	}

	void start_xhr_fragment(Request& request)
	{
		if (!request.getParam(HTTP_X_AJAX_FRAGMENT))
			request << "<div id='fragment'>\r\n";
	}

	void end_xhr_fragment(Request& request)
	{
		if (!request.getParam(HTTP_X_AJAX_FRAGMENT))
			request << "</div> <!-- #fragment -->\r\n";
	}

	void start_xhr_section(Request& request, const char* name)
	{
		request << "<section id='" << name << "'>\r\n";
	}

	void end_xhr_section(Request& request)
	{
		request << "</section>\r\n";
	}

}}}} // FastCGI::app::reader::settings
