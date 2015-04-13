# Makefile for OpenJPEG JPEG2000 library

# This makefile is for MS Visual C++ on Windows 32/64bit.
# It builds the OpenJPEG library as a statically linkable library (.LIB).
# (with JPWL support)

# set directory where binaries will be placed by target install
IDIR = bin

#
# set compiler
#
CC = cl

# Compile objects suitable for multithreaded GBM.DLL
CWARNS         = /W3 /WX
CFLAGS         = /c $(CWARNS) /Ijpwl /O2 /nologo /MT /D_CRT_SECURE_NO_DEPRECATE /DOPJ_STATIC /DUSE_JPWL /DNDEBUG /DWIN32
CFLAGS_DBG     = /c $(CWARNS) /Ijpwl /Od /nologo /Zi /GS /RTC1 /MTd /D_CRT_SECURE_NO_DEPRECATE /DOPJ_STATIC /DUSE_JPWL /DDEBUG /DWIN32
LIBFLAGS       = /NOLOGO

# For debugging call nmake or wmake with debug=on
!ifdef debug
CFLAGS   = $(CFLAGS_DBG)
!endif

# Template command for compiling .c to .obj
.c.obj :
	$(CC) $(CFLAGS) $*.c

#

INCLS      = bio.h cio.h dwt.h event.h fix.h image.h int.h j2k.h j2k_lib.h jp2.h jpt.h mct.h mqc.h openjpeg.h pi.h raw.h t1.h t2.h tcd.h tgt.h opj_includes.h opj_malloc.h \
             cidx_manager.h indexbox_manager.h
SRCS       = bio.c cio.c dwt.c event.c image.c j2k.c j2k_lib.c jp2.c jpt.c mct.c mqc.c openjpeg.c pi.c raw.c t1.c t2.c tcd.c tgt.c \
             cidx_manager.c phix_manager.c ppix_manager.c thix_manager.c tpix_manager.c
OBJS       = bio.obj cio.obj dwt.obj event.obj image.obj j2k.obj j2k_lib.obj jp2.obj jpt.obj mct.obj mqc.obj openjpeg.obj pi.obj raw.obj t1.obj t2.obj tcd.obj tgt.obj \
             cidx_manager.obj phix_manager.obj ppix_manager.obj thix_manager.obj tpix_manager.obj
JPWL_INCLS = jpwl\crc.h jpwl\jpwl.h jpwl\jpwl_lib.c jpwl\rs.h
JPWL_SRCS  = jpwl\crc.c jpwl\jpwl.c jpwl\jpwl_lib.c jpwl\rs.c
JPWL_OBJS  = jpwl\crc.obj jpwl\jpwl.obj jpwl\jpwl_lib.obj jpwl\rs.obj

all:   ojpeg.lib

ojpeg.lib: $(OBJS) $(JPWL_OBJS)
           -del $@ 2> nul
           lib $(LIBFLAGS) /OUT:$@ $(OBJS) $(JPWL_OBJS)
                               
openjpeg.obj:     openjpeg.c     $(INCLS)
bio.obj:          bio.c          $(INCLS)
cidx_manager.obj: cidx_manager.c $(INCLS)
cio.obj:          cio.c          $(INCLS)
dwt.obj:          dwt.c          $(INCLS)
event.obj:        event.c        $(INCLS)
image.obj:        image.c        $(INCLS)
j2k.obj:          j2k.c          $(INCLS)
j2k_lib.obj:      j2k_lib.c      $(INCLS)
mct.obj:          mct.c          $(INCLS)
mqc.obj:          mqc.c          $(INCLS)
phix_manager.obj: phix_manager.c $(INCLS)
ppix_manager.obj: ppix_manager.c $(INCLS)
pi.obj:           pi.c           $(INCLS)
raw.obj:          raw.c          $(INCLS)
t1.obj:           t1.c           $(INCLS)
t2.obj:           t2.c           $(INCLS)
tcd.obj:          tcd.c          $(INCLS)
tgt.obj:          tgt.c          $(INCLS)
thix_manager.obj: thix_manager.c $(INCLS)
tpix_manager.obj: tpix_manager.c $(INCLS)
jpt.obj:          jpt.c          $(INCLS)
jp2.obj:          jp2.c          $(INCLS)

jpwl\crc.obj:      jpwl\crc.c $(JPWL_INCLS)
	               $(CC) $(CFLAGS) $*.c /Fojpwl\crc.obj

jpwl\jpwl.obj:     jpwl\jpwl.c $(JPWL_INCLS)
	               $(CC) $(CFLAGS) $*.c /Fojpwl\jpwl.obj

jpwl\jpwl_lib.obj: jpwl\jpwl_lib.c $(JPWL_INCLS)
	               $(CC) $(CFLAGS) $*.c /Fojpwl\jpwl_lib.obj

jpwl\rs.obj:       jpwl\rs.c $(JPWL_INCLS)
	               $(CC) $(CFLAGS) $*.c /Fojpwl\rs.obj


# ------------------------
# Build management targets
# ------------------------

clean:
	 -del /F $(OBJS) $(JPWL_OBJS) 2>nul
	 -del /F /Q *.pdb 2>nul

clobber:
	 -del /F $(OBJS) $(JPWL_OBJS) ojpeg.lib 2>nul
	 -del /F ojpeg.lib 2>nul
	 -del /F /Q *.pdb $(IDIR)\* 2>nul
	 -del /F /Q $(IDIR)\* 2>nul

install:
         copy ojpeg.lib  $(IDIR)
         copy openjpeg.h $(IDIR)

package:
         -del /F $(IDIR)\libojpeg.zip 2>nul
         zip -9 $(IDIR)\libojpeg.zip ojpeg.lib openjpeg.h


