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
#include "auth_forms.hpp"
#include <utils.hpp>

namespace FastCGI { namespace app { namespace reader { namespace auth {

	void AuthControl::render(Request& request)
	{
		getControlString(request);
		request << "\r\n";
	}

	void AuthControl::renderSimple(Request& request)
	{
		getSimpleControlString(request);
	}

	void Checkbox::bindData(Request& request, const Strings& data)
	{
		if (m_name.empty()) return;

		m_userValue = false;
		m_value.erase();

		if (request.getVariable("posted") != nullptr)
		{
			param_t value = request.getVariable(m_name.c_str());
			m_userValue = true;
			if (value)
				m_value = value;
			else
				m_value.erase();
			m_checked = value != nullptr;
			return;
		}

		auto _it = data.find(m_name);
		if (_it != data.end())
		{
			m_value = _it->second;
			m_checked = m_value == "1" || m_value == "true";
			return;
		}
	}

	AuthForm::AuthForm(const std::string& title, const std::string& method, const std::string& action, const std::string& mime)
		: m_title(title)
		, m_method(method)
		, m_action(action)
		, m_mime(mime)
	{
	}

	void AuthForm::render(SessionPtr session, Request& request, PageTranslation& tr)
	{
		request << "\r\n<form method='" << m_method << "'";
		if (!m_action.empty()) request << " action='" << m_action << "'";
		if (!m_mime.empty()) request << " enctype='" << m_mime << "'";
		request << ">\r\n<input type='hidden' name='posted' value='1' />\r\n";

		for (auto&& hidden: m_hidden) {
			param_t var = request.getVariable(hidden.c_str());
			if (var != nullptr)
				request << "<input type='hidden' name='" << hidden << "' value='" << url::htmlQuotes(var) << "' />\r\n";
		};

		request << "\r\n"
			"<div class='form'>\r\n"
			"<h1>" << getPageTitle(tr) << "</h1>\r\n";

		for (auto&& m : m_messages)
			request << "<p class='message'>" << m << "</p>\r\n";

		if (!m_error.empty())
			request << "<p class='error'>" << m_error << "</p>\r\n";

		for (auto&& ctrl : m_controls)
			ctrl->render(request);

		if (!m_buttons.empty())
		{
			request << "\r\n<li><hr/></li>\r\n";

			for (auto&& ctrl : m_buttons)
				ctrl->render(request);

			request << "\r\n";
		}
		request << "\r\n"
			"</ul>\r\n"
			"</div>\r\n"
			"</form>\r\n";
	}

	Control* AuthForm::findControl(const std::string& name)
	{
		for (auto&& ctrl : m_controls)
		{
			auto ptr = ctrl->findControl(name);
			if (ptr)
				return ptr;
		}

		for (auto&& ctrl : m_buttons)
		{
			auto ptr = ctrl->findControl(name);
			if (ptr)
				return ptr;
		}

		return nullptr;
	}

	void AuthForm::bind(Request& request, const Strings& data)
	{
		for (auto&& ctrl : m_controls)
		{
			ctrl->bind(request, data);
		};

		for (auto&& ctrl: m_buttons)
		{
			ctrl->bind(request, data);
		};
	}

}}}} // FastCGI::app::reader::auth
