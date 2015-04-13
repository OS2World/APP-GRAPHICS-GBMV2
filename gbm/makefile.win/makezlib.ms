# Makefile for Zlib (static)

# This makefile is for MS Visual C++ on Windows 32/64bit.
# It builds the Zlib library as a statically linkable library (.LIB).
#

# set directory where binaries will be placed by target install
IDIR = bin

#
# set compiler
#
CC = cl

# Compile objects suitable for multithreaded GBM.DLL
CWARNS         = /W3 /WX
CFLAGS         = /c $(CWARNS) /O2 /nologo /MT /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG /DWIN32
CFLAGS_DBG     = /c $(CWARNS) /Od /nologo /Zi /GS /RTC1 /MTd /D_CRT_SECURE_NO_DEPRECATE /DWIN32
LIBFLAGS       = /NOLOGO

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
         lib $(LIBFLAGS) /OUT:$@ $(OBJS)

z.obj:   zlib.h zconf.h trees.h deflate.h zutil.h \
         inftrees.h inffast.h inflate.h inffixed.h crc32.h gzguts.h
         $(CC) $(CFLAGS) $*.c


# ------------------------
# Build management targets
# ------------------------

clean:
		 -del /F /Q *.obj 2>nul
		 -del /F /Q *.pdb 2>nul

clobber:
		 -del /F /Q *.obj 2>nul
		 -del /F /Q *.pdb 2>nul
		 -del /F /Q *.lib 2>nul
		 -del /F /Q $(IDIR)\* 2>nul

install:
         copy z.lib     $(IDIR)
         copy zlib.h    $(IDIR)
         copy zconf.h   $(IDIR)

package:
         -del /F $(IDIR)\zlib.zip 2>nul
         zip -9 $(IDIR)\zlib.zip z.lib zlib.h zconf.h

