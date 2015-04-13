#
# GBM program to help generate OS/2 logo files
#
# This makefile is for Open Watcom C++ on OS/2.
#

# include configuration setting for nmake (except compiler options)
!INCLUDE ..\..\make.opt

GBM  = ..\..\gbm
IDIR = ..\bin

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

LFLAGS     = SYSTEM os2v2 op q op el
LFLAGS_DBG = SYSTEM os2v2 op q d all

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

gbmlogo.exe:	gbmlogo.obj \
		$(GBM)\gbm.lib $(GBM)\gbmmem.lib $(GBM)\gbmtrunc.lib $(GBM)\gbmerr.lib $(GBM)\gbmht.lib $(GBM)\gbmtool.lib
		$(CL) $(LFLAGS) name $@ file gbmlogo.obj library $(GBM)\gbm.lib,$(GBM)\gbmmem.lib,$(GBM)\gbmtrunc.lib,$(GBM)\gbmerr.lib,$(GBM)\gbmht.lib,$(GBM)\gbmtool.lib

gbmlogo.obj:	gbmlogo.c $(GBM)\gbm.h $(GBM)\gbmmem.h $(GBM)\gbmtrunc.h $(GBM)\gbmerr.h $(GBM)\gbmht.h $(GBM)\gbmtool.h

#

clean:
		-del /F /N *.obj 2>nul

clobber:
		-del /F /N *.obj *.exe $(IDIR)\gbmlogo.exe 2>nul

#

install:
		copy *.exe $(IDIR)

#

exepack:
         $(EXE_PACKER) *.exe

#

package:
        -del /F /N $(IDIR)\gbmlogo_exe.zip 2>nul
        zip -9 $(IDIR)\gbmlogo_exe.zip *.exe

