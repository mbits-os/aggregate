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
common_incl = [root+"3rd/libfcgi/inc", root+"libenv/includes", root+"libremote/includes"]

_3rd = Project("3rdparty",
               ["HAVE_CONFIG_H", "POSIX", "ZLIB", "L_ENDIAN", "HAVE_MEMMOVE"],
               [], [root+"3rd/libfcgi/inc", root+"3rd/"], kStaticLibrary, predef)

libenv = Project("libenv",
               ["HAVE_CONFIG_H", "POSIX", "ZLIB", "L_ENDIAN"],
               [], [root+"libenv"] + common_incl, kStaticLibrary, predef)

libremote = Project("libremote",
               ["POSIX"],
               [], [root+"libremote"] + common_incl, kStaticLibrary, predef)

dbtool = Project("dbtool", 
                 ["POSIX"],
                 libs, [root+"dbtool"] + common_incl, kApplication, predef)

server = Project("server",
                 ["POSIX"],
                 libs + ["gd"], [root+"server"] + common_incl, kApplication, predef)

libenv.out = "env"
libremote.out = "remote"

libenv.depends_on(_3rd)
dbtool.depends_on(libenv)
dbtool.depends_on(_3rd)
server.depends_on(libremote)
server.depends_on(libenv)
server.depends_on(_3rd)

projects = [_3rd, libenv, libremote, dbtool, server]

print "include common.mak"
print

for pro in projects: pro.print_declaration()


out_safe = " ".join([pro.safename + "_out" for pro in projects])
all_safe = " ".join(["all_" + pro.safename for pro in projects])
print ".PHONY: $(PHONIES) %s %s" % (out_safe, all_safe)
print

sys.stdout.write("all: strings out_dirs " + all_safe)
print
sys.stdout.write("out_dirs: out_dir " + out_safe)
print

#sys.stdout.write("install:")
#for pro in projects: sys.stdout.write(" install_" + pro.safename)
#print

print """
clean: clean_strings clean_distro
\t@if [ -e $(TMP) ]; then { echo 'RM $(TMP)'; $(RM) -r $(TMP); }; fi

out_dir: $(OUT)
"""

for pro in projects: pro.print_makefile()

print """############################################
# DIRECTORIES
############################################
"""
dirs = ["$(OUT)", "$(PREFIX)"]
for pro in projects: dirs.append("$(%s_TMP)" % pro.safename.upper())
print "DIRS =", " ".join(dirs)

print """$(DIRS):
\t@if ! [ -e $@ ]; then { echo '[DIR ] $@'; mkdir -p $@; }; fi

############################################
# INSTALL
############################################

preinstall: $(PREFIX) strings $(PREFIX)/dbtool $(PREFIX)/server

$(PREFIX)/dbtool: $(OUT)/dbtool
\t@echo '[ CP ] $@'; cp '$(OUT)/dbtool' '$(PREFIX)';

$(PREFIX)/server: $(OUT)/server
\t@echo '[ CP ] $@'; cp '$(OUT)/server' '$(PREFIX)';

clean_strings:
\t@$(MAKE) -C '$(ROOT)/strings' clean

strings:
\t@$(MAKE) -C '$(ROOT)/strings'

include distro.mak
"""
