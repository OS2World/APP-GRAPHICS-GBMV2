#
# Simple full screen bitmap animation viewer
#
# This makefile is for Open Watcom C++ on OS/2.
# (The program requires fullscreen mode which doesn't seem to work.)

# include configuration setting for nmake (except compiler options)
!INCLUDE ..\..\make.opt

GBM    = ..\..\gbm
IDIR   = ..\bin

# ------------------
# Configure compiler
# ------------------

#
# Using Open Watcom C++:
#
CC      = wcc386
CL      = wlink

# Builds gbm objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS     = -wx
CFLAGS     = $(CWARNS) -i$(GBM) -bt=os2 -zq -onatxh -sg -ei -6r -fp6 -fpi87 -bm -mf /DNDEBUG
CFLAGS_DBG = $(CWARNS) -i$(GBM) -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 /DDEBUG

LFLAGS     = SYSTEM os2v2 fullscreen op q op el op stack=0x16384
LFLAGS_DBG = SYSTEM os2v2 fullscreen op q d all op stack=0x16384

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS     = $(CFLAGS_DBG)
LFLAGS     = $(LFLAGS_DBG)
!endif

#

.SUFFIXES:	.c .obj

.c.obj:
		$(CC) $(CFLAGS) $*.c

#

gbmvfsa.exe:	gbmvfsa.obj gbmvfsa.lnk $(GBM)\gbm.lib $(GBM)\gbmtool.lib
		$(CL) $(LFLAGS) @gbmvfsa name $@ file gbmvfsa.obj library $(GBM)\gbm.lib,$(GBM)\gbmtool.lib

gbmvfsa.obj:	gbmvfsa.c $(GBM)\gbm.h $(GBM)\gbmtool.h

#

clean:
		-del /F /N *.obj 2>nul

clobber:
		-del /F /N *.obj *.exe $(IDIR)\gbmvfsa.exe 2>nul

#


install:
		copy gbmvfsa.exe $(IDIR)

#

exepack:
         $(EXE_PACKER) *.exe

#

package:
       -del /F /N $(IDIR)\gbmvfsa_exe.zip 2>nul
       zip -9 $(IDIR)\gbmvfsa_exe.zip gbmvfsa.exe

