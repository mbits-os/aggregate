#! /usr/bin/python
# -*- coding: utf-8 -*-

import os, sys, re, struct

def main():
    if len(sys.argv) < 3:
        print os.path.basename(sys.argv[0]), \
              "<input.txt> <output.cp>"
        return -1
    r = re.compile("^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)")
    f = open(sys.argv[1], "r")
    tab = [-1] * 256
    for line in f:
        match = r.match(line)
        if match:
            _id, val = match.group(1, 2)
            _id = int(_id, 16)
            val = int(val, 16)
            if _id > 255: continue
            if _id < 0: continue
            tab[_id] = val
    f.close()
    f = open(sys.argv[2], "wb")
    f.write(struct.pack("i" * 256, *tab))
    f.close()

if __name__ == "__main__": main()
