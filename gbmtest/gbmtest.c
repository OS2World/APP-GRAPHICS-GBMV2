#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#if defined(AIX) || defined(LINUX) || defined(SUN) || defined(MACOSX) || defined(IPHONE)
#include <unistd.h>
#else
#include <io.h>
#endif
#include <fcntl.h>

#ifdef MAC
#include <types.h>
#include <stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef __OS2__
  #define INCL_DOSDATETIME
  #include <os2.h>
#endif

#include "gbm.h"
#include "gbmmem.h"

/* ------------------------------ */

/* #defined DEBUG_IO 1 */
#define TEMPDIR  "out"

#if defined(_MAX_FNAME)
  #define MAX_FNAME  _MAX_FNAME
#else
  #define MAX_FNAME  4096
#endif

#if defined(ENABLE_J2K) && defined(ENABLE_JBIG)
  static const char * formats[]            = { "BMP","GIF","IFF" ,"IMG","JPG","PCX","PNG"          ,"PBM","PGM","PPM","PNM","TGA","TGA" ,"TIF","TIF"    ,"TIF" ,"TIF"         ,"TIF"     ,"TIF" ,"VID","KPS","SPR","XBM","XPM","JP2","J2K","JBG" };
  static const char * formatsPostfix[]     = { ""   ,""   ,""    ,""   ,""   ,""   ,""             ,""   ,""   ,""   ,""   ,""   ,"_rle",""   ,"_def"   ,"_lzw","_lzw2"       ,"_pack"   ,"_jpg",""   ,""   ,""   ,""   ,""   ,""   ,""   ,""    };
  static const char * formatOptions[]      = { ""   ,""   ,"HAM8","pal",""   ,""   ,"compression=3",""   ,""   ,""   ,""   ,""   ,"rle" ,""   ,"deflate","lzw" ,"lzw_pred_hor","packbits","jpeg",""   ,""   ,""   ,""   ,""   ,""   ,""   ,""    };
  static const int    formatNoWrite1bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite4bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite8bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite24bpp[] = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,0             ,0         ,0     ,0    ,0    ,0    ,0    ,0    ,0    ,0    ,0     };
#elif defined(ENABLE_J2K)                                                                                                  
  static const char * formats[]            = { "BMP","GIF","IFF" ,"IMG","JPG","PCX","PNG"          ,"PBM","PGM","PPM","PNM","TGA","TGA" ,"TIF","TIF"    ,"TIF" ,"TIF"         ,"TIF"     ,"TIF" ,"VID","KPS","SPR","XBM","XPM","JP2","J2K" };
  static const char * formatsPostfix[]     = { ""   ,""   ,""    ,""   ,""   ,""   ,""             ,""   ,""   ,""   ,""   ,""   ,"_rle",""   ,"_def"   ,"_lzw","_lzw2"       ,"_pack"   ,"_jpg",""   ,""   ,""   ,""   ,""   ,""   ,""    };
  static const char * formatOptions[]      = { ""   ,""   ,"HAM8","pal",""   ,""   ,"compression=3",""   ,""   ,""   ,""   ,""   ,"rle" ,""   ,"deflate","lzw" ,"lzw_pred_hor","packbits","jpeg",""   ,""   ,""   ,""   ,""   ,""   ,""    };
  static const int    formatNoWrite1bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite4bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite8bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite24bpp[] = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,0             ,0         ,0     ,0    ,0    ,0    ,0    ,0    ,0    ,0     };
#elif defined(ENABLE_JBIG)                                                                                                 
  static const char * formats[]            = { "BMP","GIF","IFF" ,"IMG","JPG","PCX","PNG"          ,"PBM","PGM","PPM","PNM","TGA","TGA" ,"TIF","TIF"    ,"TIF","TIF"         ,"TIF"     ,"TIF"  ,"VID","KPS","SPR","XBM","XPM","JBG" };
  static const char * formatsPostfix[]     = { ""   ,""   ,""    ,""   ,""   ,""   ,""             ,""   ,""   ,""   ,""   ,""   ,"_rle",""   ,"_def"   ,"_lzw","_lzw2"       ,"_pack"   ,"_jpg",""   ,""   ,""   ,""   ,""   ,""    };
  static const char * formatOptions[]      = { ""   ,""   ,"HAM8","pal",""   ,""   ,"compression=3",""   ,""   ,""   ,""   ,""   ,"rle" ,""   ,"deflate","lzw" ,"lzw_pred_hor","packbits","jpeg",""   ,""   ,""   ,""   ,""   ,""    };
  static const int    formatNoWrite1bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite4bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite8bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite24bpp[] = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0     ,0             ,0         ,0     ,0    ,0    ,0    ,0    ,0    ,0     };
#else                                                                                                                      
  static const char * formats[]            = { "BMP","GIF","IFF" ,"IMG","JPG","PCX","PNG"          ,"PBM","PGM","PPM","PNM","TGA","TGA" ,"TIF","TIF"    ,"TIF","TIF"         ,"TIF"     ,"TIF" ,"VID","KPS","SPR","XBM","XPM" };
  static const char * formatsPostfix[]     = { ""   ,""   ,""    ,""   ,""   ,""   ,""             ,""   ,""   ,""   ,""   ,""   ,"_rle",""   ,"_def"   ,"_lzw","_lzw2"       ,"_pack"   ,"_jpg",""   ,""   ,""   ,""   ,""   };
  static const char * formatOptions[]      = { ""   ,""   ,"HAM8","pal",""   ,""   ,"compression=3",""   ,""   ,""   ,""   ,""   ,"rle" ,""   ,"deflate","lzw","lzw_pred_hor","packbits","jpeg",""   ,""   ,""   ,""   ,""    };
  static const int    formatNoWrite1bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0    ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite4bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0    ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite8bpp[]  = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0    ,1             ,0         ,1     ,0    ,0    ,0    ,0    ,0     };
  static const int    formatNoWrite24bpp[] = { 0    ,0    ,0     ,0    ,0    ,0    ,0              ,0    ,0    ,0    ,0    ,0    ,0     ,0    ,0        ,0    ,0             ,0         ,0     ,0    ,0    ,0    ,0    ,0     };
#endif
static const int numFormats = sizeof(formats) / sizeof(formats[0]);

#if defined(ENABLE_J2K) && defined(ENABLE_JBIG)
  static const char * formats_cmp[]              = { "BMP","GIF","IFF","IMG","PCX","PNG","PBM","PGM","PPM","PNM","TGA","TGA" ,"TIF","TIF" ,"TIF" ,"TIF"  ,"TIF"  ,"TIF" ,"KPS","JP2","J2K","JBG" };
  static const char * formatsPostfix_cmp[]       = { ""   ,""   ,""    ,""   ,""   ,""   ,""  ,""   ,""   ,""   ,""   ,"_rle",""   ,"_def","_lzw","_lzw2","_pack","_jpg",""   ,""   ,""   ,""    };
  static const char * formatOptions_cmp[]        = { ""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""    ,""   ,""    ,""    ,""     ,""     ,""    ,""   ,""   ,""   ,""    };
  static const int    formatPalcheck_cmp[]       = { 1    , 1   , 1   , 0   , 1   , 1   , 0   , 0   , 1   , 1   , 1   , 1    , 1   , 1    , 1    , 1     , 1     , 1    , 1   , 1   , 1   , 0    };
  /* some codecs only write black/white, white/black or gray palette/data, skip them as this is not predictable */
  static const int    formatCheck_Ignore_1bpp[]  = { 0    , 0   , 0   , 0   , 1   , 0   , 1   , 0   , 0   , 1   , 1   , 1    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 0   , 0   , 1    };
  static const int    formatCheck_Ignore_4bpp[]  = { 0    , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 1   , 0   , 0    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 0   , 0   , 0    };
  static const int    formatCheck_Ignore_8bpp[]  = { 0    , 0   , 0   , 0   , 0   , 0   , 0   , 1   , 0   , 1   , 0   , 0    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 1   , 1   , 1    };
  static const int    formatCheck_Ignore_24bpp[] = { 0    , 1   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0    , 0   , 0    , 0    , 0     , 0     , 1    , 0   , 0   , 0   , 0    };
#elif defined(ENABLE_J2K)                                                                                       
  static const char * formats_cmp[]              = { "BMP","GIF","IFF","IMG","PCX","PNG","PBM","PGM","PPM","PNM","TGA","TGA" ,"TIF","TIF" ,"TIF" ,"TIF"  ,"TIF"  ,"TIF" ,"KPS","JP2","J2K" };
  static const char * formatsPostfix_cmp[]       = { ""   ,""   ,""    ,""   ,""   ,""   ,""  ,""   ,""   ,""   ,""   ,"_rle",""   ,"_def","_lzw","_lzw2","_pack","_jpg",""   ,""   ,""    };
  static const char * formatOptions_cmp[]        = { ""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""    ,""   ,""    ,""    ,""     ,""     ,""    ,""   ,""   ,""    };
  static const int    formatPalcheck_cmp[]       = { 1    , 1   , 1   , 0   , 1   , 1   , 0   , 0   , 1   , 0   , 1   , 1    , 1   , 1    , 1    , 1     , 1     , 1    , 1   , 1   , 1    };
  /* some codecs only write black/white, white/black or gray palette/data, skip them as this is not predictable */
  static const int    formatCheck_Ignore_1bpp[]  = { 0    , 0   , 0   , 0   , 1   , 0   , 1   , 0   , 0   , 1   , 1   , 1    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 0   , 0    };
  static const int    formatCheck_Ignore_4bpp[]  = { 0    , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 1   , 0   , 0    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 0   , 0    };
  static const int    formatCheck_Ignore_8bpp[]  = { 0    , 0   , 0   , 0   , 0   , 0   , 0   , 1   , 0   , 1   , 0   , 0    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 1   , 1    };
  static const int    formatCheck_Ignore_24bpp[] = { 0    , 1   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0    , 0   , 0    , 0    , 0     , 0     , 1    , 0   , 0   , 0    };
#elif defined(ENABLE_JBIG)                                                                                      
  static const char * formats_cmp[]              = { "BMP","GIF","IFF","IMG","PCX","PNG","PBM","PGM","PPM","PNM","TGA","TGA" ,"TIF","TIF" ,"TIF" ,"TIF"  ,"TIF"  ,"TIF" ,"KPS","JBG" };
  static const char * formatsPostfix_cmp[]       = { ""   ,""   ,""    ,""   ,""   ,""   ,""  ,""   ,""   ,""   ,""   ,"_rle",""   ,"_def","_lzw","_lzw2","_pack","_jpg",""   ,""    };
  static const char * formatOptions_cmp[]        = { ""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""    ,""   ,""    ,""    ,""     ,""     ,""    ,""   ,""    };
  static const int    formatPalcheck_cmp[]       = { 1    , 1   , 1   , 0   , 1   , 1   , 0   , 0   , 1   , 0   , 1   , 1    , 1   , 1    , 1    , 1     , 1     , 1    , 1   , 0    };
  /* some codecs only write black/white, white/black or gray palette/data, skip them as this is not predictable */
  static const int    formatCheck_Ignore_1bpp[]  = { 0    , 0   , 0   , 0   , 1   , 0   , 1   , 0   , 0   , 1   , 1   , 1    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 1    };
  static const int    formatCheck_Ignore_4bpp[]  = { 0    , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 1   , 0   , 0    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 0    };
  static const int    formatCheck_Ignore_8bpp[]  = { 0    , 0   , 0   , 0   , 0   , 0   , 0   , 1   , 0   , 1   , 0   , 0    , 0   , 0    , 1    , 1     , 0     , 1    , 0   , 1    };
  static const int    formatCheck_Ignore_24bpp[] = { 0    , 1   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0    , 0   , 0    , 0    , 0     , 0     , 1    , 0   , 0    };
#else                                                                                                           
  static const char * formats_cmp[]              = { "BMP","GIF","IFF","IMG","PCX","PNG","PBM","PGM","PPM","PNM","TGA","TGA" ,"TIF","TIF" ,"TIF" ,"TIF"  ,"TIF"  ,"TIF" ,"KPS" };
  static const char * formatsPostfix_cmp[]       = { ""   ,""   ,""    ,""   ,""   ,""   ,""  ,""   ,""   ,""   ,""   ,"_rle",""   ,"_def","_lzw","_lzw2","_pack","_jpg",""    };
  static const char * formatOptions_cmp[]        = { ""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""   ,""    ,""   ,""    ,""    ,""     ,""     ,""    ,""    };
  static const int    formatPalcheck_cmp[]       = { 1    , 1   , 1   , 0   , 1   , 1   , 0   , 0   , 1   , 0   , 1   , 1    , 1   , 1    , 1    , 1     , 1     , 1    , 1    };
  /* some codecs only write black/white, white/black or gray palette/data, skip them as this is not predictable */
  static const int    formatCheck_Ignore_1bpp[]  = { 0    , 0   , 0   , 0   , 1   , 0   , 1   , 0   , 0   , 1   , 1   , 1    , 0   , 0    , 1    , 1     , 0     , 1    , 0    };
  static const int    formatCheck_Ignore_4bpp[]  = { 0    , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 1   , 0   , 0    , 0   , 0    , 1    , 1     , 0     , 1    , 0    };
  static const int    formatCheck_Ignore_8bpp[]  = { 0    , 0   , 0   , 0   , 0   , 0   , 0   , 1   , 0   , 1   , 0   , 0    , 0   , 0    , 1    , 1     , 0     , 1    , 0    };
  static const int    formatCheck_Ignore_24bpp[] = { 0    , 1   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   , 0    , 0   , 0    , 0    , 0     , 0     , 1    , 0    };
#endif
static const int numFormats_cmp = sizeof(formats_cmp) / sizeof(formats_cmp[0]);

/* ------------------------------ */

/* map supported GBM_O_* defines to compiler specific ones */
static int get_checked_internal_open_mode(int mode)
{
   const static int supported_open_modes = GBM_O_RDONLY |
                                           GBM_O_WRONLY |
                                           GBM_O_RDWR   |
                                           GBM_O_EXCL   |
                                           GBM_O_NOINHERIT;

   /* internal, compiler specific open mode */
   int open_mode = 0;

   /* map supported public defines to compiler specific ones */

   /* check if only supported modes are provided */
   if (!(mode & supported_open_modes))
   {
      return 0xffffffff;
   }

   /* mask external binary mode bit */
   mode &= ~GBM_O_BINARY;

   if (mode & GBM_O_RDONLY)
   {
      open_mode |= O_RDONLY;
   }
   else if (mode & GBM_O_WRONLY)
   {
      open_mode |= O_WRONLY;
   }
   else if (mode & GBM_O_RDWR)
   {
      open_mode |= O_RDWR;
   }
   else
   {
      return 0xffffffff;
   }

   if (mode & GBM_O_EXCL)
   {
      open_mode |= O_EXCL;
   }
   #ifdef O_NOINHERIT
   if (mode & GBM_O_NOINHERIT)
   {
      open_mode |= O_NOINHERIT;
   }
   #endif

   /* force binary mode if necessary */
   #ifdef O_BINARY
     open_mode |= O_BINARY;
   #endif

   return open_mode;
}

/* map supported GBM_SEEK_* defines to compiler specific ones */
static int get_checked_internal_lseek_mode(const int whence)
{
   int internal_whence = -1;

   switch(whence)
   {
      case GBM_SEEK_SET:
         internal_whence = SEEK_SET;
         break;

      case GBM_SEEK_CUR:
         internal_whence = SEEK_CUR;
         break;

      case GBM_SEEK_END:
         internal_whence = SEEK_END;
         break;

      default:
         /* as we provide an illegal whence value, the OS takes care
          * for correct error reporting
          */
         break;
   }

   return internal_whence;
}

/* ****************************************************************** */
/* C functions for export to the GBM.DLL (_Optlink on OS/2) */
/* ****************************************************************** */

#if defined(WIN32) && defined(_MSC_VER)
  /* Use ISO variants */
  #define	fd_creat _creat
  #define	fd_open  _open
  #define	fd_close _close
  #define	fd_read  _read
  #define	fd_write _write
  #define	fd_lseek _lseek
#else
  /* Use POSIX variants */
  #define	fd_creat creat
  #define	fd_open  open
  #define	fd_close close
  #define	fd_read  read
  #define	fd_write write
  #define	fd_lseek lseek
#endif

/* map memory file functions to gbm */
static int GBMENTRY gbmtest_open(const char *fn, int mode)
{
   const int fd = fd_open(fn, get_checked_internal_open_mode(mode));
#ifdef DEBUG_IO
   printf("open(\"%s\",%x -> %x) = %d\n", fn, mode, get_checked_internal_open_mode(mode), fd);
#endif
   return fd;
}

static int GBMENTRY gbmtest_create(const char *fn, int mode)
{
#ifdef MAC
   const int fd = open(fn, O_CREAT | O_TRUNC | get_checked_internal_open_mode(mode));
        /* S_IREAD and S_IWRITE won't exist on the Mac until MacOS/X */
#else
 #if defined(S_IWOTH) /* Unix like system */
   const int fd = fd_open(fn, O_CREAT | O_TRUNC | get_checked_internal_open_mode(mode), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
 #else
   const int fd = fd_open(fn, O_CREAT | O_TRUNC | get_checked_internal_open_mode(mode), S_IREAD | S_IWRITE);
 #endif
#endif

#ifdef DEBUG_IO
   printf("create(\"%s\",%x -> %x) = %d\n", fn, mode, get_checked_internal_open_mode(mode), fd);
#endif
   return fd;
}

static void GBMENTRY gbmtest_close(int fd)
{
#ifdef DEBUG_IO
   printf("close(%d)\n", fd);
#endif
   fd_close(fd);
}

static long GBMENTRY gbmtest_lseek(int fd, long pos, int whence)
{
   const long ld = fd_lseek(fd, pos, get_checked_internal_lseek_mode(whence));
#ifdef DEBUG_IO
   printf("lseek(%d, %ld, %d -> %d) = %ld\n", fd, pos, whence, get_checked_internal_lseek_mode(whence), ld);
#endif
   return ld;
}

static int GBMENTRY gbmtest_read(int fd, void *buf, int len)
{
   const int rd = fd_read(fd, buf, len);
#ifdef DEBUG_IO
   printf("read(%d, buf, %d) = %d\n", fd, len, rd);
#endif
   return rd;
}

static int GBMENTRY gbmtest_write(int fd, const void *buf, int len)
{
   const int wd = fd_write(fd, buf, len);
#ifdef DEBUG_IO
   printf("write(%d, buf, %d) = %d\n", fd, len, wd);
#endif
   return wd;
}

/* ------------------------------ */
/* ------------------------------ */

static int isReadSupportedFormat(const int ft, const int bpp)
{
   GBMFT readFormat;
   int   readFormatFlag = 0;

   switch (bpp)
   {
      case 64: readFormatFlag = GBM_FT_R64; break;
      case 48: readFormatFlag = GBM_FT_R48; break;
      case 32: readFormatFlag = GBM_FT_R32; break;
      case 24: readFormatFlag = GBM_FT_R24; break;
      case  8: readFormatFlag = GBM_FT_R8;  break;
      case  4: readFormatFlag = GBM_FT_R4;  break;
      case  1: readFormatFlag = GBM_FT_R1;  break;
      default: readFormatFlag = 0;          break;
   }

   if (gbm_query_filetype(ft, &readFormat) != GBM_ERR_OK)
   {
      return 0;
   }

   if ((readFormat.flags & readFormatFlag) == 0)
   {
      return 0;
   }

   return 1;
}

/* ------------------------------ */

static int getWriteSupportedFormat(const char * format, const int bpp)
{
   int   ft;
   int   flag = 0;
   GBMFT gbmft;
   char  buffer[2000];
   
   sprintf(buffer, "FMT.%s", format);

   /* search the file type for destination */
   if ( gbm_guess_filetype(buffer, &ft) != GBM_ERR_OK )
   {
      return -1;
   }

   /* is bitmap color depth supported */
   gbm_query_filetype(ft, &gbmft);

   switch (bpp)
   {
      case 64 : flag = GBM_FT_W64; break;
      case 48 : flag = GBM_FT_W48; break;
      case 32 : flag = GBM_FT_W32; break;
      case 24 : flag = GBM_FT_W24; break;
      case 8  : flag = GBM_FT_W8;  break;
      case 4  : flag = GBM_FT_W4;  break;
      case 1  : flag = GBM_FT_W1;  break;
      default : flag = 0;          break;
   }

   if ( (gbmft.flags & flag) == 0 )
   {
      /* bitmap color depth is not supported */
      return -1;
   }

   return ft;
}

/* ------------------------------ */
/* ------------------------------ */

/* Load/save bitmaps of different formats */
static int gbmtest_io(const char *filename, const char *tempfilename)
{
   int   i;
   int   ftr;
   int errorCount = 0;

   int     fd;
   int     ftw;
   int     dataSize;
   GBM     gbm;
   GBMRGB  gbmrgb[0x100];
   gbm_u8 *data = NULL;

   char full_tempfilename[MAX_FNAME + sizeof(TEMPDIR) + 14];

   /* check if file format is supported */
   if (gbm_guess_filetype(filename, &ftr) != GBM_ERR_OK)
   {
      printf("Input format is not supported.\n");
      return -1;
   }

   /* ================================================= */
   /* Read bitmap file */
   /* ================================================= */

   /* open bitmap file */
   fd = gbm_io_open(filename, GBM_O_RDONLY);
   if (fd == -1)
   {
      printf("FAILED, Cannot open file.\n");
      return -1;
   }

   /* read bitmap header */
   if (gbm_read_header(filename, fd, ftr, &gbm, "") != GBM_ERR_OK)
   {
      printf("FAILED, Cannot read file header.\n");
      gbm_io_close(fd);
      return -1;
   }

   /* check whether we can support the provided color depth (restrict to algorithms) */
   if (! isReadSupportedFormat(ftr, gbm.bpp))
   {
      printf("FAILED: Input format color depth is not supported.\n");
      gbm_io_close(fd);
      return -1;
   }

   /* read bitmap palette */
   memset(gbmrgb, 0, sizeof(gbmrgb));
   if (gbm_read_palette(fd, ftr, &gbm, gbmrgb) != GBM_ERR_OK)
   {
      printf("FAILED, Cannot read bitmap palette.\n");
      gbm_io_close(fd);
      return -1;
   }

   /* calc needed memory and load bitmap data */
   dataSize = (((gbm.w * gbm.bpp + 31) / 32) * 4) * gbm.h;
   if ((data = (gbm_u8 *) gbmmem_malloc(dataSize)) == NULL)
   {
      printf("FAILED, Out of memory.\n");
      gbm_io_close(fd);
      return -1;
   }
   memset(data, 0, dataSize);

   /* read bitmap data */
   if (gbm_read_data(fd, ftr, &gbm, data) != GBM_ERR_OK)
   {
      printf("FAILED, Cannot read bitmap data.\n");
      gbmmem_free(data);
      gbm_io_close(fd);
      return -1;
   }

   gbm_io_close(fd);

   /* ================================================= */

   for (i = 0; i < numFormats; i++)
   {
      printf("Testing writing of format \"%s\": ", formats[i]);

      /* ================================================= */
      /* Write bitmap file */
      /* ================================================= */

      /* check if output format supports color depth */
      switch (gbm.bpp)
      {
         case 24:
           ftw = formatNoWrite24bpp[i];
           break;
            
         case 8:
           ftw = formatNoWrite8bpp[i];
           break;
            
         case 4:
           ftw = formatNoWrite4bpp[i];
           break;
            
         case 1:
           ftw = formatNoWrite1bpp[i];
           break;
            
         default:
           ftw = 0;
           break;
      }
      if (ftw != 0)
      {
         printf("SKIPPED, Output format %s does not support color depth %d.\n", formats[i], gbm.bpp);
         continue;
      }
      ftw = getWriteSupportedFormat(formats[i], gbm.bpp);
      if (ftw < 0)
      {
         printf("SKIPPED, Output format %s does not support color depth %d.\n", formats[i], gbm.bpp);
         continue;
      }

      /* create bitmap file */
      sprintf(full_tempfilename, "%s%s.%s", tempfilename, formatsPostfix[i], formats[i]);

      fd = gbm_io_create(full_tempfilename, GBM_O_WRONLY);
      if (fd == -1)
      {
         printf("FAILED, Cannot create file \"%s\".\n", full_tempfilename);
         gbmmem_free(data);
         errorCount++;
         continue;
      }

      /* write data */
      if (gbm_write(full_tempfilename, fd, ftw, &gbm, gbmrgb, data, formatOptions[i]) != GBM_ERR_OK)
      {
         printf("FAILED, Cannot write bitmap.\n");
         gbm_io_close(fd);
         gbmmem_free(data);
         errorCount++;
         continue;
      }

      gbm_io_close(fd);

      printf("PASSED.\n");
   }

   gbmmem_free(data);

   if (errorCount > 0)
   {
      printf("FAILED, %d write bitmap tests failed.\n", errorCount);
      return -1;
   }

   return 0;
}

/* ------------------------------ */

/* Compare bitmaps */
static int gbmtest_cmp(const char *filename, const char *tempfilename)
{
   int   i;
   int   ftr;
   int errorCount = 0;

   int     fd;
   int     dataSize;
   GBM     gbm;
   GBMRGB  gbmrgb[0x100];
   gbm_u8 *data = NULL;

   char full_tempfilename[MAX_FNAME + sizeof(TEMPDIR) + 14];

   /* check if file format is supported */
   if (gbm_guess_filetype(filename, &ftr) != GBM_ERR_OK)
   {
      printf("Input format is not supported.\n");
      return -1;
   }

   /* ================================================= */
   /* Read bitmap file */
   /* ================================================= */

   /* open bitmap file */
   fd = gbm_io_open(filename, GBM_O_RDONLY);
   if (fd == -1)
   {
      printf("FAILED, Cannot open file.\n");
      return -1;
   }

   /* read bitmap header */
   if (gbm_read_header(filename, fd, ftr, &gbm, "") != GBM_ERR_OK)
   {
      printf("FAILED, Cannot read file header.\n");
      gbm_io_close(fd);
      return -1;
   }

   /* check whether we can support the provided color depth (restrict to algorithms) */
   if (! isReadSupportedFormat(ftr, gbm.bpp))
   {
      printf("FAILED: Input format color depth is not supported.\n");
      gbm_io_close(fd);
      return -1;
   }

   /* read bitmap palette */
   memset(gbmrgb, 0, sizeof(gbmrgb));
   if (gbm_read_palette(fd, ftr, &gbm, gbmrgb) != GBM_ERR_OK)
   {
      printf("FAILED, Cannot read bitmap palette.\n");
      gbm_io_close(fd);
      return -1;
   }

   /* calc needed memory and load bitmap data */
   dataSize = (((gbm.w * gbm.bpp + 31) / 32) * 4) * gbm.h;
   if ((data = (gbm_u8 *) gbmmem_malloc(dataSize)) == NULL)
   {
      printf("FAILED, Out of memory.\n");
      gbm_io_close(fd);
      return -1;
   }
   memset(data, 0, dataSize);

   /* read bitmap data */
   if (gbm_read_data(fd, ftr, &gbm, data) != GBM_ERR_OK)
   {
      printf("FAILED, Cannot read bitmap data.\n");
      gbmmem_free(data);
      gbm_io_close(fd);
      return -1;
   }

   gbm_io_close(fd);

   /* ================================================= */

   for (i = 0; i < numFormats_cmp; i++)
   {
      int     fdc;
      int     ftc;
      int     ftw;
      int     dataSize_c;
      GBM     gbmc;
      GBMRGB  gbmrgbc[0x100];
      gbm_u8 * datac = NULL;

      printf("Comparing bitmap of format \"%s\": ", formats_cmp[i]);

      /* ================================================= */
      /* Read compare bitmap file */
      /* ================================================= */

      /* check if output format has been written based on supported color depth */
      switch (gbm.bpp)
      {
         case 24:
           ftw = formatCheck_Ignore_24bpp[i];
           break;
            
         case 8:
           ftw = formatCheck_Ignore_8bpp[i];
           break;
            
         case 4:
           ftw = formatCheck_Ignore_4bpp[i];
           break;
            
         case 1:
           ftw = formatCheck_Ignore_1bpp[i];
           break;
            
         default:
           ftw = 0;
           break;
      }
      if (ftw != 0)
      {
         printf("SKIPPED, Format %s does not support color depth %d.\n", formats_cmp[i], gbm.bpp);
         continue;
      }
      ftw = getWriteSupportedFormat(formats_cmp[i], gbm.bpp);
      if (ftw < 0)
      {
         printf("SKIPPED, Format %s does not support color depth %d.\n", formats_cmp[i], gbm.bpp);
         continue;
      }

      /* create bitmap file */
      sprintf(full_tempfilename, "%s%s.%s", tempfilename,  formatsPostfix_cmp[i], formats_cmp[i]);

      /* check if file format is supported */
      if (gbm_guess_filetype(full_tempfilename, &ftc) != GBM_ERR_OK)
      {
         printf("Input format is not supported.\n");
         errorCount++;
         continue;
      }

      /* open bitmap file */
      fdc = gbm_io_open(full_tempfilename, GBM_O_RDONLY);
      if (fdc == -1)
      {
         printf("FAILED, Cannot open file %s.\n", full_tempfilename);
         errorCount++;
         continue;
      }

      /* read bitmap header */
      if (gbm_read_header(full_tempfilename, fdc, ftc, &gbmc, formatOptions_cmp[i]) != GBM_ERR_OK)
      {
         printf("FAILED, Cannot read file header.\n");
         gbm_io_close(fdc);
         errorCount++;
         continue;
      }

      /* check whether we can support the provided color depth (restrict to algorithms) */
      if (! isReadSupportedFormat(ftc, gbmc.bpp))
      {
         printf("FAILED: Input format color depth is not supported.\n");
         gbm_io_close(fdc);
         errorCount++;
         continue;
      }

      /* read bitmap palette */
      memset(gbmrgbc, 0, sizeof(gbmrgbc));
      if (gbm_read_palette(fdc, ftc, &gbmc, gbmrgbc) != GBM_ERR_OK)
      {
        printf("FAILED, Cannot read bitmap palette.\n");
        gbm_io_close(fdc);
        errorCount++;
        continue;
      }

      /* calc needed memory and load bitmap data */
      dataSize_c = (((gbmc.w * gbmc.bpp + 31) / 32) * 4) * gbmc.h;
      if ((datac = (gbm_u8 *) gbmmem_malloc(dataSize_c)) == NULL)
      {
         printf("FAILED, Out of memory.\n");
         gbm_io_close(fdc);
         return -1;
      }
      memset(datac, 0, dataSize_c);

      /* read bitmap data */
      if (gbm_read_data(fdc, ftc, &gbmc, datac) != GBM_ERR_OK)
      {
         printf("FAILED, Cannot read bitmap data.\n");
         gbmmem_free(datac);
         gbm_io_close(fdc);
         errorCount++;
         continue;
      }

      gbm_io_close(fdc);

      if (gbmc.bpp > gbm.bpp)	
      {
        printf("SKIPPED, Color depth of compare image is higher bpp than reference.\n");
        gbmmem_free(datac);
        continue;
      }
	
      if ( ((gbmc.bpp ==  1) && formatCheck_Ignore_1bpp[i]) ||
           ((gbmc.bpp ==  4) && formatCheck_Ignore_4bpp[i]) ||
           ((gbmc.bpp ==  8) && formatCheck_Ignore_8bpp[i])  ||
           ((gbmc.bpp == 24) && formatCheck_Ignore_24bpp[i]) )
      {
        printf("SKIPPED, Due to hard-coded palette/data comparison not predictable.\n");
        gbmmem_free(datac);
        continue;
      }

      /* compare color palette */
      if (formatPalcheck_cmp[i] && (gbmc.bpp == gbm.bpp))
      {
         if (memcmp(gbmrgb, gbmrgbc, sizeof(gbmrgb)))
         {
           printf("FAILED, Color palette does not match.\n");
           gbmmem_free(datac);
           errorCount++;
           continue;
         }
      }

      /* compare data */
      if (gbmc.bpp == gbm.bpp)	
      {
          gbm_boolean foundDiff = GBM_FALSE;
          int    y;
          int    cmpRowBytes = (gbm.w * gbm.bpp + 7) / 8;
          gbm_u8 * p  = data;
          gbm_u8 * pc = datac;

          assert(cmpRowBytes > 0);

          for (y = 0; y < gbm.h; y++)
          {
              if (memcmp(p, pc, cmpRowBytes))
              {
                foundDiff = GBM_TRUE;
                break;
              }
          }
          if (foundDiff)
          {
              gbmmem_free(datac);
              errorCount++;
              printf("FAILED, Bitmap data does not match.\n");
              continue;
          }
      }

      gbmmem_free(datac);
      printf("PASSED.\n");
   }

   gbmmem_free(data);

   if (errorCount > 0)
   {
      printf("======================================\n");
      printf("FAILED, %d bitmap compare tests failed.\n", errorCount);
      return -1;
   }

   return 0;
}

/* ------------------------------ */

int main(int argc, char *argv[])
{
   const char * read_filename = NULL;

   char temp_filename[MAX_FNAME + sizeof(TEMPDIR) + 10] = { 0 };

#ifdef __OS2__
   DATETIME start_time, end_time, full_start_time, full_end_time;
   double   time_s;
#endif

   gbm_init();

   if (argc < 2)
   {
      printf("Usage: gbmtest filename\n\n");
      printf("       filename - bitmap filename used for the test\n");
      gbm_deinit();
      return 1;
   }

   read_filename = argv[1];

   printf("\n-------------\n");
   printf("Testing file: %s\n", read_filename);
   printf("-------------\n\n");

   /* extract base filename */
  #if defined(_MAX_FNAME)
   {
     char basename[_MAX_FNAME+1] = { 0 };
     char filename[_MAX_PATH+1]  = { 0 };

     strcpy(filename, read_filename);
     _splitpath(filename, NULL, NULL, basename, NULL);

     if (strlen(basename) < 1)
     {
       printf("FAILED, cannot decode source filename.\n");
       gbm_deinit();
       return 1;
     }
     sprintf(temp_filename, "%s\\%s", TEMPDIR, basename);
   }
  #else
   {
     const char *end = NULL;
     char basename[512] = { 0 };

    #if defined(__OS2__) || defined(DOS) || defined(WIN32)
     const char * pbasename = rindex(read_filename, '\\');
     if (pbasename == NULL)
     {
       printf("FAILED, cannot decode source filename.\n");
       gbm_deinit();
       return 1;
     }
     if (*pbasename != 0) /* skip the backslash if we are not at the end */
     {
       pbasename++;
     }
     strcpy(basename, pbasename);
     /* look for . as separator and skip everything behind */
     end = strstr(basename, ".");
     if (end != NULL)
     {
       basename[end - basename] = 0;
     }
     sprintf(temp_filename, "%s\\%s", TEMPDIR, basename);
    #else
     const char * pbasename = rindex(read_filename, '/');
     if (pbasename == NULL)
     {
       printf("FAILED, cannot decode source filename.\n");
       gbm_deinit();
       return 1;
     }
     if (*pbasename != 0) /* skip the slash if we are not at the end */
     {
       pbasename++;
     }
     strcpy(basename, pbasename);
     /* look for . as separator and skip everything behind */
     end = strstr(basename, ".");
     if (end != NULL)
     {
       basename[end - basename] = 0;
     }
     sprintf(temp_filename, "%s/%s", TEMPDIR, basename);
    #endif
   }
  #endif

   /* Test with gbm i/o functions remapped for the whole process. */
   printf("----------------------------------------------------\n");
   printf("Testing remapped process file I/O function callbacks\n");
   printf("----------------------------------------------------\n");

#ifdef __OS2__
   DosGetDateTime(&full_start_time);
   start_time = full_start_time;
#endif
   if (gbm_io_setup(gbmtest_open,
                    gbmtest_create,
                    gbmtest_close,
                    gbmtest_lseek,
                    gbmtest_read,
                    gbmtest_write) != GBM_ERR_OK)
   {
       printf("FAILED, Cannot register IO callbacks.\n");
       gbm_deinit();
       return 1;
   }
   printf("--> Test load/save: %s\n\n", (gbmtest_io (read_filename, temp_filename) < 0) ? "FAILED" : "PASSED");
   printf("--> Test compare: %s\n\n",   (gbmtest_cmp(read_filename, temp_filename) < 0) ? "FAILED" : "PASSED");
#ifdef __OS2__
   DosGetDateTime(&end_time);
   time_s = ((double) (end_time  .minutes * 60) + end_time  .seconds + (end_time  .hundredths/100.0)) -
            ((double) (start_time.minutes * 60) + start_time.seconds + (start_time.hundredths/100.0));
   printf("Elapsed time: %lf\n", time_s);
#endif

   /* Test with default gbm i/o functions. */
   printf("------------------------------------------------\n");
   printf("Testing GBM standard file I/O function callbacks\n");
   printf("------------------------------------------------\n");
   gbm_restore_io_setup();
#ifdef __OS2__
   DosGetDateTime(&start_time);
#endif
   printf("--> Test load/save: %s\n\n", (gbmtest_io (read_filename, temp_filename) < 0) ? "FAILED" : "PASSED");
   printf("--> Test compare: %s\n\n",   (gbmtest_cmp(read_filename, temp_filename) < 0) ? "FAILED" : "PASSED");
#ifdef __OS2__
   DosGetDateTime(&full_end_time);
   end_time = full_end_time;

   time_s = ((double) (end_time  .minutes * 60) + end_time  .seconds + (end_time  .hundredths/100.0)) -
            ((double) (start_time.minutes * 60) + start_time.seconds + (start_time.hundredths/100.0));
   printf("Elapsed time: %lf\n", time_s);

   time_s = ((double) (full_end_time  .minutes * 60) + full_end_time  .seconds + (full_end_time  .hundredths/100.0)) -
            ((double) (full_start_time.minutes * 60) + full_start_time.seconds + (full_start_time.hundredths/100.0));
   printf("\n\nOverall Elapsed test time: %lf\n", time_s);

#endif

   gbm_deinit();
   return 0;
}


