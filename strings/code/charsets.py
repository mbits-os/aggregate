#! /usr/bin/python
# -*- coding: utf-8 -*-

import os, sys, re, struct

class Table:
    def __init__(self, fileName):
        self.path = fileName
        self.size = os.stat(fileName).st_size
        self.name = os.path.splitext(os.path.split(fileName)[1])[0].replace('_', '-')
        self.str = 0
        self.table = 0

def main():
    if len(sys.argv) < 3:
        print os.path.basename(sys.argv[0]), \
              "<output.db> <input.dat> [<input.dat>...]"
        return -1

    tables = [Table(table) for table in sys.argv[2:]]
    tables.sort(key=lambda tab: tab.name);

    stringsize = 0;
    for table in tables:
        stringsize += len(table.name) + 1

    str_off = 12 + len(tables)*12
    table_off_base = str_off + stringsize
    table_off = ((table_off_base + 3) / 4) * 4
    table_off_pad = table_off - table_off_base

    for table in tables:
        table.str = str_off
        table.table = table_off

        str_off += len(table.name) + 1

        table_size= table.size
        table_size += 3
        table_size /= 4
        table_size *= 4

        table_off += table_size

    out = open(sys.argv[1], "wb")
    out.write("CSET")
    out.write(struct.pack("ii", len(tables), stringsize))

    for table in tables:
        out.write(struct.pack("iii", table.str, table.table, table.size))

    for table in tables:
        out.write(table.name)
        out.write(struct.pack("c", chr(0)))
    for i in range(table_off_pad):
        out.write(struct.pack("c", chr(0)))

    for table in tables:
        f = open(table.path, "rb")
        out.write(f.read())
        f.close()

    out.close()

if __name__ == "__main__": main()
