#!/usr/bin/python

import sys, os, shutil

if len(sys.argv) < 2: exit(1)

DISTRO = sys.argv[1]
DISTRO_LEN = len(DISTRO)

def mkdir(dname):
    if not dname: return
    if os.path.islink(dname):
        print "[LINK]", dname, "->", os.readlink(dname), "(removing)"
        os.remove(dname)
    if os.path.isdir(dname):
        return

    print "[ >> ]", dname
    os.mkdir(dname, 0775)

def cp(fname):
    srcname = os.path.join(DISTRO, "." + fname)
    copy = not os.path.exists(fname)
    if not copy:
        srctime = os.path.getmtime(srcname)
        dsttime = os.path.getmtime(fname)
        copy = srctime > dsttime
    if not copy: return
    print "[ -> ]", fname
    shutil.copy(srcname, fname)

for root, dirs, files in os.walk(DISTRO):
    mkdir(root[DISTRO_LEN:])

    for name in files:
        cp(os.path.join(root, name)[DISTRO_LEN:])
