# Makefile for JBIG Kit library

# This makefile is for MS Visual C++ on Windows 32/64bit.
# It builds the library as a statically linkable library (.LIB).
#
# Stripped down makefile for the Generalised Bitmap Module.

# set directory where binaries will be placed by target install
IDIR = bin

#
# set compiler
#
CC = cl

# Compile objects suitable for multithreaded GBM.DLL
CWARNS         = /W3 /WX
CFLAGS         = /c $(CWARNS) /I.. /O2 /nologo /MT /D_CRT_SECURE_NO_DEPRECATE
CFLAGS_DBG     = /c $(CWARNS) /I.. /Od /nologo /Zi /GS /RTC1 /MTd /D_CRT_SECURE_NO_DEPRECATE /DDEBUG
LIBFLAGS       = /NOLOGO

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS   = $(CFLAGS_DBG)
!endif

# Template command for compiling .c to .obj
.c.obj :
	$(CC) $(CFLAGS) $*.c

all: jbig.c jbig_ar.c jbig.lib

jbig.lib: jbig.obj jbig_ar.obj
          lib $(LIBFLAGS) /OUT:$@ $**


# ------------------------
# Build management targets
# ------------------------

clean:
	 -del /F jbig.obj jbig_ar.obj 2>nul
	 -del /F /Q *.pdb 2>nul

clobber:
	 -del /F jbig.obj jbig_ar.obj 2>nul
	 -del /F jbig.lib 2>nul
	 -del /F /Q *.pdb $(IDIR)\* 2>nul
	 -del /F /Q $(IDIR)\* 2>nul

install:
         copy jbig.lib     $(IDIR)
         copy jbig.h       $(IDIR)
         copy jbig_ar.h    $(IDIR)

package:
         -del /F $(IDIR)\libjbig.zip 2>nul
         zip -9 $(IDIR)\libjbig.zip jbig.lib jbig.h jbig_ar.h


jbig.obj:    jbig.c jbig.h jbig_ar.h
jbig_ar.obj: jbig_ar.c jbig_ar.h

