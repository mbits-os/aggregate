/*
 * Copyright (C) 2013 Aggregate
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

#ifndef __AUTH_HPP__
#define __AUTH_HPP__

#include "handlers.h"
#include "forms.h"
#include "crypt.hpp"

namespace FastCGI { namespace app { namespace reader {

	class AuthPageHandler: public PageHandler
	{
	protected:
		void onAuthFinished(Request& request)
		{
			param_t _continue = request.getVariable("continue");
			if (_continue != nullptr)
				request.redirectUrl(_continue);
			request.redirect("/");
		}
		virtual bool restrictedPage() { return false; }

		void headElement(SessionPtr session, Request& request, PageTranslation& tr)
		{
			PageHandler::headElement(session, request, tr);
			request << "    <style type=\"text/css\">@import url(\"/css/forms.css\");</style>\r\n";
		}
	};

	class Message: public Control
	{
	public:
		Message(const std::string& name, const std::string&, const std::string& hint)
			: Control(name, std::string(), hint)
		{
		}
		virtual void getControlString(Request& request)
		{
			if (!m_value.empty())
				request << "<td></td><td><span class='message'>" << m_value << "</span></td>";
		}
		void bindUI() {}
	};
	typedef std::shared_ptr<Message> MessagePtr;

	struct UserInfo
	{
		long long m_id;
		std::string m_name;
		std::string m_email;
		std::string m_hash;

		UserInfo(): m_id(0) {}

		static UserInfo fromDB(db::ConnectionPtr db, const char* email)
		{
			UserInfo out;

			db::StatementPtr query = db->prepare(
				"SELECT _id, name, passphrase "
				"FROM user "
				"WHERE email=?"
				);

			if (query.get() && query->bind(0, email))
			{
				db::CursorPtr c = query->query();
				if (c.get() && c->next())
				{
					out.m_id = c->getLongLong(0);
					out.m_name = c->getText(1);
					out.m_email = email;
					out.m_hash = c->getText(2);
				}
			}
			return out;
		}

		bool passwordValid(const char* pass)
		{
			return Crypt::verify(pass, m_hash.c_str());
		}
	};

}}} // FastCGI::app::reader

#endif // __AUTH_HPP__