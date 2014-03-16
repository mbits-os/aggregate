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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <fast_cgi.hpp>
#include <handlers.hpp>
#include <forms.hpp>
#include <crypt.hpp>
#include <map>
#include <forms/vertical_renderer.hpp>
#include "../reader_form.hpp"

namespace FastCGI { namespace app { namespace reader { namespace settings {

	enum class PAGE
	{
		GENERAL,
		PROFILE,
		FOLDERS,
		FEEDS,
		ADMIN
	};

	lng::LNG page_title(PAGE type);
	void render_tabs(const SessionPtr& session, Request& request, PageTranslation& tr, PAGE here);
	void start_xhr_fragment(Request& request);
	void end_xhr_fragment(Request& request);
	void start_xhr_section(Request& request, const char* name);
	void end_xhr_section(Request& request);

	inline void xhr_js_sections_helper(Request& request) {}

	template <typename T>
	inline void xhr_js_sections_helper(Request& request, T&& name)
	{
		request << '"' << name << '"';
	}

	template <typename T1, typename T2, typename... Rest>
	inline void xhr_js_sections_helper(Request& request, T1&& name1, T2&& name2, Rest&&... rest)
	{
		request << '"' << name1 << "\", ";
		xhr_js_sections_helper(request, std::forward<T2>(name2), std::forward<Rest>(rest)...);
	}

	template <typename... Names>
	inline void xhr_js_sections(Request& request, Names&&... names)
	{
		request << "<script type=\"text/javascript\">$(function(){\r\n\t$.fn.xhr_sections(";
		xhr_js_sections_helper(request, std::forward<Names>(names)...);
		request << ");\r\n});</script>\r\n";
	}

	struct xhr_section
	{
		Request& request;
		xhr_section(Request& request, const char* name) : request(request)
		{
			start_xhr_section(request, name);
		}
		~xhr_section()
		{
			end_xhr_section(request);
		}
	};

	template <typename Container>
	class SettingsFormBase : public FormImpl<VerticalRenderer, Container, Content>
	{
		using ParentT = FormImpl<VerticalRenderer, Container, Content>;
		using RendererT = typename ParentT::RendererT;
		using ButtonsT = typename ParentT::ButtonsT;
		using ControlsT = typename ParentT::ControlsT;

		PAGE          m_page_type;
		bool          m_title_created;
	public:
		SettingsFormBase(PAGE page_type, const std::string& method = "POST", const std::string& action = std::string(), const std::string& mime = std::string())
			: ParentT(std::string(), method, action, mime)
			, m_page_type(page_type)
			, m_title_created(false)
		{}

		const char* getPageTitle(PageTranslation& tr) override
		{
			if (!m_title_created)
			{
				this->m_title = tr(lng::LNG_SETTINGS_TITLE);
				this->m_title += " &raquo; ";
				this->m_title += tr(page_title(m_page_type));

				m_title_created = true;
			}

			return this->m_title.c_str();
		}

		const char* getFormTitle(PageTranslation& tr) override { return tr(lng::LNG_SETTINGS_TITLE); }
		void render(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			start_xhr_fragment(request);

			RendererT renderer;
			FormBase::formStart(session, request, tr);

			renderer.getFormStart(request, getFormTitle(tr));

			render_tabs(session, request, tr, this->m_page_type);

			{
				xhr_section relative{ request, "messages-container" };
				xhr_section absolute{ request, "messages" };

				renderer.getMessagesString(request, this->m_messages);

				if (!this->m_error.empty())
					renderer.getErrorString(request, this->m_error);
			}

			{
				xhr_section relative{ request, "controls-container" };
				xhr_section absolute{ request, "controls" };

				ControlsT::renderControls(request, renderer);
			}

			{
				xhr_section relative{ request, "buttons-container" };
				xhr_section absolute{ request, "buttons" };

				ButtonsT::renderControls(request, renderer);
			}

			renderer.getFormEnd(request);
			FormBase::formEnd(session, request, tr);

			end_xhr_fragment(request);

			if (!request.getParam(HTTP_X_AJAX_FRAGMENT))
				xhr_js_sections(request, "pages", "messages", "controls", "buttons");
		}
	};

	using SimpleForm = SettingsFormBase<ControlsContainer>;
	using SectionForm = SettingsFormBase<SectionsContainer>;

}}}} // FastCGI::app::reader::settings

namespace FastCGI { namespace app { namespace reader {

	class SettingsPageHandler : public ReaderFormPageHandler
	{
	protected:
		virtual bool restrictedPage() { return true; }
	};

}}} // FastCGI::app::reader

#endif // __SETTINGS_H__
