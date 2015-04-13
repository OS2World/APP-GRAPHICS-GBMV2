# Makefile for libpng (static)

# This makefile is for Open Watcom C++ on OS/2.
# It builds the libpng library as a statically
# linkable library (.LIB).

# Modify this line to point to the zlib library
ZLIB = ..\zlib

# set directory where binaries will be placed by target install
IDIR = bin

#
# Using Open Watcom C++:
#
CC      = wcc386
CLIB    = wlib

# Builds libpng objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS     = -wx -wcd=124 -wcd=136 -we
CFLAGS     = $(CWARNS) -i$(ZLIB) -bt=os2 -zq -onatxh -oe=50 -sg -ei -6r -fp6 -fpi87 -bm -mf /DPNG_USER_CONFIG /DNDEBUG
CFLAGS_DBG = $(CWARNS) -i$(ZLIB) -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 /DPNG_USER_CONFIG /DDEBUG

LIBFLAGS     = -q -n -b -c

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS   = $(CFLAGS_DBG)
!endif

.SUFFIXES:	.c .obj

.c.obj:
		$(CC) $(CFLAGS) $*.c

#

OBJS = png.obj pngset.obj pngget.obj pngrutil.obj pngtrans.obj pngwutil.obj \
       pngread.obj pngrio.obj pngwio.obj pngwrite.obj pngrtran.obj          \
       pngwtran.obj pngmem.obj pngerror.obj pngpread.obj

all:   png.lib

png.lib:  $(OBJS)
          $(CLIB) $(LIBFLAGS) $@ +png.obj +pngset.obj +pngget.obj +pngrutil.obj +pngtrans.obj +pngwutil.obj\
                                 +pngread.obj +pngrio.obj +pngwio.obj +pngwrite.obj +pngrtran.obj\
                                 +pngwtran.obj +pngmem.obj +pngerror.obj +pngpread.obj

png.obj:  png.c pngusr.h png.h pngconf.h pnglibconf.h pngpriv.h pngstruct.h pnginfo.h pngdebug.h
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
         -del /F /N *.exe 2>nul
         -del /F /N *.lib 2>nul
         -del /F /N $(IDIR)\* 2>nul

install:
         copy png.lib     $(IDIR)
         copy png.h       $(IDIR)
         copy pngconf.h   $(IDIR)

package:
         -del /F /N $(IDIR)\libpng.zip 2>nul
         zip -9 $(IDIR)\libpng.zip png.lib png.h

