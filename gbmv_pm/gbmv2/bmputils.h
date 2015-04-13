/*

bmputils.h - PM Bitmap utilities

All bitmaps referred to below should not be selected into a hps at time of
call. They will be selected into screen or printer compatible hps's during the
call and be unselected afterwards. Because of this, you will find that the
colours change in the bitmap when using the BmpPrint call. The solution is
to copy the desired bitmap, call BmpPrint on it, then to discard it.

*/

#ifndef BMPUTILS_H
#define	BMPUTILS_H

typedef USHORT BMP_ERR;
#define	BMP_ERR_OK	0		/* All went ok                       */
#define	BMP_ERR_MEM	1		/* Out of memory                     */
#define	BMP_ERR_RES	2		/* Out of PM resources (DC, PS etc.) */
#define	BMP_ERR_QUERY	3		/* Can't query queue info            */
#define	BMP_ERR_PRINTER	4		/* Can't find the printer            */
#define	BMP_ERR_PRINT	5		/* Error in the printing itself      */

#ifndef _BMPUTILS_

extern BMP_ERR BmpBlitter(
	HAB hab,
	HBITMAP hbmSrc, RECTL rclSrc,
	HBITMAP hbmDst, RECTL rclDst
	);

extern BMP_ERR BmpCopySubrectangle(
	HAB hab,
	HBITMAP hbmSrc,
	RECTL rclSrc,
	HBITMAP *phbmDst
	);

extern BMP_ERR BmpCopy(HAB hab, HBITMAP hbmSrc, HBITMAP *phbmDst);

extern BMP_ERR BmpPrint(
	HAB hab,
	HBITMAP hbm, LONG lColorFg, LONG lColorBg,
	const CHAR *szDocName,
	const CHAR *szComment,
	CHAR *szQueueToUse
	);

extern CHAR *BmpErrorMessage(BMP_ERR rc, CHAR *sz);

#endif

#endif
