#!/bin/python

import sys, time

f = open(sys.argv[1], 'rU')

input = f.readlines()
f.close()

locale = sys.argv[2]

print """# Copyright (C) 2012 midnightBITS
# This file is distributed under the same license as the Aggregate package.
# Marcin Zdun <mzdun@midnightbits.com>, 2012.
#"""
print 'msgid ""'
print 'msgstr ""'

for i in range(0, len(input)): input[i] = input[i].strip()

for i in range(0, len(input)):
	if input[i] == 'msgstr ""':
		input = input[i+1:]
		break;

decl=[]

for i in range(0, len(input)):
	if input[i] == '':
		decl = input[:i]
		input = input[i:]
		break;

def getPODate():
	#PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE
	now = time.localtime()
	tz = time.altzone/60
	sign = '-'
	if tz < 0:
		sign = '+'
		tz = -tz
	htz = tz/60
	mtz = tz%60
	
	if htz < 10: htz = "0%s" % htz
	if mtz < 10: mtz = "0%s" % mtz

	return "%s%s%s%s" % (time.strftime("%Y-%m-%d %H:%M", now), sign, htz, mtz)

poDate = getPODate()

for line in decl:
	cmd = line.split('"')[1].split(':')[0]
	if cmd == 'Project-Id-Version': print '"Project-Id-Version: Aggregate 1.0\\n"'
	elif cmd == 'Content-Type': print '"Content-Type: text/plain; charset=UTF-8\\n"'
	elif cmd == 'PO-Revision-Date': print '"PO-Revision-Date: %s\\n"' % poDate
	elif cmd == 'Last-Translator' : print '"Last-Translator: Marcin Zdun <mzdun@midnightbits.com>\\n"'
	elif cmd == 'Language-Team'   :
		print '"Language-Team: Translators <translate@midnightbits.com>\\n"'
		print '"Language: %s\\n"' % locale
	elif cmd == 'Language': pass
	else: print line

for line in input: print line
