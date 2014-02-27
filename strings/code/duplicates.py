#!/usr/bin/python

import os, re

strings = {}
exclude = [
    'LNG_GLOBAL_SITENAME',
    'LNG_GLOBAL_PRODUCT',
    'LNG_RESET_TITLE',
    'LNG_MSG_SENT_TITLE'
    ]

f = open(os.path.join(os.path.dirname(__file__), "../site_strings.txt"))
for line in f:
    line = line.strip()
    l = line.split("=", 1)
    if len(l) == 1: continue
    name = l[0].strip()
    if name in exclude: continue
    value = l[1].strip()[1:]
    value = value[:len(value)-1]
    key = value.lower()
    if key[-1:] in ".!?:": key = key[:len(key)-1]
    if key in strings:
        strings[key].append([name, value])
    else:
        strings[key] = [[name, value]]

f.close()

for s in strings:
    names = strings[s]
    if len(names) == 1: continue
    allSame = True
    for name in names:
        if name[1] != names[0][1]:
            allSame = False
            break
    if allSame:
        print "[ -- ] Duplicates found:", names[0][1]
        for name in names:
            print "      ", name[0]
    else:
        print "[ -- ] Similar strings found:";
        for name in names:
            print "      ", name[0] + ":", name[1]
    print
