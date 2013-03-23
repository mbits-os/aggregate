#! /usr/bin/python
# -*- coding: utf-8 -*-

import sys
from locales import LocaleFile

def main():
    if len(sys.argv) < 5:
        print os.path.basename(sys.argv[0]), \
              "<input.txt> <input.mo> <output.lng> <iso-CODE>"
        return -1

    locale = LocaleFile()
    locale.write(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
    return 0

if __name__ == "__main__": main()
