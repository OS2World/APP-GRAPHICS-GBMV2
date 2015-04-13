# Makefile for Zlib (static)

# This makefile is for Open Watcom C++ on OS/2 32bit.
# It builds the Zlib library as a statically linkable library (.LIB).
#

# set directory where binaries will be placed by target install
IDIR = bin

#
# Using Open Watcom C++:
#
CC      = wcc386
CL      = wlink
CLIB    = wlib

# Builds zlib objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS     = -wx -wcd=201 -wcd=302 -we
CFLAGS     = $(CWARNS) -bt=os2 -zq -onatxh -oe=50 -sg -ei -6r -fp6 -fpi87 -bm -mf /DNDEBUG
CFLAGS_DBG = $(CWARNS) -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 /DDEBUG

LIBFLAGS       = -q -n -b -c

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS   = $(CFLAGS_DBG)
!endif


.SUFFIXES:	.c .obj

.c.obj:
		$(CC) $(CFLAGS) $*.c

#

OBJS = adler32.obj compress.obj crc32.obj uncompr.obj deflate.obj trees.obj \
       zutil.obj inflate.obj infback.obj inftrees.obj inffast.obj \
       gzclose.obj gzlib.obj gzread.obj gzwrite.obj


all:   z.lib

z.lib:   $(OBJS)
         $(CLIB) $(LIBFLAGS) $@ +adler32.obj +compress.obj +crc32.obj +uncompr.obj\
                                +deflate.obj +trees.obj +zutil.obj +inflate.obj +infback.obj\
                                +inftrees.obj +inffast.obj\
                                +gzclose.obj +gzlib.obj +gzread.obj +gzwrite.obj

z.obj:   zlib.h zconf.h trees.h deflate.h zutil.h \
         inftrees.h inffast.h inflate.h inffixed.h crc32.h gzguts.h
         $(CC) $(CFLAGS) $*.c


# ------------------------
# Build management targets
# ------------------------

clean:
		 -del /F /N *.obj 2>nul
		 -del /F /N *.lst 2>nul

clobber:
		 -del /F /N *.obj 2>nul
		 -del /F /N *.lst 2>nul
		 -del /F /N *.lib 2>nul
		 -del /F /N $(IDIR)\* 2>nul

install:
         copy z.lib     $(IDIR)
         copy zlib.h    $(IDIR)
         copy zconf.h   $(IDIR)

package:
         -del /F /N $(IDIR)\zlib.zip 2>nul
         zip -9 $(IDIR)\zlib.zip z.lib zlib.h zconf.h

