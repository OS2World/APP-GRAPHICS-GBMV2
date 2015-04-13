#
# Generalised Bitmap Module
#
# This makefile is for MS Visual C++ on Windows 32/64bit.
#
# Builds GBM.DLL which is compiled multithreaded
# Therefore all users should also be multithreaded

# directory of the platform makefiles
MAKEFILE_SRC2 = makefile.win
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
LIBRAW = libraw
LIBRAW_INCL = $(LIBRAW)\libraw

# Documentation directory
DOCDIR = doc

# Installation directory: gbm.dll, gbm.lib and gbm.h files are copied to this directory
IDIR =  bin

# -------------------
# Configure lib usage
# -------------------

CJPEG    = /DENABLE_IJG
LIB_IJG  = $(IJG)\jpeg.lib

CJ2K     = /DENABLE_J2K
LIB_J2K  = $(OPENJPEG)\ojpeg.lib

# ZLIB is required for LIBPNG
CPNG     = /DENABLE_PNG
LIB_ZLIB = $(ZLIB)\z.lib
LIB_PNG  = $(LIBPNG)\png.lib

# ZLIB, IJG and JBIG (optional) are required for LIBTIFF
# (JBIG support is included dependent on definition of JBIG macro)
CTIFF    = /DENABLE_TIF
LIB_TIFF = $(LIBTIFF)\tiff.lib

!ifdef JBIG
CJBIG    = /DENABLE_JBIG
LIB_JBIG = $(JBIG)\jbig.lib
!endif

CRAW    = /DENABLE_RAW
LIB_RAW = $(LIBRAW)\raw.lib


# ------------------
# Configure compiler
# ------------------

#
# Using MS Visual C++:
#
CC      = cl
CL      = link
CLIB    = lib

# Builds gbm objects which are compiled multithreaded
# Therefore all users should also be multithreaded

CWARNS           = /W3 /WX
CFLAGS           = /c $(CWARNS) /O2 /MT /nologo /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG /DWIN32 $(CJPEG) $(CJ2K) $(CJBIG) $(CPNG) $(CTIFF) $(CRAW)
CFLAGS_DBG       = /c $(CWARNS) /Od /Zi /GS /RTC1 /MTd /nologo /D_CRT_SECURE_NO_DEPRECATE /DDEBUG /DWIN32 $(CJPEG) $(CJ2K) $(CJBIG) $(CPNG) $(CTIFF) $(CRAW)
CFLAGS_EXE       = $(CFLAGS)
CFLAGS_EXE_DBG   = $(CFLAGS_DBG)
CFLAGS_DLL       = /LD  $(CFLAGS)
CFLAGS_DLL_DBG   = /LDd $(CFLAGS_DBG)

LFLAGS         = /NOLOGO /INCREMENTAL:NO /NXCOMPAT /LARGEADDRESSAWARE /RELEASE
LFLAGS_DBG     = /NOLOGO /INCREMENTAL:NO /NXCOMPAT /LARGEADDRESSAWARE
LFLAGS_EXE     = $(LFLAGS) /STACK:0x10000
LFLAGS_EXE_DBG = $(LFLAGS_DBG) /STACK:0x10000 /DEBUG
LFLAGS_DLL     = $(LFLAGS) /DLL
LFLAGS_DLL_DBG = $(LFLAGS_DBG) /DLL /DEBUG
LIBFLAGS       = /NOLOGO

# For debugging call nmake with debug=on
!ifdef debug
CFLAGS_EXE     = $(CFLAGS_EXE_DBG)
CFLAGS_DLL     = $(CFLAGS_DLL_DBG)
LFLAGS_EXE     = $(LFLAGS_EXE_DBG)
LFLAGS_DLL     = $(LFLAGS_DLL_DBG)
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
   @echo ---------------------------------------
   @echo RAW support, via LIBRAW
   @echo ---------------------------------------
   @echo.

ijg:
   @echo ---------------------
   @echo Compiling IJG library
   @echo ---------------------
!ifdef debug
   (cd $(IJG) && nmake /C /f $(MAKEFILE_SRC)\makeijg.ms debug=yes)
!else
   (cd $(IJG) && nmake /C /f $(MAKEFILE_SRC)\makeijg.ms)
!endif
   @echo.

#

j2k:
   @echo --------------------------
   @echo Compiling OpenJPEG library
   @echo --------------------------
!ifdef debug
   (cd $(OPENJPEG) && nmake /C /f $(MAKEFILE_SRC)\makej2k.ms debug=yes)
!else
   (cd $(OPENJPEG) && nmake /C /f $(MAKEFILE_SRC)\makej2k.ms)
!endif
   @echo.
#

jbig:
!ifdef JBIG
   @echo --------------------------
   @echo Compiling JBIG Kit library
   @echo --------------------------
!ifdef debug
   (cd $(JBIG) && nmake /C /f $(MAKEFILE_SRC)\makejbig.ms debug=yes)
!else
   (cd $(JBIG) && nmake /C /f $(MAKEFILE_SRC)\makejbig.ms)
!endif
   @echo.
!endif

#

png:
   @echo ----------------------
   @echo Compiling ZLIB library
   @echo ----------------------
!ifdef debug
   (cd $(ZLIB)   && nmake /C /f $(MAKEFILE_SRC)\makezlib.ms debug=yes)
!else
   (cd $(ZLIB)   && nmake /C /f $(MAKEFILE_SRC)\makezlib.ms)
!endif
   @echo.
   @echo ------------------------
   @echo Compiling LIBPNG library
   @echo ------------------------
!ifdef debug
   (cd $(LIBPNG) && nmake /C /f $(MAKEFILE_SRC)\makepng.ms debug=yes)
!else
   (cd $(LIBPNG) && nmake /C /f $(MAKEFILE_SRC)\makepng.ms)
!endif
   @echo.

#

tiff:
   @echo -------------------------
   @echo Compiling LIBTIFF library
   @echo -------------------------
!ifdef debug
!ifdef JBIG
   (cd $(LIBTIFF) && nmake /C /f $(MAKEFILE_SRC)\maketif.ms ENABLE_JBIG=1 debug=yes)
!else
   (cd $(LIBTIFF) && nmake /C /f $(MAKEFILE_SRC)\maketif.ms debug=yes)
!endif
!else
!ifdef JBIG
   (cd $(LIBTIFF) && nmake /C /f $(MAKEFILE_SRC)\maketif.ms ENABLE_JBIG=1)
!else
   (cd $(LIBTIFF) && nmake /C /f $(MAKEFILE_SRC)\maketif.ms)
!endif
!endif
   @echo.

#

raw:
   @echo ------------------------
   @echo Compiling LIBRAW library
   @echo ------------------------
!ifdef debug
   (cd $(LIBRAW) && nmake /C /f $(MAKEFILE_SRC)\makeraw.ms debug=yes)
!else
   (cd $(LIBRAW) && nmake /C /f $(MAKEFILE_SRC)\makeraw.ms)
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

gbm.dll:  $(MAKEFILE_SRC2)\gbm.res gbm.obj gbmpbm.obj gbmpgm.obj gbmppm.obj gbmpnm.obj  \
          gbmbmp.obj gbmtga.obj gbmkps.obj gbmiax.obj gbmpcx.obj  \
          gbmtif.obj gbmlbm.obj gbmvid.obj gbmgif.obj gbmxbm.obj  \
          gbmspr.obj gbmpsg.obj gbmgem.obj gbmcvp.obj gbmjbg.obj gbmjpg.obj  \
          gbmj2k.obj gbmpng.obj gbmxpm.obj gbmxpmcn.obj gbmraw.obj gbmhelp.obj gbmmap.obj gbmmem.obj
          $(CL) $(LFLAGS_DLL) /OUT:$@ gbmmthrd.lib $(LIB_IJG) $(LIB_J2K) $(LIB_ZLIB) $(LIB_PNG) $(LIB_TIFF) $(LIB_RAW) \
!ifdef JBIG
          $(LIB_JBIG) \
!endif
          $**

gbm.lib:
          $(CLIB) $(LIBFLAGS) /OUT:$@ gbm.dll

gbms.lib: gbms.obj   gbmpbm.obj gbmpgm.obj gbmppm.obj gbmpnm.obj  \
          gbmbmp.obj gbmtga.obj gbmkps.obj gbmiax.obj gbmpcx.obj  \
          gbmtif.obj gbmlbm.obj gbmvid.obj gbmgif.obj gbmxbm.obj  \
          gbmspr.obj gbmpsg.obj gbmgem.obj gbmcvp.obj gbmjbg.obj gbmjpg.obj  \
          gbmj2k.obj gbmpng.obj gbmxpm.obj gbmxpmcn.obj gbmraw.obj gbmhelp.obj gbmmap.obj gbmmem.obj
          $(CLIB) $(LIBFLAGS) /OUT:$@ gbmmthrd.lib $(LIB_IJG) $(LIB_J2K) $(LIB_ZLIB) $(LIB_PNG) $(LIB_TIFF) $(LIB_RAW) \
!ifdef JBIG
          $(LIB_JBIG) \
!endif
          $**

# This is the provider of DLL symbols for linking GBM.DLL
gbm.obj:  gbm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h gbmpbm.h gbmpgm.h gbmpnm.h gbmppm.h gbmbmp.h gbmtga.h gbmkps.h gbmiax.h gbmpcx.h gbmtif.h gbmlbm.h gbmvid.h gbmgif.h gbmxbm.h gbmspr.h gbmpsg.h gbmgem.h gbmcvp.h gbmjpg.h gbmpng.h gbmxpm.h gbmxpmcn.h gbmj2k.h gbmjbg.h gbmraw.h
          $(CC) $(CFLAGS_DLL) $*.c

gbms.obj:  gbm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h gbmpbm.h gbmpgm.h gbmpnm.h gbmppm.h gbmbmp.h gbmtga.h gbmkps.h gbmiax.h gbmpcx.h gbmtif.h gbmlbm.h gbmvid.h gbmgif.h gbmxbm.h gbmspr.h gbmpsg.h gbmgem.h gbmcvp.h gbmjpg.h gbmpng.h gbmxpm.h gbmxpmcn.h gbmj2k.h gbmjbg.h gbmraw.h
		  $(CC) $(CFLAGS_EXE) /Fo$@ gbm.c

$(MAKEFILE_SRC2)\gbm.res:  $(MAKEFILE_SRC2)\gbm.rc
          rc /nologo $*.rc


# ------------
# File formats
# ------------

gbmpbm.obj:	gbmpbm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmpgm.obj:	gbmpgm.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h

gbmpnm.obj:	gbmpnm.c gbm.h gbmhelp.h gbmdesc.h

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
		$(CC) $(CFLAGS_EXE) /I$(IJG) $*.c

gbmj2k.obj:	gbmj2k.c gbmj2k.h gbm.h gbmhelp.h gbmdesc.h
		$(CC) $(CFLAGS_EXE) /DOPJ_STATIC /DUSE_JPWL /I$(OPENJPEG) $*.c

gbmpng.obj:	gbmpng.c gbm.h gbmhelp.h gbmmap.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) /DPNG_USER_CONFIG /I$(LIBPNG) /I$(ZLIB) $*.c

gbmtif.obj:	gbmtif.c gbm.h gbmhelp.h gbmmap.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) /I$(IJG) /I$(ZLIB) /I$(LIBTIFF) $*.c

gbmjbg.obj:	gbmjbg.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) \
!ifdef JBIG
        /I$(JBIG) \
!endif        
        $*.c

gbmraw.obj:	gbmraw.c gbm.h gbmhelp.h gbmdesc.h gbmmem.h
		$(CC) $(CFLAGS_EXE) /I$(LIBRAW_INCL) /DLIBRAW_NODLL $*.c

# ------------
# Bitmap tools
# ------------

gbmtool.obj:    gbmtool.c gbmtool.h gbm.h

gbmtwin.obj:    gbmtwin.c gbmtool.h gbm.h

gbmtool.lib:	gbmtool.obj gbmtwin.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

#

gbmmem.obj:	gbmmem.c gbm.h gbmmem.h

gbmmem.lib:	gbmmem.obj
            $(CLIB) $(LIBFLAGS) /OUT:$@ $**

#

gbmmthrd.obj:	gbmmthrd.c gbm.h gbmmthrd.h

gbmmthrd.lib:	gbmmthrd.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

#

gbmhdr.exe:	gbmhdr.obj gbm.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmhdr.obj:	gbmhdr.c gbm.h

#

gbmmir.lib:	gbmmir.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

gbmmir.obj:	gbmmir.c gbmmir.h gbmmem.h

gbmref.exe:	gbmref.obj gbm.lib gbmmem.lib gbmmir.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmref.obj:	gbmref.c gbm.h gbmmem.h gbmmir.h

#

gbmrect.lib:	gbmrect.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

gbmrect.obj:	gbmrect.c

gbmsub.exe:	gbmsub.obj gbm.lib gbmmem.lib gbmrect.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmsub.obj:	gbmsub.c gbm.h gbmmem.h gbmrect.h

#

gbmerr.lib:	gbmerr.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

gbmerr.obj:	gbmerr.c gbmmem.h

#

gbmscale.lib:	gbmscale.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

gbmscale.obj:	gbmscale.c gbmscale.h gbmmem.h

gbmsize.exe:	gbmsize.obj gbm.lib gbmmem.lib gbmscale.lib gbmtool.lib gbmmthrd.lib
		        $(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmsize.obj:	gbmsize.c gbm.h gbmmem.h gbmscale.h

#

gbmtrunc.lib:	gbmtrunc.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

gbmtrunc.obj:	gbmtrunc.c gbmtrunc.h

#

gbmht.lib:	gbmht.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

gbmht.obj:	gbmht.c gbmht.h

#

gbmhist.lib:	gbmhist.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

gbmhist.obj:	gbmhist.c gbmmem.h

#

gbmmcut.lib:	gbmmcut.obj
                $(CLIB) $(LIBFLAGS) /OUT:$@ $**

gbmmcut.obj:	gbmmcut.c gbmmem.h

#

gbmbpp.exe:	gbmbpp.obj gbm.lib gbmmem.lib gbmerr.lib gbmtrunc.lib gbmht.lib gbmhist.lib gbmmcut.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmbpp.obj:	gbmbpp.c gbm.h gbmmem.h gbmerr.h gbmtrunc.h gbmht.h gbmhist.h gbmmcut.h

#

gbmcpal.exe:	gbmcpal.obj gbm.lib gbmmem.lib gbmhist.lib gbmmcut.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmcpal.obj:	gbmcpal.c gbm.h gbmmem.h gbmhist.h gbmmcut.h

#

gbmgamma.exe:	gbmgamma.obj gbm.lib gbmmem.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmgamma.obj:	gbmgamma.c gbm.h gbmmem.h

#

gbmconv.exe:	gbmconv.obj gbm.lib gbmmem.lib gbmtool.lib
		$(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmconv.obj:	gbmconv.c gbm.h gbmmem.h

#
# On Windows we load gbm.dll dynamically, so we don't need gbm.lib.

gbmver.exe:	gbmverwin.obj
  		$(CL) $(LFLAGS_EXE) /OUT:$@ $**

gbmverwin.obj:	gbmverwin.c gbm.h


# ------------------------
# Build management targets
# ------------------------

clean:  prep
	-del /F $(IJG)\jmorecfg.h $(LIBPNG)\pngusr.h 2>nul
	-del /F /Q *.manifest 2>nul
	-del /F /Q *.obj 2>nul
    -del /F /Q $(MAKEFILE_SRC2)\gbm.res 2>nul
	-del /F /Q *.exp 2>nul
	-del /F /Q *.pdb 2>nul
	(cd $(IJG)      && nmake /C /f $(MAKEFILE_SRC)\makeijg.ms  clean)
	(cd $(OPENJPEG) && nmake /C /f $(MAKEFILE_SRC)\makej2k.ms  clean)
!ifdef JBIG
	(cd $(JBIG)     && nmake /C /f $(MAKEFILE_SRC)\makejbig.ms clean)
!endif
	(cd $(ZLIB)     && nmake /C /f $(MAKEFILE_SRC)\makezlib.ms clean)
	(cd $(LIBPNG)   && nmake /C /f $(MAKEFILE_SRC)\makepng.ms  clean)
	(cd $(LIBTIFF)  && nmake /C /f $(MAKEFILE_SRC)\maketif.ms  clean)
	(cd $(LIBRAW)   && nmake /C /f $(MAKEFILE_SRC)\makeraw.ms  clean)

clobber: prep
	 -del /F $(IJG)\jmorecfg.h $(LIBPNG)\pngusr.h 2>nul
	 -del /F /Q *.manifest 2>nul
	 -del /F /Q *.obj 2>nul
     -del /F /Q $(MAKEFILE_SRC2)\gbm.res 2>nul
	 -del /F /Q *.exp 2>nul
 	 -del /F /Q *.pdb 2>nul
	 -del /F /Q *.exe 2>nul
	 -del /F /Q *.lib 2>nul
	 -del /F /Q *.dll 2>nul
	 -del /F /Q $(IDIR)\* 2>nul
	 (cd $(IJG)      && nmake /C /f $(MAKEFILE_SRC)\makeijg.ms  clobber)
	 (cd $(OPENJPEG) && nmake /C /f $(MAKEFILE_SRC)\makej2k.ms  clobber)
!ifdef JBIG
	 (cd $(JBIG)     && nmake /C /f $(MAKEFILE_SRC)\makejbig.ms clobber)
!endif
	 (cd $(ZLIB)     && nmake /C /f $(MAKEFILE_SRC)\makezlib.ms clobber)
	 (cd $(LIBPNG)   && nmake /C /f $(MAKEFILE_SRC)\makepng.ms  clobber)
	 (cd $(LIBTIFF)  && nmake /C /f $(MAKEFILE_SRC)\maketif.ms  clobber)
 	 (cd $(LIBRAW)   && nmake /C /f $(MAKEFILE_SRC)\makeraw.ms  clobber)

#

install:
	     (cd $(IJG)      && nmake /C /f $(MAKEFILE_SRC)\makeijg.ms  install)
	     (cd $(OPENJPEG) && nmake /C /f $(MAKEFILE_SRC)\makej2k.ms  install)
!ifdef JBIG
	     (cd $(JBIG)     && nmake /C /f $(MAKEFILE_SRC)\makejbig.ms install)
!endif
	     (cd $(ZLIB)     && nmake /C /f $(MAKEFILE_SRC)\makezlib.ms install)
	     (cd $(LIBPNG)   && nmake /C /f $(MAKEFILE_SRC)\makepng.ms  install)
	     (cd $(LIBTIFF)  && nmake /C /f $(MAKEFILE_SRC)\maketif.ms  install)
	     (cd $(LIBRAW)   && nmake /C /f $(MAKEFILE_SRC)\makeraw.ms  install)
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

#

package:
	    (cd $(IJG)      && nmake /C /f $(MAKEFILE_SRC)\makeijg.ms  package)
	    (cd $(OPENJPEG) && nmake /C /f $(MAKEFILE_SRC)\makej2k.ms  package)
!ifdef JBIG
	    (cd $(JBIG)     && nmake /C /f $(MAKEFILE_SRC)\makejbig.ms package)
!endif
	    (cd $(ZLIB)     && nmake /C /f $(MAKEFILE_SRC)\makezlib.ms package)
	    (cd $(LIBPNG)   && nmake /C /f $(MAKEFILE_SRC)\makepng.ms  package)
	    (cd $(LIBTIFF)  && nmake /C /f $(MAKEFILE_SRC)\maketif.ms  package)
	    (cd $(LIBRAW)   && nmake /C /f $(MAKEFILE_SRC)\makeraw.ms  package)
        -del /F $(IDIR)\gbm.zip 2>nul
        zip -9 $(IDIR)\gbm.zip *.exe *.dll *.lib gbm.h gbmerr.h gbmhist.h  \
                               gbmht.h gbmmcut.h gbmmir.h gbmrect.h  \
                               gbmscale.h gbmtrunc.h gbmtool.h gbmmem.h \
                               gbmmthrd.h $(DOCDIR)\readme_gbm.txt

