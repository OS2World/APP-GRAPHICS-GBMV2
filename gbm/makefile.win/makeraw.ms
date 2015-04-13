# Makefile for libraw library (static)

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
CWARNS         = /W0 /WX
CCOMMON        = /DWIN32 /D_CRT_SECURE_NO_DEPRECATE
CFLAGS         = /c $(CWARNS) /I. /I.. /O2 /EHsc /nologo /MT $(CCOMMON) /DLIBRAW_NODLL /DNDEBUG
CFLAGS_DBG     = /c $(CWARNS) /I. /I.. /Od /nologo /EHsc /Zi /GS /RTC1 /MTd $(CCOMMON) /DLIBRAW_NODLL /DDEBUG
LIBFLAGS       = /NOLOGO

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS   = $(CFLAGS_DBG)
!endif

# Template command for compiling .cpp to .obj
.cpp.obj :
	$(CC) $(CFLAGS) $*.cpp

all: raw.lib

raw.lib: internal\dcraw_common.obj internal\dcraw_fileio.obj internal\demosaic_packs.obj \
         src\libraw_cxx.obj src\libraw_c_api.obj src\libraw_datastream.obj
          lib $(LIBFLAGS) /OUT:$@ $**
     
internal\dcraw_common.obj:  internal\dcraw_common.cpp
                            $(CC) $(CFLAGS) /Fo"internal\dcraw_common.obj" internal\dcraw_common.cpp

internal\demosaic_packs.obj:  internal\demosaic_packs.cpp
                            $(CC) $(CFLAGS) /Fo"internal\demosaic_packs.obj" internal\demosaic_packs.cpp
                            
internal\dcraw_fileio.obj:  internal\dcraw_fileio.cpp
                            $(CC) $(CFLAGS) /Fo"internal\dcraw_fileio.obj" internal\dcraw_fileio.cpp

src\libraw_cxx.obj:    src\libraw_cxx.cpp
                       $(CC) $(CFLAGS) /Fo"src\libraw_cxx.obj" src\libraw_cxx.cpp

src\libraw_c_api.obj:  src\libraw_c_api.cpp
                       $(CC) $(CFLAGS) /Fo"src\libraw_c_api.obj" src\libraw_c_api.cpp
     
src\libraw_datastream.obj:  src\libraw_datastream.cpp
                       $(CC) $(CFLAGS) /Fo"src\libraw_datastream.obj" src\libraw_datastream.cpp

# ------------------------
# Build management targets
# ------------------------

clean:
	 -del /F internal\*.obj src\*.obj 2>nul
	 -del /F /Q *.pdb 2>nul

clobber:
	 -del /F internal\*.obj src\*.obj 2>nul
	 -del /F raw.lib 2>nul
	 -del /F /Q *.pdb $(IDIR)\* 2>nul
	 -del /F /Q $(IDIR)\* 2>nul

install:
         copy raw.lib     $(IDIR)
         copy libraw\*.h  $(IDIR)

package:
         -del /F $(IDIR)\libraw.zip 2>nul
         zip -9 $(IDIR)\libraw.zip raw.lib *.h

