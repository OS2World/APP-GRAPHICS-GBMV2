#
# GBM enhanced file dialog (German/English)
#
# This makefile is for Open Watcom C on OS/2.
#
# Builds GBMDLG.DLL which is compiled multithreaded
# Therefore all users should also be multithreaded
#
# For English build call wmake with lang=en (default is lang=de)
#

# include configuration setting for wmake (except compiler options)
!INCLUDE ..\..\make.opt

GBM  = ..\..\gbm
IDIR = ..\bin

# ------------------
# Configure compiler
# ------------------

#
# Using Open Watcom C:
#
CC      = wcc386
CL      = wlink
CLIB    = wlib
RC      = wrc

# Builds gbm objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS         = -wx -we
CFLAGS         = $(CWARNS) -i$(GBM) -bd -bt=os2 -zq -onatxh -sg -ei -6r -fp6 -fpi87 -bm -mf /DNDEBUG
CFLAGS_DBG     = $(CWARNS) -i$(GBM) -bd -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 /DDEBUG
CFLAGS_DLL     = $(CFLAGS) -bd
CFLAGS_DLL_DBG = $(CFLAGS_DBG) -bd

LFLAGS      = sys os2v2_dll initi termi op many op caseexact op q op el
LFLAGS_DBG  = sys os2v2_dll initi termi op many op caseexact op q d all

LIBFLAGS    = -q -n -b -c

RCCFLAGS    = -q -r -zm
RCLFLAGS    = -q

# For debugging call wmake with debug=on
!ifdef debug
CFLAGS     = $(CFLAGS_DBG)
CFLAGS_DLL = $(CFLAGS_DLL_DBG)
LFLAGS     = $(LFLAGS_DBG)
!endif

# For English build call wmake with lang=en
LANG_DIR=de
IPFCOPT=/country=049 /codepage=850 /language=DEU /X /W3

!ifdef lang
!if ( "$(lang)" == "en" )
LANG_DIR=en
IPFCOPT=/country=001 /codepage=437 /language=ENG /X /W3
!endif
!endif

LANG_DIR_RES=$(LANG_DIR)\WarpSans

#

.SUFFIXES:	.c .obj

.c.obj:
		$(CC) $(CFLAGS_DLL) $*.c

#

all:		gbmdlg.dll gbmdlg.lib gbmdlg.hlp

#

gbmdlg.dll: gbmdlg.obj $(GBM)\gbm.lib $(GBM)\gbmmem.lib gbmdlg$(LANG_DIR).lnk gbmdlg.res
            $(CL) $(LFLAGS) @gbmdlg$(LANG_DIR) name $@ file gbmdlg.obj library $(GBM)\gbm.lib,$(GBM)\gbmmem.lib,$(GBM)\gbmmthrd.lib,$(GBM)\gbmscale.lib
            $(RC) $(RCLFLAGS) gbmdlg.res $@

gbmdlg.lib: gbmdlg$(LANG_DIR).lnk
            $(CLIB) $(LIBFLAGS) $@ +gbmdlg.dll

gbmdlg.obj: $(LANG_DIR)\gbmdlg.c gbmdlg.h gbmdlgrc.h $(GBM)\gbm.h $(GBM)\gbmmem.h $(GBM)\gbmscale.h
            $(CC) $(CFLAGS_DLL) -fo=$@ $(LANG_DIR)\gbmdlg.c

gbmdlg.res: $(LANG_DIR_RES)\gbmdlg.rc gbmdlgrc.h
            $(RC) $(RCCFLAGS) -fo=$@ $(LANG_DIR_RES)\gbmdlg.rc

gbmdlg.hlp: $(LANG_DIR)\gbmdlg.scr gbmdlgrc.h
            ipfcprep $(LANG_DIR)\gbmdlg.scr gbmdlg.ipf
            ipfc $(IPFCOPT) gbmdlg.ipf

#

clean:
		-del /F /N *.obj *.res *.ipf 2>nul

clobber:
		-del /F /N *.dll *.lib *.obj *.res *.ipf *.hlp 2>nul
		-del /F /N $(IDIR)\gbmdlg.dll $(IDIR)\gbmdlg.hlp $(IDIR)\gbmdlg.h $(IDIR)\gbmdlg.lib 2>nul

#

install:
                copy gbmdlg.dll $(IDIR)
                copy gbmdlg.hlp $(IDIR)
                copy gbmdlg.h   $(IDIR)
                copy gbmdlg.lib $(IDIR)

#

exepack:
         $(DLL_PACKER) *.dll

#

package:
        -del /F /N $(IDIR)\gbmdlg_dll.zip 2>nul
        zip -9 $(IDIR)\gbmdlg_dll.zip  gbmdlg.dll gbmdlg.hlp gbmdlg.h gbmdlg.lib

