#
# Generalised Bitmap Module
#
# This makefile is for Open Watcom C++ on OS/2.
#
# Builds GBM.DLL which is compiled multithreaded
# Therefore all users should also be multithreaded

# include configuration setting for nmake (except compiler options)
!INCLUDE ..\make.opt

# directory of the platform makefiles
MAKEFILE_SRC2 = makefile.os2
MAKEFILE_SRC  = ..\$(MAKEFILE_SRC2)

# Modify this line to point to the Independant JPEG groups library
IJG = libjpeg

# Modify this line to point to the Open JPEG library
OPENJPEG = libojpeg

# Modify these 2 lines to point to the libpng library and the zlib
ZLIB   = zlib
LIBPNG = libpng

# Modify this line to point to the libtiff library
LIBTIFF = libtiff

# Modify this line to point to the JBIG Kit library.
# If you haven't got it or want to skip support for JBIG because of
# of patenting issues, comment out the line instead.
JBIG = libjbig

# Modify this line to point to the LibRaw library
LIBRAW      = libraw
LIBRAW_INCL = $(LIBRAW)\libraw

# Documentation directory
DOCDIR = doc

# Installation directory: gbm.dll, gbm.lib and gbm.h files are copied to this directory
IDIR =  bin

# -------------------
# Configure lib usage
# -------------------

CJPEG    = -DENABLE_IJG
LIB_IJG  = $(IJG)\jpeg.lib

CJ2K     = -DENABLE_J2K
LIB_J2K  = $(OPENJPEG)\ojpeg.lib

# ZLIB is required for LIBPNG
CPNG     = -DENABLE_PNG
LIB_ZLIB = $(ZLIB)\z.lib
LIB_PNG  = $(LIBPNG)\png.lib

# ZLIB, IJG and JBIG (optional) are required for LIBTIFF
# (JBIG support is included dependent on definition of JBIG macro)
CTIFF    = -DENABLE_TIF
LIB_TIFF = $(LIBTIFF)\tiff.lib

!ifdef JBIG
CJBIG    = -DENABLE_JBIG
LIB_JBIG = $(JBIG)\jbig.lib
!endif

CRAW    = -DENABLE_RAW
LIB_RAW = $(LIBRAW)\raw.lib


# ------------------
# Configure compiler
# ------------------

#
# Using Open Watcom C++:
#
CC      = wcc386
CL      = wlink
CLIB    = wlib

# Builds gbm objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS         = -wx -we
CFLAGS         = $(CWARNS) -bt=os2 -ze -zq -onatxh -oe=100 -sg -ei -6r -fp6 -fpi87 -bm -mf -DNDEBUG $(CJPEG) $(CJ2K) $(CJBIG) $(CPNG) $(CTIFF) $(CRAW)
CFLAGS_DBG     = $(CWARNS) -bt=os2 -ze -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 -DDEBUG $(CJPEG) $(CJ2K) $(CJBIG) $(CPNG) $(CTIFF) $(CRAW)
CFLAGS_EXE     = $(CFLAGS)
CFLAGS_EXE_DBG = $(CFLAGS_DBG)
CFLAGS_DLL     = -bd $(CFLAGS)
CFLAGS_DLL_DBG = -bd $(CFLAGS_DBG)

LFLAGS         = op q op el
LFLAGS_DBG     = op q d all
LFLAGS_EXE     = SYSTEM os2v2 op stack=0x10000 $(LFLAGS)
LFLAGS_EXE_DBG = SYSTEM os2v2 op stack=0x10000 $(LFLAGS_DBG)
LFLAGS_DLL     = sys os2v2_dll initi termi op many op caseexact $(LFLAGS)
LFLAGS_DLL_DBG = sys os2v2_dll initi termi op many op caseexact $(LFLAGS_DBG)

LIBFLAGS       = -q -n -b -c

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS_EXE     = $(CFLAGS_EXE_DBG)
CFLAGS_DLL     = $(CFLAGS_DLL_DBG)
LFLAGS_EXE     = $(LFLAGS_EXE_DBG)
LFLAGS_DLL     = $(LFLAGS_DLL_DBG)
!endif

# Enable for Pentium profiling (also combined with debug above)
# For profiling call nmake or wmake with debug=on
!ifdef profile
CFLAGS_EXE     = $(CFLAGS_EXE) -et
CFLAGS_DLL     = $(CFLAGS_DLL) -et
!endif

#

.SUFFIXES:	.c .obj

.c.obj:
		$(CC) $(CFLAGS_EXE) $*.c

#

all:    prep ijg j2k jbig png tiff raw \
        gbmmthrd.lib \
        gbmdllout \
        gbm.dll gbm.lib gbms.lib gbmmem.lib \
        gbmtoolsout \
        gbmhdr.exe \
        gbmmir.lib gbmref.exe \
        gbmrect.lib gbmsub.exe \
        gbmscale.lib gbmsize.exe \
        gbmerr.lib gbmtrunc.lib gbmht.lib gbmhist.lib gbmmcut.lib gbmbpp.exe \
        gbmcpal.exe  \
        gbmgamma.exe \
        gbmconv.exe \
        gbmver.exe

#

prep:
   @echo -----------------------------
   @echo JPEG support, via IJG library
   @echo -----------------------------
   @copy $(IJG)\jconfig.txt $(IJG)\jconfig.h
   @copy jmorecfg.h $(IJG)
   @echo ---------------------------------------
   @echo JPEG 2000 support, via OpenJPEG library
   @echo ---------------------------------------
!ifdef JBIG
   @echo ---------------------------------------
   @echo JBIG support, via JBIG Kit library
   @echo ---------------------------------------
!endif
   @echo --------------------------------
   @echo PNG support, via LIBPNG and ZLIB
   @echo --------------------------------
   @copy pngusr.h     $(LIBPNG)
   @echo ---------------------------------------
   @echo TIFF support, via LIBTIFF, IJG and ZLIB
   @echo ---------------------------------------
   @echo.

ijg:
   @echo ---------------------
   @echo Compiling IJG library
   @echo ---------------------
!ifdef debug
   (cd $(IJG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeijg.wat debug=yes)
!else
   (cd $(IJG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeijg.wat)
!endif
   @echo.

#

j2k:
   @echo --------------------------
   @echo Compiling OpenJPEG library
   @echo --------------------------
!ifdef debug
   (cd $(OPENJPEG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makej2k.wat debug=yes)
!else
   (cd $(OPENJPEG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makej2k.wat)
!endif
   @echo.

#

jbig:
!ifdef JBIG
   @echo --------------------------
   @echo Compiling JBIG Kit library
   @echo --------------------------
!ifdef debug
   (cd $(JBIG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makejbig.wat debug=yes)
!else
   (cd $(JBIG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makejbig.wat)
!endif
   @echo.
!endif

#

png:
   @echo ----------------------
   @echo Compiling ZLIB library
   @echo ----------------------
!ifdef debug
   (cd $(ZLIB)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makezlib.wat debug=yes)
!else
   (cd $(ZLIB)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makezlib.wat)
!endif
   @echo.
   @echo ------------------------
   @echo Compiling LIBPNG library
   @echo ------------------------
!ifdef debug
   (cd $(LIBPNG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makepng.wat debug=yes)
!else
   (cd $(LIBPNG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makepng.wat)
!endif
   @echo.

#

tiff:
   @echo -------------------------
   @echo Compiling LIBTIFF library
   @echo -------------------------
!ifdef debug
!ifdef JBIG
   (cd $(LIBTIFF) && wmake -c -ms -h -f $(MAKEFILE_SRC)\maketif.wat ENABLE_JBIG=1 debug=yes)
!else
   (cd $(LIBTIFF) && wmake -c -ms -h -f $(MAKEFILE_SRC)\maketif.wat debug=yes)
!endif
!else
!ifdef JBIG
   (cd $(LIBTIFF) && wmake -c -ms -h -f $(MAKEFILE_SRC)\maketif.wat ENABLE_JBIG=1)
!else
   (cd $(LIBTIFF) && wmake -c -ms -h -f $(MAKEFILE_SRC)\maketif.wat)
!endif
!endif
   @echo.

#

raw:
   @echo ------------------------
   @echo Compiling LIBRAW library
   @echo ------------------------
!ifdef debug
   (cd $(LIBRAW) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeraw.wat debug=yes)
!else
   (cd $(LIBRAW) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeraw.wat)
!endif
   @echo.

#

gbmdllout:
   @echo ---------------------
   @echo Compiling GBM library
   @echo ---------------------

gbmtoolsout:
   @echo -------------------
   @echo Compiling GBM tools
   @echo -------------------

#

gbm.dll:  gbm.obj    gbmpbm.obj gbmpgm.obj gbmppm.obj gbmpnm.obj \
          gbmbmp.obj gbmtga.obj gbmkps.obj gbmiax.obj gbmpcx.obj \
          gbmtif.obj gbmlbm.obj gbmvid.obj gbmgif.obj gbmxbm.obj \
          gbmspr.obj gbmpsg.obj gbmgem.obj gbmcvp.obj gbmjbg.obj gbmjpg.obj \
          gbmj2k.obj gbmpng.obj gbmxpm.obj gbmxpmcn.obj gbmraw.obj gbmhelp.obj gbmmap.obj gbmmem.obj
          $(CL) $(LFLAGS_DLL) @$(MAKEFILE_SRC2)\gbm.lnk name $@ file gbm.obj,gbmpbm.obj,gbmpgm.obj,gbmppm.obj,\
           gbmpnm.obj,gbmbmp.obj,gbmtga.obj,gbmkps.obj,gbmiax.obj,gbmpcx.obj,gbmtif.obj,gbmlbm.obj,\
           gbmvid.obj,gbmgif.obj,gbmxbm.obj,gbmspr.obj,gbmpsg.obj,gbmgem.obj,gbmcvp.obj,gbmjbg.obj,\
           gbmjpg.obj,gbmj2k.obj,gbmpng.obj,gbmxpm.obj,gbmxpmcn.obj,gbmraw.obj,gbmhelp.obj,gbmmap.obj,gbmmem.obj,\
!ifdef JBIG
           gbmmthrd.lib,$(LIB_IJG),$(LIB_J2K),$(LIB_ZLIB),$(LIB_PNG),$(LIB_TIFF),$(LIB_RAW),$(LIB_JBIG)
!else
           gbmmthrd.lib,$(LIB_IJG),$(LIB_J2K),$(LIB_ZLIB),$(LIB_PNG),$(LIB_TIFF),$(LIB_RAW)
!endif

gbm.lib:  $(MAKEFILE_SRC2)\gbm.lnk
          $(CLIB) $(LIBFLAGS) $@ +gbm.dll

gbms.lib: gbms.obj   gbmpbm.obj gbmpgm.obj gbmppm.obj gbmpnm.obj \
          gbmbmp.obj gbmtga.obj gbmkps.obj gbmiax.obj gbmpcx.obj \
          gbmtif.obj gbmlbm.obj gbmvid.obj gbmgif.obj gbmxbm.obj \
          gbmspr.obj gbmpsg.obj gbmgem.obj gbmcvp.obj gbmjbg.obj gbmjpg.obj \
          gbmj2k.obj gbmpng.obj gbmxpm.obj gbmxpmcn.obj gbmraw.obj gbmhelp.obj gbmmap.obj gbmmem.obj
          $(CLIB) $(LIBFLAGS) $@ +gbms.obj +gbmpbm.obj +gbmpgm.obj +gbmppm.obj +gbmpnm.obj\
           +gbmbmp.obj +gbmtga.obj +gbmkps.obj +gbmiax.obj +gbmpcx.obj +gbmtif.obj +gbmlbm.obj +gbmvid.obj\
           +gbmgif.obj +gbmxbm.obj +gbmspr.obj +gbmpsg.obj +gbmgem.obj +gbmcvp.obj +gbmjbg.obj +gbmjpg.obj\
           +gbmj2k.obj +gbmpng.obj +gbmxpm.obj +gbmxpmcn.obj +gbmraw.obj +gbmhelp.obj +gbmmap.obj +gbmmem.obj\
!ifdef JBIG
           +gbmmthrd.lib +$(LIB_IJG) +$(LIB_J2K) +$(LIB_ZLIB) +$(LIB_PNG) +$(LIB_TIFF) +$(LIB_RAW) +$(LIB_JBIG)
!else
           +gbmmthrd.lib +$(LIB_IJG) +$(LIB_J2K) +$(LIB_ZLIB) +$(LIB_PNG) +$(LIB_TIFF) +$(LIB_RAW)
!endif

# This is the provider of DLL symbols for linking GBM.DLL
gbm.obj:  gbm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h gbmpbm.h gbmpgm.h gbmpnm.h gbmppm.h gbmbmp.h gbmtga.h gbmkps.h gbmiax.h gbmpcx.h gbmtif.h gbmlbm.h gbmvid.h gbmgif.h gbmxbm.h gbmspr.h gbmpsg.h gbmgem.h gbmcvp.h gbmjpg.h gbmpng.h gbmxpm.h gbmxpmcn.h gbmj2k.h gbmjbg.h gbmraw.h
          $(CC) $(CFLAGS_DLL) $*.c

gbms.obj:  gbm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h gbmpbm.h gbmpgm.h gbmpnm.h gbmppm.h gbmbmp.h gbmtga.h gbmkps.h gbmiax.h gbmpcx.h gbmtif.h gbmlbm.h gbmvid.h gbmgif.h gbmxbm.h gbmspr.h gbmpsg.h gbmgem.h gbmcvp.h gbmjpg.h gbmpng.h gbmxpm.h gbmxpmcn.h gbmj2k.h gbmjbg.h gbmraw.h
          $(CC) $(CFLAGS_EXE) -fo=$@ gbm.c

# ------------
# File formats
# ------------

gbmpbm.obj:	gbmpbm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmpgm.obj:	gbmpgm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmpnm.obj:	gbmpbm.c gbm.h gbmhelp.h gbmdesc.h

gbmppm.obj:	gbmppm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmbmp.obj:	gbmbmp.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmtga.obj:	gbmtga.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmkps.obj:	gbmkps.c gbm.h gbmhelp.h gbmdesc.h

gbmiax.obj:	gbmiax.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmpcx.obj:	gbmpcx.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmlbm.obj:	gbmlbm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmvid.obj:	gbmvid.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmgif.obj:	gbmgif.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmxbm.obj:	gbmxbm.c gbm.h gbmhelp.h gbmdesc.h

gbmxpm.obj:	gbmxpm.c gbm.h gbmhelp.h gbmdesc.h gbmmap.h gbmmem.h

gbmxpmcn.obj:	gbmxpmcn.c gbmxpmcn.h gbm.h gbmmap.h

gbmspr.obj:	gbmspr.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmpsg.obj:	gbmpsg.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmgem.obj:	gbmgem.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmcvp.obj:	gbmcvp.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

# ------------------

gbmhelp.obj:	gbmhelp.c gbm.h gbmmem.h

gbmmap.obj:	gbmmap.c gbm.h gbmmap.h

# ------------------

gbmjpg.obj:	gbmjpg.c gbmjpg.h gbm.h gbmhelp.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) -i=$(IJG) $*.c

gbmj2k.obj:	gbmj2k.c gbmj2k.h gbm.h gbmhelp.h gbmdesc.h
		$(CC) $(CFLAGS_EXE) -DOPJ_STATIC -DUSE_JPWL -i=$(OPENJPEG) $*.c

gbmpng.obj:	gbmpng.c gbm.h gbmhelp.h gbmmap.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) -DPNG_USER_CONFIG -i=$(LIBPNG) -i=$(ZLIB) $*.c

gbmtif.obj:	gbmtif.c gbm.h gbmhelp.h gbmmap.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) -i=$(IJG) -i=$(ZLIB) -i=$(LIBTIFF) $*.c

gbmjbg.obj:	gbmjbg.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) \
!ifdef JBIG
        -i=$(JBIG) \
!endif        
         $*.c

gbmraw.obj:	gbmraw.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) -i=$(LIBRAW_INCL) -DLIBRAW_NODLL $*.c

# ------------
# Bitmap tools
# ------------

gbmtool.obj:    gbmtool.c gbmtool.h gbm.h

gbmtos2.obj:    gbmtos2.c gbmtool.h gbm.h

gbmtool.lib:	gbmtool.obj gbmtos2.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmtool.obj +gbmtos2.obj

#

gbmmem.obj:	gbmmem.c gbm.h gbmmem.h

gbmmem.lib:	gbmmem.obj
            $(CLIB) $(LIBFLAGS) $@ +gbmmem.obj

#

gbmmthrd.obj:	gbmmthrd.c gbm.h gbmmthrd.h

gbmmthrd.lib:	gbmmthrd.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmmthrd.obj

#

gbmhdr.exe:	gbmhdr.obj gbm.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) name $@ file gbmhdr.obj library gbm.lib,gbmtool.lib

gbmhdr.obj:	gbmhdr.c gbm.h

#

gbmmir.lib:	gbmmir.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmmir.obj

gbmmir.obj:	gbmmir.c gbmmir.h gbmmem.h

gbmref.exe:	gbmref.obj gbm.lib gbmmem.lib gbmmir.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) name $@ file gbmref.obj library gbm.lib,gbmmem.lib,gbmmir.lib,gbmtool.lib

gbmref.obj:	gbmref.c gbm.h gbmmem.h gbmmir.h

#

gbmrect.lib:	gbmrect.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmrect.obj

gbmrect.obj:	gbmrect.c

gbmsub.exe:	gbmsub.obj gbm.lib gbmmem.lib gbmrect.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) name $@ file gbmsub.obj library gbm.lib,gbmmem.lib,gbmrect.lib,gbmtool.lib

gbmsub.obj:	gbmsub.c gbm.h gbmmem.h gbmrect.h

#

gbmerr.lib:	gbmerr.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmerr.obj

gbmerr.obj:	gbmerr.c gbmmem.h

#

gbmscale.lib:	gbmscale.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmscale.obj

gbmscale.obj:	gbmscale.c gbmmem.h gbmmthrd.h

gbmsize.exe:	gbmsize.obj gbm.lib gbmmem.lib gbmscale.lib gbmtool.lib gbmmthrd.lib
		$(CL) $(LFLAGS_EXE) name $@ file gbmsize.obj library gbm.lib,gbmmem.lib,gbmscale.lib,gbmtool.lib,gbmmthrd.lib

gbmsize.obj:	gbmsize.c gbm.h gbmmem.h gbmscale.h

#

gbmtrunc.lib:	gbmtrunc.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmtrunc.obj

gbmtrunc.obj:	gbmtrunc.c gbmtrunc.h

#

gbmht.lib:	gbmht.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmht.obj

gbmht.obj:	gbmht.c gbmht.h

#

gbmhist.lib:	gbmhist.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmhist.obj

gbmhist.obj:	gbmhist.c gbmmem.h

#

gbmmcut.lib:	gbmmcut.obj
                $(CLIB) $(LIBFLAGS) $@ +gbmmcut.obj

gbmmcut.obj:	gbmmcut.c gbmmem.h

#

gbmbpp.exe:	gbmbpp.obj gbm.lib gbmmem.lib gbmerr.lib gbmtrunc.lib gbmht.lib gbmhist.lib gbmmcut.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) name $@ file gbmbpp.obj \
                                  library gbm.lib,gbmmem.lib,gbmerr.lib,gbmtrunc.lib,gbmht.lib,gbmhist.lib,gbmmcut.lib,gbmtool.lib

gbmbpp.obj:	gbmbpp.c gbm.h gbmmem.h gbmerr.h gbmtrunc.h gbmht.h gbmhist.h gbmmcut.h

#

gbmcpal.exe:	gbmcpal.obj gbm.lib gbmmem.lib gbmhist.lib gbmmcut.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) name $@ file gbmcpal.obj library gbm.lib,gbmmem.lib,gbmhist.lib,gbmmcut.lib,gbmtool.lib

gbmcpal.obj:	gbmcpal.c gbm.h gbmmem.h gbmhist.h gbmmcut.h

#

gbmgamma.exe:	gbmgamma.obj gbm.lib gbmmem.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) name $@ file gbmgamma.obj library gbm.lib,gbmmem.lib,gbmtool.lib

gbmgamma.obj:	gbmgamma.c gbm.h gbmmem.h

#

gbmconv.exe:	gbmconv.obj gbm.lib gbmmem.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) name $@ file gbmconv.obj library gbm.lib,gbmmem.lib,gbmtool.lib

gbmconv.obj:	gbmconv.c gbm.h gbmmem.h

#
# On OS/2 we load gbm.dll dynamically, so we don't need gbm.lib.

gbmver.exe:	gbmveros2.obj
		$(CL) $(LFLAGS_EXE) name $@ file gbmveros2.obj

gbmveros2.obj:	gbmveros2.c gbm.h


# ------------------------
# Build management targets
# ------------------------

clean:  prep
	-del /F /N $(IJG)\jmorecfg.h $(LIBPNG)\pngusr.h 2>nul
	-del /F /N *.obj 2>nul
	-del /F /N *.err 2>nul
	-del /F /N *.prf 2>nul
	(cd $(IJG)      && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeijg.wat  clean)
	(cd $(OPENJPEG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makej2k.wat  clean)
!ifdef JBIG
	(cd $(JBIG)     && wmake -c -ms -h -f $(MAKEFILE_SRC)\makejbig.wat clean)
!endif
	(cd $(ZLIB)     && wmake -c -ms -h -f $(MAKEFILE_SRC)\makezlib.wat clean)
	(cd $(LIBPNG)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makepng.wat  clean)
	(cd $(LIBTIFF)  && wmake -c -ms -h -f $(MAKEFILE_SRC)\maketif.wat  clean)
	(cd $(LIBRAW)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeraw.wat  clean)

clobber: prep
	 -del /F /N $(IJG)\jmorecfg.h $(LIBPNG)\pngusr.h 2>nul
	 -del /F /N *.obj 2>nul
	 -del /F /N *.err 2>nul
	 -del /F /N *.prf 2>nul
	 -del /F /N *.exe 2>nul
	 -del /F /N *.lib 2>nul
	 -del /F /N *.dll 2>nul
	 -del /F /N $(IDIR)\* 2>nul
	 (cd $(IJG)      && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeijg.wat  clobber)
	 (cd $(OPENJPEG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makej2k.wat  clobber)
!ifdef JBIG
	 (cd $(JBIG)     && wmake -c -ms -h -f $(MAKEFILE_SRC)\makejbig.wat clobber)
!endif
	 (cd $(ZLIB)     && wmake -c -ms -h -f $(MAKEFILE_SRC)\makezlib.wat clobber)
	 (cd $(LIBPNG)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makepng.wat  clobber)
	 (cd $(LIBTIFF)  && wmake -c -ms -h -f $(MAKEFILE_SRC)\maketif.wat  clobber)
	 (cd $(LIBRAW)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeraw.wat  clobber)

#

install:
	     (cd $(IJG)      && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeijg.wat  install)
	     (cd $(OPENJPEG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makej2k.wat  install)
!ifdef JBIG
	     (cd $(JBIG)     && wmake -c -ms -h -f $(MAKEFILE_SRC)\makejbig.wat install)
!endif
	     (cd $(ZLIB)     && wmake -c -ms -h -f $(MAKEFILE_SRC)\makezlib.wat install)
	     (cd $(LIBPNG)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makepng.wat  install)
	     (cd $(LIBTIFF)  && wmake -c -ms -h -f $(MAKEFILE_SRC)\maketif.wat  install)
	     (cd $(LIBRAW)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeraw.wat  install)
         copy gbm.h      $(IDIR)
         copy gbmerr.h   $(IDIR)
         copy gbmhist.h  $(IDIR)
         copy gbmht.h    $(IDIR)
         copy gbmmcut.h  $(IDIR)
         copy gbmmir.h   $(IDIR)
         copy gbmrect.h  $(IDIR)
         copy gbmscale.h $(IDIR)
         copy gbmtrunc.h $(IDIR)
         copy gbmtool.h  $(IDIR)
         copy gbmmem.h   $(IDIR)
         copy gbmmthrd.h $(IDIR)
         copy *.lib      $(IDIR)
         copy *.exe      $(IDIR)
         copy *.dll      $(IDIR)
         copy $(DOCDIR)\readme_gbm.txt $(IDIR)

#

exepack:
         $(DLL_PACKER) *.dll
         $(EXE_PACKER) *.exe

#

package:
	    (cd $(IJG)      && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeijg.wat  package)
	    (cd $(OPENJPEG) && wmake -c -ms -h -f $(MAKEFILE_SRC)\makej2k.wat  package)
!ifdef JBIG
	    (cd $(JBIG)     && wmake -c -ms -h -f $(MAKEFILE_SRC)\makejbig.wat package)
!endif
	    (cd $(ZLIB)     && wmake -c -ms -h -f $(MAKEFILE_SRC)\makezlib.wat package)
	    (cd $(LIBPNG)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makepng.wat  package)
	    (cd $(LIBTIFF)  && wmake -c -ms -h -f $(MAKEFILE_SRC)\maketif.wat  package)
	    (cd $(LIBRAW)   && wmake -c -ms -h -f $(MAKEFILE_SRC)\makeraw.wat  package)
        -del /F /N $(IDIR)\gbm.zip 2>nul
        zip -9 $(IDIR)\gbm.zip *.exe *.dll *.lib gbm.h gbmerr.h gbmhist.h  \
                               gbmht.h gbmmcut.h gbmmir.h gbmrect.h  \
                               gbmscale.h gbmtrunc.h gbmtool.h gbmmem.h \
                               gbmmthrd.h $(DOCDIR)\readme_gbm.txt

