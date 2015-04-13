/*

model.c - Work on the model used in GbmV2

*/

#ifndef MODEL_H
#define	MODEL_H

#include "gbm.h"

/* These needs to encompass all the GBM errors, plus some extras */
typedef int MOD_ERR;
#define	MOD_ERR_OK	((MOD_ERR) 0)
#define	MOD_ERR_MEM	((MOD_ERR) 1)
#define	MOD_ERR_OPEN	((MOD_ERR) 2)
#define	MOD_ERR_CREATE	((MOD_ERR) 3)
#define	MOD_ERR_SUPPORT	((MOD_ERR) 4)
#define	MOD_ERR_HDC	((MOD_ERR) 5)
#define	MOD_ERR_HPS	((MOD_ERR) 6)
#define	MOD_ERR_HBITMAP	((MOD_ERR) 7)
#define	MOD_ERR_HMF	((MOD_ERR) 8)
#define	MOD_ERR_CLIP	((MOD_ERR) 9)
#define MOD_ERR_READ_BPP ((MOD_ERR) 10)
#define	MOD_ERR_GBM(rc)	((MOD_ERR) 0x8000+(rc))

typedef struct
	{
	GBM gbm;
	GBMRGB gbmrgb[0x100];
	BYTE *pbData;
	} MOD;

#pragma pack(2)
typedef struct
{
   BITMAPINFOHEADER2 bmp2;
   RGB2   argb2Color[0x100]; /* unused for bitmaps >8bpp */
   MOD    mod;
} BITMAP;
#pragma pack()


extern MOD_ERR ModCreate(
	int w, int h, int bpp, const GBMRGB gbmrgb[],
	MOD *modNew
	);

extern MOD_ERR ModDelete(MOD *mod);

extern MOD_ERR ModCopy(const MOD *mod, MOD *modNew);

extern MOD_ERR ModMove(MOD *mod, MOD *modNew);

extern MOD_ERR ModCreateFromFile(
	const CHAR *szFn, const CHAR *szOpt,
	MOD *modNew
	);

extern MOD_ERR ModWriteToFile(
	const MOD *mod,
	const CHAR *szFn, const CHAR *szOpt
	);

extern MOD_ERR ModExpandTo24Bpp(const MOD *mod, MOD *mod24);

extern MOD_ERR ModReflectHorz(const MOD *mod, MOD *modNew);
extern MOD_ERR ModReflectVert(const MOD *mod, MOD *modNew);
extern MOD_ERR ModTranspose  (const MOD *mod, MOD *modNew);
extern MOD_ERR ModRotate90   (const MOD *mod, MOD *modNew);
extern MOD_ERR ModRotate180  (const MOD *mod, MOD *modNew);
extern MOD_ERR ModRotate270  (const MOD *mod, MOD *modNew);

extern MOD_ERR ModExtractSubrectangle(
	const MOD *mod,
	int x, int y, int w, int h,
	MOD *modNew
	);

extern MOD_ERR ModBlit(
	      MOD *modDst, int dx, int dy,
	const MOD *modSrc, int sx, int sy,
	int w, int h
	);

/*...scolour adjustment mappings:0:*/
#define	CVT_I_TO_L	0
#define	CVT_I_TO_P	1
#define	CVT_L_TO_I	2
#define	CVT_L_TO_P	3
#define	CVT_P_TO_I	4
#define	CVT_P_TO_L	5
/*...e*/

extern MOD_ERR ModColourAdjust(
	const MOD *mod,
	int map, double gama, double shelf,
	MOD *modNew
	);

/*...sbpp palette and algorithm mappings:0:*/
#define	CVT_BW		0
#define	CVT_VGA		1
#define	CVT_8		2
#define	CVT_4G		3
#define	CVT_784		4
#define	CVT_666		5
#define	CVT_444		6
#define	CVT_8G		7
#define	CVT_TRIPEL	8
#define	CVT_RGB		9
#define	CVT_FREQ	10
#define	CVT_MCUT	11

#define	CVT_NEAREST	0
#define	CVT_ERRDIFF	1
#define	CVT_HALFTONE	2
/*...e*/

extern MOD_ERR ModBppMap(
	const MOD *mod,
	int iPal, int iAlg,
	int iKeepRed, int iKeepGreen, int iKeepBlue, int nCols,
	MOD *modNew
	);

extern MOD_ERR ModFixRedeye(const MOD *mod, MOD *modNew);

extern MOD_ERR ModResize(
	const MOD *mod,
	int nw, int nh,
	BOOL isGrayscale,
	BOOL useScaleFilter,
	GBM_SCALE_FILTER scaleFilter,
	MOD *modNew
	);

extern MOD_ERR ModCreateFromHPS(
	HPS hps, int w, int h, int bpp,
	MOD *modNew
	);

extern MOD_ERR ModMakeHBITMAP(
	const MOD *mod,
	HAB hab,
	HBITMAP *phbm
	);

extern MOD_ERR ModMakeBITMAP(
	const MOD *mod,
	BITMAP *pbmp
	);

extern MOD_ERR ModMakeHMF(
	HBITMAP hbm,
	HAB hab,
	HMF *phmf
	);

extern const CHAR * ModErrorString(MOD_ERR rc);

#endif
