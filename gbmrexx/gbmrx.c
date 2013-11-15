/*

gbmrx.c - Generalised Bitmap Module (GBM) REXX adapter library

Author  : Heiko Nitzsche
License : Public Domain
Requires: GBM.DLL version 1.35 or higher.

History
-------
15-May-2006: Inital version (1.0)
16-Aug-2006: Version 1.01
             * Load GBM.DLL via "GBM" rather than "GBM.DLL" so that
               it is found also with LIBPATHSTRICT enabled.
             * Add bldlevel info
27-Jan-2008: Version 1.10
             * Add scaling capability (simple + resampling)
             * Add query for available scaling algorithms
             * Add test routine for checking of supported scaling algorithm
             * Add routine to expand palette bitmaps to 24bpp true color
             * Add support for reflect and transpose
             * Add support for rotating in 90 degree increments
06-Feb-2008: Version 1.11
             * Allocate memory from high memory for bitmap data to
               stretch limit for out-of-resource exhausted errors
               (requires kernel with high memory support)
15-Aug-2008: Integrate new GBM types
02-Nov-2010: Version 1.15
             Add resampling scaler filters:
             * blackman, catmullrom, quadratic, gaussian, kaiser
             Final fix for generated moire due to rounding issues
             in contribution calculations.
*/

#define  INCL_DOS
#define  INCL_REXXSAA
#include <os2.h>

/* GCC on OS/2 using EMX headers has its own defines for REXX */
/* which are included via os2.h */
#ifndef RXSHV_SYSET
 #include <rexxsaa.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gbm.h"
#include "gbmmem.h"
#include "gbmmir.h"
#include "gbmscale.h"

#include "rexxhlp.h"


/*********************************************************************/
/* GBMRX.DLL version                                                 */
/*********************************************************************/
#define GBM_VERSION_REXX     "1.16"

/*********************************************************************/
/* Numeric Error Return Strings                                      */
/*********************************************************************/
#define  NOERROR_RET         "0"              /* No error whatsoever */

/*********************************************************************/
/* Alpha Numeric Return Strings                                      */
/*********************************************************************/
#define  ERROR_RETSTR           "ERROR"  /* Unspecific error occured */
#define  ERROR_MEM              "ERROR: Out of memory"
#define  ERROR_TOOMANY_TYPES    "ERROR: Too many filetypes found"
#define  ERROR_REXX_SET         "ERROR: Cannot set REXX variable"
#define  ERROR_FILETYPE         "ERROR: Invalid file type"
#define  ERROR_FILEOPEN         "ERROR: Cannot open file"
#define  ERROR_FILECREATE       "ERROR: Cannot create file"
#define  ERROR_BPP_UNSUPP       "ERROR: Colour depth is not supported"
#define  ERROR_PAGEQUERY_UNSUPP "ERROR: Query requires GBM.DLL >=1.35"
#define  ERROR_OUT_OF_RANGE     "ERROR: Variable is out of range"
#define  ERROR_ALGORITHM        "ERROR: Invalid algorithm"
#define  ERROR_ANGLE            "ERROR: Unsupported angle (must be multiple of 90)"

/*********************************************************************/
/* Numeric Return calls                                              */
/*********************************************************************/
#define  INVALID_ROUTINE 40            /* Raise Rexx error           */
#define  VALID_ROUTINE    0            /* Successful completion      */

/*********************************************************************/
/*  Declare all exported functions as REXX functions.                */
/*********************************************************************/

/* Unfortunately there is a prototype conflict between the     */
/* OS/2 Toolkit and the EMX headers. Thus we redefine our own. */

typedef ULONG APIENTRY MyRexxFunctionHandler(PUCHAR,
                                             ULONG,
                                             PRXSTRING,
                                             PSZ,
                                             PRXSTRING);

MyRexxFunctionHandler GBM_LoadFuncs;
MyRexxFunctionHandler GBM_DropFuncs;

MyRexxFunctionHandler GBM_Version;
MyRexxFunctionHandler GBM_VersionRexx;

MyRexxFunctionHandler GBM_Types;
MyRexxFunctionHandler GBM_IsBppSupported;

MyRexxFunctionHandler GBM_FilePages;
MyRexxFunctionHandler GBM_FileType;
MyRexxFunctionHandler GBM_FileHeader;
MyRexxFunctionHandler GBM_FilePalette;
MyRexxFunctionHandler GBM_FileData;

MyRexxFunctionHandler GBM_FileWrite;

MyRexxFunctionHandler GBM_ScaleAlgorithms;
MyRexxFunctionHandler GBM_ScaleIsSupported;
MyRexxFunctionHandler GBM_Scale;

MyRexxFunctionHandler GBM_Reflect;
MyRexxFunctionHandler GBM_Rotate;

MyRexxFunctionHandler GBM_PaletteDataTo24bpp;


/*********************************************************************/
/* RxFncTable                                                        */
/*   Array of names of the REXXUTIL functions.                       */
/*   This list is used for registration and deregistration.          */
/*********************************************************************/

static PSZ RxFncTable[] =
{
  "GBM_LoadFuncs",
  "GBM_DropFuncs",

  "GBM_Version",
  "GBM_VersionRexx",
  "GBM_Types",
  "GBM_IsBppSupported",

  "GBM_FilePages",
  "GBM_FileType",
  "GBM_FileHeader",
  "GBM_FilePalette",
  "GBM_FileData",
  "GBM_FileWrite",

  "GBM_ScaleAlgorithms",
  "GBM_ScaleIsSupported",
  "GBM_Scale",

  "GBM_Reflect",
  "GBM_Rotate",

  "GBM_PaletteDataTo24bpp"
};

typedef struct
{
  char * name;   /* filter name */
  int    filter; /* filter type */
} FILTER_NAME_TABLE_DEF;

static const int FILTER_INDEX_SIMPLE = 0;

static FILTER_NAME_TABLE_DEF FILTER_NAME_TABLE [] =
{ { "simple"         , -1 },
  { "nearestneighbor", GBM_SCALE_FILTER_NEARESTNEIGHBOR },
  { "bilinear"       , GBM_SCALE_FILTER_BILINEAR   },
  { "bell"           , GBM_SCALE_FILTER_BELL       },
  { "bspline"        , GBM_SCALE_FILTER_BSPLINE    },
  { "quadratic"      , GBM_SCALE_FILTER_QUADRATIC  },
  { "gaussian"       , GBM_SCALE_FILTER_GAUSSIAN   },
  { "mitchell"       , GBM_SCALE_FILTER_MITCHELL   },
  { "catmullrom"     , GBM_SCALE_FILTER_CATMULLROM },
  { "lanczos"        , GBM_SCALE_FILTER_LANCZOS    },
  { "blackman"       , GBM_SCALE_FILTER_BLACKMAN   },
  { "kaiser"         , GBM_SCALE_FILTER_KAISER     }
};
const int FILTER_NAME_TABLE_LENGTH = sizeof(FILTER_NAME_TABLE) /
                                     sizeof(FILTER_NAME_TABLE[0]);


/*********************************************************************/
/* Some useful macros                                                */
/*********************************************************************/
#define BUILDRXSTRING(t, s) { \
  strcpy((t)->strptr,(s));\
  (t)->strlength = strlen((s)); \
}


/*********************************************************************/
/* Helper functions                                                  */
/*********************************************************************/
static char gbm_error_msg[61] = { 0 };

static const char * formatGbmErrorMessage(const GBM_ERR errorCode)
{
  sprintf(gbm_error_msg, "ERROR: %.53s", Gbm_err(errorCode));
  return gbm_error_msg;
}

/*********************************************************************/

static BOOL isEqualLowerCase(const char *s1, const char *s2, size_t n)
{
  const size_t s1len = strlen(s1);
  const size_t s2len = strlen(s2);
  size_t i;

  for (i = 0; (i < n) && (i < s1len) && (i < s2len); i++, s1++, s2++ )
  {
      if ( tolower(*s1) != tolower(*s2) )
      {
          return FALSE;
      }
  }
  if (i < n)
  {
    return FALSE;
  }
  return TRUE;
}

/*********************************************************************/

static const char *findWord(const char *str, const char *substr)
{
         char * buf, *s;
         char   strbuf[51];
         size_t len            = strlen(substr);
   const size_t strlength      = strlen(str);
   const BOOL   needsDynBuffer = (strlength > (sizeof(strbuf)-1));

   /* only use dynamic buffer if needed to speedup a bit */
   if (needsDynBuffer)
   {
     buf = (char *) malloc(strlength + 1);
     if (buf == NULL)
     {
       return NULL;
     }
   }
   else
   {
     buf = strbuf;
   }

   for ( s  = strtok(strcpy(buf, str), " \t,");
         s != NULL;
         s  = strtok(NULL, " \t,") )
   {
     if ( isEqualLowerCase(s, substr, len) && s[len] == '\0' )
     {
       int inx = s - buf;
       if (needsDynBuffer)
       {
         free(buf);
       }
       return str + inx;
     }
   }
   if (needsDynBuffer)
   {
     free(buf);
   }
   return NULL;
}

/*********************************************************************/

static int lookupGbmFileType(const char * extension)
{
  int numFileTypes = -1;

  if (Gbm_query_n_filetypes(&numFileTypes) == GBM_ERR_OK)
  {
    int     ft;
    GBMFT   gbmft;

    for (ft = 0; ft < numFileTypes; ft++)
    {
      if (Gbm_query_filetype(ft, &gbmft) == GBM_ERR_OK)
      {
        if (findWord(gbmft.extensions, extension) != NULL)
        {
          return ft;
        }
      }
    }
  }
  return -1;
}

/*********************************************************************/

/* Depending on GBM.DLL version the number of pages can be retrieved (versions > 1.35) */
/* or the functionality does not yet exist.                                            */
/*                                                                                     */
/* Dynamically link the specific function to support also older versions of GBM.DLL.   */

BOOL getNumberOfPages(const char * fileName, const int fd, const int ft, int * numPages)
{
   HMODULE hmod;
   PFN     functionAddr = NULL;
   APIRET  rc = 0;

   /* check version first */
   if (Gbm_version() < 135)
   {
      return FALSE;
   }

   /* now dynamically link GBM.DLL */
   rc = DosLoadModule("", 0, "GBM", &hmod);
   if (rc)
   {
      return FALSE;
   }

   /* lookup gbm_read_imgcount() */
   rc = DosQueryProcAddr(hmod, 0L, "gbm_read_imgcount", &functionAddr);
   if (rc)
   {
      DosFreeModule(hmod);
      return FALSE;
   }

   /* call gbm_read_imgcount(const char *fn, int fd, int ft, int *pimgcnt) */
   if (functionAddr(fileName, fd, ft, numPages) != GBM_ERR_OK)
   {
      DosFreeModule(hmod);
      return FALSE;
   }

   DosFreeModule(hmod);

   return TRUE;
}

/*********************************************************************/

static gbm_boolean isGrayscalePalette(const GBMRGB *gbmrgb, const int entries)
{
    if ((entries > 0) && (entries <= 0x100))
    {
        int i;
        for (i = 0; i < entries; i++)
        {
            if ((gbmrgb[i].r != gbmrgb[i].g) ||
                (gbmrgb[i].r != gbmrgb[i].b) ||
                (gbmrgb[i].g != gbmrgb[i].b))
            {
                return GBM_FALSE;
            }
        }
        return GBM_TRUE;
    }
    return GBM_FALSE;
}

/*********************************************************************/

static gbm_boolean expandTo24Bit(GBM *gbm, GBMRGB *gbmrgb, gbm_u8 **ppbData, size_t *dataLen)
{
  const size_t stride     = ((gbm->w * gbm->bpp + 31)/32) * 4;
  const size_t new_stride = ((gbm->w * 3 + 3) & ~3);
  size_t bytes;
  int y;
  gbm_u8 *pbDataNew;

  if (gbm->bpp == 24)
  {
    return ( GBM_TRUE );
  }

  bytes = new_stride * gbm->h;

  pbDataNew = gbmmem_malloc(bytes);
  if (pbDataNew == NULL)
  {
    return ( GBM_FALSE );
  }

  for ( y = 0; y < gbm -> h; y++ )
  {
    gbm_u8 *src = *ppbData + stride * y;
    gbm_u8 *dest = pbDataNew + new_stride * y;
    int   x;

    switch ( gbm -> bpp )
    {
      case 1:
      {
        gbm_u8  c = 0;
        for ( x = 0; x < gbm -> w; x++ )
        {
          if ( (x & 7) == 0 )
              c = *src++;
          else
              c <<= 1;

          *dest++ = gbmrgb [(c & 0x80) != 0].b;
          *dest++ = gbmrgb [(c & 0x80) != 0].g;
          *dest++ = gbmrgb [(c & 0x80) != 0].r;
        }
      }
      break;

      case 4:
        for ( x = 0; x + 1 < gbm -> w; x += 2 )
        {
          gbm_u8 c = *src++;

          *dest++ = gbmrgb [c >> 4].b;
          *dest++ = gbmrgb [c >> 4].g;
          *dest++ = gbmrgb [c >> 4].r;
          *dest++ = gbmrgb [c & 15].b;
          *dest++ = gbmrgb [c & 15].g;
          *dest++ = gbmrgb [c & 15].r;
        }

        if ( x < gbm -> w )
        {
          gbm_u8 c = *src;

          *dest++ = gbmrgb [c >> 4].b;
          *dest++ = gbmrgb [c >> 4].g;
          *dest++ = gbmrgb [c >> 4].r;
        }
        break;

      case 8:
        for ( x = 0; x < gbm -> w; x++ )
        {
          gbm_u8 c = *src++;

          *dest++ = gbmrgb [c].b;
          *dest++ = gbmrgb [c].g;
          *dest++ = gbmrgb [c].r;
        }
        break;
    }
  }

  gbmmem_free(*ppbData);
  *ppbData = pbDataNew;
  *dataLen = bytes;

  gbm->bpp = 24;

  return ( GBM_TRUE );
}

/*********************************************************************/

/* set the REXX header variables from GBM struct */
static gbm_boolean setRexxHeader(const GBM      * gbm,
                                 const char     * headerStem,
                                       ULONG    * retcode,
                                       RXSTRING * retstr)
{
  const int    headerStemLen = strlen(headerStem);
  const char * headerStemSep = (headerStem[headerStemLen-1] != '.') ? "." : "";
        char * stemNameBuffer = NULL;

  char valBuf[21] = { 0 };

  *retcode = INVALID_ROUTINE;

  /* Allocate buffer for full stem name + 10 (stem subnames)) */
  stemNameBuffer = (char *) malloc(strlen(headerStem) + strlen(headerStemSep) + 12);
  if (stemNameBuffer == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  /* set the values into the stems */

  /* bitmap width */
  sprintf(stemNameBuffer, "%s%swidth", headerStem, headerStemSep);
  sprintf(valBuf, "%i", gbm->w);
  if (! SetRexxVariable(stemNameBuffer, valBuf))
  {
    free(stemNameBuffer);
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  /* bitmap height */
  sprintf(stemNameBuffer, "%s%sheight", headerStem, headerStemSep);
  sprintf(valBuf, "%i", gbm->h);
  if (! SetRexxVariable(stemNameBuffer, valBuf))
  {
    free(stemNameBuffer);
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  /* bitmap colour depth */
  sprintf(stemNameBuffer, "%s%sbpp", headerStem, headerStemSep);
  sprintf(valBuf, "%i", gbm->bpp);
  if (! SetRexxVariable(stemNameBuffer, valBuf))
  {
    free(stemNameBuffer);
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  /* number of elements */
  sprintf(stemNameBuffer, "%s%s0", headerStem, headerStemSep);
  if (! SetRexxVariable(stemNameBuffer, "3"))
  {
    free(stemNameBuffer);
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }
  free(stemNameBuffer);

  *retcode = VALID_ROUTINE;
  return GBM_TRUE;
}

/*********************************************************************/

/* get the REXX header variables and copy them to GBM struct */
static gbm_boolean extractRexxHeader(const char * headerStem,
                                     GBM        * gbm,
                                     ULONG      * retcode,
                                     RXSTRING   * retstr)
{
        int    headerStemLen = strlen(headerStem);
  const char * headerStemSep = (headerStem[headerStemLen-1] != '.') ? "." : "";
        char * stemNameBuffer = NULL;

  char value[21] = { 0 };

  headerStemLen += strlen(headerStemSep);

  *retcode = INVALID_ROUTINE;

  /* allocate the name buffer for full stem names: basename + 3 digits + 2 dots + 20 subnames */
  stemNameBuffer = (char *) malloc(headerStemLen + 3 + 2 + 20 + 1);
  if (stemNameBuffer == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  /* header.width */
  sprintf(stemNameBuffer, "%s%swidth", headerStem, headerStemSep);
  if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }
  if (sscanf(value, "%i", &(gbm->w)) != 1)
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }

  /* header.height */
  sprintf(stemNameBuffer, "%s%sheight", headerStem, headerStemSep);
  if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }
  if (sscanf(value, "%i", &(gbm->h)) != 1)
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }

  /* header.bpp */
  sprintf(stemNameBuffer, "%s%sbpp", headerStem, headerStemSep);
  if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }
  if (sscanf(value, "%i", &(gbm->bpp)) != 1)
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }

  free(stemNameBuffer); stemNameBuffer = NULL;

  *retcode = VALID_ROUTINE;
  return GBM_TRUE;
}

/*********************************************************************/

/* get the REXX size variables and return them */
static gbm_boolean extractRexxSize(const char * sizeStem,
                                   int        * w,
                                   int        * h,
                                   ULONG      * retcode,
                                   RXSTRING   * retstr)
{
        int    sizeStemLen = strlen(sizeStem);
  const char * sizeStemSep = (sizeStem[sizeStemLen-1] != '.') ? "." : "";
        char * stemNameBuffer = NULL;

  char value[21] = { 0 };

  sizeStemLen += strlen(sizeStemSep);

  *retcode = INVALID_ROUTINE;

  /* allocate the name buffer for full stem names: basename + 3 digits + 2 dots + 20 subnames */
  stemNameBuffer = (char *) malloc(sizeStemLen + 3 + 2 + 20 + 1);
  if (stemNameBuffer == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  /* size.width */
  sprintf(stemNameBuffer, "%s%swidth", sizeStem, sizeStemSep);
  if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }
  if (sscanf(value, "%i", w) != 1)
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }

  /* size.height */
  sprintf(stemNameBuffer, "%s%sheight", sizeStem, sizeStemSep);
  if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }
  if (sscanf(value, "%i", h) != 1)
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }

  free(stemNameBuffer); stemNameBuffer = NULL;

  *retcode = VALID_ROUTINE;
  return GBM_TRUE;
}

/*********************************************************************/

static gbm_boolean setRexxPalette(const GBMRGB   * gbmrgb,
                                  const int        palette_entries,
                                  const char     * paletteStem,
                                       ULONG     * retcode,
                                       RXSTRING  * retstr)
{
  const int    paletteStemLen = strlen(paletteStem);
  const char * paletteStemSep = (paletteStem[paletteStemLen-1] != '.') ? "." : "";
        char * stemNameBuffer = NULL;

  char valBuf[21] = { 0 };

  *retcode = INVALID_ROUTINE;

  /* Allocate buffer for full stem name + 3 digits for the index + 10 (stem subnames)) */
  stemNameBuffer = (char *) malloc(strlen(paletteStem) + strlen(paletteStemSep) + 3 + 12);
  if (stemNameBuffer == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  /* only colour depth up to 8bpp have a palette */
  if ((palette_entries < 0) || (palette_entries > 256))
  {
    BUILDRXSTRING(retstr, ERROR_BPP_UNSUPP);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  if (palette_entries > 0)
  {
    int i;

    /* set the values into the stems */
    for (i = 0; i < palette_entries; i++)
    {
      /* red */
      sprintf(stemNameBuffer, "%s%s%i.red", paletteStem, paletteStemSep, i+1);
      sprintf(valBuf, "%i", gbmrgb[i].r);
      if (! SetRexxVariable(stemNameBuffer, valBuf))
      {
        free(stemNameBuffer);
        BUILDRXSTRING(retstr, ERROR_REXX_SET);
        *retcode = VALID_ROUTINE;
        return GBM_FALSE;
      }
      /* green */
      sprintf(stemNameBuffer, "%s%s%i.green", paletteStem, paletteStemSep, i+1);
      sprintf(valBuf, "%i", gbmrgb[i].g);
      if (! SetRexxVariable(stemNameBuffer, valBuf))
      {
        free(stemNameBuffer);
        BUILDRXSTRING(retstr, ERROR_REXX_SET);
        *retcode = VALID_ROUTINE;
        return GBM_FALSE;
      }
      /* blue */
      sprintf(stemNameBuffer, "%s%s%i.blue", paletteStem, paletteStemSep, i+1);
      sprintf(valBuf, "%i", gbmrgb[i].b);
      if (! SetRexxVariable(stemNameBuffer, valBuf))
      {
        free(stemNameBuffer);
        BUILDRXSTRING(retstr, ERROR_REXX_SET);
        *retcode = VALID_ROUTINE;
        return GBM_FALSE;
      }
    }
  }

  /* number of palette entries */
  sprintf(stemNameBuffer, "%s%s0", paletteStem, paletteStemSep);
  sprintf(valBuf, "%i", palette_entries);
  if (! SetRexxVariable(stemNameBuffer, valBuf))
  {
    free(stemNameBuffer);
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  free(stemNameBuffer); stemNameBuffer = NULL;

  *retcode = VALID_ROUTINE;
  return GBM_TRUE;
}

/*********************************************************************/

/* get the REXX palette variables and convert to GBMRGB struct */
static gbm_boolean extractRexxPalette(const char * paletteStem,
                                      GBMRGB     * gbmrgb,
                                      ULONG      * retcode,
                                      RXSTRING   * retstr)
{
        int    paletteStemLen = strlen(paletteStem);
  const char * paletteStemSep = (paletteStem[paletteStemLen-1] != '.') ? "." : "";
        char * stemNameBuffer = NULL;

  int i, palette_entries = 0;
  char value[21] = { 0 };

  paletteStemLen += strlen(paletteStemSep);

  *retcode = INVALID_ROUTINE;

  /* allocate the name buffer for full stem names: basename + 3 digits + 2 dots + 20 subnames */
  stemNameBuffer = (char *) malloc(paletteStemLen + 3 + 2 + 20 + 1);
  if (stemNameBuffer == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    *retcode = VALID_ROUTINE;
    return GBM_FALSE;
  }

  memset(gbmrgb, 0, sizeof(GBMRGB) * 0x100);

  /* number of entries */
  sprintf(stemNameBuffer, "%s%s0", paletteStem, paletteStemSep);
  if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }
  if (sscanf(value, "%i", &palette_entries) != 1)
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }
  if (palette_entries > 0x100)
  {
    free(stemNameBuffer);
    return GBM_FALSE;
  }

  /* copy palette entries */
  for (i = 0; i < palette_entries; i++)
  {
    int r, g, b;

    /* red */
    sprintf(stemNameBuffer, "%s%s%i.red", paletteStem, paletteStemSep, i+1);
    if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
    {
      free(stemNameBuffer);
      return GBM_FALSE;
    }
    if (sscanf(value, "%i", &r) != 1)
    {
      free(stemNameBuffer);
      return GBM_FALSE;
    }

    /* green */
    sprintf(stemNameBuffer, "%s%s%i.green", paletteStem, paletteStemSep, i+1);
    if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
    {
      free(stemNameBuffer);
      return GBM_FALSE;
    }
    if (sscanf(value, "%i", &g) != 1)
    {
      free(stemNameBuffer);
      return GBM_FALSE;
    }

    /* blue */
    sprintf(stemNameBuffer, "%s%s%i.blue", paletteStem, paletteStemSep, i+1);
    if (! GetRexxVariable(stemNameBuffer, value, sizeof(value)-1))
    {
      free(stemNameBuffer);
      return GBM_FALSE;
    }
    if (sscanf(value, "%i", &b) != 1)
    {
      free(stemNameBuffer);
      return GBM_FALSE;
    }

    if (((r < 0) || (r > 255)) ||
        ((g < 0) || (g > 255)) ||
        ((b < 0) || (b > 255)))
    {
      free(stemNameBuffer);
      return GBM_FALSE;
    }
    gbmrgb[i].r = r;
    gbmrgb[i].g = g;
    gbmrgb[i].b = b;
  }

  free(stemNameBuffer); stemNameBuffer = NULL;

  *retcode = VALID_ROUTINE;
  return GBM_TRUE;
}

/*********************************************************************/

/* get the REXX bitmap data and return as new allocated buffer */
static gbm_u8 * extractRexxBitmapData(const char * dataStem,
                                      const GBM  * gbm,
                                      size_t     * dataLen)
{
   const size_t stride = ( ((gbm->w * gbm->bpp + 31)/32) * 4 );
   gbm_u8 * data = NULL;
   *dataLen = stride * gbm->h;

   /* Allocate a buffer for the bitmap data */
   data = gbmmem_malloc(*dataLen);
   if (data == NULL)
   {
     *dataLen = 0;
     return NULL;
   }
   if (! GetRexxBinaryVariable(dataStem, data, *dataLen))
   {
     gbmmem_free(data);
     *dataLen = 0;
     return NULL;
   }
   return data;
}

/*********************************************************************/
/*********************************************************************/
/*********************************************************************/


/*********************************************************************/
/* Function:  GBM_LoadFuncs                                          */
/*                                                                   */
/* Syntax:    call GBM_LoadFuncs                                     */
/*                                                                   */
/* Params:    none                                                   */
/*                                                                   */
/* Return:    null string                                            */
/*********************************************************************/

ULONG GBM_LoadFuncs(PUCHAR name, ULONG numargs, RXSTRING args[],
                    PSZ queuename, RXSTRING *retstr)
{
  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retstr->strlength = 0; /* return a null string result*/

  if (numargs != 0)
  {
    return INVALID_ROUTINE;
  }

  RegisterRexxFunctions("GBMRX",
                        RxFncTable,
                        sizeof(RxFncTable)/sizeof(PSZ));

  return VALID_ROUTINE;  /* no error on call */
}

/*********************************************************************/
/* Function:  GBM_DropFuncs                                          */
/*                                                                   */
/* Syntax:    call GBM_DropFuncs                                     */
/*                                                                   */
/* Params:    none                                                   */
/*                                                                   */
/* Return:    null string                                            */
/*********************************************************************/

ULONG GBM_DropFuncs(PUCHAR name, ULONG numargs, RXSTRING args[],
                    PSZ queuename, RXSTRING *retstr)
{
  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retstr->strlength = 0;           /* return a null string result*/

  if (numargs != 0)                /* no arguments for this      */
  {
    return INVALID_ROUTINE;
  }

  UnregisterRexxFunctions(RxFncTable,
                          sizeof(RxFncTable)/sizeof(PSZ));

  return VALID_ROUTINE;  /* no error on call */
}


/*********************************************************************/
/* Function:  GBM_Version                                            */
/*                                                                   */
/* Syntax:    version = GBM_Version()                                */
/*                                                                   */
/* Return:    Version number: "major.minor"                          */
/*********************************************************************/
#define GET_GBM_VERSION(version)  ((float)version/100.0)

ULONG GBM_Version(PUCHAR name, ULONG numargs, RXSTRING args[],
                  PSZ queuename, RXSTRING *retstr)
{
  int version;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retstr->strlength = 0;           /* return a null string result*/

  if (numargs != 0)                /* no arguments for this      */
  {
    return INVALID_ROUTINE;
  }

  Gbm_init();
  version = Gbm_version();
  Gbm_deinit();

  /* build return string ("major.minor")*/
  sprintf(retstr->strptr, "%.2f", GET_GBM_VERSION(version));
  retstr->strlength = strlen(retstr->strptr);

  return VALID_ROUTINE;  /* no error on call */
}


/*********************************************************************/
/* Function:  GBM_VersionRexx                                        */
/*                                                                   */
/* Syntax:    version = GBM_VersionRexx()                            */
/*                                                                   */
/* Return:    Version number: "major.minor"                          */
/*********************************************************************/

ULONG GBM_VersionRexx(PUCHAR name, ULONG numargs, RXSTRING args[],
                      PSZ queuename, RXSTRING *retstr)
{
  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retstr->strlength = 0;           /* return a null string result*/

  if (numargs != 0)                /* no arguments for this      */
  {
    return INVALID_ROUTINE;
  }

  /* build return string ("major.minor")*/
  BUILDRXSTRING(retstr, GBM_VERSION_REXX);

  return VALID_ROUTINE;  /* no error on call */
}


/*******************************************************************/
/* Function:  GBM_Types                                            */
/*                                                                 */
/* Syntax:    rc = GBM_Types(stem)                                 */
/*                                                                 */
/* Params:    stem out - bitmap format information consisting of   */
/*                       stem.0           : number of info blocks  */
/*                       stem.X.extensions: bitmap extensions      */
/*                       stem.X.shortname : short description      */
/*                       stem.X.longname  : long description       */
/*                                                                 */
/*                       X: Ranging from 1 to stem.0               */
/*                                                                 */
/* Return:   "0", an error message or "ERROR" for an unknown error */
/*******************************************************************/

ULONG GBM_Types(PUCHAR name, ULONG numargs, RXSTRING args[],
                PSZ queuename, RXSTRING *retstr)
{
  int       numFileTypes = 0;
  char    * infoStem     = NULL;
  char    * infoStemSep  = "";
  GBM_ERR   rc           = GBM_ERR_NOT_SUPP;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if (numargs != 1)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  infoStem    = args[0].strptr;
  infoStemSep = (infoStem[strlen(infoStem)-1] != '.') ? "." : "";

  Gbm_init();

  /* build return string */
  rc = Gbm_query_n_filetypes(&numFileTypes);
  if (rc == GBM_ERR_OK)
  {
    int     ft;
    GBMFT   gbmft;
    char  * stemBuf    = NULL;
    char    varBuf[21] = { 0 };

    /* prepare the stem buffer */
    const int maxStemLength = strlen(infoStem) + strlen(infoStemSep);

    /* we support only 999 file types which should be enough */
    if (numFileTypes > 999)
    {
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_TOOMANY_TYPES);
      return VALID_ROUTINE;
    }

    /* allocate the stem name buffer + 3 digits for the index + info name (12) */
    stemBuf = (char *) malloc(maxStemLength + 3 /* stem index */ + 12 /* info name */ + 1);
    if (stemBuf == NULL)
    {
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_MEM);
      return VALID_ROUTINE;
    }

    /* now set the types to the stems */
    for (ft = 0; ft < numFileTypes; ft++)
    {
       rc = Gbm_query_filetype(ft, &gbmft);
      if (rc != GBM_ERR_OK)
      {
        BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
        Gbm_deinit();
        return VALID_ROUTINE;
      }

      /* set to the stem */
      sprintf(stemBuf, "%s%s%i.extensions", infoStem, infoStemSep, ft + 1);
      if (! SetRexxVariable(stemBuf, gbmft.extensions))
      {
        free(stemBuf);
        Gbm_deinit();
        BUILDRXSTRING(retstr, ERROR_REXX_SET);
        return VALID_ROUTINE;
      }
      sprintf(stemBuf, "%s%s%i.shortname", infoStem, infoStemSep, ft + 1);
      if (! SetRexxVariable(stemBuf, gbmft.short_name))
      {
        free(stemBuf);
        Gbm_deinit();
        BUILDRXSTRING(retstr, ERROR_REXX_SET);
        return VALID_ROUTINE;
      }
      sprintf(stemBuf, "%s%s%i.longname", infoStem, infoStemSep, ft + 1);
      if (! SetRexxVariable(stemBuf, gbmft.long_name))
      {
        free(stemBuf);
        Gbm_deinit();
        BUILDRXSTRING(retstr, ERROR_REXX_SET);
        return VALID_ROUTINE;
      }
    }

    /* set the number of types into stem.0 */
    sprintf(varBuf, "%i", numFileTypes);
    sprintf(stemBuf, "%s%s0", infoStem, infoStemSep);
    if (! SetRexxVariable(stemBuf, varBuf))
    {
      free(stemBuf);
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_REXX_SET);
      return VALID_ROUTINE;
    }

    free(stemBuf);
    Gbm_deinit();

    BUILDRXSTRING(retstr, NOERROR_RET);
    return VALID_ROUTINE;
  }

  BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));

  Gbm_deinit();
  return VALID_ROUTINE;
}


/************************************************************************/
/* Function:  GBM_IsBppSupported                                        */
/*                                                                      */
/* Syntax:    sup = GBM_IsBppSupported(fileExtension, bpp, rw)          */
/*                                                                      */
/* Params:    string  in - the filetype extension reported by GBM_Types */
/*                                                                      */
/*            string  in - bpp (bits per pixel) to test                 */
/*                                                                      */
/*            string  in - "r" for testing for read support             */
/*                         "w" for testing for read support             */
/*                                                                      */
/* Return:   "0" (unsupported) or "1" (supported)                       */
/************************************************************************/

ULONG GBM_IsBppSupported(PUCHAR name, ULONG numargs, RXSTRING args[],
                         PSZ queuename, RXSTRING *retstr)
{
  char    * extension    = NULL;
  char    * readwrite    = NULL;
  int       filetype     = -1;
  int       bpp          = -1;
  GBMFT     gbmft        = { 0 };
  GBM_ERR   rc           = GBM_ERR_NOT_SUPP;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, "0");

  if (numargs != 3)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  extension = args[0].strptr;
  readwrite = args[2].strptr;

  if (sscanf(args[1].strptr, "%i", &bpp) != 1)
  {
    return INVALID_ROUTINE;
  }

  Gbm_init();

  filetype = lookupGbmFileType(extension);
  if (filetype < 0)
  {
    Gbm_deinit();
    return VALID_ROUTINE;
  }

  /* check if the output format supports the requested color depth */
  rc = Gbm_query_filetype(filetype, &gbmft);
  if (rc != GBM_ERR_OK)
  {
    Gbm_deinit();
    return VALID_ROUTINE;
  }

  Gbm_deinit();

  if (isEqualLowerCase(readwrite, "r", 1))
  {
    int flag = 0;
    switch(bpp)
    {
      case 64: flag = GBM_FT_R64; break;
      case 48: flag = GBM_FT_R48; break;
      case 32: flag = GBM_FT_R32; break;
      case 24: flag = GBM_FT_R24; break;
      case  8: flag = GBM_FT_R8;  break;
      case  4: flag = GBM_FT_R4;  break;
      case  1: flag = GBM_FT_R1;  break;
      default: flag = 0;          break;
    }
    if ( (gbmft.flags & flag) == 0 )
    {
      return VALID_ROUTINE;
    }
    BUILDRXSTRING(retstr, "1");
    return VALID_ROUTINE;
  }

  if (isEqualLowerCase(readwrite, "w", 1))
  {
    int flag = 0;
    switch(bpp)
    {
      case 64: flag = GBM_FT_W64; break;
      case 48: flag = GBM_FT_W48; break;
      case 32: flag = GBM_FT_W32; break;
      case 24: flag = GBM_FT_W24; break;
      case  8: flag = GBM_FT_W8;  break;
      case  4: flag = GBM_FT_W4;  break;
      case  1: flag = GBM_FT_W1;  break;
      default: flag = 0;          break;
    }
    if ( (gbmft.flags & flag) == 0 )
    {
      return VALID_ROUTINE;
    }
    BUILDRXSTRING(retstr, "1");
    return VALID_ROUTINE;
  }

  return VALID_ROUTINE;  /* no error on call */
}


/*******************************************************************/
/* Function:  GBM_FileType                                         */
/*                                                                 */
/* Syntax:    rc = GBM_FileType(filename, stem)                    */
/*                                                                 */
/* Params:    string  in - filename                                */
/*                                                                 */
/*            stem   out - bitmap format information consisting of */
/*                         stem.extensions: bitmap extensions      */
/*                         stem.shortname : short description      */
/*                         stem.longname  : long description       */
/*                                                                 */
/* Return:   "0", an error message or "ERROR" for an unknown error */
/*                                                                 */
/* Note: This function does only check the codec database but not  */
/*       the file content. Use GBM_FileHeader for this.            */
/*                                                                 */
/*******************************************************************/

ULONG GBM_FileType(PUCHAR name, ULONG numargs, RXSTRING args[],
                   PSZ queuename, RXSTRING *retstr)
{
  char    * filename     = NULL;
  char    * infoStem     = NULL;
  char    * infoStemSep  = NULL;
  char    * infoStemFull = NULL;
  int       filetype     = -1;
  GBMFT     gbmft        = { 0 };
  GBM_ERR   rc           = GBM_ERR_NOT_SUPP;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if (numargs != 2)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  filename    = args[0].strptr;
  infoStem    = args[1].strptr;
  infoStemSep = (infoStem[strlen(infoStem)-1] != '.') ? "." : "";

  Gbm_init();

  /* query filetype from file extension */
  rc = Gbm_guess_filetype(filename, &filetype);
  if (rc != GBM_ERR_OK)
  {
    BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
    Gbm_deinit();
    return VALID_ROUTINE;
  }
  rc = Gbm_query_filetype(filetype, &gbmft);
  if (rc != GBM_ERR_OK)
  {
    BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
    Gbm_deinit();
    return VALID_ROUTINE;
  }
  Gbm_deinit();

  /* allocate the stem name buffer + info name (12) */
  infoStemFull = (char *) malloc(strlen(infoStem) + strlen(infoStemSep) + 12 /* info name */ + 1);
  if (infoStemFull == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    return VALID_ROUTINE;
  }

  /* set the values into the stems */
  sprintf(infoStemFull, "%s%sextensions", infoStem, infoStemSep);
  if (! SetRexxVariable(infoStemFull, gbmft.extensions))
  {
    free(infoStemFull);
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    return VALID_ROUTINE;
  }
  sprintf(infoStemFull, "%s%sshortname", infoStem, infoStemSep);
  if (! SetRexxVariable(infoStemFull, gbmft.short_name))
  {
    free(infoStemFull);
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    return VALID_ROUTINE;
  }
  sprintf(infoStemFull, "%s%slongname", infoStem, infoStemSep);
  if (! SetRexxVariable(infoStemFull, gbmft.long_name))
  {
    free(infoStemFull);
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    return VALID_ROUTINE;
  }
  free(infoStemFull);

  /* build return string */
  BUILDRXSTRING(retstr, NOERROR_RET);
  return VALID_ROUTINE;  /* no error on call */
}


/*********************************************************************/
/* Function:  GBM_FilePages                                          */
/*                                                                   */
/* Syntax:    rc = GBM_FilePages(filename [, fileExtension])         */
/*                                                                   */
/* Params:    string  in - filename                                  */
/*                                                                   */
/*            string  in - the optional filetype extension reported  */
/*                         by GBM_Types to override autodetection    */
/*                         based on the file extension.              */
/*                                                                   */
/* Return:   "0", an error message or "ERROR" for an unknown error   */
/*********************************************************************/

ULONG GBM_FilePages(PUCHAR name, ULONG numargs, RXSTRING args[],
                    PSZ queuename, RXSTRING *retstr)
{
  int       fd       = -1;
  int       imgCount = 0;
  char    * filename = NULL;
  int       filetype = -1;
  GBM_ERR   rc       = GBM_ERR_NOT_SUPP;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if ((numargs < 1) || (numargs > 2))
  {
    return INVALID_ROUTINE;
  }

  Gbm_init();

  filename = args[0].strptr;
  if (numargs == 2)
  {
    filetype = lookupGbmFileType(args[1].strptr);
    if (filetype < 0)
    {
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_FILETYPE);
      return VALID_ROUTINE;
    }
  }

  /* open file */
  fd = Gbm_io_open(filename, GBM_O_RDONLY);
  if (fd != -1)
  {
    /* check if the client has specified no file type, use autodetection */
    if (filetype == -1)
    {
      /* query filetype from file extension */
      rc = Gbm_guess_filetype(filename, &filetype);
      if (rc != GBM_ERR_OK)
      {
        BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
        Gbm_io_close(fd);
        Gbm_deinit();
        return VALID_ROUTINE;
      }
    }

    /* get image count */
    if (! getNumberOfPages(filename, fd, filetype, &imgCount))
    {
      BUILDRXSTRING(retstr, ERROR_PAGEQUERY_UNSUPP);
      Gbm_io_close(fd);
      Gbm_deinit();
      return VALID_ROUTINE;
    }

    Gbm_io_close(fd);
    Gbm_deinit();

    /* build return string */
    sprintf(retstr->strptr, "%i", imgCount);
    retstr->strlength = strlen(retstr->strptr);

    return VALID_ROUTINE;  /* no error on call */
  }

  BUILDRXSTRING(retstr, ERROR_FILEOPEN);

  Gbm_deinit();
  return VALID_ROUTINE;
}


/*****************************************************************************/
/* Function:  GBM_FileHeader                                                 */
/*                                                                           */
/* Syntax:    rc = GBM_FileHeader(filename, options, stem [, fileExtension]) */
/*                                                                           */
/* Params:    string  in - filename                                          */
/*                                                                           */
/*            string  in - comma separated options                           */
/*                         (see GBM format documentation)                    */
/*                                                                           */
/*            stem   out - header information consisting of                  */
/*                         stem.width : bitmap width                         */
/*                         stem.height: bitmap height                        */
/*                         stem.bpp   : bitmap colour depth (bpp)            */
/*                                                                           */
/*            string  in - the optional filetype extension reported          */
/*                         by GBM_Types to override autodetection            */
/*                         based on the file extension.                      */
/*                                                                           */
/* Return:   "0", an error message or "ERROR" for an unknown error           */
/*****************************************************************************/

ULONG GBM_FileHeader(PUCHAR name, ULONG numargs, RXSTRING args[],
                     PSZ queuename, RXSTRING *retstr)
{
  char    * filename    = NULL;
  char    * options     = NULL;
  char    * headerStem  = NULL;
  int       fd          = -1;
  int       filetype    = -1;
  GBM       gbm         = { 0 };
  GBM_ERR   rc          = GBM_ERR_NOT_SUPP;
  ULONG     retcode     = INVALID_ROUTINE;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if ((numargs < 3) || (numargs > 4))
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  filename   = args[0].strptr;
  options    = args[1].strptr;
  headerStem = args[2].strptr;

  Gbm_init();

  /* check if filetype override is provided */
  if (numargs > 3)
  {
    filetype = lookupGbmFileType(args[3].strptr);
    if (filetype < 0)
    {
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_FILETYPE);
      return VALID_ROUTINE;
    }
  }

  /* open file */
  fd = Gbm_io_open(filename, GBM_O_RDONLY);
  if (fd != -1)
  {
    /* check if the client has specified no file type, use autodetection */
    if (filetype == -1)
    {
      /* query filetype from file extension */
      rc = Gbm_guess_filetype(filename, &filetype);
      if (rc != GBM_ERR_OK)
      {
        BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
        Gbm_io_close(fd);
        Gbm_deinit();
        return VALID_ROUTINE;
      }
    }

    /* get file header */
    rc = Gbm_read_header(filename, fd, filetype, &gbm, options);
    if (rc != GBM_ERR_OK)
    {
      BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
      Gbm_io_close(fd);
      Gbm_deinit();
      return VALID_ROUTINE;
    }

    Gbm_io_close(fd);
    Gbm_deinit();

    /* set the values into the stem */
    retcode = VALID_ROUTINE;
    if (! setRexxHeader(&gbm, headerStem, &retcode, retstr))
    {
        /* error string and code provided by called function */
        return retcode;
    }

    /* build return string */
    BUILDRXSTRING(retstr, NOERROR_RET);

    return VALID_ROUTINE;  /* no error on call */
  }

  BUILDRXSTRING(retstr, ERROR_FILEOPEN);

  Gbm_deinit();
  return VALID_ROUTINE;
}


/*****************************************************************************/
/* Function: GBM_FilePalette                                                 */
/*                                                                           */
/* Syntax:   rc = GBM_FilePalette(filename, options, stem [, fileExtension]) */
/*                                                                           */
/* Params:   string  in - filename                                           */
/*                                                                           */
/*           string  in - comma separated options                            */
/*                        (see GBM format documentation)                     */
/*                                                                           */
/*           stem   out - palette information consisting of                  */
/*                        stem.0      : number of palette entries            */
/*                        stem.X.red  : red value                            */
/*                        stem.X.green: green value                          */
/*                        stem.X.blue : blue value                           */
/*                                                                           */
/*                        X: Ranging from 1 to stem.0                        */
/*                                                                           */
/*                        Note: Not all palette entries must be              */
/*                              referenced by the bitmap data.               */
/*                                                                           */
/*           string  in - the optional filetype extension reported           */
/*                        by GBM_Types to override autodetection             */
/*                        based on the file extension.                       */
/*                                                                           */
/* Return:   "0", an error message or "ERROR" for an unknown error           */
/*****************************************************************************/

ULONG GBM_FilePalette(PUCHAR name, ULONG numargs, RXSTRING args[],
                      PSZ queuename, RXSTRING *retstr)
{
  char    * filename    = NULL;
  char    * options     = NULL;
  char    * paletteStem = NULL;
  int       fd          = -1;
  int       filetype    = -1;
  GBM       gbm         = { 0 };
  GBM_ERR   rc          = GBM_ERR_NOT_SUPP;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if ((numargs < 3) || (numargs > 4))
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  filename    = args[0].strptr;
  options     = args[1].strptr;
  paletteStem = args[2].strptr;

  Gbm_init();

  /* check if filetype override is provided */
  if (numargs > 3)
  {
    filetype = lookupGbmFileType(args[3].strptr);
    if (filetype < 0)
    {
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_FILETYPE);
      return VALID_ROUTINE;
    }
  }

  /* open file */
  fd = Gbm_io_open(filename, GBM_O_RDONLY);
  if (fd != -1)
  {
    int palette_entries  = 0;
    GBMRGB gbmrgb[0x100] = { { 0, 0, 0 } };
    ULONG  retcode       = INVALID_ROUTINE;

    /* check if the client has specified no file type, use autodetection */
    if (filetype == -1)
    {
      /* query filetype from file extension */
      rc = Gbm_guess_filetype(filename, &filetype);
      if (rc != GBM_ERR_OK)
      {
        BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
        Gbm_io_close(fd);
        Gbm_deinit();
        return VALID_ROUTINE;
      }
    }

    /* get file header */
    rc = Gbm_read_header(filename, fd, filetype, &gbm, options);
    if (rc != GBM_ERR_OK)
    {
      BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
      Gbm_io_close(fd);
      Gbm_deinit();
      return VALID_ROUTINE;
    }

    /* get bitmap palette */
    rc = Gbm_read_palette(fd, filetype, &gbm, gbmrgb);
    if (rc != GBM_ERR_OK)
    {
      BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
      Gbm_io_close(fd);
      Gbm_deinit();
      return VALID_ROUTINE;
    }
    Gbm_io_close(fd);
    Gbm_deinit();

    /* only colour depth up to 8bpp have a palette */
    retcode = VALID_ROUTINE;
    if (gbm.bpp <= 8)
    {
      palette_entries = 1 << gbm.bpp;
    }
    /* set the values into the stems */
    if (! setRexxPalette(gbmrgb, palette_entries, paletteStem, &retcode, retstr))
    {
        /* error string and code provided by called function */
        return retcode;
    }

    /* build return string */
    BUILDRXSTRING(retstr, NOERROR_RET);

    return VALID_ROUTINE;  /* no error on call */
  }

  BUILDRXSTRING(retstr, ERROR_FILEOPEN);

  Gbm_deinit();
  return VALID_ROUTINE;
}


/*****************************************************************************/
/* Function:  GBM_FileData                                                   */
/*                                                                           */
/* Syntax:    rc = GBM_FileData(filename, options, stem [, fileExtension])   */
/*                                                                           */
/* Params:    string  in - filename                                          */
/*                                                                           */
/*            string  in - comma separated options                           */
/*                         (see GBM format documentation)                    */
/*                                                                           */
/*            stem   out - The binary bitmap data.                           */
/*                                                                           */
/*            string  in - the optional filetype extension reported          */
/*                         by GBM_Types to override autodetection            */
/*                         based on the file extension.                      */
/*                                                                           */
/* Return:   "0", an error message or "ERROR" for an unknown error           */
/*****************************************************************************/

ULONG GBM_FileData(PUCHAR name, ULONG numargs, RXSTRING args[],
                   PSZ queuename, RXSTRING *retstr)
{
  char    * filename = NULL;
  char    * options  = NULL;
  char    * dataStem = NULL;
  int       fd       = -1;
  int       filetype = -1;
  GBM       gbm      = { 0 };
  GBM_ERR   rc       = GBM_ERR_NOT_SUPP;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if ((numargs < 3) || (numargs > 4))
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  filename = args[0].strptr;
  options  = args[1].strptr;
  dataStem = args[2].strptr;

  Gbm_init();

  /* check if filetype override is provided */
  if (numargs > 3)
  {
    filetype = lookupGbmFileType(args[3].strptr);
    if (filetype < 0)
    {
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_FILETYPE);
      return VALID_ROUTINE;
    }
  }

  /* open file */
  fd = Gbm_io_open(filename, GBM_O_RDONLY);
  if (fd != -1)
  {
    size_t stride  = 0;
    size_t datalen = 0;
    gbm_u8 * data    = NULL;

    /* check if the client has specified no file type, use autodetection */
    if (filetype == -1)
    {
      /* query filetype from file extension */
      rc = Gbm_guess_filetype(filename, &filetype);
      if (rc != GBM_ERR_OK)
      {
        BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
        Gbm_io_close(fd);
        Gbm_deinit();
        return VALID_ROUTINE;
      }
    }

    /* get file header */
    rc = Gbm_read_header(filename, fd, filetype, &gbm, options);
    if (rc != GBM_ERR_OK)
    {
      BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
      Gbm_io_close(fd);
      Gbm_deinit();
      return VALID_ROUTINE;
    }

    /* Allocate a buffer for the bitmap data */
    stride  = ( ((gbm.w * gbm.bpp + 31)/32) * 4 );
    datalen = stride * gbm.h;

    data = gbmmem_malloc(datalen);
    if (data == NULL)
    {
      Gbm_io_close(fd);
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_MEM);
      return VALID_ROUTINE;
    }

    /* Read the bitmap data */
    rc = Gbm_read_data(fd, filetype, &gbm, data);
    if (rc != GBM_ERR_OK)
    {
      gbmmem_free(data);
      BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
      Gbm_io_close(fd);
      Gbm_deinit();
      return VALID_ROUTINE;
    }

    Gbm_io_close(fd);
    Gbm_deinit();

    /* set the data into the stem */
    if (! SetRexxBinaryVariable(dataStem, data, datalen))
    {
      gbmmem_free(data);
      BUILDRXSTRING(retstr, ERROR_REXX_SET);
      return VALID_ROUTINE;
    }

    gbmmem_free(data);

    /* build return string */
    BUILDRXSTRING(retstr, NOERROR_RET);

    return VALID_ROUTINE;  /* no error on call */
  }

  BUILDRXSTRING(retstr, ERROR_FILEOPEN);

  Gbm_deinit();
  return VALID_ROUTINE;
}


/*******************************************************************************************/
/* Function:  GBM_FileWrite                                                                */
/*                                                                                         */
/* Syntax:    rc = GBM_FileWrite(filename, options, stem1, stem2, stem3 [, fileExtension]) */
/*                                                                                         */
/* Params:    string  in - filename                                                        */
/*                                                                                         */
/*            string  in - comma separated options                                         */
/*                         (see GBM format documentation)                                  */
/*                                                                                         */
/*            stem1   in - header information consisting of                                */
/*                         stem1.width : bitmap width                                      */
/*                         stem1.height: bitmap height                                     */
/*                         stem1.bpp   : bitmap colour depth (bpp)                         */
/*                                                                                         */
/*            stem2   in - palette information consisting of                               */
/*                         stem2.0      : number of palette entries                        */
/*                         stem2.X.red  : red value                                        */
/*                         stem2.X.green: green value                                      */
/*                         stem2.X.blue : blue value                                       */
/*                                                                                         */
/*                         X: Ranging from 1 to stem2.0                                    */
/*                                                                                         */
/*                         Note: The number of entries must match                          */
/*                               stem1.bpp^2. For true color images it                     */
/*                               can be 0.                                                 */
/*                                                                                         */
/*            stem3   in - The binary bitmap data.                                         */
/*                                                                                         */
/*            string  in - the optional filetype extension reported                        */
/*                         by GBM_Types to override autodetection                          */
/*                         based on the file extension.                                    */
/*                                                                                         */
/* Return:    "0", an error message or "ERROR" for an unknown error                        */
/*******************************************************************************************/

ULONG GBM_FileWrite(PUCHAR name, ULONG numargs, RXSTRING args[],
                    PSZ queuename, RXSTRING *retstr)
{
  char * filename       = NULL;
  char * options        = NULL;

  char * headerStem     = NULL;
  char * paletteStem    = NULL;
  char * dataStem       = NULL;

  int    fd             = -1;
  int    filetype       = -1;
  int    flag           = 0;

  GBM    gbm            = { 0 };
  GBMFT  gbmft          = { 0 };
  GBMRGB gbmrgb[0x100]  = { { 0, 0, 0 } };

  gbm_u8 * data    = NULL;
  size_t   dataLen = 0;

  GBM_ERR rc            = GBM_ERR_NOT_SUPP;
  ULONG  retcode;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if ((numargs < 5) || (numargs > 6))
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  filename       = args[0].strptr;
  options        = args[1].strptr;
  headerStem     = args[2].strptr;
  paletteStem    = args[3].strptr;
  dataStem       = args[4].strptr;

  /* get the REXX header variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxHeader(headerStem, &gbm, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }

  /* get the REXX palette variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxPalette(paletteStem, gbmrgb, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }

  /* get the REXX bitmap data */
  data = extractRexxBitmapData(dataStem, &gbm, &dataLen);
  if (data == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    return VALID_ROUTINE;
  }

  Gbm_init();

  /* check if filetype override is provided */
  if (numargs > 5)
  {
    filetype = lookupGbmFileType(args[5].strptr);
    if (filetype < 0)
    {
      gbmmem_free(data);
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_FILETYPE);
      return VALID_ROUTINE;
    }
  }
  else
  {
    /* query filetype from file extension */
    rc = Gbm_guess_filetype(filename, &filetype);
    if (rc != GBM_ERR_OK)
    {
      gbmmem_free(data);
      BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
      Gbm_deinit();
      return VALID_ROUTINE;
    }
  }

  /* check if the output format supports the requested color depth */
  rc = Gbm_query_filetype(filetype, &gbmft);
  if (rc != GBM_ERR_OK)
  {
    BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
    Gbm_deinit();
    return VALID_ROUTINE;
  }

  switch(gbm.bpp)
  {
    case 64: flag = GBM_FT_W64; break;
    case 48: flag = GBM_FT_W48; break;
    case 32: flag = GBM_FT_W32; break;
    case 24: flag = GBM_FT_W24; break;
    case  8: flag = GBM_FT_W8;  break;
    case  4: flag = GBM_FT_W4;  break;
    case  1: flag = GBM_FT_W1;  break;
    default: flag = 0;          break;
  }
  if ( (gbmft.flags & flag) == 0 )
  {
    gbmmem_free(data);
    Gbm_deinit();
    BUILDRXSTRING(retstr, ERROR_BPP_UNSUPP);
    return VALID_ROUTINE;
  }

  /* create file */
  fd = Gbm_io_create(filename, GBM_O_WRONLY);
  if (fd != -1)
  {
    rc = Gbm_write(filename, fd, filetype, &gbm, gbmrgb, data, options);
    if (rc != GBM_ERR_OK)
    {
      BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
      gbmmem_free(data);
      Gbm_io_close(fd);
      remove(filename);
      Gbm_deinit();
      return VALID_ROUTINE;
    }

    gbmmem_free(data);
    Gbm_io_close(fd);

    /* build return string */
    BUILDRXSTRING(retstr, NOERROR_RET);

    return VALID_ROUTINE;
  }

  /* build return string */
  BUILDRXSTRING(retstr, ERROR_FILECREATE);

  Gbm_deinit();
  return VALID_ROUTINE;
}


/********************************************************************/
/* Function:  GBM_ScaleAlgorithms                                   */
/*                                                                  */
/* Syntax:    rc = GBM_ScaleAlgorithms(stem)                        */
/*                                                                  */
/* Params:    stem out - supported scaling algorithms consisting of */
/*                       stem.0           : number of algorithms    */
/*                       stem.X.algorithm : algorithm               */
/*                                                                  */
/*                       X: Ranging from 1 to stem.0                */
/*                                                                  */
/* Return:   "0", an error message or "ERROR" for an unknown error  */
/********************************************************************/

ULONG GBM_ScaleAlgorithms(PUCHAR name, ULONG numargs, RXSTRING args[],
                          PSZ queuename, RXSTRING *retstr)
{
  char * infoStem    = NULL;
  char * infoStemSep = "";

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if (numargs != 1)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  infoStem    = args[0].strptr;
  infoStemSep = (infoStem[strlen(infoStem)-1] != '.') ? "." : "";

  Gbm_init();

  /* build returned info */
  {
    char stemBuf[51] = { 0 };
    char varBuf [21] = { 0 };

    /* now set the types to the stems */

    int n;
    for (n = 0; n < FILTER_NAME_TABLE_LENGTH; n++)
    {
        /* set to the stem */
        sprintf(stemBuf, "%s%s%i.algorithm", infoStem, infoStemSep, n + 1);
        if (! SetRexxVariable(stemBuf, FILTER_NAME_TABLE[n].name))
        {
          Gbm_deinit();
          BUILDRXSTRING(retstr, ERROR_REXX_SET);
          return VALID_ROUTINE;
        }
    }

    /* set the number of types into stem.0 */
    sprintf(varBuf, "%i", FILTER_NAME_TABLE_LENGTH);
    sprintf(stemBuf, "%s%s0", infoStem, infoStemSep);
    if (! SetRexxVariable(stemBuf, varBuf))
    {
      Gbm_deinit();
      BUILDRXSTRING(retstr, ERROR_REXX_SET);
      return VALID_ROUTINE;
    }
  }

  Gbm_deinit();

  BUILDRXSTRING(retstr, NOERROR_RET);
  return VALID_ROUTINE;
}

/*******************************************************************************************/
/* Function:  GBM_ScaleIsSupported                                                         */
/*                                                                                         */
/* Syntax:    sup = GBM_ScaleIsSupported(algorithm, bpp, stem)                             */
/*                                                                                         */
/* Params:    string  in - scaling algorithm                                               */
/*                         (simple,nearestneighbor,bilinear,bell,bspline,mitchell,lanczos) */
/*                                                                                         */
/*            string  in - bpp (bits per pixel) to test                                    */
/*                                                                                         */
/*            stem    in - palette information consisting of                               */
/*                         stem2.0      : number of palette entries                        */
/*                         stem2.X.red  : red value                                        */
/*                         stem2.X.green: green value                                      */
/*                         stem2.X.blue : blue value                                       */
/*                                                                                         */
/*                         X: Ranging from 1 to stem.0                                     */
/*                                                                                         */
/*                         Note: The number of entries must match                          */
/*                               bpp^2. For true color images it                           */
/*                               can be 0.                                                 */
/*                                                                                         */
/* Return:   "0" (unsupported) or "1" (supported)                                          */
/*******************************************************************************************/

ULONG GBM_ScaleIsSupported(PUCHAR name, ULONG numargs, RXSTRING args[],
                           PSZ queuename, RXSTRING *retstr)
{
  char    * algorithm      = NULL;
  char    * paletteStem    = NULL;
  int       bpp            = -1;
  GBMRGB    gbmrgb[0x100]  = { { 0, 0, 0 } };
  ULONG     retcode        = VALID_ROUTINE;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  BUILDRXSTRING(retstr, "0");

  if (numargs != 3)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  algorithm      = args[0].strptr;
  paletteStem    = args[2].strptr;

  if (sscanf(args[1].strptr, "%i", &bpp) != 1)
  {
    return INVALID_ROUTINE;
  }
  if ((bpp !=  1) && (bpp !=  4) && (bpp !=  8) &&
      (bpp != 24) && (bpp != 32) && (bpp != 48) && (bpp != 64))
  {
    return VALID_ROUTINE;
  }

  /* get the REXX palette variables */
  if (! extractRexxPalette(paletteStem, gbmrgb, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }

  /* check if the algorithm is supported for the color depth */
  {
      int     filterIndex = FILTER_INDEX_SIMPLE;
      gbm_boolean qualityScalingEnabled = GBM_FALSE;

      int n;
      for (n = 0; n < FILTER_NAME_TABLE_LENGTH; n++)
      {
          if (strcmp(FILTER_NAME_TABLE[n].name, algorithm) == 0)
          {
              filterIndex = n;
              break;
          }
      }
      if (filterIndex == -1)
      {
         return VALID_ROUTINE;
      }
      if (filterIndex != FILTER_INDEX_SIMPLE)
      {
         qualityScalingEnabled = GBM_TRUE;
      }
      if (qualityScalingEnabled)
      {
          if ((bpp <= 8) && (! isGrayscalePalette(gbmrgb, 1 << bpp)))
          {
              return VALID_ROUTINE;
          }
      }
  }

  BUILDRXSTRING(retstr, "1");

  return VALID_ROUTINE;  /* no error on call */
}

/*******************************************************************************************/
/* Function:  GBM_Scale                                                                    */
/*                                                                                         */
/* Syntax:    rc = GBM_Scale(algorithm, stem1, stem2, stem3, stem4)                        */
/*                                                                                         */
/* Params:    string  in - scaling algorithm                                               */
/*                         (simple,nearestneighbor,bilinear,bell,bspline,mitchell,lanczos) */
/*                                                                                         */
/*          stem1 in/out - header information consisting of                                */
/*                         stem1.width : bitmap width                                      */
/*                         stem1.height: bitmap height                                     */
/*                         stem1.bpp   : bitmap colour depth (bpp)                         */
/*                                                                                         */
/*          stem2 in/out - palette information consisting of                               */
/*                         stem2.0      : number of palette entries                        */
/*                         stem2.X.red  : red value                                        */
/*                         stem2.X.green: green value                                      */
/*                         stem2.X.blue : blue value                                       */
/*                                                                                         */
/*                         X: Ranging from 1 to stem2.0                                    */
/*                                                                                         */
/*                         Note: The number of entries must match                          */
/*                               stem1.bpp^2.                                              */
/*                                                                                         */
/*          stem3 in/out - The binary bitmap data.                                         */
/*                                                                                         */
/*            stem4   in - destination size                                                */
/*                         stem.width : bitmap width                                       */
/*                         stem.height: bitmap height                                      */
/*                                                                                         */
/* Return:    "0", an error message or "ERROR" for an unknown error                        */
/*******************************************************************************************/

ULONG GBM_Scale(PUCHAR name, ULONG numargs, RXSTRING args[],
                PSZ queuename, RXSTRING *retstr)
{
  char * algorithm      = NULL;

  char * headerStem     = NULL;
  char * paletteStem    = NULL;
  char * dataStem       = NULL;
  char * sizeStem       = NULL;

  GBM    gbm            = { 0 };
  GBM    gbm2           = { 0 };
  GBMRGB gbmrgb[0x100]  = { { 0, 0, 0 } };
  ULONG  retcode;

  gbm_u8 * data    = NULL;
  size_t   dataLen = 0;
  int    w = 0, h = 0;

  GBM_ERR rc = GBM_ERR_NOT_SUPP;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retcode = VALID_ROUTINE;
  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if (numargs != 5)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  algorithm      = args[0].strptr;
  headerStem     = args[1].strptr;
  paletteStem    = args[2].strptr;
  dataStem       = args[3].strptr;
  sizeStem       = args[4].strptr;

  /* get the REXX destination size variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxSize(sizeStem, &w, &h, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }
  if ((w <= 0) || (h <= 0) || (w > 10000) || (h > 10000))
  {
    BUILDRXSTRING(retstr, ERROR_OUT_OF_RANGE);
    return VALID_ROUTINE;
  }

  /* get the REXX header variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxHeader(headerStem, &gbm, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }

  if ((gbm.bpp !=  1) && (gbm.bpp !=  4) && (gbm.bpp !=  8) &&
      (gbm.bpp != 24) && (gbm.bpp != 32) && (gbm.bpp != 48) && (gbm.bpp != 64))
  {
    BUILDRXSTRING(retstr, ERROR_BPP_UNSUPP);
    return VALID_ROUTINE;
  }

  /* get the REXX palette variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxPalette(paletteStem, gbmrgb, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }

  /* get the REXX bitmap data */
  data = extractRexxBitmapData(dataStem, &gbm, &dataLen);
  if (data == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    return VALID_ROUTINE;
  }

  /* check if the algorithm is supported for the color depth */
  {
      int     palette_entries = 0;
      int     filterIndex     = FILTER_INDEX_SIMPLE;
      gbm_boolean qualityScalingEnabled = GBM_FALSE;
      gbm_boolean isGrayscale = GBM_FALSE;
      gbm_u8  * dataDst     = NULL;
      size_t strideDst  = 0;
      size_t dataDstLen = 0;

      int n;
      for (n = 0; n < FILTER_NAME_TABLE_LENGTH; n++)
      {
          if (strcmp(FILTER_NAME_TABLE[n].name, algorithm) == 0)
          {
              filterIndex = n;
              break;
          }
      }
      if (filterIndex == -1)
      {
         BUILDRXSTRING(retstr, ERROR_ALGORITHM);
         gbmmem_free(data);
         return VALID_ROUTINE;
      }

      gbm2   = gbm;
      gbm2.w = w;
      gbm2.h = h;

      if (filterIndex != FILTER_INDEX_SIMPLE)
      {
         qualityScalingEnabled = GBM_TRUE;

         if (gbm.bpp <= 8)
         {
             isGrayscale = isGrayscalePalette(gbmrgb, 1 << gbm.bpp);
         }
         if ((gbm.bpp <= 8) && (! isGrayscale))
         {
             BUILDRXSTRING(retstr, ERROR_BPP_UNSUPP);
             gbmmem_free(data);
             return VALID_ROUTINE;
         }
         if (isGrayscale)
         {
             gbm2.bpp = 8;
         }
      }

      strideDst  = ( ((gbm2.w * gbm2.bpp + 31)/32) * 4 );
      dataDstLen = strideDst * gbm2.h;

      dataDst = gbmmem_malloc(dataDstLen);
      if (dataDst == NULL)
      {
         BUILDRXSTRING(retstr, ERROR_MEM);
         gbmmem_free(data);
         return VALID_ROUTINE;
      }

      if (qualityScalingEnabled)
      {
          if (isGrayscale)
          {
              rc = gbm_quality_scale_gray(data   , gbm.w , gbm.h , gbm.bpp, gbmrgb,
                                          dataDst, gbm2.w, gbm2.h, gbmrgb,
                                          FILTER_NAME_TABLE[filterIndex].filter);
          }
          else
          {
              rc = gbm_quality_scale_bgra(data   , gbm.w , gbm.h,
                                          dataDst, gbm2.w, gbm2.h, gbm.bpp,
                                          FILTER_NAME_TABLE[filterIndex].filter);
          }
      }
      else
      {
          rc = gbm_simple_scale(data, gbm.w, gbm.h, dataDst, gbm2.w, gbm2.h, gbm.bpp);
      }

      gbmmem_free(data); data = NULL;

      if (rc != GBM_ERR_OK)
      {
        BUILDRXSTRING(retstr, formatGbmErrorMessage(rc));
        gbmmem_free(dataDst);
        return VALID_ROUTINE;
      }

      /* set the values into the stem */
      retcode = VALID_ROUTINE;
      if (! setRexxHeader(&gbm2, headerStem, &retcode, retstr))
      {
        gbmmem_free(dataDst);
        /* error string and code provided by called function */
        return retcode;
      }

      if (gbm2.bpp <= 8)
      {
        palette_entries = 1 << gbm2.bpp;
      }
      /* set the values into the stems */
      if (! setRexxPalette(gbmrgb, palette_entries, paletteStem, &retcode, retstr))
      {
        gbmmem_free(dataDst);
        /* error string and code provided by called function */
        return retcode;
      }

      /* set the scaled data into the stem */
      if (! SetRexxBinaryVariable(dataStem, dataDst, dataDstLen))
      {
        BUILDRXSTRING(retstr, ERROR_REXX_SET);
        gbmmem_free(dataDst);
        return VALID_ROUTINE;
      }

      gbmmem_free(dataDst); dataDst = NULL;
  }

  /* build return string */
  BUILDRXSTRING(retstr, NOERROR_RET);

  return VALID_ROUTINE;
}


/*******************************************************************************************/
/* Function:  GBM_Reflect                                                                  */
/*                                                                                         */
/* Syntax:    rc = GBM_Reflect(algorithm, stem1, stem2)                                    */
/*                                                                                         */
/* Params:    string  in - algorithm                                                       */
/*                         (horizontal, vertical, transpose)                               */
/*                                                                                         */
/*          stem1 in/out - header information consisting of                                */
/*                         stem1.width : bitmap width                                      */
/*                         stem1.height: bitmap height                                     */
/*                         stem1.bpp   : bitmap colour depth (bpp)                         */
/*                                                                                         */
/*          stem2 in/out - The binary bitmap data.                                         */
/*                                                                                         */
/* Return:    "0", an error message or "ERROR" for an unknown error                        */
/*******************************************************************************************/

ULONG GBM_Reflect(PUCHAR name, ULONG numargs, RXSTRING args[],
                  PSZ queuename, RXSTRING *retstr)
{
  char * algorithm  = NULL;
  char * headerStem = NULL;
  char * dataStem   = NULL;

  gbm_u8 * data    = NULL;
  size_t   dataLen = 0;

  GBM    gbm       = { 0 };
  ULONG  retcode;

  gbm_boolean isHorizontal = GBM_FALSE;
  gbm_boolean isVertical   = GBM_FALSE;
  gbm_boolean isTranspose  = GBM_FALSE;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retcode = VALID_ROUTINE;
  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if (numargs != 3)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  algorithm   = args[0].strptr;
  headerStem  = args[1].strptr;
  dataStem    = args[2].strptr;

  /* check supported algorithms */
  isHorizontal = (strncmp(algorithm, "horizontal", 10) == 0) ? GBM_TRUE : GBM_FALSE;
  isVertical   = (strncmp(algorithm, "vertical"  ,  8) == 0) ? GBM_TRUE : GBM_FALSE;
  isTranspose  = (strncmp(algorithm, "transpose" ,  9) == 0) ? GBM_TRUE : GBM_FALSE;

  if ((! isHorizontal) && (! isVertical) && (! isTranspose))
  {
    BUILDRXSTRING(retstr, ERROR_ALGORITHM);
    return VALID_ROUTINE;
  }

  /* get the REXX header variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxHeader(headerStem, &gbm, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }

  /* get the REXX bitmap data */
  data = extractRexxBitmapData(dataStem, &gbm, &dataLen);
  if (data == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    return VALID_ROUTINE;
  }

  /* flip the image data */
  if (isHorizontal)
  {
    if (! gbm_ref_horz(&gbm, data))
    {
      BUILDRXSTRING(retstr, ERROR_MEM);
      gbmmem_free(data);
      return VALID_ROUTINE;
    }
  }
  else if (isVertical)
  {
    if (! gbm_ref_vert(&gbm, data))
    {
      BUILDRXSTRING(retstr, ERROR_MEM);
      gbmmem_free(data);
      return VALID_ROUTINE;
    }
  }
  else if (isTranspose)
  {
    const size_t stride_dst  = ((gbm.h * gbm.bpp + 31) / 32) * 4;
    const size_t dataLen_dst = stride_dst * gbm.w;
          gbm_u8 * data_dst  = NULL;
          int    t;

    data_dst = gbmmem_malloc(dataLen_dst);
    if (data_dst == NULL)
    {
      BUILDRXSTRING(retstr, ERROR_MEM);
      return VALID_ROUTINE;
    }

    gbm_transpose(&gbm, data, data_dst);
    t = gbm.w; gbm.w = gbm.h; gbm.h = t;

    gbmmem_free(data);
    data    = data_dst;
    dataLen = dataLen_dst;
  }
  else
  {
    BUILDRXSTRING(retstr, ERROR_ALGORITHM);
    gbmmem_free(data);
    return VALID_ROUTINE;
  }

  /* set the scaled data into the stem */
  retcode = VALID_ROUTINE;
  if (! setRexxHeader(&gbm, headerStem, &retcode, retstr))
  {
    gbmmem_free(data);
    /* error string and code provided by called function */
    return retcode;
  }

  if (! SetRexxBinaryVariable(dataStem, data, dataLen))
  {
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    gbmmem_free(data);
    return VALID_ROUTINE;
  }

  gbmmem_free(data); data = NULL;

  /* build return string */
  BUILDRXSTRING(retstr, NOERROR_RET);

  return VALID_ROUTINE;
}


/*******************************************************************************************/
/* Function:  GBM_Rotate                                                                   */
/*                                                                                         */
/* Syntax:    rc = GBM_Rotate(angle, stem1, stem2)                                         */
/*                                                                                         */
/* Params:    string  in - angle                                                           */
/*                         (must be multiple of 90)                                        */
/*                                                                                         */
/*          stem1 in/out - header information consisting of                                */
/*                         stem1.width : bitmap width                                      */
/*                         stem1.height: bitmap height                                     */
/*                         stem1.bpp   : bitmap colour depth (bpp)                         */
/*                                                                                         */
/*          stem2 in/out - The binary bitmap data.                                         */
/*                                                                                         */
/* Return:    "0", an error message or "ERROR" for an unknown error                        */
/*******************************************************************************************/

ULONG GBM_Rotate(PUCHAR name, ULONG numargs, RXSTRING args[],
                 PSZ queuename, RXSTRING *retstr)
{
  char * angleString = NULL;
  char * headerStem  = NULL;
  char * dataStem    = NULL;

  int    angle       = 0;

  gbm_u8 * data    = NULL;
  size_t   dataLen = 0;

  GBM    gbm        = { 0 };
  ULONG  retcode;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retcode = VALID_ROUTINE;
  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if (numargs != 3)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  angleString = args[0].strptr;
  headerStem  = args[1].strptr;
  dataStem    = args[2].strptr;

  /* check supported angle */
  if (sscanf(angleString, "%i", &angle) != 1)
  {
    return INVALID_ROUTINE;
  }
  if ((angle == 0) || (angle == 360))
  {
    BUILDRXSTRING(retstr, NOERROR_RET);
    return VALID_ROUTINE;
  }

  while (angle < 0)
  {
    angle += 360;
  }
  while (angle > 360)
  {
    angle -= 360;
  }

  if ((angle != 90) && (angle != 180) && (angle != 270))
  {
    BUILDRXSTRING(retstr, ERROR_ANGLE);
    return VALID_ROUTINE;
  }

  /* get the REXX header variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxHeader(headerStem, &gbm, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }

  /* get the REXX bitmap data */
  data = extractRexxBitmapData(dataStem, &gbm, &dataLen);
  if (data == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    return VALID_ROUTINE;
  }

  /* rotate the image */
  switch (angle)
  {
    case 90:
    {
      const size_t stride_dst  = ((gbm.h * gbm.bpp + 31) / 32) * 4;
      const size_t dataLen_dst = stride_dst * gbm.w;
            gbm_u8 * data_dst  = NULL;
            int    tmp;

      data_dst = gbmmem_malloc(dataLen_dst);
      if (data_dst == NULL)
      {
        BUILDRXSTRING(retstr, ERROR_MEM);
        return VALID_ROUTINE;
      }
      else
      {
        gbm_transpose(&gbm, data, data_dst);
        tmp   = gbm.w;
        gbm.w = gbm.h;
        gbm.h = tmp;

        gbm_ref_vert(&gbm, data_dst);

        gbmmem_free(data);
        data    = data_dst;
        dataLen = dataLen_dst;
      }
      break;
    }

    case 180:
      gbm_ref_vert(&gbm, data);
      gbm_ref_horz(&gbm, data);
      break;

    case 270:
    {
      const size_t stride_dst  = ((gbm.h * gbm.bpp + 31) / 32) * 4;
      const size_t dataLen_dst = stride_dst * gbm.w;
            gbm_u8 * data_dst  = NULL;
            int    tmp;

      data_dst = gbmmem_malloc(dataLen_dst);
      if (data_dst == NULL)
      {
        BUILDRXSTRING(retstr, ERROR_MEM);
        return VALID_ROUTINE;
      }
      else
      {
        gbm_transpose(&gbm, data, data_dst);
        tmp   = gbm.w;
        gbm.w = gbm.h;
        gbm.h = tmp;

        gbm_ref_horz(&gbm, data_dst);

        gbmmem_free(data);
        data    = data_dst;
        dataLen = dataLen_dst;
      }
      break;
    }

    default:
      return INVALID_ROUTINE;
  }

  /* set the scaled data into the stem */
  retcode = VALID_ROUTINE;
  if (! setRexxHeader(&gbm, headerStem, &retcode, retstr))
  {
    gbmmem_free(data);
    /* error string and code provided by called function */
    return retcode;
  }

  if (! SetRexxBinaryVariable(dataStem, data, dataLen))
  {
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    gbmmem_free(data);
    return VALID_ROUTINE;
  }

  gbmmem_free(data); data = NULL;

  /* build return string */
  BUILDRXSTRING(retstr, NOERROR_RET);

  return VALID_ROUTINE;
}


/*******************************************************************************************/
/* Function:  GBM_PaletteDataTo24bpp                                                       */
/*                                                                                         */
/* Syntax:    rc = GBM_PaletteDataTo24bpp(stem1, stem2, stem3)                             */
/*                                                                                         */
/* Params:  stem1 in/out - header information consisting of                                */
/*                         stem1.width : bitmap width                                      */
/*                         stem1.height: bitmap height                                     */
/*                         stem1.bpp   : bitmap colour depth (bpp)                         */
/*                                                                                         */
/*              stem2 in - palette information consisting of                               */
/*                         stem2.0      : number of palette entries                        */
/*                         stem2.X.red  : red value                                        */
/*                         stem2.X.green: green value                                      */
/*                         stem2.X.blue : blue value                                       */
/*                                                                                         */
/*                         X: Ranging from 1 to stem2.0                                    */
/*                                                                                         */
/*                         Note: The number of entries must match stem1.bpp^2.             */
/*                                                                                         */
/*          stem3 in/out - The binary bitmap data.                                         */
/*                                                                                         */
/* Return:    "0", an error message or "ERROR" for an unknown error                        */
/*******************************************************************************************/

ULONG GBM_PaletteDataTo24bpp(PUCHAR name, ULONG numargs, RXSTRING args[],
                             PSZ queuename, RXSTRING *retstr)
{
  char * headerStem     = NULL;
  char * paletteStem    = NULL;
  char * dataStem       = NULL;

  gbm_u8 * data     = NULL;
  size_t   dataLen  = 0;

  GBM    gbm            = { 0 };
  GBMRGB gbmrgb[0x100]  = { { 0, 0, 0 } };
  ULONG  retcode;

  /* prevent compiler warnings */
  args      = args;
  name      = name;
  queuename = queuename;

  retcode = VALID_ROUTINE;
  BUILDRXSTRING(retstr, ERROR_RETSTR);

  if (numargs != 3)
  {
    return INVALID_ROUTINE;
  }

  /* extract arguments */
  headerStem     = args[0].strptr;
  paletteStem    = args[1].strptr;
  dataStem       = args[2].strptr;

  /* get the REXX header variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxHeader(headerStem, &gbm, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }
  if ((gbm.bpp != 1) && (gbm.bpp != 4) && (gbm.bpp != 8) && (gbm.bpp != 24))
  {
    BUILDRXSTRING(retstr, ERROR_BPP_UNSUPP);
    return VALID_ROUTINE;
  }

  /* get the REXX palette variables */
  retcode = VALID_ROUTINE;
  if (! extractRexxPalette(paletteStem, gbmrgb, &retcode, retstr))
  {
      /* error string and code provided by called function */
      return retcode;
  }

  /* get the REXX bitmap data */
  data = extractRexxBitmapData(dataStem, &gbm, &dataLen);
  if (data == NULL)
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    return VALID_ROUTINE;
  }

  /* convert to 24bpp */
  if (! expandTo24Bit(&gbm, gbmrgb, &data, &dataLen))
  {
    BUILDRXSTRING(retstr, ERROR_MEM);
    gbmmem_free(data);
    return VALID_ROUTINE;
  }

  /* set the scaled data into the stem */
  retcode = VALID_ROUTINE;
  if (! setRexxHeader(&gbm, headerStem, &retcode, retstr))
  {
    gbmmem_free(data);
    /* error string and code provided by called function */
    return retcode;
  }

  if (! SetRexxBinaryVariable(dataStem, data, dataLen))
  {
    BUILDRXSTRING(retstr, ERROR_REXX_SET);
    gbmmem_free(data);
    return VALID_ROUTINE;
  }

  gbmmem_free(data); data = NULL;

  /* build return string */
  BUILDRXSTRING(retstr, NOERROR_RET);

  return VALID_ROUTINE;
}


