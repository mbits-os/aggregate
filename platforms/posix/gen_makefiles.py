import sys
sys.path.append("../solver")
from os import path
from IniReader import *
from Project import *

predef = Macros()
predef.add_macro("POSIX", "", Location("<command-line>", 0))
predef.add_macro("USE_POSIX", "", Location("<command-line>", 0))
predef.add_macro("EXTERNAL_OPENSSL", "", Location("<command-line>", 0))
#predef.add_macro("EXTERNAL_EXPAT", "", Location("<command-line>", 0))
predef.add_macro("EXTERNAL_CURL", "", Location("<command-line>", 0))

_3rd = Project("3rdparty",
               ["HAVE_CONFIG_H", "USE_POSIX", "ZLIB", "L_ENDIAN", "HAVE_MEMMOVE"],
               [],
               [root+"3rd/libfcgi/inc",
                root+"3rd/libzlib/inc",
                root+"3rd/libexpat/inc",
                root+"3rd/"], kStaticLibrary, predef)

libenv = Project("libenv",
               ["HAVE_CONFIG_H", "USE_POSIX", "ZLIB", "L_ENDIAN"],
               [],
               [root+"libenv",
                root+"libenv/includes",
                root+"3rd/libfcgi/inc",
                root+"3rd/libzlib/inc"], kStaticLibrary, predef)

dbtool = Project("dbtool", 
                 ["USE_POSIX"],
                 ["c", "stdc++", "curl", "crypto", "ssl", "dl", "z", "m", "rt", "pthread", "mysqlclient"],
                 [root+"3rd/libfcgi/inc",
                  root+"3rd/libzlib/inc",
                  root+"3rd/libexpat/inc",
                  root+"libenv/includes"], kApplication, predef)

server = Project("server",
                 ["USE_POSIX"],
                 ["c", "stdc++", "curl", "crypto", "ssl", "dl", "z", "m", "rt", "pthread", "mysqlclient"],
                 [root+"server",
                  root+"3rd/libfcgi/inc",
                  root+"3rd/libzlib/inc",
                  root+"3rd/libexpat/inc",
                  root+"libenv/includes"], kApplication, predef)

libenv.out = "env"
server.out = "index.app"

libenv.depends_on(_3rd)
dbtool.depends_on(libenv)
dbtool.depends_on(_3rd)
server.depends_on(libenv)
server.depends_on(_3rd)

print """CFLAGS = -g3 -Wno-system-headers
CPPFLAGS = -std=c++11
CORE_CFLAGS= -fvisibility=hidden

CC = gcc
LIBTOOL = g++
LD_DIRS = -L/usr/lib/x86_64-linux-gnu -L$(OUT)

LD_LIBRARY_PATH=.

C_COMPILE = gcc $(INCLUDES) $(CFLAGS) $(DEFS) -x c
CPP_COMPILE = $(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(DEFS) -x c++

CCLD = $(CC)
LINK = $(LIBTOOL) $(LD_DIRS) $(CFLAGS) $(LDFLAGS)
RM = rm
RMDIR = rmdir

AR_FLAGS = rcs

OUT_ROOT = %soutput
OUT_ = $(OUT_ROOT)/posix
OUT = $(OUT_)/release

TMP = ./int

""" % root

_3rd.print_declaration()
libenv.print_declaration()
dbtool.print_declaration()
server.print_declaration()

print """

all: %s %s %s %s

clean:
\t@if [ -e $(TMP) ]; then { echo 'RM $(TMP)'; $(RM) -r $(TMP); }; fi
""" % (_3rd.get_dest(), libenv.get_dest(), server.get_dest(), dbtool.get_dest())

for d in ["$(OUT_ROOT):", "$(OUT): $(OUT_)", "$(OUT_): $(OUT_ROOT)", "$(TMP):", "$(A3RDPARTY_TMP): $(TMP)", "$(LIBENV_TMP): $(TMP)", "$(DBTOOL_TMP): $(TMP)", "$(SERVER_TMP): $(TMP)"]:
    print "%s\n\t@if ! [ -e $@ ]; then { echo 'mkdir $@'; mkdir $@; }; fi\n" % d

_3rd.print_link()
libenv.print_link()
dbtool.print_link()
server.print_link()

_3rd.print_compile()
libenv.print_compile()
dbtool.print_compile()
server.print_compile()
