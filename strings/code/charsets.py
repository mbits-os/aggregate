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

class Alias:
    def __init__(self, name, target):
        self.name = name
        self.target = target
        self.str = 0
        self.table = 0
        self.size = 0

def main():
    if len(sys.argv) < 4:
        print os.path.basename(sys.argv[0]), \
              "<output.db> <aliases.txt> <input.dat> [<input.dat>...]"
        return -1

    tables = [Table(table) for table in sys.argv[3:]]
    aliases = [Alias(table.name, table.name) for table in tables]

    f = open(sys.argv[2])
    for line in f:
        key, value = line.split(":", 2)
        key = key.strip()
        value = value.strip()
        if '%' in key:
            reg = re.compile( value.replace('%', '(.*)') )
            cands = []
            for table in tables:
                match = reg.match(table.name)
                if match:
                    cands.append(Alias(key.replace('%', match.group(1)), table.name))
            aliases += cands
        else:
            aliases.append(Alias(key, value))
    f.close()

    aliases.sort(key=lambda alias: alias.name);

    stringsize = 0;
    for alias in aliases:
        stringsize += len(alias.name) + 1

    str_off = 12 + len(tables)*12
    table_off_base = str_off + stringsize
    table_off = ((table_off_base + 3) / 4) * 4
    table_off_pad = table_off - table_off_base

    for table in tables:
        table.table = table_off

        table_size  = table.size
        table_size += 3
        table_size /= 4
        table_size *= 4

        table_off += table_size

    for alias in aliases:
        alias.str = str_off
        str_off += len(table.name) + 1

        alias.table = -1
        for table in tables:
            if table.name == alias.target:
                alias.table = table.table
                alias.size = table.size
                break
        if alias.table == -1:
            raise Exception

    out = open(sys.argv[1], "wb")
    out.write("CSET")
    out.write(struct.pack("ii", len(tables), stringsize))

    for alias in aliases:
        out.write(struct.pack("iii", alias.str, alias.table, alias.size))

    for alias in aliases:
        out.write(alias.name)
        out.write(struct.pack("c", chr(0)))
    for i in range(table_off_pad):
        out.write(struct.pack("c", chr(0)))

    for table in tables:
        f = open(table.path, "rb")
        out.write(f.read())
        f.close()

    out.close()

if __name__ == "__main__": main()
