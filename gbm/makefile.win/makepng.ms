# Makefile for libpng (static)

# This makefile is for MS Visual C++ on Windows 32/64bit.
# It builds the libpng library as a statically
# linkable library (.LIB).

# Modify this line to point to the zlib library 1.2.3 or higher
ZLIB = ..\zlib

# set directory where binaries will be placed by target install
IDIR = bin

#
# set compiler
#
CC = cl

# Compile objects suitable for multithreaded GBM.DLL
CWARNS         = /W3 /WX
CFLAGS         = /c $(CWARNS) /I$(ZLIB) /O2 /nologo /MT /D_CRT_SECURE_NO_DEPRECATE /DPNG_USER_CONFIG /DNDEBUG /DWIN32
CFLAGS_DBG     = /c $(CWARNS) /I$(ZLIB) /Od /Zi /GS /RTC1 /nologo /MTd /D_CRT_SECURE_NO_DEPRECATE /DPNG_USER_CONFIG /DDEBUG /DWIN32
LIBFLAGS       = /NOLOGO

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
          lib $(LIBFLAGS) /OUT:$@ $(OBJS)

png.obj:  png.c pngusr.h png.h pngconf.h pnglibconf.h pngpriv.h pngstruct.h pnginfo.h pngdebug.h
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
         -del /F /Q *.exe 2>nul
         -del /F /Q *.lib 2>nul
         -del /F /Q $(IDIR)\* 2>nul

install:
         copy png.lib     $(IDIR)
         copy png.h       $(IDIR)
         copy pngconf.h   $(IDIR)

package:
         -del /F $(IDIR)\libpng.zip 2>nul
         zip -9 $(IDIR)\libpng.zip png.lib png.h

