#
# Generalised Bitmap Module Mozilla/Firefox/SeaMonkey adapter
#
# Copyright (C) 2006-2012 Heiko Nitzsche
#
#   This software is provided 'as-is', without any express or implied
#   warranty.  In no event will the author be held liable for any damages
#   arising from the use of this software.
#
#   Permission is granted to anyone to use this software for any purpose,
#   including commercial applications, and to alter it and redistribute it
#   freely, subject to the following restrictions:
#
#   1. The origin of this software must not be misrepresented; you must not
#      claim that you wrote the original software. If you use this software
#      in a product, an acknowledgment in the product documentation would be
#      appreciated but is not required.
#   2. Altered source versions must be plainly marked as such, and must not be
#      misrepresented as being the original software.
#   3. This notice may not be removed or altered from any source distribution.
#
# This makefile is for Open Watcom C++ on OS/2.
#
# Builds the library NPGBM.DLL.
#

# include configuration setting for nmake (except compiler options)
!INCLUDE ..\..\..\make.opt

OS2TK_INC    = $(OS2TK)\h
OS2TK_LIB    = $(OS2TK)\lib

# Location of gbm libs and public header files
COMMON = ..\..\common
GBM    = ..\..\..\gbm
GBMDLG = ..\..\..\gbmv_pm\gbmdlg
NP_SDK = ..\sdk\os2

# Directory where npgbm.dll will be placed
DLL_OUT = ..

# ------------------
# Configure compiler
# ------------------

#
# Using Open Watcom C++:
#
CWARNS         = -w3 -we
CFLAGS         = $(CWARNS) -bt=os2 -zq -zv -onatxh -sg -ei -6r -fp6 -fpi87 -bm -mf -xs -xr /DOS2 /DNDEBUG -i. -i$(GBM) -i$(GBMDLG) -i$(COMMON) -i$(NP_SDK) -i$(OS2TK_INC) /D_USRDLL /D_MBCS
CFLAGS_DBG     = $(CWARNS) -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -xs -xr -i. -i$(GBM) -i$(GBMDLG) -i$(COMMON) -i$(NP_SDK) -i$(OS2TK_INC) -d2  /D_USRDLL /D_MBCS /DOS2 /DDEBUG
CFLAGS_DLL     = -bd $(CFLAGS)
CFLAGS_DLL_DBG = -bd $(CFLAGS_DBG)

LFLAGS         = op q op el op vfr
LFLAGS_DBG     = op q d all
LFLAGS_DLL     = sys os2v2_dll initi termi pm op many op caseexact $(LFLAGS)
LFLAGS_DLL_DBG = sys os2v2_dll initi termi pm op many op caseexact $(LFLAGS_DBG)

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS_DLL = $(CFLAGS_DLL_DBG)
LFLAGS_DLL = $(LFLAGS_DLL_DBG)
!endif

# Enable for Pentium profiling (also combined with debug above)
#CFLAGS_DLL     = $(CFLAGS_DLL) -et

#

.SUFFIXES: .cpp .obj .rc .res

.cpp.obj:
           wpp386 $(CFLAGS_DLL) $*.cpp

.rc.res:
           wrc -q -bt=os2 -r -zm -i=$(NP_SDK) -i$(GBMDLG) $*.rc
#           rc -i $(NP_SDK) -i $(GBMDLG) -n -r -x2 $*.rc

#

all:   common.lib $(DLL_OUT)\npgbm.dll

#

$(DLL_OUT)\npgbm.dll:  $(NP_SDK)\npos2.obj npgbm.obj GbmBitmapViewer.obj GbmSaveFileDialog.obj npgbm.lnk npgbm.res
                       wlink $(LFLAGS_DLL) @npgbm name $@ file $(NP_SDK)\npos2.obj,npgbm.obj,GbmBitmapViewer.obj,GbmSaveFileDialog.obj \
                                                          lib  $(COMMON)\common.lib,$(GBM)\gbmmem.lib,$(GBM)\gbmmthrd.lib,$(GBM)\gbmscale.lib,$(GBM)\gbmmir.lib,$(GBM)\gbmrect.lib
                       wrc -q npgbm.res $@
#                       rc -n -x2 npgbm.res $@

$(NP_SDK)\npos2.obj:  $(NP_SDK)\npos2.cpp $(NP_SDK)\npapi.h $(NP_SDK)\npupp.h
                      wpp386 $(CFLAGS_DLL) -fo="$(NP_SDK)\npos2.obj" $(NP_SDK)\npos2.cpp

npgbm.obj:     npgbm.cpp $(NP_SDK)\npapi.h $(COMMON)\os2defs.h \
                 $(COMMON)\GbmException.hpp $(COMMON)\GbmAccessor.hpp $(COMMON)\GbmDocument.hpp $(COMMON)\GbmDocumentPage.hpp $(COMMON)\GbmRenderer.hpp \
                 $(COMMON)\ConfigHandler.hpp $(GBM)\gbm.h $(GBM)\gbmmem.h $(GBM)\gbmscale.h $(GBM)\gbmmir.h $(GBM)\gbmrect.h

GbmBitmapViewer.obj:  GbmBitmapViewer.cpp GbmBitmapViewer.hpp GbmBitmapViewer_rc.h $(COMMON)\os2defs.h \
                 $(COMMON)\GbmException.hpp $(COMMON)\GbmAccessor.hpp $(COMMON)\GbmDialogAccessor.hpp $(COMMON)\GbmDocument.hpp $(COMMON)\GbmDocumentPage.hpp $(COMMON)\GbmRenderer.hpp \
                 $(COMMON)\Semaphore.hpp $(COMMON)\ConfigHandler.hpp $(GBM)\gbm.h $(GBM)\gbmmem.h $(GBM)\gbmscale.h $(GBM)\gbmmir.h $(GBM)\gbmrect.h \
                 $(GBMDLG)\gbmdlg.h

GbmSaveFileDialog.obj:  GbmSaveFileDialog.cpp GbmSaveFileDialog.hpp GbmSaveFileDialog_rc.h $(COMMON)\os2defs.h $(COMMON)\GbmAccessor.hpp $(COMMON)\GbmDialogAccessor.hpp $(GBM)\gbmmem.h $(GBM)\gbmscale.h

npgbm.res:     npgbm.rc GbmBitmapViewer.rc GbmSaveFileDialog.rc GbmBitmapViewer_rc.h GbmSaveFileDialog_rc.h $(NP_SDK)\npapi.h




common.lib:
!ifdef debug
               (cd $(COMMON) && wmake -c -ms -h -f makefile.wat_os2 debug=yes)
!else
               (cd $(COMMON) && wmake -c -ms -h -f makefile.wat_os2)
!endif

# ------------------------
# Build management targets
# ------------------------

clean:
    (cd $(COMMON) && wmake -c -ms -h -f makefile.wat_os2 clean)
    -del /F /N *.obj $(NP_SDK)\*.obj *.res 2>nul

clobber:
    (cd $(COMMON) && wmake -c -ms -h -f makefile.wat_os2 clobber)
    -del /F /N *.obj $(NP_SDK)\*.obj *.res $(DLL_OUT)\npgbm.dll 2>nul

#

install:

#

exepack:
         $(DLL_PACKER) $(DLL_OUT)\*.dll

#

package:


