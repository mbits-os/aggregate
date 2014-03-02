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

	SettingsForm::SettingsForm(PAGE page_type, const std::string& method, const std::string& action, const std::string& mime)
		: SimpleForm<VerticalRenderer>(std::string(), method, action, mime)
		, m_page_type(page_type)
		, m_title_created(false)
	{
	}

	const char* SettingsForm::getPageTitle(PageTranslation& tr)
	{
		if (!m_title_created)
		{
			m_title = tr(lng::LNG_SETTINGS_TITLE);
			m_title += " &raquo; ";
			m_title += tr(tabs[(int)m_page_type].title);

			m_title_created = true;
		}

		return m_title.c_str();
	}

	void SettingsForm::render(const SessionPtr& session, Request& request, PageTranslation& tr)
	{
		FormBase::formStart(session, request, tr);
		RendererT::getFormStart(request, getFormTitle(tr));

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		request <<
			"<div class='tabs'>\r\n"
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

			if ((PAGE)i == m_page_type)
				request << "<li class='selected'>" << tr(tab.title) << "</li>\r\n";
			else
				request << "<li><a href='" << tab.url << "'>" << tr(tab.title) << "</a></li>\r\n";
			++i;
			//if (tab.)
		}

		request << "</ul>\r\n"
			"</ul>\r\n"
			"</div>\r\n";
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		RendererT::getMessagesString(request, m_messages);

		if (!m_error.empty())
			RendererT::getErrorString(request, m_error);

		ControlsT::renderControls(request);
		ButtonsT::renderControls(request);

		RendererT::getFormEnd(request);
		FormBase::formEnd(session, request, tr);
	}

	SectionForm::SectionForm(PAGE page_type, const std::string& method, const std::string& action, const std::string& mime)
		: BaseT(std::string(), method, action, mime)
		, m_page_type(page_type)
		, m_title_created(false)
	{
	}

	const char* SectionForm::getPageTitle(PageTranslation& tr)
	{
		if (!m_title_created)
		{
			m_title = tr(lng::LNG_SETTINGS_TITLE);
			m_title += " &raquo; ";
			m_title += tr(tabs[(int)m_page_type].title);

			m_title_created = true;
		}

		return m_title.c_str();
	}

	void SectionForm::render(const SessionPtr& session, Request& request, PageTranslation& tr)
	{
		FormBase::formStart(session, request, tr);
		RendererT::getFormStart(request, getFormTitle(tr));

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		request <<
			"<div class='tabs'>\r\n"
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

			if ((PAGE)i == m_page_type)
				request << "<li class='selected'>" << tr(tab.title) << "</li>\r\n";
			else
				request << "<li><a href='" << tab.url << "'>" << tr(tab.title) << "</a></li>\r\n";
			++i;
			//if (tab.)
		}

		request << "</ul>\r\n"
			"</ul>\r\n"
			"</div>\r\n";
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		RendererT::getMessagesString(request, m_messages);

		if (!m_error.empty())
			RendererT::getErrorString(request, m_error);

		ControlsT::renderControls(request);
		ButtonsT::renderControls(request);

		RendererT::getFormEnd(request);
		FormBase::formEnd(session, request, tr);
	}

}}}} // FastCGI::app::reader::settings
