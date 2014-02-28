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

#ifndef __AUTH_HPP__
#define __AUTH_HPP__

#include <handlers.hpp>
#include <forms.hpp>
#include <crypt.hpp>
#include <forms/vertical_renderer.hpp>

namespace FastCGI { namespace app { namespace reader { namespace auth {

	using AuthForm = SimpleForm<VerticalRenderer>;

	inline std::string format_username(const std::string& name, const std::string& login)
	{
		std::string out;
		out.reserve(name.length() + login.length() + 17);

		auto both = !name.empty() && !login.empty();

		if (!name.empty())
		{
			out.append("<b>");
			out.append(name);
			out.append("</b>");
		}

		if (both)
			out.append(" (");

		if (!login.empty())
		{
			out.append("<i>");
			out.append(login);
			out.append("</i>");
		}

		if (both)
			out.append(")");

		return out;
	}

	inline std::string format_username(const SessionPtr& session)
	{
		return format_username(session->getName(), session->getLogin());
	}

}}}} // FastCGI::app::reader::auth

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

		void header(SessionPtr session, Request& request, PageTranslation& tr) override
		{
			request << "<!DOCTYPE html "
				"PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
				"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n"
				"<html>\r\n"
				"  <head>\r\n";
			headElement(session, request, tr);
#if DEBUG_CGI
			request <<
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/fd_icons.css\");</style>\r\n";
#endif
			request <<
				"    <style type=\"text/css\">@import url(\"" << static_web << "css/auth.css\");</style>\r\n"
				"  </head>\r\n"
				"  <body>\r\n";
			bodyStart(session, request, tr);
		}

		void bodyStart(SessionPtr session, Request& request, PageTranslation& tr) override
		{
			request <<
				"    <div class='auth-logo'><div><a href='/'><img src='" << static_web << "images/auth_logo.png' /><span>" << tr(lng::LNG_GLOBAL_DESCRIPTION) << "</span></a></div></div>\r\n"
				"    <div id=\"auth-content\">\r\n";
		}

		void bodyEnd(SessionPtr session, Request& request, PageTranslation& tr) override
		{
			request << "\r\n"
				"    </div>\r\n";
		}
	};

	struct UserInfo
	{
		long long m_id;
		std::string m_login;
		std::string m_name;
		std::string m_email;
		std::string m_hash;

		UserInfo(): m_id(0) {}

		static UserInfo fromDB(db::ConnectionPtr db, const char* email)
		{
			UserInfo out;

			db::StatementPtr query = db->prepare(
				"SELECT _id, login, name, passphrase "
				"FROM user "
				"WHERE email=?"
				);

			if (query && query->bind(0, email))
			{
				db::CursorPtr c = query->query();
				if (c && c->next())
				{
					out.m_id = c->getLongLong(0);
					out.m_login = c->getText(1);
					out.m_name = c->getText(2);
					out.m_email = email;
					out.m_hash = c->getText(3);
				}
				else
				{
					query = db->prepare(
						"SELECT _id, name, email, passphrase "
						"FROM user "
						"WHERE login=?"
						);

					if (query && query->bind(0, email))
					{
						db::CursorPtr c = query->query();
						if (c && c->next())
						{
							out.m_id = c->getLongLong(0);
							out.m_login = email;
							out.m_name = c->getText(1);
							out.m_email = c->getText(2);
							out.m_hash = c->getText(3);
						}
					}
				}
			}
			return out;
		}

		static UserInfo fromSession(db::ConnectionPtr db, const SessionPtr& session)
		{
			UserInfo out;
			out.m_id = -1;

			db::StatementPtr query = db->prepare(
				"SELECT passphrase "
				"FROM user "
				"WHERE _id=?"
				);

			if (query && query->bind(0, session->getId()))
			{
				db::CursorPtr c = query->query();
				if (c && c->next())
				{
					out.m_id = session->getId();
					out.m_login = session->getLogin();
					out.m_name = session->getName();
					out.m_email = session->getEmail();
					out.m_hash = c->getText(0);
				}
			}
			return out;
		}

		bool passwordValid(const char* pass)
		{
			return Crypt::verify(pass, m_hash.c_str());
		}

		bool changePasswd(const db::ConnectionPtr& db, const char* passwd)
		{
			Crypt::password_t hash;
			Crypt::password(passwd, hash);

			db::StatementPtr update = db->prepare("UPDATE user SET passphrase=? WHERE email=?");
			if (!update)
				return false;

			if (!update->bind(0, hash)) return false;
			if (!update->bind(1, m_email.c_str())) return false;

			return update->execute();
		}

		static UserInfo fromRecoveryId(const db::ConnectionPtr& db, const char* recoveryId)
		{
			constexpr tyme::time_t DAY = 24 * 60 * 60;
			UserInfo out;
			out.m_id = -1;

			auto select = db->prepare(
				"SELECT user._id, user.login, user.name, user.email, user.passphrase, recovery.started "
				"FROM recovery LEFT JOIN user ON (recovery.user_id = user._id) "
				"WHERE recovery._id=?"
				);

			if (select && select->bind(0, recoveryId))
			{
				auto c = select->query();
				if (c && c->next())
				{
					auto started = c->getTimestamp(5);
					if (tyme::now() > started + DAY)
					{
						out.m_id = -2;
						return out;
					}

					out.m_id = c->getLongLong(0);
					out.m_login = c->getText(1);
					out.m_name = c->getText(2);
					out.m_email = c->getText(3);
					out.m_hash = c->getText(4);
				}
			}
			return out;
		}

		std::string createRecoverySession(const db::ConnectionPtr& db)
		{
			tyme::time_t now = tyme::now();
			std::string sessionId = digest<Crypt::SHA512Hash>() + "_" + digest<Crypt::MD5Hash>();

			auto insert = db->prepare("INSERT INTO recovery (_id, user_id, started) VALUES (?, ?, ?)");

			if (insert &&
				insert->bind(0, sessionId) &&
				insert->bind(1, m_id) &&
				insert->bindTime(2, now) &&
				insert->execute())
			{
				return sessionId;
			}

			return std::string();
		}

		static bool deleteRecoverySession(const db::ConnectionPtr& db, const char* sessionId)
		{
			auto stmt = db->prepare("DELETE FROM recovery WHERE _id=?");

			return stmt && stmt->bind(0, sessionId) && stmt->execute();
		}

	private:
		template <typename Hash>
		static inline std::string digest()
		{
			char seed[20];
			typename Hash::hash_t out;
			Crypt::newSalt(seed);
			Hash::digest(seed, out);

			return out;
		}
	};

}}} // FastCGI::app::reader

#endif // __AUTH_HPP__