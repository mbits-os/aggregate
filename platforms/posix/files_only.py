#!/bin/python

import sys, os

first = True
for arg in sys.argv[1:]:
    if os.path.isdir(arg): continue
    if first: first = False
    else: sys.stdout.write(" ")
    sys.stdout.write(arg)
