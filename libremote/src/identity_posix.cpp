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
#include <remote/identity.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <iostream>
#include <string>
#include <vector>

//#define DEBUG_IDENT
#define NULLOR(str) (str ? str : std::string())

namespace remote
{
	namespace posix
	{
		template <typename final>
		struct id_data
		{
			template <typename T>
			static final from_(T reader)
			{
				using item_t = typename final::item_t;
				item_t item;
				item_t *result = nullptr;

				size_t bufsize = sysconf(final::size_max);
				if (bufsize == -1)          /* Value was indeterminate */
					bufsize = 16384;        /* Should be more than enough */

				char* buffer = (char*)malloc(bufsize);
				if (!buffer)
					return final(identity::oom);

				int s = reader(&item, buffer, bufsize, &result);
				if (result)
				{
					final out{ item };
					free(buffer);
					return out;
				}

				free(buffer);
				if (!s)
					return final(identity::name_unknown);
				return final(identity::no_access);

			}
		};

		class user : id_data<user>
		{
			friend class id_data<user>;
			using item_t = passwd;
			enum { size_max = _SC_GETPW_R_SIZE_MAX };

			std::string m_name;
#ifdef DEBUG_IDENT
			std::string m_dir;
			std::string m_shell;
			std::string m_gecos;
#endif
			pid_t m_id = 0;
			gid_t m_gid = 0;

			identity m_error = identity::ok;

			explicit user(const item_t& pwd)
				: m_name(NULLOR(pwd.pw_name))
#ifdef DEBUG_IDENT
				, m_dir(NULLOR(pwd.pw_dir))
				, m_shell(NULLOR(pwd.pw_shell))
				, m_gecos(NULLOR(pwd.pw_gecos))
#endif
				, m_id(pwd.pw_uid)
				, m_gid(pwd.pw_gid)
			{
			}

			explicit user(identity error) : m_error(error) {}

			struct name_reader
			{
				const char* m_name;
				name_reader(const char* name) : m_name(name) {}
				int operator()(item_t* pwd, char* buffer, size_t size, item_t** result)
				{
					return getpwnam_r(m_name, pwd, buffer, size, result);
				}
			};

			struct uid_reader
			{
				uid_t m_id;
				uid_reader(uid_t uid) : m_id(uid) {}
				int operator()(item_t* pwd, char* buffer, size_t size, item_t** result)
				{
					return getpwuid_r(m_id, pwd, buffer, size, result);
				}
			};
		public:

			static user from_name(const char* name)
			{
				return from_(name_reader(name));
			}

			static user from_uid(uid_t uid)
			{
				return from_(uid_reader(uid));
			}

			explicit operator bool() const { return m_error == identity::ok; }

			const std::string& name() const { return m_name; }
#ifdef DEBUG_IDENT
			const std::string& dir() const { return m_dir; }
			const std::string& shell() const { return m_shell; }
			const std::string& gecos() const { return m_gecos; }
#endif
			pid_t id() const { return m_id; }
			gid_t gid() const { return m_gid; }

			identity error() const { return m_error; }
		};

		class group : id_data<group>
		{
			friend class id_data<group>;
			using item_t = ::group;
			enum { size_max = _SC_GETGR_R_SIZE_MAX };

#ifdef DEBUG_IDENT
			using strings = std::vector<std::string>;
#endif

			std::string m_name;
#ifdef DEBUG_IDENT
			std::string m_passwd;
			strings m_members;
#endif
			pid_t m_id = 0;

			identity m_error = identity::ok;

			explicit group(const ::group& grp)
				: m_name(NULLOR(grp.gr_name))
#ifdef DEBUG_IDENT
				, m_passwd(NULLOR(grp.gr_passwd))
#endif
				, m_id(grp.gr_gid)
			{
#ifdef DEBUG_IDENT
				char** ptr = grp.gr_mem;
				while (*ptr)
					m_members.push_back(*ptr++);
#endif
			}

			explicit group(identity error) : m_error(error) {}

			struct name_reader
			{
				const char* m_name;
				name_reader(const char* name) : m_name(name) {}
				int operator()(item_t* pwd, char* buffer, size_t size, item_t** result)
				{
					return getgrnam_r(m_name, pwd, buffer, size, result);
				}
			};

			struct gid_reader
			{
				gid_t m_id;
				gid_reader(gid_t uid) : m_id(uid) {}
				int operator()(item_t* pwd, char* buffer, size_t size, item_t** result)
				{
					return getgrgid_r(m_id, pwd, buffer, size, result);
				}
			};
		public:

			static group from_name(const char* name)
			{
				return from_(name_reader(name));
			}

			static group from_gid(uid_t gid)
			{
				return from_(gid_reader(gid));
			}

			explicit operator bool() const { return m_error == identity::ok; }

			const std::string& name() const { return m_name; }
#ifdef DEBUG_IDENT
			const std::string& passwd() const { return m_passwd; }
			const strings& members() const { return m_members; }
#endif
			pid_t id() const { return m_id; }

			identity error() const { return m_error; }
		};
	}

	identity get_uid(const char* name, uid_t& uid, gid_t& gid)
	{
		auto usr = posix::user::from_name(name);
		if (!usr)
			return usr.error();

#ifdef DEBUG_IDENT
		std::string grname = "<unknown>";
		auto grp = posix::group::from_gid(usr.gid());
		if (grp)
			grname = grp.name();

		std::cerr << '\n' << usr.name()
			<< ":\n  UID/GID: " << usr.id() << '/' << usr.gid() << " (" << grname << ')'
			<< ";\n  home:    " << usr.dir()
			<< ";\n  shell:   " << usr.shell()
			<< ";\n  gekos:   " << usr.gecos() << std::endl;
#endif

		uid = usr.id();
		gid = usr.gid();
		return identity::ok;
	}

	identity get_gid(const char* name, gid_t& gid)
	{
		auto grp = posix::group::from_name(name);
		if (!grp)
			return grp.error();

#ifdef DEBUG_IDENT
		std::cerr << '\n' << grp.name()
			<< ":\n  GID:     " << grp.id()
			<< ";\n  passwd:  " << grp.passwd()
			<< ";\n  members: ";

		bool first = true;
		for (auto&& mem: grp.members())
		{
			if (first) first = false;
			else std::cerr << ",\n           ";

			std::cerr << mem;
		}

		std::cerr << std::endl;
#endif

		gid = grp.id();
		return identity::ok;
	}

#define RETURN_IF_ERROR(expr) do { identity ret = (expr); if (ret != identity::ok) return ret; } while(0)

	static inline identity proxy(int val, const char* name)
	{
		if (!val)
			return identity::ok;

		std::cerr << name << " returned " << val << " and errno is " << errno << std::endl;
		return identity::no_access;
	}
	identity change_identity(const char* uname, const char* gname)
	{
		uid_t uid = 0;
		gid_t gid = 0;

#ifdef DEBUG_IDENT
		auto test_uid = getuid();
		auto test_gid = getgid();
		auto test_euid = geteuid();
		auto test_egid = getegid();
		std::cerr << "\nBefore: real: " << test_uid << "/" << test_gid << "; effective: " << test_euid << "/" << test_egid << std::endl;
#endif

		RETURN_IF_ERROR(get_uid(uname, uid, gid));

		if (gname && *gname)
			RETURN_IF_ERROR(get_gid(gname, gid));

		RETURN_IF_ERROR(proxy(setgid(gid), "setgid"));
		RETURN_IF_ERROR(proxy(setuid(uid), "setuid"));

#ifdef DEBUG_IDENT
		test_uid = getuid();
		test_gid = getgid();
		test_euid = geteuid();
		test_egid = getegid();
		std::cerr << "\nAfter: real: " << test_uid << "/" << test_gid << "; effective: " << test_euid << "/" << test_egid << std::endl;
#endif

		return identity::ok;
	}
}
