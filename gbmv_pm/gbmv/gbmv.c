/*

gbmv.c - General Bitmap Viewer

History:
--------
(Heiko Nitzsche)

26-Apr-2006: Fix issue with comma separation between file and options.
             Now the file can have quotes and thus clearly separating
             it from the options.
             On OS/2 command line use: "\"file.ext\",options"

27-May-2006: Fix display of 1bpp color bitmap for which no RGB color
             table was created and thus only the closest pure color
             was used. This caused wrong colors.

07-Sep-2007  Add support for resampled scaling (option -f)
             Restrict window size to visible area and downsize image
             otherwise.

21-Jan-2008  Add support for 1bpp and 4bpp grayscale resampled scaling.

08-Feb-2008  Allocate memory from high memory for bitmap data to
             stretch limit for out-of-memory errors
             (requires kernel with high memory support)

15-Aug-2008  Integrate new GBM types

10-Oct-2008: Changed recommended file specification template to
             "file.ext"\",options   or   "file.ext"\",\""options"
*/

/* includes */
#define    INCL_DOS
#define    INCL_WIN
#define    INCL_GPI
#define    INCL_DEV
#define    INCL_BITMAPFILEFORMAT
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gbm.h"
#include "gbmmem.h"
#include "gbmerr.h"
#include "gbmht.h"
#include "gbmtool.h"
#include "gbmscale.h"
#include "gbmv.h"

/* --------------------------------------- */

typedef struct
{
  char * name;   /* filter name */
  int    filter; /* filter type */
} FILTER_NAME_TABLE_DEF;

static const int FILTER_INDEX_SIMPLE = 0;

static FILTER_NAME_TABLE_DEF FILTER_NAME_TABLE [] =
{ { "simple"         , -1 },
  { "nearestneighbor", GBM_SCALE_FILTER_NEARESTNEIGHBOR },
  { "bilinear"       , GBM_SCALE_FILTER_BILINEAR },
  { "bell"           , GBM_SCALE_FILTER_BELL     },
  { "bspline"        , GBM_SCALE_FILTER_BSPLINE  },
  { "mitchell"       , GBM_SCALE_FILTER_MITCHELL },
  { "lanczos"        , GBM_SCALE_FILTER_LANCZOS  }
};
const int FILTER_NAME_TABLE_LENGTH = sizeof(FILTER_NAME_TABLE) /
                                     sizeof(FILTER_NAME_TABLE[0]);

/* --------------------------------------- */

static int get_opt_value_filterIndex(const char *s,
                                     const FILTER_NAME_TABLE_DEF *table,
                                     const int tableLength)
{
   int n;
   for (n = 0; n < tableLength; n++)
   {
       if (strcmp(table[n].name, s) == 0)
       {
           return n;
       }
   }
   return -1;
}

/* --------------------------------------- */

static VOID Error(HWND hwnd, const CHAR *szFmt, ...)
{
  va_list vars;
  CHAR sz [256+1];

  va_start(vars, szFmt);
  vsprintf(sz, szFmt, vars);
  va_end(vars);
  WinMessageBox(HWND_DESKTOP, hwnd, sz, NULL, 0, MB_OK | MB_ERROR | MB_MOVEABLE);
}

static VOID Fatal(HWND hwnd, const CHAR *sz)
{
  Error(hwnd, sz);
  exit(1);
}

static VOID Usage(HWND hwnd)
{
  Fatal(hwnd, "Usage:\ngbmv [-e] [-h] [-f filter] [--] \"filename.ext\"{\\\",\\\"\"opt\"}");
}

static VOID Warning(HWND hwnd, const CHAR *szFmt, ...)
{
  va_list vars;
  CHAR sz [256+1];

  va_start(vars, szFmt);
  vsprintf(sz, szFmt, vars);
  va_end(vars);
  WinMessageBox(HWND_DESKTOP, hwnd, sz, NULL, 0, MB_OK | MB_WARNING | MB_MOVEABLE);
}

/* --------------------------------------- */

#define WC_GBMV "GbmViewerClass"

static HWND    hwndFrame, hwndClient;
static HBITMAP hbmBmp;
static LONG    lColorBg, lColorFg;

/* --------------------------------------- */

static BOOL LoadBitmap(
  HWND hwnd,
  const CHAR *szFn, const CHAR *szOpt,
  GBM *gbm, GBMRGB *gbmrgb, BYTE **ppbData )
{
  GBM_ERR rc;
  int fd, ft;
  ULONG cb;
  USHORT usStride;

  if ( gbm_guess_filetype(szFn, &ft) != GBM_ERR_OK )
  {
    Warning(hwnd, "Can't deduce bitmap format from file extension: %s", szFn);
    return ( FALSE );
  }

  if ( (fd = gbm_io_open(szFn, GBM_O_RDONLY)) == -1 )
  {
    Warning(hwnd, "Can't open file: %s", szFn);
    return ( FALSE );
  }

  if ( (rc = gbm_read_header(szFn, fd, ft, gbm, szOpt)) != GBM_ERR_OK )
  {
    gbm_io_close(fd);
    Warning(hwnd, "Can't read file header of %s: %s", szFn, gbm_err(rc));
    return ( FALSE );
  }

  /* check for color depth supported */
  switch ( gbm->bpp )
  {
    case 24:
    case 8:
    case 4:
    case 1:
      break;
    default:
    {
      gbm_io_close(fd);
      Warning(hwnd, "Color depth %d is not supported", gbm->bpp);
      return ( FALSE );
    }
  }

  if ( (rc = gbm_read_palette(fd, ft, gbm, gbmrgb)) != GBM_ERR_OK )
  {
    gbm_io_close(fd);
    Warning(hwnd, "Can't read file palette of %s: %s", szFn, gbm_err(rc));
    return ( FALSE );
  }

  usStride = ((gbm -> w * gbm -> bpp + 31)/32) * 4;
  cb = gbm -> h * usStride;
  if ( (*ppbData = gbmmem_malloc(cb)) == NULL )
  {
    gbm_io_close(fd);
    Warning(hwnd, "Out of memory requesting %ld bytes", cb);
    return ( FALSE );
  }

  if ( (rc = gbm_read_data(fd, ft, gbm, *ppbData)) != GBM_ERR_OK )
  {
    gbmmem_free(*ppbData);
    gbm_io_close(fd);
    Warning(hwnd, "Can't read file data of %s: %s", szFn, gbm_err(rc));
    return ( FALSE );
  }

  gbm_io_close(fd);
  return ( TRUE );
}

/* --------------------------------------- */

static BOOL To24Bit(GBM *gbm, GBMRGB *gbmrgb, BYTE **ppbData)
{
  unsigned long stride     = (((unsigned long)gbm -> w * gbm -> bpp + 31)/32) * 4;
  unsigned long new_stride = (((unsigned long)gbm -> w * 3 + 3) & ~3);
  unsigned long bytes;
  int y;
  gbm_u8 *pbDataNew;

  if ( gbm -> bpp == 24 )
  {
    return ( TRUE );
  }

  bytes = new_stride * gbm -> h;
  if ( (pbDataNew = gbmmem_malloc(bytes)) == NULL )
  {
    return ( FALSE );
  }

  for ( y = 0; y < gbm -> h; y++ )
  {
    gbm_u8 *src = *ppbData + y * stride;
    gbm_u8 *dest = pbDataNew + y * new_stride;
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
  gbm->bpp = 24;

  return ( TRUE );
}

/* --------------------------------------- */

static BOOL isGrayscalePalette(const GBMRGB *gbmrgb, const int entries)
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
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* --------------------------------------- */

static BOOL resizeToScreenSize(const BOOL qualityScalingEnabled,
                               const BOOL isGray,
                               const int  filterIndex,
                               GBM *gbm, GBMRGB *gbmrgb, BYTE **ppbData)
{
  /* get max client area size */
  const LONG maxClientWidth  = WinQuerySysValue(HWND_DESKTOP, SV_CXFULLSCREEN);
  const LONG maxClientHeight = WinQuerySysValue(HWND_DESKTOP, SV_CYFULLSCREEN);

  if ((gbm->w > maxClientWidth) || (gbm->h > maxClientHeight))
  {
    GBM_ERR rc;
    gbm_u8 *pbDataNew = NULL;
    int newWidth    = gbm->w;
    int newHeight   = gbm->h;
    int newBpp      = gbm->bpp;
    unsigned long newStride = 0;

    if (((float)maxClientWidth / gbm->w) <= ((float)maxClientHeight / gbm->h))
    {
      newHeight = ( gbm->h * maxClientWidth ) / gbm->w;
      newWidth  = ( gbm->w * newHeight ) / gbm->h;
    }
    else
    {
      newWidth  = ( gbm->w * maxClientHeight ) / gbm->h;
      newHeight = ( gbm->h * newWidth ) / gbm->w;
    }

    if ((qualityScalingEnabled) &&
        (filterIndex >= 0)      &&
        (filterIndex < FILTER_NAME_TABLE_LENGTH) &&
        isGray)
    {
      newBpp = 8;
    }

    newStride = (((unsigned long)newWidth * newBpp + 31)/32) * 4;
    if ( (pbDataNew = gbmmem_malloc(newStride * newHeight)) == NULL )
    {
      return FALSE;
    }

    /* select the algorithm */

    if ((qualityScalingEnabled) &&
        (filterIndex >= 0)      &&
        (filterIndex < FILTER_NAME_TABLE_LENGTH))
    {
      if (isGray)
      {
        rc = gbm_quality_scale_gray(*ppbData  , gbm->w, gbm->h, gbm->bpp, gbmrgb,
                                     pbDataNew, newWidth, newHeight, gbmrgb,
                                     FILTER_NAME_TABLE[filterIndex].filter);
      }
      else
      {
        rc = gbm_quality_scale_bgra(*ppbData  , gbm->w, gbm->h,
                                     pbDataNew, newWidth, newHeight,
                                     gbm->bpp,
                                     FILTER_NAME_TABLE[filterIndex].filter);
      }
    }
    else
    {
      rc = gbm_simple_scale(*ppbData  , gbm->w, gbm->h,
                             pbDataNew, newWidth, newHeight,
                             gbm->bpp);
    }
    if (rc != GBM_ERR_OK)
    {
      gbmmem_free(pbDataNew);
      return FALSE;
    }

    gbmmem_free(*ppbData);
    *ppbData = pbDataNew;
    gbm->w   = newWidth;
    gbm->h   = newHeight;
    gbm->bpp = newBpp;
  }
  return TRUE;
}

/* --------------------------------------- */

static BOOL MakeBitmap(
  HWND hwnd,
  const GBM *gbm, const GBMRGB *gbmrgb, const BYTE *pbData,
  HBITMAP *phbm )
{
  HAB hab = WinQueryAnchorBlock(hwnd);
  USHORT cRGB, usCol;
  SIZEL sizl;
  HDC hdc;
  HPS hps;

  #pragma pack(2)
  struct
  {
    BITMAPINFOHEADER2 bmp2;
    RGB2 argb2Color [0x100];
  } bm;
  #pragma pack()

  /* Got the data in memory, now make bitmap */

  memset(&bm, 0, sizeof(bm));

  bm.bmp2.cbFix     = sizeof(BITMAPINFOHEADER2);
  bm.bmp2.cx        = gbm -> w;
  bm.bmp2.cy        = gbm -> h;
  bm.bmp2.cBitCount = gbm -> bpp;
  bm.bmp2.cPlanes   = 1;

  cRGB = ( (1 << gbm -> bpp) & 0x1ff );
      /* 1 -> 2, 4 -> 16, 8 -> 256, 24 -> 0 */

  for ( usCol = 0; usCol < cRGB; usCol++ )
  {
    bm.argb2Color [usCol].bRed   = gbmrgb [usCol].r;
    bm.argb2Color [usCol].bGreen = gbmrgb [usCol].g;
    bm.argb2Color [usCol].bBlue  = gbmrgb [usCol].b;
  }

  if ( (hdc = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL)) == (HDC) NULL )
  {
    Warning(hwnd, "DevOpenDC failure");
    return ( FALSE );
  }

  sizl.cx = bm.bmp2.cx;
  sizl.cy = bm.bmp2.cy;
  if ( (hps = GpiCreatePS(hab, hdc, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
  {
    Warning(hwnd, "GpiCreatePS failure");
    DevCloseDC(hdc);
    return ( FALSE );
  }

  if ( cRGB == 2 )
  {
    /* handle 1bpp case */
    /*
      1bpp presentation spaces have a reset or background colour.
      They also have a contrast or foreground colour.
      When data is mapped into a 1bpp presentation space :-
      Data which is the reset colour, remains reset, and is stored as 0's.
      All other data becomes contrast, and is stored as 1's.
      The reset colour for 1bpp screen HPSs is white.
      I want 1's in the source data to become 1's in the HPS.
      We seem to have to reverse the ordering here to get the desired effect.
    */
    static RGB2 argb2Black = { 0x00, 0x00, 0x00 };
    static RGB2 argb2White = { 0xff, 0xff, 0xff };
    bm.argb2Color [0] = argb2Black; /* Contrast */
    bm.argb2Color [1] = argb2White; /* Reset */
  }

  if ( (*phbm = GpiCreateBitmap(hps, &(bm.bmp2), CBM_INIT, (BYTE *) pbData, (BITMAPINFO2 *) &(bm.bmp2))) == (HBITMAP) NULL )
  {
    Warning(hwnd, "GpiCreateBitmap failure");
    GpiDestroyPS(hps);
    DevCloseDC(hdc);
    return ( FALSE );
  }

  GpiSetBitmap(hps, (HBITMAP) NULL);
  GpiDestroyPS(hps);
  DevCloseDC(hdc);

  return ( TRUE );
}

/* --------------------------------------- */

MRESULT _System GbmVWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch ( (int) msg )
  {
    /* WM_COMMAND menu command */
    case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      {
        /* MID_EXIT initiate shutdown of this app */
        case MID_EXIT:
          WinPostMsg(hwnd, WM_QUIT, 0L, 0L);
          break;
      }
      return ( (MRESULT) 0 );

    case WM_PAINT:
    {
      static SIZEL sizl = { 0, 0 };
      HPS hps = WinBeginPaint(hwnd, (HPS) NULL, NULL);
      HAB hab = WinQueryAnchorBlock(hwnd);
      HDC hdcBmp = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL);
      HPS hpsBmp = GpiCreatePS(hab, hdcBmp, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
      POINTL aptl [4];
      RECTL rcl;
      BITMAPINFOHEADER bmp;
      LONG colorTable[2] = { 0 };

      GpiQueryBitmapParameters(hbmBmp, &bmp);

      GpiSetBitmap(hpsBmp, hbmBmp);

      colorTable[0] = lColorBg;
      colorTable[1] = lColorFg;
      GpiCreateLogColorTable(hps, 0, LCOLF_CONSECRGB, 0, 2, colorTable);
      /* if something goes wrong when creating color table, use the closest color */
      GpiSetBackColor(hps, GpiQueryColorIndex(hps, 0, lColorBg));
      GpiSetColor    (hps, GpiQueryColorIndex(hps, 0, lColorFg));

      WinQueryWindowRect(hwnd, &rcl);
      aptl [0].x = rcl.xLeft;
      aptl [0].y = rcl.yBottom;
      aptl [1].x = rcl.xRight;
      aptl [1].y = rcl.yTop;
      aptl [2].x = 0;
      aptl [2].y = 0;
      aptl [3].x = bmp.cx;
      aptl [3].y = bmp.cy;
      GpiBitBlt(hps, hpsBmp, 4L, aptl, ROP_SRCCOPY, BBO_IGNORE);

      GpiSetBitmap(hpsBmp, (HBITMAP) NULL);
      GpiDestroyPS(hpsBmp);
      DevCloseDC(hdcBmp);

      WinEndPaint(hps);
    }
    return ( (MRESULT) 0 );
  }
  return ( WinDefWindowProc(hwnd, msg, mp1, mp2) );
}

/* --------------------------------------- */

int main(int argc, CHAR *argv [])
{
  HAB hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(hab, 0);/* 0 => default size for message Q   */
  static ULONG flFrameFlags = FCF_TITLEBAR  | FCF_SYSMENU    |
                              FCF_MINBUTTON | FCF_TASKLIST   |
                              FCF_BORDER    | FCF_ACCELTABLE |
                              FCF_NOBYTEALIGN;
  QMSG qMsg;

  GBMTOOL_FILEARG gbmfilearg;
  char fn_src[GBMTOOL_FILENAME_MAX+1+50], opt_src[GBMTOOL_OPTIONS_MAX+1];

  GBM gbm;
  GBMRGB gbmrgb [0x100];
  BYTE *pbData;
  BOOL fHalftone = FALSE, fErrdiff = FALSE, fOk;
  BOOL qualityScalingEnabled = FALSE;
  int  filterIndex = FILTER_INDEX_SIMPLE;
  int i;

  /* process command line options */
  for ( i = 1; i < argc; i++ )
  {
    if ( argv [i][0] != '-' )
    {
      break;
    }
    else if ( argv[i][1] == '-' )
    {
      ++i;
      break;
    }
    switch ( argv [i][1] )
    {
      case 'e':
        fErrdiff = TRUE;
        break;

      case 'h':
        fHalftone = TRUE;
        break;

       case 'f':
        if ( ++i == argc ) Usage(HWND_DESKTOP);
        filterIndex = get_opt_value_filterIndex(argv[i],
                                                FILTER_NAME_TABLE,
                                                FILTER_NAME_TABLE_LENGTH);
        break;

      default:
        Usage(HWND_DESKTOP);
        break;
    }
  }

  if ( fErrdiff && fHalftone )
  {
    Fatal(HWND_DESKTOP, "error-diffusion and halftoning can't both be done at once");
  }

  if ( i == argc )
  {
    Usage(HWND_DESKTOP);
  }

  if (filterIndex == -1)
  {
    Fatal(HWND_DESKTOP, "wrong filter type");
  }
  else if (filterIndex != FILTER_INDEX_SIMPLE)
  {
     qualityScalingEnabled = TRUE;
  }

  /* Split filename and file options. */
  gbmfilearg.argin = argv[i];
  if (strcmp(gbmfilearg.argin, "\"\"") == 0)
  {
    Usage(HWND_DESKTOP);
  }
  if (gbmtool_parse_argument(&gbmfilearg, FALSE) != GBM_ERR_OK)
  {
    Fatal(HWND_DESKTOP, "can't parse filename");
  }
  strcpy(fn_src , gbmfilearg.files->filename);
  strcpy(opt_src, gbmfilearg.options);
  gbmtool_free_argument(&gbmfilearg);

  gbm_init();

  if ( LoadBitmap(HWND_DESKTOP, fn_src, opt_src, &gbm, gbmrgb, &pbData) )
  {
    const int src_width  = gbm.w;
    const int src_height = gbm.h;
    const int src_bpp    = gbm.bpp;
    const BOOL isGray    = src_bpp > 8 ? FALSE : isGrayscalePalette(gbmrgb, 1 << src_bpp);

    HPS hps = WinGetPS(HWND_DESKTOP);
    HDC hdc = GpiQueryDevice(hps);
    LONG lPlanes, lBitCount;

    DevQueryCaps(hdc, CAPS_COLOR_PLANES  , 1L, &lPlanes  );
    DevQueryCaps(hdc, CAPS_COLOR_BITCOUNT, 1L, &lBitCount);
    WinReleasePS(hps);

    if ( fHalftone || fErrdiff ||
         (qualityScalingEnabled && (gbm.bpp <= 8) && (! isGray)) )
    {
        fOk = To24Bit(&gbm, gbmrgb, &pbData);
    }
    else
    {
        fOk = TRUE;
    }

    if ( fOk )
    /* can now go on to resizing if necessary */
    {
      fOk = resizeToScreenSize(qualityScalingEnabled, isGray, filterIndex,
                               &gbm, gbmrgb, &pbData);
    }

    if ( fOk )
    /* can now go on to errordiffuse/halftone and display */
    {
      if ( fHalftone )
      {
        switch ( (int) lBitCount )
        {
          case 4:
            gbm_ht_pal_VGA(gbmrgb);
            gbm_ht_VGA_3x3(&gbm, pbData, pbData);
            gbm.bpp = 4;
            break;
          case 8:
            gbm_ht_pal_7R8G4B(gbmrgb);
            gbm_ht_7R8G4B_2x2(&gbm, pbData, pbData);
            gbm.bpp = 8;
            break;
          case 16:
            gbm_ht_24_2x2(&gbm, pbData, pbData, 0xf8, 0xfc, 0xf8);
            gbm.bpp = 24;
            break;
        }
      }
      else if ( fErrdiff )
      {
        switch ( (int) lBitCount )
        {
          case 4:
            gbm_errdiff_pal_VGA(gbmrgb);
            gbm_errdiff_VGA(&gbm, pbData, pbData);
            gbm.bpp = 4;
            break;
          case 8:
            gbm_errdiff_pal_7R8G4B(gbmrgb);
            gbm_errdiff_7R8G4B(&gbm, pbData, pbData);
            gbm.bpp = 8;
            break;
          case 16:
            if ( !gbm_errdiff_24(&gbm, pbData, pbData, 0xf8, 0xfc, 0xf8) )
                break;
            gbm.bpp = 24;
            break;
        }
      }
      fOk = MakeBitmap(HWND_DESKTOP, &gbm, gbmrgb, pbData, &hbmBmp);
      if ( gbm.bpp == 1 )
        /* remember Bg and Fg colours */
      {
        lColorBg = (gbmrgb [0].r << 16) + (gbmrgb [0].g << 8) + gbmrgb [0].b;
        lColorFg = (gbmrgb [1].r << 16) + (gbmrgb [1].g << 8) + gbmrgb [1].b;
      }

      gbmmem_free(pbData);
      if ( fOk )
        /* all ok, do PM windows etc */
      {
        RECTL rcl;
        SHORT cxScreen, cyScreen, cxWindow, cyWindow;
        BITMAPINFOHEADER bmp;

        WinRegisterClass(
            hab,            /* Anchor block handle               */
            WC_GBMV,        /* Window class name                 */
            GbmVWndProc,    /* Responding procedure for class    */
            CS_SIZEREDRAW,  /* Class style                       */
            0);             /* Extra bytes to reserve for class  */

        hwndFrame = WinCreateStdWindow(
            HWND_DESKTOP,   /* Parent window handle              */
            0L,             /* Style of frame window             */
            &flFrameFlags,  /* Pointer to control data           */
            WC_GBMV,        /* Client window class name          */
            NULL,           /* Title bar text                    */
            0L,             /* Style of client window            */
            (HMODULE) NULL, /* Module handle for resources       */
            RID_STANDARD,   /* ID of resources                   */
            &hwndClient);   /* Pointer to client window handle   */

        if (opt_src[0])
        {
          strcat(fn_src, ",");
          strcat(fn_src, opt_src);
        }

        strcat(fn_src, " (");
        sprintf(fn_src + strlen(fn_src), "%dx%dx%d", src_width, src_height, src_bpp);
        if ((src_width  != gbm.w) ||
            (src_height != gbm.h) ||
            (src_bpp    != gbm.bpp))
        {
          strcat(fn_src, " -> ");
          sprintf(fn_src + strlen(fn_src), "%dx%dx%d", gbm.w, gbm.h, gbm.bpp);
        }
        strcat(fn_src, ")");

        if ( fErrdiff )
        {
          strcat(fn_src, " -e");
        }
        else if ( fHalftone )
        {
          strcat(fn_src, " -h");
        }
        if ( qualityScalingEnabled )
        {
          strcat(fn_src, " -f ");
          strcat(fn_src, FILTER_NAME_TABLE[filterIndex].name);
        }

        WinSetWindowText(hwndFrame, fn_src);

        GpiQueryBitmapParameters(hbmBmp, &bmp);

        rcl.xLeft   = 0L;
        rcl.xRight  = bmp.cx;
        rcl.yBottom = 0L;
        rcl.yTop    = bmp.cy;
        WinCalcFrameRect(hwndFrame, &rcl, FALSE);

        cxWindow = (SHORT) (rcl.xRight - rcl.xLeft),
        cyWindow = (SHORT) (rcl.yTop - rcl.yBottom),
        cxScreen = (SHORT) WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
        cyScreen = (SHORT) WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);

        WinSetWindowPos(hwndFrame,
                HWND_TOP,
                (cxScreen - cxWindow) / 2,
                (cyScreen - cyWindow) / 2,
                cxWindow,
                cyWindow,
                SWP_MOVE | SWP_SIZE | SWP_ACTIVATE | SWP_SHOW);

        while ( WinGetMsg(hab, &qMsg, (HWND) NULL, 0, 0) )
        {
            WinDispatchMsg(hab, &qMsg);
        }
        WinDestroyWindow(hwndFrame);
      }
    }
    else
    {
      gbmmem_free(pbData);
      Error(HWND_DESKTOP, "Out of memory");
    }
  }

  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);

  gbm_deinit();

  return ( 0 );
}

