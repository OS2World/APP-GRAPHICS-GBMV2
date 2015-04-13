# Makefile for Libtiff (static)

# This makefile is for Open Watcom C++ on OS/2.
# It builds the Libtiff library as a statically linkable library (.LIB).

# set directory where binaries will be placed by target install
IDIR = bin

# include configuration setting for nmake (except compiler options)
!INCLUDE ..\makefile.os2\maketif.options

#
# Using Open Watcom C++:
#
CC      = wcc386
CL      = wlink
CLIB    = wlib

# Builds libtiff objects which are compiled multithreaded
# Therefore all users should also be multithreaded
CWARNS     = -wx -we
CFLAGS     = $(CWARNS) -i$(ZLIBDIR) -i$(JPEGDIR) -i$(JBIGDIR) -i.. $(EXTRAFLAGS) -bt=os2 -zq -onatxh -oe=50 -sg -ei -6r -fp6 -fpi87 -bm -mf /DNDEBUG /DGBMMEM
CFLAGS_DBG = $(CWARNS) -i$(ZLIBDIR) -i$(JPEGDIR) -i$(JBIGDIR) -i.. $(EXTRAFLAGS) -bt=os2 -zq -sg -ei -6r -fp6 -fpi87 -bm -mf -d2 /DDEBUG /DGBMMEM

LIBFLAGS     = -q -n -b -c

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS   = $(CFLAGS_DBG)
!endif

.SUFFIXES:	.c .obj

.c.obj:
		$(CC) $(CFLAGS) $*.c

#

OBJS_SYSDEP_MODULE = tif_os2.obj

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
          $(CLIB) $(LIBFLAGS) $@ +tif_aux.obj +tif_close.obj +tif_codec.obj +tif_color.obj\
                                 +tif_compress.obj +tif_dir.obj +tif_dirinfo.obj +tif_dirread.obj\
                                 +tif_dirwrite.obj +tif_dumpmode.obj +tif_error.obj +tif_extension.obj\
                                 +tif_fax3.obj +tif_fax3sm.obj +tif_getimage.obj +tif_jbig.obj +tif_jpeg.obj\
                                 +tif_flush.obj +tif_luv.obj +tif_lzw.obj +tif_next.obj +tif_ojpeg.obj +tif_open.obj\
                                 +tif_packbits.obj +tif_pixarlog.obj +tif_predict.obj +tif_print.obj\
                                 +tif_read.obj +tif_swab.obj +tif_strip.obj +tif_thunder.obj\
                                 +tif_tile.obj +tif_version.obj +tif_warning.obj +tif_write.obj\
                                 +tif_zip.obj +$(OBJS_SYSDEP_MODULE)


# ------------------------
# Build management targets
# ------------------------

clean:
         -del /F /N $(OBJS) tif_win32.obj 2>nul
         -del /F /N *.lst 2>nul

clobber:
         -del /F /N $(OBJS) tif_win32.obj 2>nul
         -del /F /N tiff.lib 2>nul
         -del /F /N *.lst 2>nul
         -del /F /N $(IDIR)\* 2>nul

install:
         copy tiff.lib     $(IDIR)
         copy tiff.h       $(IDIR)
         copy tiffio.h     $(IDIR)
         copy tiffconf.h   $(IDIR)
		 copy tif_config.h $(IDIR)

package:
         -del /F /N $(IDIR)\libtiff.zip 2>nul
         zip -9 $(IDIR)\libtiff.zip tiff.lib tiff.h tiffio.h tiffconf.h tif_config.h

