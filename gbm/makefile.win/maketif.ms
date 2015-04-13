# Makefile for Libtiff (static)

# This makefile is for MS Visual C++ on Windows 32/64bit.
# It builds the Libtiff library as a statically linkable library (.LIB).

# set directory where binaries will be placed by target install
IDIR = bin

# include configuration setting for nmake (except compiler options)
!INCLUDE ..\makefile.win\maketif.options

#
# set compiler
#
CC = cl

# Compile objects suitable for multithreaded GBM.DLL
CWARNS         = /W3 /WX
CFLAGS         = /c $(CWARNS) /I$(ZLIBDIR) /I$(JPEGDIR) /I$(JBIGDIR) /I.. $(EXTRAFLAGS) /O2 /nologo /MT /D_CRT_SECURE_NO_DEPRECATE /DNDEBUG /DWIN32 /DUSE_WIN32_FILEIO
CFLAGS_DBG     = /c $(CWARNS) /I$(ZLIBDIR) /I$(JPEGDIR) /I$(JBIGDIR) /I.. $(EXTRAFLAGS) /Od /Zi /GS /RTC1 /nologo /MTd /D_CRT_SECURE_NO_DEPRECATE /DDEBUG /DWIN32 /DUSE_WIN32_FILEIO
LIBFLAGS       = /NOLOGO

# For debugging call nmake with debug=on
!ifdef debug
CFLAGS   = $(CFLAGS_DBG)
!endif

.SUFFIXES:	.c .obj

.c.obj:
		$(CC) $(CFLAGS) $*.c

#

OBJS_SYSDEP_MODULE = tif_win32.obj

OBJS =  tif_aux.obj \
	tif_close.obj \
	tif_codec.obj \
	tif_color.obj \
	tif_compress.obj \
	tif_dir.obj \
	tif_dirinfo.obj \
	tif_dirread.obj \
	tif_dirwrite.obj \
	tif_dumpmode.obj \
	tif_error.obj \
	tif_extension.obj \
	tif_fax3.obj \
	tif_fax3sm.obj \
	tif_getimage.obj \
	tif_jbig.obj \
	tif_jpeg.obj \
	tif_flush.obj \
	tif_luv.obj \
	tif_lzw.obj \
	tif_next.obj \
	tif_ojpeg.obj \
	tif_open.obj \
	tif_packbits.obj \
	tif_pixarlog.obj \
	tif_predict.obj \
	tif_print.obj \
	tif_read.obj \
	tif_swab.obj \
	tif_strip.obj \
	tif_thunder.obj \
	tif_tile.obj \
	tif_version.obj \
	tif_warning.obj \
	tif_write.obj \
	tif_zip.obj \
	$(OBJS_SYSDEP_MODULE)

#

all:   tiff.lib

tiff.lib: $(OBJS)
          lib $(LIBFLAGS) /OUT:$@ $(OBJS)


# ------------------------
# Build management targets
# ------------------------

clean:
         -del /F $(OBJS) tif_os2.obj 2>nul
         -del /F /Q *.pdb 2>nul

clobber:
         -del /F $(OBJS) tif_os2.obj 2>nul
         -del /F tiff.lib 2>nul
         -del /F /Q *.pdb 2>nul
         -del /F /Q $(IDIR)\* 2>nul

install:
         copy tiff.lib     $(IDIR)
         copy tiff.h       $(IDIR)
         copy tiffio.h     $(IDIR)
         copy tiffconf.h   $(IDIR)
         copy tif_config.h $(IDIR)

package:
         -del /F $(IDIR)\libtiff.zip 2>nul
         zip -9 $(IDIR)\libtiff.zip tiff.lib tiff.h tiffio.h tiffconf.h tif_config.h

