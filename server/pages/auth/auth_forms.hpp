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

#ifndef __AUTH_FORMS_H__
#define __AUTH_FORMS_H__

#include <fast_cgi.hpp>
#include <forms.hpp>
#include <map>

namespace FastCGI { namespace app { namespace reader { namespace auth {

	class AuthControl;
	class Input;
	class Text;
	class Checkbox;
	class Submit;
	class Reset;
	class Link;
	class Form;

	using AuthControlPtr = std::shared_ptr<AuthControl>;
	using InputPtr       = std::shared_ptr<Input>;
	using TextPtr        = std::shared_ptr<Text>;
	using CheckboxPtr    = std::shared_ptr<Checkbox>;
	using SubmitPtr      = std::shared_ptr<Submit>;
	using ResetPtr       = std::shared_ptr<Reset>;
	using LinkPtr        = std::shared_ptr<Link>;
	using FormPtr        = std::shared_ptr<Form>;

	using Strings = std::map<std::string, std::string>;

	class AuthControl : public Control
	{
	public:
		AuthControl(const std::string& name, const std::string& label, const std::string& hint) : Control(name, label, hint) {}
		void render(Request& request) override;
		void renderSimple(Request& request) override;
	};

	class Input: public AuthControl
	{
	public:
		Input(const std::string& type, const std::string& name, const std::string& label, const std::string& hint)
			: AuthControl(name, label, hint)
		{
			setAttr("type", type);
			if (!label.empty())
				setAttr("placeholder", label);
		}

		void getControlString(Request& request)
		{
			if (m_hasError)
				request << "<li class='error'>";
			else
				request << "<li>";
			getElement(request, "input");
			request << "</li>";
		}

		void getSimpleControlString(Request& request)
		{
			getElement(request, "input");
		}
	};

	class Text: public Input
	{
		bool m_pass;
	public:
		Text(const std::string& name, const std::string& label, bool pass, const std::string& hint)
			: Input(pass ? "password" : "text", name, label, hint)
			, m_pass(pass)
		{
		}

		void bindUI()
		{
			if (!m_pass)
				Input::bindUI();
		}
	};

	class Checkbox: public Input
	{
		bool m_checked;
	public:
		Checkbox(const std::string& name, const std::string& label, const std::string& hint)
			: Input("checkbox", name, label, hint)
			, m_checked(false)
		{
		}
		void getControlString(Request& request)
		{
			if (m_hasError)
				request << "<li class='error'>";
			else
				request << "<li>";
			getElement(request, "input");
			getLabelString(request);
			request << "</li>";
		}

		void bindData(Request& request, const Strings& data);

		void bindUI()
		{
			if (m_checked)
				setAttr("checked", "checked");
		}

		bool isChecked() const { return m_checked; }
	};

	class Submit: public Input
	{
	public:
		Submit(const std::string& name, const std::string& label, const std::string& hint)
			: Input("submit", name, std::string(), hint)
		{
			setAttr("value", label);
		}

		void bindUI() {}
	};

	class Reset: public Input
	{
	public:
		Reset(const std::string& name, const std::string& label, const std::string& hint)
			: Input("reset", name, std::string(), hint)
		{
			setAttr("value", label);
		}

		void bindUI() {}
	};

	class Link : public AuthControl
	{
		std::string m_link;
		std::string m_text;
	public:
		Link(const std::string& name, const std::string& link, const std::string& text)
			: AuthControl(name, std::string(), std::string())
			, m_link(link)
			, m_text(text)
		{
		}
		virtual void getControlString(Request& request)
		{
			if (m_hasError)
				request << "<li class='link error'>";
			else
				request << "<li class='link'>";
			request << "<a href='" << m_link << "'>" << m_text << "</a></li>";
		}
		void bindUI() {}
	};

	class AuthForm: public Content
	{
		std::string   m_title;
		std::string   m_method;
		std::string   m_action;
		std::string   m_mime;
		std::list<std::string> m_hidden;
		std::string   m_error;
		std::list<std::string> m_messages;
		Controls      m_controls;
		Controls      m_buttons;
	public:
		AuthForm(const std::string& title, const std::string& method = "POST", const std::string& action = std::string(), const std::string& mime = std::string());

		const char* getPageTitle(PageTranslation& tr) { return m_title.c_str(); }
		void render(SessionPtr session, Request& request, PageTranslation& tr);

		AuthForm& hidden(const std::string& name)
		{
			m_hidden.push_back(name);
			return *this;
		}

		template <typename Class>
		std::shared_ptr<Class> control(const std::string& name, const std::string& label = std::string(), const std::string& hint = std::string())
		{
			auto obj = std::make_shared<Class>(name, label, hint);
			if (obj.get())
				m_controls.push_back(obj);
			return obj;
		}

		TextPtr text(const std::string& name, const std::string& label = std::string(), bool pass = false, const std::string& hint = std::string())
		{
			auto obj = std::make_shared<Text>(name, label, pass, hint);
			if (obj.get())
				m_controls.push_back(obj);
			return obj;
		}

		CheckboxPtr checkbox(const std::string& name, const std::string& label = std::string(), const std::string& hint = std::string())
		{
			return control<Checkbox>(name, label, hint);
		}

		SubmitPtr submit(const std::string& name, const std::string& label = std::string(), const std::string& hint = std::string())
		{
			auto ptr = std::make_shared<Submit>(name, label, hint);

			if (ptr.get())
				m_buttons.push_back(ptr);

			return ptr;
		}

		ResetPtr reset(const std::string& name, const std::string& label = std::string(), const std::string& hint = std::string())
		{
			auto ptr = std::make_shared<Reset>(name, label, hint);

			if (ptr.get())
				m_buttons.push_back(ptr);

			return ptr;
		}

		LinkPtr link(const std::string& name, const std::string& link, const std::string& text)
		{
			auto ptr = std::make_shared<Link>(name, link, text);

			if (ptr.get())
				m_buttons.push_back(ptr);

			return ptr;
		}

		Control* findControl(const std::string& name);
		void bind(Request& request, const Strings& data = Strings());
		void setError(const std::string& error) { m_error = error; }
		void addMessage(const std::string& message) { m_messages.push_back(message); }
	};

}}}} // FastCGI::app::reader::auth

#endif // __AUTH_FORMS_H__
