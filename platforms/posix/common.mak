CFLAGS = -Wno-system-headers
CXXFLAGS =
CPPFLAGS = -std=c++11 $(CXXFLAGS)
CORE_CFLAGS= -fvisibility=hidden

CC = gcc-4.8
LIBTOOL = g++-4.8
LD_DIRS = -L/usr/lib -L$(OUT)

LD_LIBRARY_PATH=.

C_COMPILE = $(CC) $(INCLUDES) $(CFLAGS) $(DEFS) -o3 -x c
CPP_COMPILE = $(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(DEFS) -o3 -x c++
C_MAKEDEPEND = $(CC) -M $(INCLUDES) $(CFLAGS) $(DEFS)
CPP_MAKEDEPEND = $(CC) -M $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(DEFS)

CCLD = $(CC)
LINK = $(LIBTOOL) $(LD_DIRS) $(CFLAGS) -s $(LDFLAGS)
RM = rm
RMDIR = rmdir

AR_FLAGS = rcs

ROOT = ../..

OUT_ROOT = $(ROOT)/bin
OUT_ = $(OUT_ROOT)/posix
OUT = $(OUT_)/release

TMP = $(ROOT)/int/posix

PREFIX = $(ROOT)/httpd/bin


PHONIES = all clean preinstall install clean_strings strings out_dirs out_dir clean_distro
