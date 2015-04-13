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
# This makefile is for Open Watcom C++ on Windows.
# NOTE: OpenWatcom build does not yet work!!! (crashes when plugin is loaded)
#
# Builds the adapter library for GBM.DLL.
# As GBM.DLL is built multithreaded, all users should also be multithreaded.

# Location of gbm libs and public header files
COMMON    = ..\..\common
GBM       = ..\..\..\gbm
NP_SDK    = ..\sdk\win
GECKO_SDK = ..\sdk\gecko

# Directory where npgbm.dll will be placed
DLL_OUT = ..

# ------------------
# Configure compiler
# ------------------
INCL = -i. -i$(COMMON) -i$(GBM) -i$(NP_SDK) -i$(GECKO_SDK)\plugin\include -i$(GECKO_SDK)\nspr\include -i$(GECKO_SDK)\java\include

#
# Using Open Watcom C++:
#
CWARNS         = -w3 -we
CFLAGS         = $(CWARNS) -bt=nt -zq -zv -onatxh -sg -ei -6r -fp6 -fpi87 -bg -bm -mf -xs -xr $(INCL) /D_X86_ /DWIN32 /D_WINDOWS /DXP_WIN /D_WINDLL /DNDEBUG
CFLAGS_DBG     = $(CWARNS) -bt=nt -zq -sg -ei -6r -fp6 -fpi87 -bg -bm -mf -xs -xr $(INCL) -d2 /D_X86_ /DWIN32 /D_WINDOWS /DXP_WIN /D_WINDLL /DDEBUG
CFLAGS_DLL     = -bd $(CFLAGS)
CFLAGS_DLL_DBG = -bd $(CFLAGS_DBG)

LFLAGS         = op q op el op vfr
LFLAGS_DBG     = op q d all
LFLAGS_DLL     = sys nt_dll initi termi runtime native=5.0 op caseexact op nostdcall $(LFLAGS)
LFLAGS_DLL_DBG = sys nt_dll initi termi runtime native=5.0 op caseexact op nostdcall $(LFLAGS_DBG)
LFLAGS_DLL_VISTA     = sys nt_dll initi termi runtime native=6.0 op caseexact op nostdcall $(LFLAGS)
LFLAGS_DLL_VISTA_DBG = sys nt_dll initi termi runtime native=6.0 op caseexact op nostdcall $(LFLAGS_DBG)

# Default Windows version is Windows 2000 and higher
WIN_VERSION = /D_WIN32_WINNT=0x0500 /DWINVER=0x0500
LIBS       = $(COMMON)\common.lib,$(GBM)\gbmmem.lib,$(GBM)\gbmmthrd.lib,$(GBM)\gbmscale.lib,$(GBM)\gbmmir.lib,$(GBM)\gbmrect.lib,kernel32.lib,user32.lib,gdi32.lib,comdlg32.lib,comctl32.lib
LIBS_VISTA = ole32.lib,shlwapi.lib,shell32.lib

SUBMAKEOPT =

# For support of the new Vista/Win7 file dialog (not Windows 2000/XP compatible) call nmake with vista=yes
!ifdef vista
WIN_VERSION    = /D_WIN32_WINNT=0x0600 /DWINVER=0x0600
LIBS           = $(LIBS),$(LIBS_VISTA)
LFLAGS_DLL     = $(LFLAGS_DLL_VISTA)
LFLAGS_DLL_DBG = $(LFLAGS_DLL_VISTA_DBG)
!endif

# For debugging call nmake with debug=yes
!ifdef debug
CFLAGS_DLL = $(CFLAGS_DLL_DBG)
LFLAGS_DLL = $(LFLAGS_DLL_DBG)
SUBMAKEOPT = $(SUBMAKEOPT) debug=yes
!endif

#

.SUFFIXES: .cpp .obj .rc .res

.cpp.obj:
           wpp386 $(CFLAGS_DLL) $(WIN_VERSION) $*.cpp

.rc.res:
           wrc -q -r -zm $*.rc

#

all:   common.lib $(DLL_OUT)\npgbm.dll

#

$(DLL_OUT)\npgbm.dll:  $(NP_SDK)\np_entry.obj $(NP_SDK)\npn_gate.obj $(NP_SDK)\npp_gate.obj \
                       npgbm.obj GbmBitmapViewer.obj GbmSaveFileDialog.obj unicode.obj npgbm.lnk npgbm.res
                       wlink $(LFLAGS_DLL) @npgbm name $@ file $(NP_SDK)\np_entry.obj,$(NP_SDK)\npn_gate.obj,$(NP_SDK)\npp_gate.obj,npgbm.obj,GbmBitmapViewer.obj,GbmSaveFileDialog.obj,unicode.obj \
                                                          lib  $(LIBS)
                       wrc -q npgbm.res $@

$(NP_SDK)\np_entry.obj:  $(NP_SDK)\np_entry.cpp $(NP_SDK)\npplat.h $(NP_SDK)\pluginbase.h $(GECKO_SDK)\plugin\include\npapi.h
                         wpp386 $(CFLAGS_DLL) $(WIN_VERSION) -fo="$(NP_SDK)\np_entry.obj" $(NP_SDK)\np_entry.cpp

$(NP_SDK)\npn_gate.obj:  $(NP_SDK)\npn_gate.cpp $(NP_SDK)\npplat.h $(NP_SDK)\pluginbase.h $(GECKO_SDK)\plugin\include\npapi.h
                         wpp386 $(CFLAGS_DLL) $(WIN_VERSION) -fo="$(NP_SDK)\npn_gate.obj" $(NP_SDK)\npn_gate.cpp

$(NP_SDK)\npp_gate.obj:  $(NP_SDK)\npp_gate.cpp $(NP_SDK)\npplat.h $(NP_SDK)\pluginbase.h $(GECKO_SDK)\plugin\include\npapi.h
                         wpp386 $(CFLAGS_DLL) $(WIN_VERSION) -fo="$(NP_SDK)\npp_gate.obj" $(NP_SDK)\npp_gate.cpp

npgbm.obj:     npgbm.cpp npgbmrc.h $(NP_SDK)\pluginbase.h $(NP_SDK)\npplat.h $(GECKO_SDK)\plugin\include\npapi.h $(COMMON)\windefs.h \
                 $(COMMON)\GbmException.hpp $(COMMON)\GbmAccessor.hpp $(COMMON)\GbmDocument.hpp $(COMMON)\GbmDocumentPage.hpp $(COMMON)\GbmRenderer.hpp \
                 $(COMMON)\ConfigHandler.hpp $(GBM)\gbm.h $(GBM)\gbmmem.h $(GBM)\gbmscale.h $(GBM)\gbmmir.h $(GBM)\gbmrect.h

GbmBitmapViewer.obj:  GbmBitmapViewer.cpp GbmBitmapViewer.hpp unicode.hpp npgbmrc.h $(COMMON)\windefs.h \
                        $(COMMON)\GbmException.hpp $(COMMON)\GbmAccessor.hpp $(COMMON)\GbmDocument.hpp $(COMMON)\GbmDocumentPage.hpp $(COMMON)\GbmRenderer.hpp \
                        $(COMMON)\Semaphore.hpp $(COMMON)\ConfigHandler.hpp $(GBM)\gbm.h $(GBM)\gbmmem.h $(GBM)\gbmscale.h $(GBM)\gbmmir.h $(GBM)\gbmrect.h

GbmSaveFileDialog.obj:  GbmSaveFileDialog.cpp GbmSaveFileDialog.hpp unicode.hpp $(COMMON)\windefs.h $(COMMON)\GbmAccessor.hpp

unicode.obj:    unicode.cpp unicode.hpp

npgbm.res:     npgbm.rc npgbmrc.h $(GECKO_SDK)\plugin\include\npapi.h

common.lib:
        (cd $(COMMON) && wmake -c -ms -h -f makefile.wat_w32 $(SUBMAKEOPT))

# ------------------------
# Build management targets
# ------------------------

clean:
    (cd $(COMMON) && wmake -c -ms -h -f makefile.wat_w32 clean)
    -del /F /Q *.obj $(NP_SDK)\*.obj *.res *.pdb *.map $(DLL_OUT)\npgbm.exp $(DLL_OUT)\npgbm.map $(DLL_OUT)\npgbm.lib $(DLL_OUT)\npgbm.pdb 2>nul

clobber:
    (cd $(COMMON) && wmake -c -ms -h -f makefile.wat_w32 clobber)
    -del /F /Q *.obj $(NP_SDK)\*.obj *.res *.pdb *.map $(DLL_OUT)\npgbm.exp $(DLL_OUT)\npgbm.map $(DLL_OUT)\npgbm.lib $(DLL_OUT)\npgbm.pdb $(DLL_OUT)\npgbm.dll 2>nul

#

install:

#

exepack:


#

package:


