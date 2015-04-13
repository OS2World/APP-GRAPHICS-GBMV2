#
# Simple bitmap viewer II (German/English)
#
# This makefile is for Open Watcom C on OS/2.
#
# For English build call wmake with lang=en (default is lang=de)
#

# include configuration setting for wmake (except compiler options)
!INCLUDE ..\..\make.opt

GBM    = ..\..\gbm
GBMDLG = ..\gbmdlg
IDIR   = ..\bin

# ------------------
# Configure compiler
# ------------------

#
# Using Open Watcom C:
#
CC      = wcc386
CL      = wlink
RC      = wrc

# Builds gbm objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS     = -wx -we
CFLAGS     = $(CWARNS) -i. -i$(GBM) -i$(GBMDLG) -bt=os2 -zq -onatxh -sg -ei -6r -fp6 -fpi87 -bm -mf /DNDEBUG
CFLAGS_DBG = $(CWARNS) -i. -i$(GBM) -i$(GBMDLG) -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 /DDEBUG

LFLAGS     = SYSTEM os2v2_pm op q op el op stack=0x10000
LFLAGS_DBG = SYSTEM os2v2_pm op q d all op stack=0x10000

RCCFLAGS    = -q -r
RCLFLAGS    = -q

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS     = $(CFLAGS_DBG)
LFLAGS     = $(LFLAGS_DBG)
!endif

# For English build call wmake with lang=en
LANG_DIR=de
IPFCOPT=/country=049 /codepage=850 /language=DEU /X /W1

!ifdef lang
!if ( "$(lang)" == "en" )
LANG_DIR=en
IPFCOPT=/country=001 /codepage=437 /language=ENG /X /W1
!endif
!endif

LANG_DIR_RES=$(LANG_DIR)\WarpSans

#

.SUFFIXES:	.c .obj

.c.obj:
		$(CC) $(CFLAGS) $*.c

#

all:   gbmv2.exe gbmv2.hlp

OBJS = scroll.obj model.obj bmputils.obj help.obj gbmv2.obj

LIBS =  $(GBM)\gbm.lib $(GBM)\gbmmem.lib \
        $(GBM)\gbmtrunc.lib $(GBM)\gbmerr.lib $(GBM)\gbmht.lib \
        $(GBM)\gbmhist.lib $(GBM)\gbmmcut.lib \
        $(GBM)\gbmmir.lib $(GBM)\gbmrect.lib $(GBM)\gbmscale.lib \
        $(GBM)\gbmtool.lib $(GBMDLG)\gbmdlg.lib

gbmv2.exe:  $(OBJS) $(LIBS) gbmv2$(LANG_DIR).lnk gbmv2.res
            $(CL) $(LFLAGS) @gbmv2$(LANG_DIR) name $@ \
                file    scroll.obj,model.obj,bmputils.obj,help.obj,gbmv2.obj \
                library $(GBM)\gbm.lib,$(GBM)\gbmmem.lib,$(GBM)\gbmtrunc.lib,$(GBM)\gbmerr.lib,\
                        $(GBM)\gbmht.lib,$(GBM)\gbmhist.lib,$(GBM)\gbmmcut.lib,\
                        $(GBM)\gbmmir.lib,$(GBM)\gbmrect.lib,$(GBM)\gbmscale.lib,\
                        $(GBM)\gbmtool.lib,$(GBM)\gbmmthrd.lib,$(GBMDLG)\gbmdlg.lib
            $(RC) $(RCLFLAGS) gbmv2.res $@

scroll.obj:    scroll.c scroll.h

model.obj:    model.c model.h \
              $(GBM)\gbm.h $(GBM)\gbmmem.h \
              $(GBM)\gbmtrunc.h $(GBM)\gbmerr.h $(GBM)\gbmht.h \
              $(GBM)\gbmhist.h $(GBM)\gbmmcut.h \
              $(GBM)\gbmmir.h $(GBM)\gbmrect.h $(GBM)\gbmscale.h

bmputils.obj:    bmputils.c bmputils.h

help.obj:    $(LANG_DIR)\help.c gbmv2hlp.h
             $(CC) $(CFLAGS) -fo=$@ $(LANG_DIR)\help.c

gbmv2.obj:   $(LANG_DIR)\gbmv2.c gbmv2.h scroll.h model.h bmputils.h help.h \
             $(GBM)\gbm.h $(GBM)\gbmtool.h \
             $(GBMDLG)\gbmdlg.h $(GBMDLG)\gbmdlgrc.h
             $(CC) $(CFLAGS) -fo=$@ $(LANG_DIR)\gbmv2.c

gbmv2.res:   $(LANG_DIR_RES)\gbmv2.rc gbmv2.ico $(GBMDLG)\gbmdlgrc.h gbmv2.h gbmv2hlp.h
             (set INCLUDE=$(GBMDLG);$(INCLUDE) && $(RC) $(RCCFLAGS) -fo=$@ $(LANG_DIR_RES)\gbmv2.rc)

gbmv2.hlp:    $(LANG_DIR)\gbmv2.scr gbmv2hlp.h $(GBMDLG)\gbmdlgrc.h
              (set INCLUDE=$(GBMDLG);$(INCLUDE) && ipfcprep $(LANG_DIR)\gbmv2.scr gbmv2.ipf)
              ipfc $(IPFCOPT) gbmv2.ipf

#

clean:
        -del /F /N *.obj *.res *.ipf 2>nul

clobber:
        -del /F /N *.obj *.res *.ipf *.exe *.hlp 2>nul
        -del /F /N $(IDIR)\gbmv2.exe $(IDIR)\gbmv2.hlp 2>nul


install:
        copy *.exe $(IDIR)
        copy *.hlp $(IDIR)

#

exepack:
         $(EXE_PACKER) *.exe

#

package:
        -del /F /N $(IDIR)\gbmv2_exe.zip 2>nul
        zip -9 $(IDIR)\gbmv2_exe.zip *.exe *.hlp

