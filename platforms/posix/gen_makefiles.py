import sys
sys.path.append("../solver")
from os import path
from IniReader import *
from Project import *

predef = Macros()
predef.add_macro("POSIX", "", Location("<command-line>", 0))
predef.add_macro("USE_POSIX", "", Location("<command-line>", 0))
predef.add_macro("EXTERNAL_OPENSSL", "", Location("<command-line>", 0))
predef.add_macro("EXTERNAL_EXPAT", "", Location("<command-line>", 0))
predef.add_macro("EXTERNAL_CURL", "", Location("<command-line>", 0))
predef.add_macro("EXTERNAL_Z", "", Location("<command-line>", 0))

libs = ["c", "stdc++", "curl", "crypto", "ssl", "pthread", "mysqlclient", "expat", "dl", "z", "m", "rt"]
common_incl = [root+"3rd/libfcgi/inc", root+"libenv/includes"]

_3rd = Project("3rdparty",
               ["HAVE_CONFIG_H", "USE_POSIX", "ZLIB", "L_ENDIAN", "HAVE_MEMMOVE"],
               [], [root+"3rd/libfcgi/inc", root+"3rd/"], kStaticLibrary, predef)

libenv = Project("libenv",
               ["HAVE_CONFIG_H", "USE_POSIX", "ZLIB", "L_ENDIAN"],
               [], [root+"libenv"] + common_incl, kStaticLibrary, predef)

dbtool = Project("dbtool", 
                 ["USE_POSIX"],
                 libs, [root+"dbtool"] + common_incl, kApplication, predef)

server = Project("server",
                 ["USE_POSIX"],
                 libs, [root+"server"] + common_incl, kApplication, predef)

libenv.out = "env"
server.out = "index.app"

libenv.depends_on(_3rd)
dbtool.depends_on(libenv)
dbtool.depends_on(_3rd)
server.depends_on(libenv)
server.depends_on(_3rd)

projects = [_3rd, libenv, dbtool, server]

print """CFLAGS = -g3 -Wno-system-headers
CPPFLAGS = -std=c++11
CORE_CFLAGS= -fvisibility=hidden

CC = gcc
LIBTOOL = g++
LD_DIRS = -L/usr/lib -L$(OUT)

LD_LIBRARY_PATH=.

C_COMPILE = gcc $(INCLUDES) $(CFLAGS) $(DEFS) -x c
CPP_COMPILE = $(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(DEFS) -x c++

CCLD = $(CC)
LINK = $(LIBTOOL) $(LD_DIRS) $(CFLAGS) $(LDFLAGS)
RM = rm
RMDIR = rmdir

AR_FLAGS = rcs

OUT_ROOT = %sbin
OUT_ = $(OUT_ROOT)/posix
OUT = $(OUT_)/release

TMP = ./int

""" % root

for pro in projects: pro.print_declaration()

sys.stdout.write("""

all:""")

for pro in projects: sys.stdout.write(" " + pro.get_dest())

print """

clean:
\t@if [ -e $(TMP) ]; then { echo 'RM $(TMP)'; $(RM) -r $(TMP); }; fi
"""

for pro in projects: pro.print_link()

dirs = ["$(OUT_ROOT):", "$(OUT): $(OUT_)", "$(OUT_): $(OUT_ROOT)", "$(TMP):"]
for pro in projects: dirs.append("$(%s_TMP): $(TMP)" % pro.safename.upper())

for d in dirs:
    print "%s\n\t@if ! [ -e $@ ]; then { echo 'mkdir $@'; mkdir $@; }; fi\n" % d

for pro in projects: pro.print_compile()
