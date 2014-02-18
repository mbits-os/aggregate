#!/bin/python

import sys, os

def nonempty(d):
    try:
        if not os.path.exists(d): return False
        if not os.path.isdir(d): return True
        return len(os.listdir(d)) > 0
    except:
        return False;

first = True
dirs = list(set([os.path.abspath(arg) for arg in sys.argv[1:]]))

whole = []
while len(dirs):
    next = []
    for arg in dirs:
        whole.append(arg)
        parent = os.path.abspath(os.path.join(arg, os.pardir))
        if parent != arg:
            next.append(parent)
        dirs = list(set(next))
whole = reversed(sorted(list(set(whole))))

for arg in whole:
    if nonempty(arg):
        continue

    print "RM", arg
    try: os.rmdir(arg)
    except: pass
