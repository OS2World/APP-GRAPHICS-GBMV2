#
# Simple bitmap viewer
#
# This makefile is for Open Watcom C++ on OS/2.
#

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
RC      = wrc

# Builds gbm objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS     = -wx -we
CFLAGS     = $(CWARNS) -i$(GBM) -bt=os2 -zq -onatxh -sg -ei -6r -fp6 -fpi87 -bm -mf /DNDEBUG
CFLAGS_DBG = $(CWARNS) -i$(GBM) -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 /DDEBUG

LFLAGS     = SYSTEM os2v2_pm op q op el op stack=0x10000
LFLAGS_DBG = SYSTEM os2v2_pm op q d all op stack=0x10000

RCCFLAGS    = -q -r -zm
RCLFLAGS    = -q

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS     = $(CFLAGS_DBG)
LFLAGS     = $(LFLAGS_DBG)
!endif

#

.SUFFIXES: .c .obj

.c.obj:
        $(CC) $(CFLAGS) $*.c

#

gbmv.exe: gbmv.obj gbmv.lnk $(GBM)\gbm.lib $(GBM)\gbmmem.lib $(GBM)\gbmerr.lib $(GBM)\gbmht.lib $(GBM)\gbmscale.lib $(GBM)\gbmtool.lib gbmv.res
          $(CL) $(LFLAGS) @gbmv name $@ file gbmv.obj library $(GBM)\gbm.lib,$(GBM)\gbmmem.lib,$(GBM)\gbmmthrd.lib,$(GBM)\gbmerr.lib,$(GBM)\gbmht.lib,$(GBM)\gbmscale.lib,$(GBM)\gbmtool.lib
          $(RC) $(RCLFLAGS) gbmv.res $@

gbmv.obj: gbmv.c gbmv.h $(GBM)\gbm.h $(GBM)\gbmmem.h $(GBM)\gbmerr.h $(GBM)\gbmht.h $(GBM)\gbmscale.h $(GBM)\gbmtool.h

gbmv.res: gbmv.rc gbmv.h gbmv.ico
          $(RC) $(RCCFLAGS) gbmv.rc

#

clean:
         -del /F /N *.obj *.res 2>nul

clobber:
         -del /F /N *.obj *.res *.exe $(IDIR)\gbmv.exe 2>nul

#

install:
         copy *.exe $(IDIR)

#

exepack:
         $(EXE_PACKER) *.exe

#

package:
        -del /F /N $(IDIR)\gbmv_exe.zip 2>nul
        zip -9 $(IDIR)\gbmv_exe.zip *.exe

