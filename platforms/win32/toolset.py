#! /usr/bin/python
# -*- coding: utf-8 -*-

import os, sys, re, shutil

def vcxproj(sln):
    f = open(sln)
    out = []
    tmplt = re.compile('Project\("\{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942\}"\) = "[^"]+", "([^"]+)",')
    for line in f:
        match = tmplt.match(line)
        if match:
            out.append(match.group(1))
    f.close()
    return out

def set_toolset(vcxproj, toolset):
    print "%s:" % vcxproj
    count = 0
    lineno = 0
    f = open(vcxproj, "rb")
    tmp = open(vcxproj + ".tmp", "wb")
    tmplt = re.compile('(\s*\<PlatformToolset\>)([^<]*)(\</PlatformToolset\>)')
    toolset = toolset.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;")
    repl = "\\1%s\\3" % toolset
    for line in f:
        lineno += 1
        match = tmplt.match(line)
        if match:
            if toolset != match.group(2):
                count += 1
            line = tmplt.sub(repl, line)
            print "%s: %s" % (lineno, line.rstrip())
        tmp.write(line)
    f.close()
    tmp.close()

    if count == 0:
        os.remove(vcxproj + ".tmp")
        return
    try:
        os.remove(vcxproj + ".orig")
    except:
        pass
    os.rename(vcxproj, vcxproj + ".orig")
    os.rename(vcxproj + ".tmp", vcxproj)
    
    
def main():
    if len(sys.argv) < 3:
        print os.path.basename(sys.argv[0]), \
              "<input.sln> <PlatformToolset>"
        print ""
        print "Example:"
        print " ", os.path.basename(sys.argv[0]), \
              "file.sln CTP_Nov2013"
        return -1
    files = vcxproj(sys.argv[1])
    for f in files:
        set_toolset(f, sys.argv[2])

if __name__ == "__main__": main()
