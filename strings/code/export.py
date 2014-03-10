#! /usr/bin/python
# -*- coding: utf-8 -*-

import os, sys
import locales

def main():
    if len(sys.argv) < 4:
        print os.path.basename(sys.argv[0]), \
              "<input.txt> <input.mo> <output.js>"
        return -1
    path, iso = os.path.splitext(os.path.basename(sys.argv[3]))[0].split("-", 1)
    #print path+".strings", sys.argv[1], "./"+iso+"/"+sys.argv[2], sys.argv[3]
    print "[ JS ] %s-%s.js" % (path, iso)
    strings = locales.openLocales(sys.argv[1])
    keys = strings.keys()
    keys.sort()

    gtt = locales.openGTTFile("./"+iso+"/"+sys.argv[2])

    f = open("../platforms/"+path+".strings", "r")
    o = open(sys.argv[3], "w")
    o.write("""/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

""");

    for line in f:
        line = line.strip()
        for i in range(len(keys)):
            if keys[i] == line:
                value = "";
                if line in gtt:
                    value = gtt[line]
                else:
                    value = locales.warped(strings[line].content)
                print >>o, "var", line, "=", "\"" + locales.escape(value) + "\";"
    #print "\n".join(keys)
    f.close()
    o.close()

    return 0

if __name__ == "__main__": main()
