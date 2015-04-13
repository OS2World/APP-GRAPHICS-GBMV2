# Makefile for libraw library (static)

# This makefile is for Open Watcom C++ on OS/2.
# It builds the library as a statically linkable library (.LIB).
#
# Stripped down makefile for the Generalised Bitmap Module.

# set directory where binaries will be placed by target install
IDIR = bin

#
# Using Open Watcom C++:
#
CC = wpp386

# Compile objects suitable for multithreaded GBM.DLL
CWARNS         = -w1 -we
CFLAGS         = $(CWARNS) -i. -i.. -bt=os2 -zq -onatxh -oe=50 -sg -ei -6r -fp6 -fpi87 -bm -mf -xs -xr -DLIBRAW_NODLL -DNDEBUG
CFLAGS_DBG     = $(CWARNS) -i. -i.. -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -xs -xr -d2 -DLIBRAW_NODLL -DDEBUG
LIBFLAGS       = -q -n -b -c

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
         wlib $(LIBFLAGS) $@ +internal\dcraw_common.obj +internal\dcraw_fileio.obj +internal\demosaic_packs.obj \
                             +src\libraw_cxx.obj +src\libraw_c_api.obj +src\libraw_datastream.obj

internal\dcraw_common.obj:  internal\dcraw_common.cpp
                            $(CC) $(CFLAGS) -fo="internal\dcraw_common.obj" internal\dcraw_common.cpp

internal\demosaic_packs.obj:  internal\demosaic_packs.cpp
                            $(CC) $(CFLAGS) -fo"internal\demosaic_packs.obj" internal\demosaic_packs.cpp
                            
internal\dcraw_fileio.obj:  internal\dcraw_fileio.cpp
                            $(CC) $(CFLAGS) -fo="internal\dcraw_fileio.obj" internal\dcraw_fileio.cpp

src\libraw_cxx.obj:    src\libraw_cxx.cpp
                       $(CC) $(CFLAGS) -fo="src\libraw_cxx.obj" src\libraw_cxx.cpp

src\libraw_c_api.obj:  src\libraw_c_api.cpp
                       $(CC) $(CFLAGS) -fo="src\libraw_c_api.obj" src\libraw_c_api.cpp

src\libraw_datastream.obj:  src\libraw_datastream.cpp
                       $(CC) $(CFLAGS) -fo="src\libraw_datastream.obj" src\libraw_datastream.cpp


#simple_dcraw.exe: raw.lib simple_dcraw.obj
#	wlink SYSTEM nt op stack=0x65000 name $@ file simple_dcraw.obj library raw.lib

#simple_dcraw.obj:  simple_dcraw.cpp

#raw-identify.exe: raw.lib raw-identify.obj
#	wlink SYSTEM nt op stack=0x65000 name $@ file raw-identify.obj library raw.lib

#raw-identify.obj:  raw-identify.cpp

                       
# ------------------------
# Build management targets
# ------------------------

clean:
	 -del /F /N internal\*.obj src\*.obj 2>nul
	 -del /F /N *.lst 2>nul

clobber:
	 -del /F /N internal\*.obj src\*.obj 2>nul
	 -del /F /N raw.lib 2>nul
	 -del /F /N *.lst 2>nul
	 -del /F /N $(IDIR)\* 2>nul

install:
         copy raw.lib     $(IDIR)
         copy libraw\*.h  $(IDIR)

package:
         -del /F /N $(IDIR)\libraw.zip 2>nul
         zip -9 $(IDIR)\libraw.zip raw.lib *.h

