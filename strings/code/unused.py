#!/usr/bin/python

import os, re

dirs = ["../libenv", "../server"]

patt = re.compile("(lng::LNG_[A-Z0-9_]+)")
strings = ["LANGUAGE_NAME"] # mark as used

for d in dirs:
    for (base, subs, files) in os.walk(d):
        for f in files:
            data = open(os.path.join(base, f))
            for line in data:
                match = re.findall(patt, line)
                for m in match:
                    strings.append(m[5:])
            data.close()

f = open(os.path.join(os.path.dirname(__file__), "../client.strings"))
for line in f:
    line = line.strip()
    if len(line): strings.append(line)
f.close()

strings = set(strings)

out = []
f = open(os.path.join(os.path.dirname(__file__), "../site_strings.txt"))
for line in f:
    line = line.strip()
    l = line.split("=", 1)
    if len(l) == 1: continue
    l = l[0].strip()
    if '.' in l: continue
    if l in strings: continue
    out.append(line)
f.close()

if len(line):
    print "[ -- ] Unused strings:"
for line in out:
    print "      ", line

