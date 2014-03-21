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

#ifndef __USER_INFO_HPP__
#define __USER_INFO_HPP__

#include <fast_cgi/session.hpp>
#include <memory>
#include <vector>

namespace db
{
	struct Connection;
	typedef std::shared_ptr<Connection> ConnectionPtr;
}

namespace FastCGI
{
	namespace app
	{

		enum SUBSCRIBE_ERROR
		{
			SERR_INTERNAL_ERROR = -500,
			SERR_4xx_ANSWER = -4,
			SERR_5xx_ANSWER = -5,
			SERR_OTHER_ANSWER = -6,
			SERR_NOT_A_FEED = -7,
			SERR_DISCOVERY_EMPTY = -8,
			SERR_DISCOVERY_MULTIPLE = -9
		};

		struct Discovery
		{
			std::string href;
			std::string title;
			std::string comment;
		};
		using Discoveries = std::vector<Discovery>;

		enum ENTRY_STATE
		{
			ENTRY_UNREAD,
			ENTRY_STARRED,
			ENTRY_IMPORTANT
		};

		enum
		{
			UI_VIEW_ONLY_UNREAD = 1,
			UI_VIEW_ONLY_TITLES = 2,
			UI_VIEW_OLDEST_FIRST = 4
		};

		class UserInfoFactory : public FastCGI::UserInfoFactory
		{
		public:
			FastCGI::UserInfoPtr fromId(const db::ConnectionPtr& db, long long id) override;
			FastCGI::UserInfoPtr fromLogin(const db::ConnectionPtr& db, const std::string& login) override;
		};

		class UserInfo : public FastCGI::UserInfo, FlagsHelper
		{
			long long m_userId = -1;
			std::string m_login;
			long long m_rootFolder = -1;
			bool m_isAdmin = false;
		public:
			UserInfo() {}
			UserInfo(long long id, const std::string& login, long long rootFolder, bool isAdmin, uint32_t flags)
				: FlagsHelper(flags)
				, m_userId(id)
				, m_login(login)
				, m_rootFolder(rootFolder)
				, m_isAdmin(isAdmin)
			{
			}

			long long userId() const override { return m_userId; }
			const std::string& login() const override { return m_login; }
			long long rootFolder() const { return m_rootFolder; }
			bool isAdmin() const { return m_isAdmin; }

			bool viewOnlyUnread() const  { return flags(UI_VIEW_ONLY_UNREAD); }
			bool viewOnlyTitles() const  { return flags(UI_VIEW_ONLY_TITLES); }
			bool viewOldestFirst() const { return flags(UI_VIEW_OLDEST_FIRST); }
			void setViewOnlyUnread(bool value = true)  { flags_set(UI_VIEW_ONLY_UNREAD, value); }
			void setViewOnlyTitles(bool value = true)  { flags_set(UI_VIEW_ONLY_TITLES, value); }
			void setViewOldestFirst(bool value = true) { flags_set(UI_VIEW_OLDEST_FIRST, value); }
			void storeFlags(const db::ConnectionPtr& db);

			long long createFolder(const db::ConnectionPtr& db, const char* name, long long parent = 0);
			long long subscribe(const db::ConnectionPtr& db, const std::string& url, Discoveries& links, bool strict, long long folder = 0);
		};

		using UserInfoPtr = std::shared_ptr<UserInfo>;
	}

	inline app::UserInfoPtr userInfo(const FastCGI::SessionPtr& session)
	{
		return session->userInfo<app::UserInfo>();
	}
}

#endif //__USER_INFO_HPP__