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

ROOT = %s

OUT_ROOT = $(ROOT)bin
OUT_ = $(OUT_ROOT)/posix
OUT = $(OUT_)/release

TMP = $(ROOT)int/posix

PREFIX = $(ROOT)httpd

""" % root

for pro in projects: pro.print_declaration()

sys.stdout.write("all:")
for pro in projects: sys.stdout.write(" all_" + pro.safename)
print

#sys.stdout.write("install:")
#for pro in projects: sys.stdout.write(" install_" + pro.safename)
#print

print """
clean: clean_strings
\t@if [ -e $(TMP) ]; then { echo 'RM $(TMP)'; $(RM) -r $(TMP); }; fi
"""

for pro in projects: pro.print_makefile()

print """############################################
# DIRECTORIES
############################################
"""
dirs = ["$(OUT)", "$(PREFIX)/www"]
for pro in projects: dirs.append("$(%s_TMP)" % pro.safename.upper())
print "DIRS =", " ".join(dirs)

print """$(DIRS):
\t@if ! [ -e $@ ]; then { echo 'DIR $@'; mkdir -p $@; }; fi

############################################
# INSTALL
############################################

install: $(PREFIX)/www strings $(PREFIX)/dbtool $(PREFIX)/www/index.app

$(PREFIX)/dbtool: $(OUT)/dbtool
\t@echo 'CP $@'; cp '$(OUT)/dbtool' '$(PREFIX)';

$(PREFIX)/www/index.app: $(OUT)/index.app
\t@echo 'CP $@'; cp '$(OUT)/index.app' '$(PREFIX)/www';

clean_strings:
\t@$(MAKE) -C '$(ROOT)/strings' clean

strings:
\t@$(MAKE) -C '$(ROOT)/strings'
"""
