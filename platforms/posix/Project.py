import sys, platform
sys.path.append("../solver")
from os import path
import uuid
from files import FileList

root = "../../"

so_ext=".so"
app_ext=""
lib_ext=".a"

so_prefix="lib"
lib_prefix="lib"

kApplication = 0
kDynamicLibrary = 1
kStaticLibrary = 2

def arglist(prefix, l):
    if len(l) == 0: return ""
    return "%s%s" % (prefix, (" %s" % prefix).join(l))

class Project:
    def __init__(self, name, defs, libs, includes, bintype, predef):
        self.name = name
        self.safename = name
        self.files = FileList()
        self.out = name
        self.defs = defs
        self.libs = libs
        self.includes = includes + ["/opt/local/include"]
        self.bintype = bintype
        self.depends = []
        self.group = "libs"
        if bintype == kApplication:
            self.group = "apps"

        self.files.read(predef, "%s%s/%s/%s.files" % (root, self.group, name, name))

        if self.safename[0] >= "0" and self.safename[0] <= "9": self.safename = "a%s" % self.safename

    def depends_on(self, project):
        dep = project.get_link_dep()
        if dep != "": self.depends.append(dep)

    def get_link_dep(self):
        if self.bintype == kDynamicLibrary: return self.get_short_dest()
        if self.bintype == kStaticLibrary: return self.get_short_dest()
        return ""

    def get_objects(self):
        out = []
        for k in self.files.cfiles + self.files.cppfiles:
            f = self.files.sec.items[k]
            out.append("$(%s_TMP)/%s.o" % (self.safename.upper(), path.split(path.splitext(f.name)[0])[1]))
        return out

    def get_code(self):
        return [self.files.sec.items[k].name for k in self.files.cfiles + self.files.cppfiles]

    def get_code_deps(self):
        out = []
        for k in self.files.cfiles + self.files.cppfiles:
            f = self.files.sec.items[k]
            out.append("$(%s_TMP)/%s.d" % (self.safename.upper(), path.split(path.splitext(f.name)[0])[1]))
        return out

    def get_dest(self):
        return "$(OUT)/"+self.get_short_dest()

    def get_short_dest(self):
        if self.bintype == kApplication:
            return "%s%s" % (self.out, app_ext)
        elif self.bintype == kDynamicLibrary:
            return "%s%s%s" % (so_prefix, self.out, so_ext)
        elif self.bintype == kStaticLibrary:
            return "%s%s%s" % (lib_prefix, self.out, lib_ext)

    def print_declaration(self):
        n = self.safename.upper()
        print "%s_DEFS = %s" % (n, arglist("-D", self.defs))
        print "%s_INCLUDES = %s" % (n, arglist("-I", self.includes))
        print "%s_C_COMPILE = $(C_COMPILE) $(%s_INCLUDES) $(%s_CFLAGS) $(%s_DEFS)" % (n, n, n, n)
        print "%s_CPP_COMPILE = $(CPP_COMPILE) $(%s_INCLUDES) $(%s_CFLAGS) $(%s_CPPFLAGS) $(%s_DEFS)" % (n, n, n, n, n)
        print "%s_C_MAKEDEPEND = $(C_MAKEDEPEND) $(%s_INCLUDES) $(%s_CFLAGS) $(%s_DEFS)" % (n, n, n, n)
        print "%s_CPP_MAKEDEPEND = $(CPP_MAKEDEPEND) $(%s_INCLUDES) $(%s_CFLAGS) $(%s_CPPFLAGS) $(%s_DEFS)" % (n, n, n, n, n)
        print "%s_DIR = $(ROOT)/%s/%s" % (n, self.group, self.name)
        print "%s_TMP = $(TMP)/%s" % (n, self.name)
        print "%s_FILES = %s" % (n, """ \\
\t""".join(self.get_code()))
        print "%s_SRC = $(addprefix $(%s_DIR)/, $(%s_FILES))" % (n, n, n)
        print "%s_OBJ = $(patsubst %%.c,%%.o,$(patsubst %%.cpp,%%.o,$(addprefix $(%s_TMP)/, $(%s_FILES))))" % (n, n, n)
        print "%s_DEP = $(patsubst %%.c,%%.d,$(patsubst %%.cpp,%%.d,$(addprefix $(%s_TMP)/, $(%s_FILES))))" % (n, n, n)
        print

    def print_makefile(self):
        out = "$(OUT)"
        dirs = self.out.split("/")
        if len(dirs) > 1:
            dirs = "/".join(dirs[:len(dirs)-1])
            out = "$(OUT)/" + dirs

        print "############################################"
        print "# %s" % self.name
        print "############################################"
        print
        print "all_" + self.safename + ": " + self.get_dest()
        print self.safename + "_out: " + out
        print 
        self.print_compile()
        self.print_link()

    def print_compile(self):
        n = self.safename.upper()
        print "# compile"

        print "$(%s_TMP)%%.o: $(%s_DIR)%%.c" % (n, n)
        print "\t@echo [ CC ] $<"
        print "\t@mkdir -p $(dir $@)"
        print "\t@echo -n $(dir $@) > $(patsubst %.o,%.d,$@)"
        print "\t@$(%s_C_MAKEDEPEND) $< >> $(patsubst %%.o,%%.d,$@)" %n # 2>/dev/null" % n
        print "\t@$(%s_C_COMPILE) -c -o $@ $<\n" % n

        print "-include $(%s_DEP)\n" % n

        print "$(%s_TMP)%%.o: $(%s_DIR)%%.cpp" % (n, n)
        print "\t@echo [ CC ] $<"
        print "\t@mkdir -p $(dir $@)"
        print "\t@echo -n $(dir $@) > $(patsubst %.o,%.d,$@)"
        print "\t@$(%s_CPP_MAKEDEPEND) $< >> $(patsubst %%.o,%%.d,$@)" %n # 2>/dev/null" % n
        print "\t@$(%s_CPP_COMPILE) -c -o $@ $<\n" % n

    def print_link(self):
        n = self.safename.upper()
        libs = arglist("-l", self.libs)
        depends = []
        for dep in self.depends:
            if dep[:3] == "lib": dep = dep[3:]
            dep, ext = path.splitext(dep)
            depends.append(dep)
        deps = arglist("-l", depends)
        deps2 = arglist("$(OUT)/", self.depends)
        if deps2 != "": deps2 = " " + deps2
        print "# link"
        print "%s: $(%s_TMP) $(%s_OBJ) %s Makefile.gen" % (self.get_dest(), n, n, deps2)
        if self.bintype == kApplication:
            print "\t@echo [LINK] $@; $(LINK) $(%s_OBJ) -o $@ %s %s\n" % (n, deps, libs)
        elif self.bintype == kDynamicLibrary:
            print "\t@echo [LINK] $@; $(LINK) -shared $(%s_OBJ) -o $@  %s %s\n" % (n, deps, libs)
        elif self.bintype == kStaticLibrary:
            print "\t@echo [ AR ] $@; $(AR) $(AR_FLAGS) $@ $(%s_OBJ)\n" % n



