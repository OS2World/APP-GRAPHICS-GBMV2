/*

bmputils.c - PM Bitmap Utilities

*/

/*...sincludes:0:*/
#define	INCL_DOS
#define	INCL_WIN
#define	INCL_GPI
#define	INCL_DEV
#define	INCL_SPL
#define	INCL_SPLDOSPRINT
#define	INCL_BITMAPFILEFORMAT
#include <os2.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#define	_BMPUTILS_
#include "bmputils.h"

/*...vbmputils\46\h:0:*/
/*...e*/

/*...sBmpBlitter          \45\ blit one part of one bitmap to part of another:0:*/
BMP_ERR BmpBlitter(
	HAB hab,
	HBITMAP hbmSrc, RECTL rclSrc,
	HBITMAP hbmDst, RECTL rclDst
	)
	{
	HDC hdcSrc, hdcDst;
	HPS hpsSrc, hpsDst;
	BITMAPINFOHEADER2 bmp;
	SIZEL sizl;
	POINTL aptl[4];

	if ( (hdcSrc = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL)) == (HDC) NULL )
		return BMP_ERR_RES;

	bmp.cbFix = sizeof(BITMAPINFOHEADER2);
	GpiQueryBitmapInfoHeader(hbmSrc, &bmp);
	sizl.cx = bmp.cx; sizl.cy = bmp.cy;
	if ( (hpsSrc = GpiCreatePS(hab, hdcSrc, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
		{
		DevCloseDC(hdcSrc);
		return BMP_ERR_RES;
		}

	if ( (hdcDst = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL)) == (HDC) NULL )
		{
		GpiDestroyPS(hpsSrc); DevCloseDC(hdcSrc);
		return BMP_ERR_RES;
		}

	bmp.cbFix = sizeof(BITMAPINFOHEADER2);
	GpiQueryBitmapInfoHeader(hbmDst, &bmp);
	sizl.cx = bmp.cx; sizl.cy = bmp.cy;
	if ( (hpsDst = GpiCreatePS(hab, hdcDst, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
		{
		DevCloseDC(hdcDst);
		GpiDestroyPS(hpsSrc); DevCloseDC(hdcSrc);
		return BMP_ERR_RES;
		}

	GpiSetBitmap(hpsSrc, hbmSrc);
	GpiSetBitmap(hpsDst, hbmDst);

	aptl[0].x = rclDst.xLeft;
	aptl[0].y = rclDst.yBottom;
	aptl[1].x = rclDst.xRight;
	aptl[1].y = rclDst.yTop;
	aptl[2].x = rclSrc.xLeft;
	aptl[2].y = rclSrc.yBottom;
	aptl[3].x = rclSrc.xRight;
	aptl[3].y = rclSrc.yTop;

	if ( aptl[1].x - aptl[0].x == aptl[3].x - aptl[2].x &&
	     aptl[1].y - aptl[0].y == aptl[3].y - aptl[2].y )
		GpiBitBlt(hpsDst, hpsSrc, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE);
	else
		GpiBitBlt(hpsDst, hpsSrc, 4L, aptl, ROP_SRCCOPY, BBO_IGNORE);

	GpiSetBitmap(hpsDst, (HBITMAP) NULL); GpiDestroyPS(hpsDst); DevCloseDC(hdcDst);
	GpiSetBitmap(hpsSrc, (HBITMAP) NULL); GpiDestroyPS(hpsSrc); DevCloseDC(hdcSrc);

	return BMP_ERR_OK;
	}
/*...e*/
/*...sBmpCopySubrectangle \45\ make a new bitmap from a bit of the first:0:*/
BMP_ERR BmpCopySubrectangle(HAB hab, HBITMAP hbmSrc, RECTL rclSrc, HBITMAP *phbmDst)
	{
	BITMAPINFOHEADER2 bmp;
	HBITMAP hbmDst;
	RECTL rclDst;
	SIZEL sizl;
	USHORT rc;
	HDC hdc;
	HPS hps;

	bmp.cbFix = sizeof(BITMAPINFOHEADER2);
	GpiQueryBitmapInfoHeader(hbmSrc, &bmp);

	bmp.cx = (USHORT) (rclSrc.xRight - rclSrc.xLeft);
	bmp.cy = (USHORT) (rclSrc.yTop - rclSrc.yBottom);

	if ( (hdc = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL)) == (HDC) NULL )
		return BMP_ERR_RES;

	sizl.cx = bmp.cx; sizl.cy = bmp.cy;
	if ( (hps = GpiCreatePS(hab, hdc, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
		{
		DevCloseDC(hdc);
		return BMP_ERR_RES;
		}

	if ( (hbmDst = GpiCreateBitmap(hps, &bmp, 0L, NULL, NULL)) == (HBITMAP) NULL )
		{
		GpiDestroyPS(hps);
		DevCloseDC(hdc);
		return BMP_ERR_MEM;
		}

	GpiDestroyPS(hps);
	DevCloseDC(hdc);

	rclDst.xLeft = 0; rclDst.xRight = bmp.cx;
	rclDst.yBottom = 0; rclDst.yTop = bmp.cy;

	if ( (rc = BmpBlitter(hab, hbmSrc, rclSrc, hbmDst, rclDst)) != BMP_ERR_OK )
		{
		GpiDeleteBitmap(hbmDst);
		return rc;
		}

	*phbmDst = hbmDst;

	return BMP_ERR_OK;
	}
/*...e*/
/*...sBmpCopy             \45\ make a new bitmap from all of the first:0:*/
BMP_ERR BmpCopy(HAB hab, HBITMAP hbmSrc, HBITMAP *phbmDst)
	{
	BITMAPINFOHEADER2 bmp;
	RECTL rclSrc;

	bmp.cbFix = sizeof(BITMAPINFOHEADER2);
	GpiQueryBitmapInfoHeader(hbmSrc, &bmp);

	rclSrc.xLeft = 0; rclSrc.xRight = bmp.cx;
	rclSrc.yBottom = 0; rclSrc.yTop = bmp.cy;

	return BmpCopySubrectangle(hab, hbmSrc, rclSrc, phbmDst);
	}
/*...e*/
/*...sBmpPrint            \45\ print a bitmap:0:*/
/*...sEnquireQueueDetails:0:*/
static BOOL EnquireQueueDetails(
	CHAR *szQueue,
	CHAR *szLogAddress,
	CHAR *szDriverName,
	CHAR *szNameWPS,
	DRIVDATA **ppdriv
	)
	{
	ULONG cb; PRQINFO3 *ppq3;

    *ppdriv = NULL;
	SplQueryQueue(NULL, szQueue, 3, NULL, 0, &cb);
	if ( cb < sizeof(PRQINFO3) )
		return FALSE;
	if ( (ppq3 = (PRQINFO3 *) malloc(cb)) == (PRQINFO3 *) NULL )
		return FALSE;
	SplQueryQueue(NULL, szQueue, 3, ppq3, cb, &cb);
	strcpy(szNameWPS, ppq3->pszComment);
	strcpy(szLogAddress, szQueue);
	sscanf(ppq3->pszDriverName, "%[^.,;]", szDriverName);

	if ( (*ppdriv = (DRIVDATA *) malloc(ppq3->pDriverData->cb)) ==  NULL )
	{
		free(ppq3);
		return FALSE;
	}
	memcpy(*ppdriv, ppq3->pDriverData, ppq3->pDriverData->cb);
	free(ppq3);
	return TRUE;
	}
/*...e*/
/*...sPrinterInfo:0:*/
static BOOL PrinterInfo(HPS hpsPrinter, const CHAR *szNameWPS)
	{
	HDC hdcPrinter = GpiQueryDevice(hpsPrinter);
	LONG lBitCount, lPlanes, lColors, lPhysColors;
	SIZEL sizlPrinter;
	CHAR sz[100+1];

	GpiQueryPS(hpsPrinter, &sizlPrinter);

	DevQueryCaps(hdcPrinter, CAPS_COLOR_BITCOUNT, 1L, &lBitCount  );
	DevQueryCaps(hdcPrinter, CAPS_COLOR_PLANES  , 1L, &lPlanes    );
	DevQueryCaps(hdcPrinter, CAPS_COLORS        , 1L, &lColors    );
	DevQueryCaps(hdcPrinter, CAPS_PHYS_COLORS   , 1L, &lPhysColors);

	sprintf(sz, "%s: %ldx%ld, %ld bits, %ld planes, %ld colors, %ld physical colors?",
		szNameWPS, sizlPrinter.cx, sizlPrinter.cy, lBitCount, lPlanes, lColors, lPhysColors);

	return WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, sz, "GbmV2 - Confirm Printer", 0, MB_OKCANCEL | MB_QUERY | MB_MOVEABLE) == MBID_OK;
	}
/*...e*/
/*...sTransferBitmapData:0:*/
/*
Put the largest possible copy of the bitmap onto the printer presentation
space. Preserve aspect ratio and correct for printer aspect ratio.
*/

/*...sMonoPrint:0:*/
static BOOL MonoPrint(HPS hpsPrinter)
	{
	HDC hdcPrinter = GpiQueryDevice(hpsPrinter);
	LONG lBitCount, lPlanes;

	DevQueryCaps(hdcPrinter, CAPS_COLOR_BITCOUNT, 1L, &lBitCount);
	DevQueryCaps(hdcPrinter, CAPS_COLOR_PLANES  , 1L, &lPlanes  );

	return lBitCount == 1 && lPlanes == 1;
	}
/*...e*/

static BMP_ERR TransferBitmapData(
	HAB hab,
	HBITMAP hbm, LONG lColorFg, LONG lColorBg,
	HDC hdcPrinter, HPS hpsPrinter
	)
	{
	SIZEL sizlMemory, sizlPrinter, sizlImage;
	SIZEL sizlMmPrinter, sizlMmImage; /* Sizes in mm */
	HDC hdcMemory;
	HPS hpsMemory;
	POINTL aptl[4];
	BITMAPINFOHEADER2 bmp;
	LONG xRes, yRes, xCentre, yCentre;

	/* Query printer size */

	GpiQueryPS(hpsPrinter, &sizlPrinter);

	/* Query bitmap size */

	bmp.cbFix = sizeof(BITMAPINFOHEADER2);
	GpiQueryBitmapInfoHeader(hbm, &bmp);
	sizlMemory.cx = (LONG) bmp.cx;
	sizlMemory.cy = (LONG) bmp.cy;

	/* Make a memory device context for the bitmap */

	if ( (hdcMemory = DevOpenDC(hab, OD_MEMORY, "*", 0L, NULL, hdcPrinter)) == (HPS) NULL )
		return BMP_ERR_RES;

	if ( (hpsMemory = GpiCreatePS(hab, hdcMemory, &sizlMemory, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
		{
		DevCloseDC(hdcMemory);
		return BMP_ERR_RES;
		}

	GpiSetBitmap(hpsMemory, hbm);

	/* Set colours ready for blitting into printer ps */

	if ( bmp.cBitCount == 1 )
		{
		GpiSetBackColor(hpsPrinter, GpiQueryColorIndex(hpsPrinter, 0, lColorBg));
		GpiSetColor    (hpsPrinter, GpiQueryColorIndex(hpsPrinter, 0, lColorFg));
		}

/*...sscale to fit page:8:*/
/* Query printer aspect ratio */

DevQueryCaps(hdcPrinter, CAPS_HORIZONTAL_RESOLUTION, 1L, &xRes);
DevQueryCaps(hdcPrinter, CAPS_VERTICAL_RESOLUTION  , 1L, &yRes);

/* Get size of page in mm */

sizlMmPrinter.cx = (1000L * sizlPrinter.cx) / xRes;
sizlMmPrinter.cy = (1000L * sizlPrinter.cy) / yRes;

/* Get largest image size in mm */

if ( sizlMemory.cx * sizlMmPrinter.cy > sizlMemory.cy * sizlMmPrinter.cx )
	/* Stretch to fill width */
	{
	sizlMmImage.cx = sizlMmPrinter.cx;
	sizlMmImage.cy = (sizlMmImage.cx * sizlMemory.cy) / sizlMemory.cx;
	}
else
	/* Stretch to fill height */
	{
	sizlMmImage.cy = sizlMmPrinter.cy;
	sizlMmImage.cx = (sizlMmImage.cy * sizlMemory.cx) / sizlMemory.cy;
	}

sizlImage.cx = (xRes * sizlMmImage.cx) / 1000L;
sizlImage.cy = (yRes * sizlMmImage.cy) / 1000L;

xCentre = ((sizlPrinter.cx - sizlImage.cx) >> 1);
yCentre = ((sizlPrinter.cy - sizlImage.cy) >> 1);

aptl[0].x = xCentre;
aptl[0].y = yCentre;
aptl[1].x = xCentre + sizlImage.cx;
aptl[1].y = yCentre + sizlImage.cy;
aptl[2].x = 0;
aptl[2].y = 0;
aptl[3].x = sizlMemory.cx;
aptl[3].y = sizlMemory.cy;

GpiBitBlt(hpsPrinter, hpsMemory, 4L, aptl, ROP_SRCCOPY,
	MonoPrint(hpsPrinter) ? BBO_AND : BBO_IGNORE);
/*...e*/
#if 0
/*...sposition in the middle if room:8:*/
if ( sizlMemory.cx < sizlPrinter.cx &&
     sizlMemory.cy < sizlPrinter.cy )
	{
	xCentre = ((sizlPrinter.cx - sizlMemory.cx) >> 1);
	yCentre = ((sizlPrinter.cy - sizlMemory.cy) >> 1);

	aptl[0].x = xCentre;
	aptl[0].y = yCentre;
	aptl[1].x = xCentre + sizlMemory.cx;
	aptl[1].y = yCentre + sizlMemory.cy;
	aptl[2].x = 0;
	aptl[2].y = 0;

	GpiBitBlt(hpsPrinter, hpsMemory, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE);
	}
/*...e*/
#endif

	GpiSetBitmap(hpsMemory, (HBITMAP) NULL);
	GpiDestroyPS(hpsMemory);
	DevCloseDC(hdcMemory);

	return BMP_ERR_OK;
	}
/*...e*/

BMP_ERR BmpPrint(
	HAB hab,
	HBITMAP hbm, LONG lColorFg, LONG lColorBg,
	const CHAR *szDocName,
	const CHAR *szComment,
	CHAR *szQueueToUse
	)
	{
	static SIZEL sizl = { 0, 0 };
	HDC hdcPrinter;
	HPS hpsPrinter;
	CHAR szLogAddress[256];
	CHAR szDriverName[256];
	CHAR szNameWPS[256];
	DEVOPENSTRUC dop;
	USHORT cb, usJobId = 0;
	CHAR szQueue[256];
	BMP_ERR rc;
	LONG lOutCount = 0;
	BOOL fQueried = FALSE;

	/* Set defaults, in case can't query information about printer */

	dop.pszLogAddress      = szLogAddress;
	dop.pszDriverName      = szDriverName;
	dop.pdriv              = NULL;
	dop.pszDataType        = "PM_Q_STD";
	dop.pszComment         = (PSZ) szComment;
	dop.pszQueueProcName   = NULL;
	dop.pszQueueProcParams = NULL;
	dop.pszSpoolerParams   = NULL;
	dop.pszNetworkParams   = NULL;

	if ( szQueueToUse != NULL )
		{
		/* Use the queue we have been told to use */
		fQueried = EnquireQueueDetails(szQueueToUse, szLogAddress, szDriverName, szNameWPS, &dop.pdriv);
		}
	else if ( (cb = (USHORT) PrfQueryProfileString(HINI_PROFILE, "PM_SPOOLER",
	     "QUEUE", NULL, szQueue, (LONG) sizeof(szQueue))) > 0 )
		/* Use application default queue */
		{
		szQueue[cb - 2] = '\0'; /* Remove semicolon at end */
		fQueried = EnquireQueueDetails(szQueue, szLogAddress, szDriverName, szNameWPS, &dop.pdriv);
		}

	if ( !fQueried )
		{
		free(dop.pdriv);
		return BMP_ERR_QUERY;
		}

	/* Try to create printer device context */

	if ( (hdcPrinter = DevOpenDC(hab, OD_QUEUED, "*", 5L, (PDEVOPENDATA) &dop, (HDC) NULL)) == (HDC) NULL )
		{
		free(dop.pdriv);
		/* Failed to open printer device context */
		return BMP_ERR_PRINTER;
		}

	/* Create a presentation space of default size (size of page) */

	if ( (hpsPrinter = GpiCreatePS(hab, hdcPrinter, &sizl, PU_PELS | GPIF_DEFAULT | GPIA_ASSOC)) == (HPS) NULL )
		/* Could not make presentation space */
		{
		DevCloseDC(hdcPrinter);
		free(dop.pdriv);
		return BMP_ERR_PRINTER;
		}

	/* Tell user about printer, and give chance to abort job */

	if ( !PrinterInfo(hpsPrinter, szNameWPS) )
		{
		GpiAssociate(hpsPrinter, (HDC) NULL);
		GpiDestroyPS(hpsPrinter);
		DevCloseDC(hdcPrinter);
		free(dop.pdriv);
		return BMP_ERR_OK;
		}

	/* Indicate a new print job is starting */

	if ( !DevEscape(hdcPrinter, DEVESC_STARTDOC, (LONG) strlen(szDocName),
		  (BYTE *) szDocName, 0L, (BYTE *) NULL) )
		{
		GpiAssociate(hpsPrinter, (HDC) NULL);
		GpiDestroyPS(hpsPrinter);
		DevCloseDC(hdcPrinter);
		free(dop.pdriv);
		return BMP_ERR_PRINT;
		}

	GpiErase(hpsPrinter); /* Blank the page to CLR_BACKGROUND */

	if ( (rc = TransferBitmapData(hab, hbm, lColorFg, lColorBg, hdcPrinter, hpsPrinter)) != BMP_ERR_OK )
		{
		DevEscape(hdcPrinter, DEVESC_ABORTDOC, 0L, NULL, 0L, NULL);
		GpiAssociate(hpsPrinter, (HDC) NULL);
		GpiDestroyPS(hpsPrinter);
		DevCloseDC(hdcPrinter);
		free(dop.pdriv);
		return rc;
		}

	/* Indicate the job is done */
	lOutCount = 2;
	usJobId   = 0;
	if ( !DevEscape(hdcPrinter, DEVESC_ENDDOC, 0L, (BYTE *) NULL, &lOutCount, (BYTE *) &usJobId) )
		{
		GpiAssociate(hpsPrinter, (HDC) NULL);
		GpiDestroyPS(hpsPrinter);
		DevCloseDC(hdcPrinter);
		free(dop.pdriv);
		return BMP_ERR_PRINT;
		}

	GpiAssociate(hpsPrinter, (HDC) NULL);
	GpiDestroyPS(hpsPrinter);
	DevCloseDC(hdcPrinter);

	free(dop.pdriv);

	return BMP_ERR_OK;
	}
/*...e*/
/*...sBmpErrorMessage     \45\ get error message from error return code:0:*/
CHAR *BmpErrorMessage(BMP_ERR rc, CHAR *sz)
	{
	static CHAR *aszBitmapErrors[] =
		{
		"",
		"out of memory",
		"out of resources",
		"can't query printer/queue details",
		"can't locate printer/queue",
		"error sending data to printer/queue",
		};

	return strcpy(sz, aszBitmapErrors[rc]);
	}
/*...e*/
