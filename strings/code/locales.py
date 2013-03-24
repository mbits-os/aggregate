#! /usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, struct

def escape(str):
    return str \
        .replace("\\", "\\\\") \
        .replace("\a", "\\a") \
        .replace("\b", "\\b") \
        .replace("\f", "\\f") \
        .replace("\n", "\\n") \
        .replace("\r", "\\r") \
        .replace("\t", "\\t") \
        .replace("\v", "\\v") \
        .replace("'", "\\'") \
        .replace("\"", "\\\"")

class String:
    def __init__(self, stringId):
        self.id = stringId
        self.content = ""
        self.comment = ""

    def potPrint(self, out):
        print >>out, ''

        if self.comment != '':
            print >>out, '#.', self.comment

        print >>out, 'msgctxt "%s"' % self.id
        print >>out, 'msgid "%s"'  % escape(self.content)
        print >>out, 'msgstr ""'

    def __str__(self):
        return "[%s, %s]" % (self.content, self.comment)

    def __repr__(self):
        return "<%s, %s>" % (self.content, self.comment)

class GetTextFile:
    def __init__(self):
        self.size = 0
        self.contents = ''
    def openFile(self, path):
        f = open(path, "rb")
        self.size = os.stat(path).st_size
        self.contents = f.read()

    def intFromOffset(self, off):
        return struct.unpack_from("I", self.contents, off)[0]

    def offsetsValid(self, off, count, hashtop):
        for i in range(0, count):
            chunk = off + i * 8
            length = self.intFromOffset(chunk)
            offset = self.intFromOffset(chunk+4)
            c = 0
            if length > 0:
                c = ord(self.contents[offset + length])
            if offset < hashtop or offset > self.size:
                return False
            if self.size - offset < length or c != 0:
                return False
        return True

    def getString(self, off, i):
        chunk = off + i * 8
        length = self.intFromOffset(chunk)
        offset = self.intFromOffset(chunk+4)
        return self.contents[offset:offset+length]

def tokenize(line):
    parts = line.split("\"");
    out = []
    part = []
    in_str = False
    esc = False
    for c in line:
        if not in_str:
            if c == "#": break
            elif c == "\"":
                out.append("".join(part))
                part = []
                in_str = True
                esc = False
            else: part.append(c)
        else:
            if not esc:
                if c == "\\": esc = True
                elif c == "\"":
                    out.append("".join(part))
                    part = []
                    in_str = False
                else: part.append(c)
            else: 
                if c == "a": part.append("\a")
                elif c == "b": part.append("\b")
                elif c == "f": part.append("\f")
                elif c == "n": part.append("\n")
                elif c == "r": part.append("\r")
                elif c == "t": part.append("\t")
                elif c == "v": part.append("\v")
                elif c == "'": part.append("\'")
                elif c == "\"": part.append("\"")
                elif c == "\\": part.append("\\")
                elif c == "?": part.append("?")
                else: part.append(c)
                esc = False
    if len(part) > 0:
       out.append("".join(part)) 

    cmd = ''
    str = ''
    if len(out) > 1:
        cmd = out[0]
        cmd = cmd.split("=")[0].strip()
        str = out[1]

    return (cmd, str)

def openLocales(path):
    strings = {}

    f = open(path, "r")
    for line in f:
        line = line.strip()
        cmd, text = tokenize(line)
        if cmd == "":
            continue
        string_id = cmd.split(".", 1)
        cmd = ''
        if len(string_id) > 1: cmd = string_id[1]
        string_id = string_id[0]
        if string_id not in strings:
            strings[string_id] = String(string_id)
        if cmd == '': strings[string_id].content = text
        elif cmd == 'dscr': strings[string_id].comment = text
    f.close()
    return strings

def openGTTFile(path):
    gtt = GetTextFile()
    gtt.openFile(path)

    tag = gtt.intFromOffset(0)
    if 0x950412de != tag: return
    tag = gtt.intFromOffset(4)
    if 0 != tag: return

    count = gtt.intFromOffset(8)
    originals = gtt.intFromOffset(12)
    translation = gtt.intFromOffset(16)
    hashsize = gtt.intFromOffset(20)
    hashpos = gtt.intFromOffset(24)

    if originals < 28: return
    if translation < originals: return
    if hashpos < translation: return
    if translation - originals < count * 4: return
    if hashpos - translation < count * 4: return

    if not gtt.offsetsValid(originals, count, hashpos + hashsize): return
    if not gtt.offsetsValid(translation, count, hashpos + hashsize): return
        
    content = {}
    for i in range(0, count):
        orig = gtt.getString(originals, i).split(chr(4))[0]
        content[orig] = gtt.getString(translation, i)
    return content

def writeDefines(strings, path):
    keys = strings.keys()
    keys.sort()

    f = open(path, "w")
    i = 1
    print >>f, """// THIS FILE IS AUTOGENERATED
#ifndef __STRINGS_INCLUDE__
#define __STRINGS_INCLUDE__

namespace lng
{
\tenum LNG
\t{"""
    for key in keys:
        print >>f, "\t\t%s = %s, // %s" % (key, i, " ".join(strings[key].content.split("\n")))
        i += 1
    print >>f, "\t};\n} // lng\n#endif // __STRINGS_INCLUDE__"
    f.close()

def writePOTFile(strings, path):    
    keys = strings.keys()
    keys.sort()

    f = open(path, "w")

    print >>f, '# Copyright (C) 2013 Aggregate'
    print >>f, '# This file is distributed under the same license as the Aggregate package.'
    print >>f, '# Marcin Zdun <mzdun@midnightbits.com>, 2013.'
    print >>f, '#'
    print >>f, '#, fuzzy'
    print >>f, 'msgid ""'
    print >>f, 'msgstr ""'
    print >>f, '"Project-Id-Version: Aggregate 1.0\\n"'
    print >>f, '"Report-Msgid-Bugs-To: \\n"'
    print >>f, '"POT-Creation-Date: 2012-10-22 20:47+0200\\n"'
    print >>f, '"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"'
    print >>f, '"Last-Translator: Marcin Zdun <mzdun@midnightbits.com>\\n"'
    print >>f, '"Language-Team: Translators <translate@midnightbits.com>\\n"'
    print >>f, '"Language: \\n"'
    print >>f, '"MIME-Version: 1.0\\n"'
    print >>f, '"Content-Type: text/plain; charset=UTF-8\\n"'
    print >>f, '"Content-Transfer-Encoding: 8bit\\n"'

    for key in keys:
        strings[key].potPrint(f);
    f.close()
    
def generateDefines(strPath, outPath):
    strings = openLocales(strPath)
    writeDefines(strings, outPath)

def generatePOTFile(strPath, outPath):
    strings = openLocales(strPath)
    writePOTFile(strings, outPath)

warps = {
    'a': 'ȧ',
    'b': 'Ƌ',
    'c': 'ç',
    'd': 'đ',
    'e': 'ê',
    'f': 'ƒ',
    'g': 'ğ',
    'h': 'ĥ',
    'i': 'ï',
    'j': 'ĵ',
    'k': 'ķ',
    'l': 'ĺ',
    'n': 'ñ',
    'o': 'ô',
    'r': 'ȓ',
    's': 'ş',
    't': 'ŧ',
    'u': 'ũ',
    'w': 'ŵ',
    'y': 'ÿ',
    'z': 'ȥ',
    'A': 'Ä',
    'B': 'ß',
    'C': 'Ç',
    'D': 'Ð',
    'E': 'Ȅ',
    'F': 'Ƒ',
    'G': 'Ġ',
    'H': 'Ħ',
    'I': 'Í',
    'J': 'Ĵ',
    'K': 'Ķ',
    'L': 'Ƚ',
    'N': 'Ñ',
    'O': 'Ö',
    'R': 'Ŕ',
    'S': 'Ş',
    'T': 'Ⱦ',
    'U': 'Ù',
    'W': 'Ŵ',
    'Y': 'Ý',
    'Z': 'Ȥ',
    '"': '?'
}

def warped(s):
    o = ""
    for c in s:
        if c in warps: o += warps[c]
        else: o += c
    return o

class Visitor:
    def visit(self, locale, keys, gtt):
        self.visitString(locale, gtt[""])
        for key in keys:
            self.visitString(locale, gtt[key])

    def visitString(self, locale, string):
        pass

class Offsets(Visitor):
    def __init__(self, count):
        self.offset = 12 + count * 8

    def visitString(self, locale, string):
        self.offset = locale.stringOffset(self.offset, string)

class Strings(Visitor):
    def visitString(self, locale, string):
        locale.writeString(string)

class LocaleFile:
    def __init__(self):
        self.f = None

    def intToBinary(self, val):
        self.f.write(struct.pack("I", val))
    
    def stringOffset(self, offset, s):
        length = len(s)
        self.intToBinary(offset)
        self.intToBinary(length)
        return offset + length + 1
    
    def writeString(self, s):
        self.f.write(s)
        self.f.write(chr(0))

    def write(self, strPath, gttPath, outPath, culture):
        strings = openLocales(strPath)
    
        keys = strings.keys()
        keys.sort()
    
        gtt = openGTTFile(gttPath)
        gtt[""] = culture
    
        for key in keys:
            if key not in gtt:
                gtt[key] = warped(strings[key].content)
                print >>sys.stderr, "Missing translation for key:", key

        self.f = open(outPath, "wb")
        self.f.write("LANG")
        count = len(keys) + 1
        self.intToBinary(0)
        self.intToBinary(count)

        v = Offsets(count)
        v.visit(self, keys, gtt)

        v = Strings()
        v.visit(self, keys, gtt)

        self.f.close()
