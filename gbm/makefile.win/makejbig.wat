# Makefile for JBIG Kit library

# This makefile is for Open Watcom C++ on OS/2.
# It builds the library as a statically linkable library (.LIB).
#
# Stripped down makefile for the Generalised Bitmap Module.

# set directory where binaries will be placed by target install
IDIR = bin

#
# Using Open Watcom C++:
#
CC = wcc386

# Compile objects suitable for multithreaded GBM.DLL
CWARNS         = -wx -we -za
CFLAGS         = $(CWARNS) -i.. -bt=nt -zq -onatxh -oe=50 -sg -ei -6r -fp6 -fpi87 -bm -mf
CFLAGS_DBG     = $(CWARNS) -i.. -bt=nt -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 /DDEBUG
LIBFLAGS       = -q -n -b -c

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS   = $(CFLAGS_DBG)
!endif

# Template command for compiling .c to .obj
.c.obj :
	$(CC) $(CFLAGS) $*.c


all: jbig.c jbig_ar.c jbig.lib

jbig.lib: jbig.obj jbig_ar.obj
          wlib $(LIBFLAGS) $@ +jbig.obj +jbig_ar.obj

# ------------------------
# Build management targets
# ------------------------

clean:
	 -del /F jbig.obj jbig_ar.obj 2>nul
	 -del /F /Q *.lst 2>nul

clobber:
	 -del /F jbig.obj jbig_ar.obj 2>nul
	 -del /F jbig.lib 2>nul
	 -del /F /Q *.lst 2>nul
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


