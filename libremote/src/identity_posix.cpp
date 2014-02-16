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

namespace remote
{
	identity get_uid(const char* name, uid_t& uid, gid_t& gid)
	{
		struct passwd pwd;
		struct passwd *result = nullptr;

		size_t bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (bufsize == -1)          /* Value was indeterminate */
			bufsize = 16384;        /* Should be more than enough */

		char* buffer = (char*)malloc(bufsize);
		if (!buffer)
			return identity::oom;

		int s = getpwnam_r(name, &pwd, buffer, bufsize, &result);
		if (result)
		{
			const char* grname = "<unknown>";
			struct group * group = getgrgid(gid);
			if (group)
				grname = group->gr_name;

#define NULLOR(str) (str ? str : "<nullptr>")

			std::cerr << '\n' << pwd.pw_name
				<< ":\n  UID/GID: " << pwd.pw_uid << '/' << pwd.pw_gid << " (" << grname << ')'
				<< ";\n  home:    " << NULLOR(pwd.pw_dir)
				<< ";\n  shell:   " << NULLOR(pwd.pw_shell)
				<< ";\n  gekos:   " << NULLOR(pwd.pw_gecos)
				<< ";\n  comment: " << NULLOR(pwd.pw_comment) << std::endl;
			uid = pwd.pw_uid;
			gid = pwd.pw_gid;
			free(buffer);
			return identity::ok;
		}

		free(buffer);
		if (!s)
			return identity::name_unknown;
		return identity::no_access;
	}

	identity get_gid(const char* name, gid_t& gid)
	{
		struct group grp;
		struct group *result = nullptr;

		size_t bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
		if (bufsize == -1)          /* Value was indeterminate */
			bufsize = 16384;        /* Should be more than enough */

		char* buffer = (char*)malloc(bufsize);
		if (!buffer)
			return identity::oom;

		int s = getgrnam_r(name, &grp, buffer, bufsize, &result);
		if (result)
		{
			std::cerr << '\n' << grp.gr_name
				<< ":\n  GID:     " << grp.gr_gid
				<< ";\n  passwd:  " << NULLOR(grp.gr_passwd)
				<< ";\n  members: ";

			char** ptr = grp.gr_mem;
			bool first = true;
			while (*ptr)
			{
				if (first) first = false;
				else std::cerr << ",\n           ";

				std::cerr << *ptr;
			}

			std::cerr << std::endl;

			gid = grp.gr_gid;
			free(buffer);
			return identity::ok;
		}

		free(buffer);
		if (!s)
			return identity::name_unknown;
		return identity::no_access;
	}

#define RETURN_IF_ERROR(expr) do { identity ret = (expr); if (ret != identity::ok) return ret; } while(0)
	identity change_identity(const char* uname, const char* gname)
	{
		uid_t uid = 0;
		gid_t gid = 0;

		auto test_uid = geteuid();
		auto test_gid = getegid();
		std::cerr << "\nBefore: " << test_uid << "/" << test_gid << std::endl;

		RETURN_IF_ERROR(get_uid(uname, uid, gid));

		if (gname && *gname)
			RETURN_IF_ERROR(get_gid(gname, gid));

		if (setuid(uid))
			return identity::no_access;

		if (setgid(gid))
			return identity::no_access;

		test_uid = geteuid();
		test_gid = getegid();
		std::cerr << "\nAfter: " << test_uid << "/" << test_gid << std::endl;

		return identity::ok;
	}
}
