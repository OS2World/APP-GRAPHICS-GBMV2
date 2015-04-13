#
# Generalised Bitmap Module Lucide adapter
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
# Builds Lucide adapter library for GBM.DLL.
# As GBM.DLL is built multithreaded, all users should also be multithreaded.

# include configuration setting for nmake (except compiler options)
!INCLUDE ..\..\make.opt

# Location of gbm libs and public header files
COMMON       = ..\common
GBM          = ..\..\gbm
LUDOC        = ludoc
OS2TK_INC    = $(OS2TK)\h
OS2TK_LIB    = $(OS2TK)\lib
OS2TKSOM     = $(OS2TK)\som
#OS2TKSOM_INC = $(OS2TKSOM)\include
# Use modified SOM includes (patched for GCC, provided by Lucide plugindev pack)
OS2TKSOM_INC = sominc
OS2TKSOM_LIB = $(OS2TKSOM)\lib

# ------------------
# Configure compiler
# ------------------

#
# Using Open Watcom C++:
#

# Builds gbm objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS         = -w3 -we
CFLAGS         = $(CWARNS) -bt=os2 -zq -zv -onatxh -sg -ei -6r -fp6 -fpi87 -bm -mf -xs -xr /DOS2 /DNDEBUG -i. -i$(GBM) -i$(COMMON) -i$(LUDOC) -i$(OS2TK_INC) -i$(OS2TKSOM_INC)
CFLAGS_DBG     = $(CWARNS) -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -xs -xr -i. -i$(GBM) -i$(COMMON) -i$(LUDOC) -i$(OS2TK_INC) -i$(OS2TKSOM_INC) -d2 /DOS2 /DDEBUG
CFLAGS_DLL     = -bd $(CFLAGS)
CFLAGS_DLL_DBG = -bd $(CFLAGS_DBG)

LFLAGS         = op q op el op vfr
LFLAGS_DBG     = op q d all
LFLAGS_DLL     = sys os2v2_dll initi termi op many op caseexact $(LFLAGS)
LFLAGS_DLL_DBG = sys os2v2_dll initi termi op many op caseexact $(LFLAGS_DBG)

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS_DLL     = $(CFLAGS_DLL_DBG)
LFLAGS_DLL     = $(LFLAGS_DLL_DBG)
!endif

# Enable for Pentium profiling (also combined with debug above)
#CFLAGS_DLL     = $(CFLAGS_DLL) -et

# Common source file for update checking
HDR_FILES = $(COMMON)\GbmException.hpp $(COMMON)\GbmDocument.hpp $(COMMON)\GbmDocumentPage.hpp \
            $(COMMON)\GbmRenderer.hpp $(COMMON)\GbmAccessor.hpp $(COMMON)\ConfigHandler.hpp $(GBM)\gbm.h $(GBM)\gbmmem.h $(GBM)\gbmscale.h $(GBM)\gbmmir.h $(GBM)\gbmrect.h \
            $(COMMON)\os2defs.h $(COMMON)\Semaphore.hpp

#

.SUFFIXES: .cpp .obj .idl .xh

.cpp.obj:
           wpp386 $(CFLAGS_DLL) $*.cpp

.idl.xh:
           sc -c -s"xc;xih;xh" -I$(LUDOC) $*.idl
           $(OS2TKSOM_INC)\cnvsomex.cmd $@

#

all:           common.lib lugbm.dll

#

lugbm.dll:     lugbm.obj WGbmDocument.obj
               wlink $(LFLAGS_DLL) @lugbm name $@ file lugbm.obj,WGbmDocument.obj \
                                                  lib $(COMMON)\common.lib,$(GBM)\gbmmem.lib,$(GBM)\gbmmthrd.lib,$(GBM)\gbmscale.lib,$(GBM)\gbmmir.lib,$(GBM)\gbmrect.lib,$(LUDOC)\ludoc.lib,$(OS2TKSOM_LIB)\somtk.lib

lugbm.obj:     lugbm.cpp lugbm.hpp lugbm.xh $(HDR_FILES)

WGbmDocument.obj:  WGbmDocument.cpp WGbmDocument.hpp $(HDR_FILES)

lugbm.cpp:     lugbm.idl

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
    -del /F /N *.obj *.xh *.xih *.xc *.map 2>nul

clobber:
    (cd $(COMMON) && wmake -c -ms -h -f makefile.wat_os2 clobber)
    -del /F /N *.obj *.xh *.xih *.xc *.map *.dll 2>nul

#

install:

#

exepack:
         $(DLL_PACKER) *.dll

#

package:

