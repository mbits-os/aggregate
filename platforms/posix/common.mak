CFLAGS = -g3 -Wno-system-headers
CXXFLAGS = 
CPPFLAGS = -std=c++11 $(CXXFLAGS)
CORE_CFLAGS= -fvisibility=hidden

CC = gcc-4.7
LIBTOOL = g++-4.7
LD_DIRS = -L/usr/lib -L$(OUT)

LD_LIBRARY_PATH=.

C_COMPILE = gcc $(INCLUDES) $(CFLAGS) $(DEFS) -x c
CPP_COMPILE = $(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(DEFS) -x c++
C_MAKEDEPEND = makedepend $(INCLUDES) $(CFLAGS) $(DEFS)
CPP_MAKEDEPEND = makedepend $(INCLUDES) $(CFLAGS) $(CXXFLAGS) $(DEFS)

CCLD = $(CC)
LINK = $(LIBTOOL) $(LD_DIRS) $(CFLAGS) $(LDFLAGS)
RM = rm
RMDIR = rmdir

AR_FLAGS = rcs

ROOT = ../..

OUT_ROOT = $(ROOT)/bin
OUT_ = $(OUT_ROOT)/posix
OUT = $(OUT_)/release

TMP = $(ROOT)/int/posix

PREFIX = $(ROOT)/httpd


PHONIES = all clean install clean_strings strings
