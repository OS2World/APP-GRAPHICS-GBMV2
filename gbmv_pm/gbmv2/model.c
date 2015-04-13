
/*

model.c - Work on the model used in GbmV2

*/

/*...sincludes:0:*/
#define	INCL_DOS
#define	INCL_WIN
#define	INCL_GPI
#define	INCL_DEV
#define	INCL_BITMAPFILEFORMAT
#include <os2.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>
#include <math.h>
#include "gbm.h"
#include "gbmmem.h"
#include "gbmtrunc.h"
#include "gbmerr.h"
#include "gbmht.h"
#include "gbmhist.h"
#include "gbmmcut.h"
#include "gbmmir.h"
#include "gbmrect.h"
#include "gbmscale.h"
#include "model.h"

/*...vgbm\46\h:0:*/
/*...vgbmtrunc\46\h:0:*/
/*...vgbmerr\46\h:0:*/
/*...vgbmht\46\h:0:*/
/*...vgbmhist\46\h:0:*/
/*...vgbmmcut\46\h:0:*/
/*...vgbmmir\46\h:0:*/
/*...vgbmrect\46\h:0:*/
/*...vgbmscale\46\h:0:*/
/*...vmodel\46\h:0:*/
/*...e*/

/*...sStrideOf:0:*/
static int StrideOf(const MOD *mod)
	{
	return ( ( mod->gbm.w * mod->gbm.bpp + 31 ) / 32 ) * 4;
	}
/*...e*/
/*...sAllocateData:0:*/
static BOOL AllocateData(MOD *mod)
	{
	const unsigned long stride = StrideOf(mod);
	if ( (mod->pbData = gbmmem_malloc(stride * mod->gbm.h)) == NULL )
		return FALSE;
	return TRUE;
	}
/*...e*/

/*...sModCreate:0:*/
MOD_ERR ModCreate(
	int w, int h, int bpp, const GBMRGB gbmrgb[],
	MOD *modNew
	)
	{
	modNew->gbm.w   = w;
	modNew->gbm.h   = h;
	modNew->gbm.bpp = bpp;
	if ( gbmrgb != NULL && bpp != 24 )
		memcpy(&(modNew->gbmrgb), gbmrgb, sizeof(GBMRGB) << bpp);
	if ( !AllocateData(modNew) )
		return MOD_ERR_MEM;
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModDelete:0:*/
MOD_ERR ModDelete(MOD *mod)
	{
	gbmmem_free(mod->pbData);
	mod->pbData = NULL;
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModCopy:0:*/
MOD_ERR ModCopy(const MOD *mod, MOD *modNew)
	{
	modNew->gbm.w   = mod->gbm.w;
	modNew->gbm.h   = mod->gbm.h;
	modNew->gbm.bpp = mod->gbm.bpp;
	if ( mod->gbm.bpp != 24 )
		memcpy(modNew->gbmrgb, mod->gbmrgb, sizeof(GBMRGB) << mod->gbm.bpp);

	if ( !AllocateData(modNew) )
		return MOD_ERR_MEM;
	memcpy(modNew->pbData, mod->pbData, StrideOf(mod) * mod->gbm.h);
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModMove:0:*/
MOD_ERR ModMove(MOD *mod, MOD *modNew)
	{
	memcpy(modNew, mod, sizeof(MOD));
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModCreateFromFile:0:*/
MOD_ERR ModCreateFromFile(
	const CHAR *szFn, const CHAR *szOpt,
	MOD *modNew
	)
	{
	GBM_ERR grc;
	int fd, ft;

	if ( (grc = gbm_guess_filetype(szFn, &ft)) != GBM_ERR_OK )
		return MOD_ERR_GBM(grc);

	if ( (fd = gbm_io_open(szFn, GBM_O_RDONLY)) == -1 )
		return MOD_ERR_OPEN;

	if ( (grc = gbm_read_header(szFn, fd, ft, &(modNew->gbm), szOpt)) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		return MOD_ERR_GBM(grc);
		}

        /* check for color depth supported */
	switch ( modNew->gbm.bpp )
		{
		case 24:
		case 8:
		case 4:
		case 1:
                   break;
                default:
 		  {
		     gbm_io_close(fd);
		     return MOD_ERR_READ_BPP;
		  }
		}

	if ( (grc = gbm_read_palette(fd, ft, &(modNew->gbm), modNew->gbmrgb)) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		return MOD_ERR_GBM(grc);
		}

	if ( !AllocateData(modNew) )
		{
		gbm_io_close(fd);
		return MOD_ERR_MEM;
		}

	if ( (grc = gbm_read_data(fd, ft, &(modNew->gbm), modNew->pbData)) != GBM_ERR_OK )
		{
		gbmmem_free(modNew->pbData);
		gbm_io_close(fd);
		return MOD_ERR_GBM(grc);
		}

	gbm_io_close(fd);

	return MOD_ERR_OK;
	}
/*...e*/
/*...sModWriteToFile:0:*/
MOD_ERR ModWriteToFile(
	const MOD *mod,
	const CHAR *szFn, const CHAR *szOpt
	)
	{
	GBM_ERR grc;
	int fd, ft, flag = 0;
	GBMFT gbmft;

	if ( (grc = gbm_guess_filetype(szFn, &ft)) != GBM_ERR_OK )
		return grc;

	gbm_query_filetype(ft, &gbmft);
	switch ( mod->gbm.bpp )
		{
		case 1:		flag = GBM_FT_W1;	break;
		case 4:		flag = GBM_FT_W4;	break;
		case 8:		flag = GBM_FT_W8;	break;
		case 24:	flag = GBM_FT_W24;	break;
		default:	flag = 0;	break;
		}

	if ( (gbmft.flags & flag) == 0 )
		return MOD_ERR_SUPPORT;

	if ( (fd = gbm_io_create(szFn, GBM_O_WRONLY)) == -1 )
		return MOD_ERR_CREATE;

	if ( (grc = gbm_write(szFn, fd, ft, &(mod->gbm), mod->gbmrgb, mod->pbData, szOpt)) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		unlink(szFn);
		return MOD_ERR_GBM(grc);
		}

	gbm_io_close(fd);

	return MOD_ERR_OK;
	}
/*...e*/
/*...sModExpandTo24Bpp:0:*/
MOD_ERR ModExpandTo24Bpp(const MOD *mod, MOD *mod24)
	{
	MOD_ERR mrc;
	int stride, stride24, y;

	if ( (mrc = ModCreate(mod->gbm.w, mod->gbm.h,
	     24, mod->gbmrgb, mod24)) != MOD_ERR_OK )
		return mrc;

	stride   = StrideOf(mod  );
	stride24 = StrideOf(mod24);

	for ( y = 0; y < mod->gbm.h; y++ )
		{
		BYTE *pbSrc  = mod  ->pbData + y * stride  ;
		BYTE *pbDest = mod24->pbData + y * stride24;
		int x;

		switch ( mod->gbm.bpp )
			{
/*...s1:24:*/
case 1:
	{
	BYTE c = 0;

	for ( x = 0; x < mod->gbm.w; x++ )
		{
		if ( (x & 7) == 0 )
			c = *pbSrc++;
		else
			c <<= 1;

		*pbDest++ = mod->gbmrgb[c >> 7].b;
		*pbDest++ = mod->gbmrgb[c >> 7].g;
		*pbDest++ = mod->gbmrgb[c >> 7].r;
		}
	}
	break;
/*...e*/
/*...s4:24:*/
case 4:
	for ( x = 0; x + 1 < mod->gbm.w; x += 2 )
		{
		BYTE c = *pbSrc++;

		*pbDest++ = mod->gbmrgb[c >> 4].b;
		*pbDest++ = mod->gbmrgb[c >> 4].g;
		*pbDest++ = mod->gbmrgb[c >> 4].r;
		*pbDest++ = mod->gbmrgb[c & 15].b;
		*pbDest++ = mod->gbmrgb[c & 15].g;
		*pbDest++ = mod->gbmrgb[c & 15].r;
		}

	if ( x < mod->gbm.w )
		{
		BYTE c = *pbSrc;

		*pbDest++ = mod->gbmrgb[c >> 4].b;
		*pbDest++ = mod->gbmrgb[c >> 4].g;
		*pbDest++ = mod->gbmrgb[c >> 4].r;
		}
	break;
/*...e*/
/*...s8:24:*/
case 8:
	for ( x = 0; x < mod->gbm.w; x++ )
		{
		BYTE c = *pbSrc++;

		*pbDest++ = mod->gbmrgb[(int)c].b;
		*pbDest++ = mod->gbmrgb[(int)c].g;
		*pbDest++ = mod->gbmrgb[(int)c].r;
		}
	break;
/*...e*/
/*...s24:24:*/
case 24:
	memcpy(pbDest, pbSrc, stride);
	break;
/*...e*/
			}
		}

	return MOD_ERR_OK;
	}
/*...e*/
/*...sModReflectHorz:0:*/
MOD_ERR ModReflectHorz(const MOD *mod, MOD *modNew)
	{
	MOD_ERR mrc;
	if ( (mrc = ModCopy(mod, modNew)) != MOD_ERR_OK )
		return mrc;
	if ( !gbm_ref_horz(&(modNew->gbm), modNew->pbData) )
		{
		gbmmem_free(modNew->pbData);
		return MOD_ERR_MEM;
		}
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModReflectVert:0:*/
MOD_ERR ModReflectVert(const MOD *mod, MOD *modNew)
	{
	MOD_ERR mrc;
	if ( (mrc = ModCopy(mod, modNew)) != MOD_ERR_OK )
		return mrc;
	if ( !gbm_ref_vert(&(modNew->gbm), modNew->pbData) )
		{
		gbmmem_free(modNew->pbData);
		return MOD_ERR_MEM;
		}
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModTranspose:0:*/
MOD_ERR ModTranspose(const MOD *mod, MOD *modNew)
	{
	modNew->gbm.w   = mod->gbm.h;
	modNew->gbm.h   = mod->gbm.w;
	modNew->gbm.bpp = mod->gbm.bpp;
	if ( mod->gbm.bpp != 24 )
		memcpy(modNew->gbmrgb, mod->gbmrgb, sizeof(GBMRGB) << mod->gbm.bpp);
	if ( !AllocateData(modNew) )
		return MOD_ERR_MEM;
	gbm_transpose(&(mod->gbm), mod->pbData, modNew->pbData);
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModRotate90:0:*/
MOD_ERR ModRotate90(const MOD *mod, MOD *modNew)
	{
	MOD_ERR mrc;
	MOD modTmp;

	mrc = ModTranspose(mod, &modTmp);
	if ( mrc != MOD_ERR_OK )
		return mrc;

	mrc = ModReflectHorz(&modTmp, modNew);
	if ( mrc  != MOD_ERR_OK )
		return mrc;

	ModDelete(&modTmp);
	return mrc;
	}
/*...e*/
/*...sModRotate180:0:*/
MOD_ERR ModRotate180(const MOD *mod, MOD *modNew)
	{
	MOD_ERR mrc;
	if ( (mrc = ModCopy(mod, modNew)) != MOD_ERR_OK )
		return mrc;
	if ( !gbm_ref_horz(&(modNew->gbm), modNew->pbData) )
		{
		gbmmem_free(modNew->pbData);
		return MOD_ERR_MEM;
		}
	if ( !gbm_ref_vert(&(modNew->gbm), modNew->pbData) )
		{
		gbmmem_free(modNew->pbData);
		return MOD_ERR_MEM;
		}
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModRotate270:0:*/
MOD_ERR ModRotate270(const MOD *mod, MOD *modNew)
	{
	MOD_ERR mrc;
	MOD modTmp;

	mrc = ModTranspose(mod, &modTmp);
	if ( mrc != MOD_ERR_OK )
		return mrc;

	mrc = ModReflectVert(&modTmp, modNew);
	if ( mrc  != MOD_ERR_OK )
		return mrc;
	ModDelete(&modTmp);
	return mrc;
	}
/*...e*/
/*...sModExtractSubrectangle:0:*/
MOD_ERR ModExtractSubrectangle(
	const MOD *mod,
	int x, int y, int w, int h,
	MOD *modNew
	)
	{
	modNew->gbm.w   = w;
	modNew->gbm.h   = h;
	modNew->gbm.bpp = mod->gbm.bpp;
	if ( mod->gbm.bpp != 24 )
		memcpy(modNew->gbmrgb, mod->gbmrgb, sizeof(GBMRGB) << mod->gbm.bpp);

	if ( !AllocateData(modNew) )
		return MOD_ERR_MEM;

	gbm_subrectangle(&(mod->gbm), x, y, w, h, mod->pbData, modNew->pbData);

	return MOD_ERR_OK;
	}
/*...e*/
/*...sModBlit:0:*/
MOD_ERR ModBlit(
	      MOD *modDst, int dx, int dy,
	const MOD *modSrc, int sx, int sy,
	int w, int h
	)
	{
	if ( sx < 0 || sx+w > modSrc->gbm.w ||
	     sy < 0 || sy+h > modSrc->gbm.h ||
	     dx < 0 || dx+w > modDst->gbm.w ||
	     dy < 0 || dy+h > modDst->gbm.h )
		return MOD_ERR_CLIP;
	gbm_blit(
		modSrc->pbData, modSrc->gbm.w, sx, sy,
		modDst->pbData, modDst->gbm.w, dx, dy,
		w, h,
		modSrc->gbm.bpp);
	return MOD_ERR_OK;
	}
/*...e*/
/*...sModColourAdjust:0:*/
/*...smap_compute:0:*/
/*...slstar_from_i:0:*/
static double lstar_from_i(double y)
	{
	y = pow(1.16 * y, 1.0/3.0) - 0.16;
	if ( y < 0.0 ) y = 0.0; else if ( y > 1.0 ) y = 1.0;
	return y;
	}
/*...e*/
/*...si_from_lstar:0:*/
static double i_from_lstar(double y)
	{
	y = pow(y + 0.16, 3.0) / 1.16;
	if ( y < 0.0 ) y = 0.0; else if ( y > 1.0 ) y = 1.0;
	return y;
	}
/*...e*/
/*...spal_from_i:0:*/
static double pal_from_i(double y, double gam, double shelf)
	{
	y = pow(y,1.0 / gam) * (1.0 - shelf) + shelf;
	if ( y < 0.0 ) y = 0.0; else if ( y > 1.0 ) y = 1.0;
	return y;
	}
/*...e*/
/*...si_from_pal:0:*/
static double i_from_pal(double y, double gam, double shelf)
	{
	if ( y >= shelf )
		y = pow((y - shelf) / (1.0 - shelf), gam);
	else
		y = 0.0;
	if ( y < 0.0 ) y = 0.0; else if ( y > 1.0 ) y = 1.0;
	return y;
	}
/*...e*/

static void map_compute(int m, gbm_u8 remap[], double gam, double shelf)
	{
	int	i;

	for ( i = 0; i < 0x100; i++ )
		{
		double y = (double) i / 255.0;

		switch ( m )
			{
			case CVT_I_TO_P: y = pal_from_i(y, gam, shelf); break;
			case CVT_P_TO_I: y = i_from_pal(y, gam, shelf); break;
			case CVT_I_TO_L: y = lstar_from_i(y); break;
			case CVT_L_TO_I: y = i_from_lstar(y); break;
			case CVT_P_TO_L: y = lstar_from_i(i_from_pal(y, gam, shelf)); break;
			case CVT_L_TO_P: y = pal_from_i(i_from_lstar(y), gam, shelf); break;
			}

		remap[i] = (gbm_u8) (y * 255.0);
		}
	}
/*...e*/
/*...smap_data:0:*/
static void map_data(gbm_u8 *data, int w, int h, const gbm_u8 remap[])
	{
	int stride = ((w * 3 + 3) & ~3);
	int x, y;

	for ( y = 0; y < h; y++, data += stride )
		for ( x = 0; x < w * 3; x++ )
			data[x] = remap[data[x]];
	}
/*...e*/
/*...smap_palette:0:*/
static void map_palette(GBMRGB *gbmrgb, int npals, const gbm_u8 remap[])
	{
	for ( ; npals--; gbmrgb++ )
		{
		gbmrgb->b = remap[gbmrgb->b];
		gbmrgb->g = remap[gbmrgb->g];
		gbmrgb->r = remap[gbmrgb->r];
		}
	}
/*...e*/

MOD_ERR ModColourAdjust(
	const MOD *mod,
	int map, double gama, double shelf,
	MOD *modNew
	)
	{
	MOD_ERR mrc;
	BYTE abRemap[0x100];

	if ( (mrc = ModCopy(mod, modNew)) != MOD_ERR_OK )
		return mrc;

	map_compute(map, abRemap, gama, shelf);

	if ( mod->gbm.bpp == 24 )
		map_data(modNew->pbData, modNew->gbm.w, modNew->gbm.h, abRemap);
	else
		map_palette(modNew->gbmrgb, 1 << modNew->gbm.bpp, abRemap);

	return MOD_ERR_OK;
	}
/*...e*/
/*...sModBppMap:0:*/
/*...sBppMap:0:*/
/*...sToGreyPal:0:*/
static VOID ToGreyPal(GBMRGB *gbmrgb)
	{
	int i;

	for ( i = 0; i < 0x100; i++ )
		gbmrgb[i].r =
		gbmrgb[i].g =
		gbmrgb[i].b = (gbm_u8) i;
	}
/*...e*/
/*...sToGrey:0:*/
static VOID ToGrey(GBM *gbm, const gbm_u8 *src_data, gbm_u8 *dest_data)
	{
	int src_stride  = ((gbm -> w * 3 + 3) & ~3);
	int dest_stride = ((gbm -> w     + 3) & ~3);
	int y;

	for ( y = 0; y < gbm -> h; y++ )
		{
		const gbm_u8 *src  = src_data;
		      gbm_u8 *dest = dest_data;
		int x;

		for ( x = 0; x < gbm -> w; x++ )
			{
			gbm_u8 b = *src++;
			gbm_u8 g = *src++;
			gbm_u8 r = *src++;

			*dest++ = (gbm_u8) (((gbm_u16) r * 77 + (gbm_u16) g * 151 + (gbm_u16) b * 28) >> 8);
			}

		src_data  += src_stride;
		dest_data += dest_stride;
		}
	gbm -> bpp = 8;
	}
/*...e*/
/*...sTripelPal:0:*/
static VOID TripelPal(GBMRGB *gbmrgb)
	{
	int i;

	memset(gbmrgb, 0, 0x100 * sizeof(GBMRGB));

	for ( i = 0; i < 0x40; i++ )
		{
		gbmrgb[i       ].r = (gbm_u8) (i << 2);
		gbmrgb[i + 0x40].g = (gbm_u8) (i << 2);
		gbmrgb[i + 0x80].b = (gbm_u8) (i << 2);
		}
	}
/*...e*/
/*...sTripel:0:*/
static VOID Tripel(GBM *gbm, const gbm_u8 *src_data, gbm_u8 *dest_data)
	{
	int src_stride  = ((gbm -> w * 3 + 3) & ~3);
	int dest_stride = ((gbm -> w     + 3) & ~3);
	int y;

	for ( y = 0; y < gbm -> h; y++ )
		{
		const gbm_u8 *src  = src_data;
		      gbm_u8 *dest = dest_data;
		int x;

		for ( x = 0; x < gbm -> w; x++ )
			{
			gbm_u8 b = *src++;
			gbm_u8 g = *src++;
			gbm_u8 r = *src++;

			switch ( (x+y)%3 )
				{
				case 0:	*dest++ = (gbm_u8)         (r >> 2) ;	break;
				case 1:	*dest++ = (gbm_u8) (0x40 + (g >> 2));	break;
				case 2:	*dest++ = (gbm_u8) (0x80 + (b >> 2));	break;
				}
			}

		src_data  += src_stride;
		dest_data += dest_stride;
		}
	gbm -> bpp = 8;
	}
/*...e*/

static BOOL BppMap(
	const MOD *mod24,
	int iPal, int iAlg,
	int iKeepRed, int iKeepGreen, int iKeepBlue, int nCols,
	MOD *modNew
	)
	{
	BYTE rm = (BYTE) (0xff00 >> iKeepRed  );
	BYTE gm = (BYTE) (0xff00 >> iKeepGreen);
	BYTE bm = (BYTE) (0xff00 >> iKeepBlue );
	BOOL ok = TRUE;

#define	SW2(a,b) (((a)<<8)|(b))
	switch ( SW2(iPal,iAlg) )
		{
		case SW2(CVT_BW,CVT_NEAREST):
			gbm_trunc_pal_BW(modNew->gbmrgb);
			gbm_trunc_BW(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_4G,CVT_NEAREST):
			gbm_trunc_pal_4G(modNew->gbmrgb);
			gbm_trunc_4G(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_8,CVT_NEAREST):
			gbm_trunc_pal_8(modNew->gbmrgb);
			gbm_trunc_8(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_VGA,CVT_NEAREST):
			gbm_trunc_pal_VGA(modNew->gbmrgb);
			gbm_trunc_VGA(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_784,CVT_NEAREST):
			gbm_trunc_pal_7R8G4B(modNew->gbmrgb);
			gbm_trunc_7R8G4B(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_666,CVT_NEAREST):
			gbm_trunc_pal_6R6G6B(modNew->gbmrgb);
			gbm_trunc_6R6G6B(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_444,CVT_NEAREST):
			gbm_trunc_pal_4R4G4B(modNew->gbmrgb);
			gbm_trunc_4R4G4B(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_8G,CVT_NEAREST):
			ToGreyPal(modNew->gbmrgb);
			ToGrey(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_TRIPEL,CVT_NEAREST):
			TripelPal(modNew->gbmrgb);
			Tripel(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_FREQ,CVT_NEAREST):
			memset(modNew->gbmrgb, 0, sizeof(modNew->gbmrgb));
			gbm_hist(&(modNew->gbm), mod24->pbData, modNew->gbmrgb, modNew->pbData, nCols, rm, gm, bm);
			break;
		case SW2(CVT_MCUT,CVT_NEAREST):
			memset(modNew->gbmrgb, 0, sizeof(modNew->gbmrgb));
			gbm_mcut(&(modNew->gbm), mod24->pbData, modNew->gbmrgb, modNew->pbData, nCols);
			break;
		case SW2(CVT_RGB,CVT_NEAREST):
			gbm_trunc_24(&(modNew->gbm), mod24->pbData, modNew->pbData, rm, gm, bm);
			break;
		case SW2(CVT_BW,CVT_ERRDIFF):
			gbm_errdiff_pal_BW(modNew->gbmrgb);
			ok = gbm_errdiff_BW(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_4G,CVT_ERRDIFF):
			gbm_errdiff_pal_4G(modNew->gbmrgb);
			ok = gbm_errdiff_4G(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_8,CVT_ERRDIFF):
			gbm_errdiff_pal_8(modNew->gbmrgb);
			ok = gbm_errdiff_8(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_VGA,CVT_ERRDIFF):
			gbm_errdiff_pal_VGA(modNew->gbmrgb);
			ok = gbm_errdiff_VGA(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_784,CVT_ERRDIFF):
			gbm_errdiff_pal_7R8G4B(modNew->gbmrgb);
			ok = gbm_errdiff_7R8G4B(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_666,CVT_ERRDIFF):
			gbm_errdiff_pal_6R6G6B(modNew->gbmrgb);
			ok = gbm_errdiff_6R6G6B(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_444,CVT_ERRDIFF):
			gbm_errdiff_pal_4R4G4B(modNew->gbmrgb);
			ok = gbm_errdiff_4R4G4B(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_RGB,CVT_ERRDIFF):
			ok = gbm_errdiff_24(&(modNew->gbm), mod24->pbData, modNew->pbData, rm, gm, bm);
			break;
		case SW2(CVT_784,CVT_HALFTONE):
			gbm_ht_pal_7R8G4B(modNew->gbmrgb);
			gbm_ht_7R8G4B_2x2(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_666,CVT_HALFTONE):
			gbm_ht_pal_6R6G6B(modNew->gbmrgb);
			gbm_ht_6R6G6B_2x2(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_444,CVT_HALFTONE):
			gbm_ht_pal_4R4G4B(modNew->gbmrgb);
			gbm_ht_4R4G4B_2x2(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_8,CVT_HALFTONE):
			gbm_ht_pal_8(modNew->gbmrgb);
			gbm_ht_8_3x3(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_VGA,CVT_HALFTONE):
			gbm_ht_pal_VGA(modNew->gbmrgb);
			gbm_ht_VGA_3x3(&(modNew->gbm), mod24->pbData, modNew->pbData);
			break;
		case SW2(CVT_RGB,CVT_HALFTONE):
			gbm_ht_24_2x2(&(modNew->gbm), mod24->pbData, modNew->pbData, rm, gm, bm);
			break;
		}
	return ok;
	}
/*...e*/

MOD_ERR ModBppMap(
	const MOD *mod,
	int iPal, int iAlg,
	int iKeepRed, int iKeepGreen, int iKeepBlue, int nCols,
	MOD *modNew
	)
	{
	MOD_ERR mrc;
	int newbpp;
	BOOL fOk;

	switch ( iPal )
		{
		case CVT_BW:
			newbpp = 1;
			break;
		case CVT_4G:
		case CVT_8:
		case CVT_VGA:
			newbpp = 4;
			break;
		case CVT_784:
		case CVT_666:
		case CVT_444:
		case CVT_8G:
		case CVT_TRIPEL:
		case CVT_FREQ:
		case CVT_MCUT:
			newbpp = 8;
			break;
		case CVT_RGB:
			newbpp = 24;
			break;
		default:
			return MOD_ERR_SUPPORT;
		}

	/* The following breakdown into cases can result in significant
	   savings in memory by eliminating costly intermediate bitmaps. */

	if ( mod->gbm.bpp == 24 )
		/* 24bpp -> anything, map direct from mod */
		{
		if ( (mrc = ModCreate(mod->gbm.w, mod->gbm.h, newbpp, NULL, modNew)) != MOD_ERR_OK )
			return mrc;
		fOk = BppMap(mod, iPal, iAlg, iKeepRed, iKeepGreen, iKeepBlue, nCols, modNew);
		}
	else if ( newbpp == 24 )
		/* !24 -> 24bpp, expand mod to modNew, map modNew inline */
		{
		if ( (mrc = ModExpandTo24Bpp(mod, modNew)) != MOD_ERR_OK )
			return mrc;
		fOk = BppMap(modNew, iPal, iAlg, iKeepRed, iKeepGreen, iKeepBlue, nCols, modNew);
		}
	else
		/* !24bpp -> 24bpp, expand mod to mod24, and map mod24 to modNew */
		{
		MOD mod24;
		if ( (mrc = ModCreate(mod->gbm.w, mod->gbm.h, newbpp, NULL, modNew)) != MOD_ERR_OK )
			return mrc;
		if ( (mrc = ModExpandTo24Bpp(mod, &mod24)) != MOD_ERR_OK )
			{
			ModDelete(modNew);
			return mrc;
			}
		fOk = BppMap(&mod24, iPal, iAlg, iKeepRed, iKeepGreen, iKeepBlue, nCols, modNew);
		ModDelete(&mod24);
		}

	return fOk ? MOD_ERR_OK : MOD_ERR_MEM;
	}
/*...e*/


/*...sModFixRedeye:0:*/
MOD_ERR ModFixRedeye(const MOD *mod, MOD *modNew)
{
    int y, w, h;
    int stride;
    MOD_ERR mrc;

    if (mod->gbm.bpp != 24)
    {
        return MOD_ERR_GBM(GBM_ERR_NOT_SUPP);
    }
    if ((mrc = ModCopy(mod, modNew)) != MOD_ERR_OK)
    {
        return mrc;
    }

    w = mod->gbm.w;
    h = mod->gbm.h;
    stride = ((w * mod->gbm.bpp + 31) / 32) * 4;
    for (y = 0; y < h; y++)
    {
        int x;
        const int rowOffset = y * stride;
        const gbm_u8 * s8 = mod->pbData    + rowOffset;
              gbm_u8 * d8 = modNew->pbData + rowOffset;
        for (x = 0; x < w; x++)
        {
            gbm_u8 blue  = *s8++;
            gbm_u8 green = *s8++;
            gbm_u8 red   = *s8++;

            if ((red > (green + 0x30)) && (red > (blue + 0x30)))
            {
                if ((green >= 0x80) && (blue >= 0x80))
                {
                    green = red;
                    blue  = red;
                }
                else /* if ((green < 0x80) || (blue < 0x80)) */
                {
                    if (green < blue)
                        red = green + 30;
                    else
                        red = blue + 30;
                }
            }
            *d8++ = blue;
            *d8++ = green;
            *d8++ = red;
        }
    }
    return MOD_ERR_OK;
}
/*...e*/

/*...sModResize:0:*/
MOD_ERR ModResize(
	const MOD *mod,
	int nw, int nh,
	BOOL isGrayscale,
	BOOL useScaleFilter,
	GBM_SCALE_FILTER scaleFilter,
	MOD *modNew
	)
	{
	MOD_ERR mrc;
	GBM_ERR grc;

    if (useScaleFilter && ((mod->gbm.bpp <= 8) && isGrayscale))
    {
        if ( (mrc = ModCreate(nw, nh, 8 /* bpp */, mod->gbmrgb, modNew)) != MOD_ERR_OK )
            return mrc;
    }
    else
    {
        if ( (mrc = ModCreate(nw, nh, mod->gbm.bpp, mod->gbmrgb, modNew)) != MOD_ERR_OK )
            return mrc;
    }

    if (useScaleFilter)
    {
      /* check if 8bpp grayscale */
      if ((mod->gbm.bpp <= 8) && isGrayscale)
      {
        grc = gbm_quality_scale_gray(mod   ->pbData, mod   ->gbm.w, mod   ->gbm.h, mod->gbm.bpp, mod->gbmrgb,
                                     modNew->pbData, modNew->gbm.w, modNew->gbm.h, modNew->gbmrgb,
                                     scaleFilter);
      }
      else
      {
        grc = gbm_quality_scale_bgra(mod   ->pbData, mod   ->gbm.w, mod   ->gbm.h,
                                     modNew->pbData, modNew->gbm.w, modNew->gbm.h,
                                     mod->gbm.bpp, scaleFilter);
      }
    }
    else
    {
      grc = gbm_simple_scale(mod   ->pbData, mod   ->gbm.w, mod   ->gbm.h,
                             modNew->pbData, modNew->gbm.w, modNew->gbm.h,
                             mod->gbm.bpp);
    }
	if ( grc != GBM_ERR_OK )
		{
		ModDelete(modNew);
		return MOD_ERR_GBM(grc);
		}

	return MOD_ERR_OK;
	}
/*...e*/

/*...sModCreateFromHPS:0:*/
MOD_ERR ModCreateFromHPS(
	HPS hps, int w, int h, int bpp,
	MOD *modNew
	)
	{
	MOD_ERR mrc;

    #pragma pack(2)
	struct
		{
		BITMAPINFOHEADER2 bmp2;
		RGB2 argb2Color[0x100];
		} bm;
    #pragma pack()

	if ( (mrc = ModCreate(w, h, bpp, NULL, modNew)) != MOD_ERR_OK )
		return mrc;

	memset(&(bm.bmp2), 0, sizeof(bm.bmp2));
	bm.bmp2.cbFix     = sizeof(BITMAPINFOHEADER2);
	bm.bmp2.cx        = w;
	bm.bmp2.cy        = h;
	bm.bmp2.cBitCount = bpp;
	bm.bmp2.cPlanes   = 1;
	GpiQueryBitmapBits(hps, 0L, h, modNew->pbData, (BITMAPINFO2 *) &bm);

	if ( bpp != 24 )
		{
		int i;
		for ( i = 0; i < (1<<bpp); i++ )
			{
			modNew->gbmrgb[i].r = bm.argb2Color[i].bRed  ;
			modNew->gbmrgb[i].g = bm.argb2Color[i].bGreen;
			modNew->gbmrgb[i].b = bm.argb2Color[i].bBlue ;
			}
		}

	return MOD_ERR_OK;
	}
/*...e*/

/*...sModMakeHBITMAP:0:*/
MOD_ERR ModMakeHBITMAP(
	const MOD *mod,
	HAB hab,
	HBITMAP *phbm
	)
	{
	SIZEL sizl;
	HDC hdc;
	HPS hps;
    MOD modTmp;
    const MOD *modSrc = mod;

    #pragma pack(2)
	struct
		{
		BITMAPINFOHEADER2 bmp2;
		RGB2 argb2Color[0x100];
		} bm;
    #pragma pack()

	/* Got the data in memory, now make bitmap */

	memset(&(bm.bmp2), 0, sizeof(bm.bmp2));
	bm.bmp2.cbFix     = sizeof(BITMAPINFOHEADER2);
	bm.bmp2.cx        = mod->gbm.w;
	bm.bmp2.cy        = mod->gbm.h;
	bm.bmp2.cBitCount = mod->gbm.bpp;
	bm.bmp2.cPlanes   = 1;

    if ((mod->gbm.bpp <= 8) && (mod->gbm.bpp > 0))
    {
        const int entries = 1 << mod->gbm.bpp;
        int i;

        /* check for Black&White bitmap which needs special data ordering for drawing */
        if ((entries == 2) &&
            (mod->gbmrgb[0].r == mod->gbmrgb[0].g) && (mod->gbmrgb[0].r == mod->gbmrgb[0].b) && (mod->gbmrgb[0].g == mod->gbmrgb[0].b) &&
            (mod->gbmrgb[1].r == mod->gbmrgb[1].g) && (mod->gbmrgb[1].r == mod->gbmrgb[1].b) && (mod->gbmrgb[1].g == mod->gbmrgb[1].b) &&
            (mod->gbmrgb[0].r != 255))
        {
            const MOD_ERR mrc = ModCopy(mod, &modTmp);
            if (mrc == MOD_ERR_OK)
            {
                const size_t stride = (((size_t)modTmp.gbm.w * modTmp.gbm.bpp + 31)/32) * 4;
                const size_t length   = stride * modTmp.gbm.h;
                const size_t numWords = length / sizeof(unsigned long *);
                const size_t numBytes = length - (numWords * sizeof(unsigned long *));
                size_t word, byte;
                unsigned long * dataWord;
                gbm_u8        * dataByte;

                /* swap color table entries */
                modTmp.gbmrgb[0] = mod->gbmrgb[1];
                modTmp.gbmrgb[1] = mod->gbmrgb[0];

                /* invert data using long pointer for better performance */
                dataWord = (unsigned long *)(modTmp.pbData);
                for (word = 0; word < numWords; ++word, ++dataWord)
                {
                    *dataWord = ~(*dataWord);
                }
                dataByte = (gbm_u8 *)dataWord;
                for (byte = 0; byte < numBytes; ++byte, ++dataByte)
                {
                    *dataByte = ~(*dataByte);
                }
                modSrc = &modTmp;
            }
            else
            {
                ModDelete(&modTmp);
                return mrc;
            }
        }

        for ( i = 0; i < entries; i++ )
        {
            bm.argb2Color[i].bRed      = modSrc->gbmrgb[i].r;
            bm.argb2Color[i].bGreen    = modSrc->gbmrgb[i].g;
            bm.argb2Color[i].bBlue     = modSrc->gbmrgb[i].b;
            bm.argb2Color[i].fcOptions = 0;
        }
    }

    if ( (hdc = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL)) == (HDC) NULL )
    {
        if (modSrc == &modTmp)
        {
            ModDelete(&modTmp);
        }
        return MOD_ERR_HDC;
    }

    sizl.cx = modSrc->gbm.w;
    sizl.cy = modSrc->gbm.h;
    if ( (hps = GpiCreatePS(hab, hdc, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
    {
        if (modSrc == &modTmp)
        {
            ModDelete(&modTmp);
        }
        DevCloseDC(hdc);
        return MOD_ERR_HPS;
    }

    if ( modSrc->gbm.bpp == 1 )
    {
        const RGB2 * pRGBA = bm.argb2Color;
        const LONG colorFg = (pRGBA[0].bRed << 16) + (pRGBA[0].bGreen << 8) + pRGBA[0].bBlue;
        const LONG colorBg = (pRGBA[1].bRed << 16) + (pRGBA[1].bGreen << 8) + pRGBA[1].bBlue;

        LONG colorTable[2];
        colorTable[0] = colorBg;
        colorTable[1] = colorFg;

        GpiCreateLogColorTable(hps, 0, LCOLF_CONSECRGB, 0, 2, (PLONG) colorTable);
        /* if something goes wrong when creating color table, use the closest color */
        GpiSetBackColor(hps, GpiQueryColorIndex(hps, 0, colorBg));
        GpiSetColor    (hps, GpiQueryColorIndex(hps, 0, colorFg));
    }

    if ( (*phbm = GpiCreateBitmap(hps, &(bm.bmp2), CBM_INIT, modSrc->pbData, (BITMAPINFO2 *) &(bm.bmp2))) == (HBITMAP) NULL )
    {
        if (modSrc == &modTmp)
        {
            ModDelete(&modTmp);
        }
        GpiDestroyPS(hps);
        DevCloseDC(hdc);
        return MOD_ERR_HBITMAP;
    }

    GpiSetBitmap(hps, (HBITMAP) NULL);
    GpiDestroyPS(hps);
    DevCloseDC(hdc);

    return MOD_ERR_OK;
}

/*...e*/

/*...sModMakeBITMAP:0:*/
MOD_ERR ModMakeBITMAP(
	const MOD *mod,
	BITMAP *pbmp
	)
	{
	/* fill bitmap data */

	memset(&(pbmp->bmp2), 0, sizeof(BITMAPINFOHEADER2));
	pbmp->bmp2.cbFix     = sizeof(BITMAPINFOHEADER2);
	pbmp->bmp2.cx        = mod->gbm.w;
	pbmp->bmp2.cy        = mod->gbm.h;
	pbmp->bmp2.cBitCount = mod->gbm.bpp;
	pbmp->bmp2.cPlanes   = 1;

    if ((mod->gbm.bpp <= 8) && (mod->gbm.bpp > 0))
    {
        const int entries = 1 << mod->gbm.bpp;
        int i;

        /* check for Black&White bitmap which needs special data ordering for drawing */
        if ((entries == 2) &&
            (mod->gbmrgb[0].r == mod->gbmrgb[0].g) && (mod->gbmrgb[0].r == mod->gbmrgb[0].b) && (mod->gbmrgb[0].g == mod->gbmrgb[0].b) &&
            (mod->gbmrgb[1].r == mod->gbmrgb[1].g) && (mod->gbmrgb[1].r == mod->gbmrgb[1].b) && (mod->gbmrgb[1].g == mod->gbmrgb[1].b) &&
            (mod->gbmrgb[0].r != 255))
        {
            const MOD_ERR mrc = ModCopy(mod, &(pbmp->mod));
            if (mrc == MOD_ERR_OK)
            {
                const size_t stride   = (((size_t)pbmp->mod.gbm.w * pbmp->mod.gbm.bpp + 31)/32) * 4;
                const size_t length   = stride * pbmp->mod.gbm.h;
                const size_t numWords = length / sizeof(unsigned long *);
                const size_t numBytes = length - (numWords * sizeof(unsigned long *));
                size_t word, byte;
                unsigned long * dataWord;
                gbm_u8        * dataByte;

                /* swap color table entries */
                pbmp->mod.gbmrgb[0] = mod->gbmrgb[1];
                pbmp->mod.gbmrgb[1] = mod->gbmrgb[0];

                pbmp->argb2Color[0].bRed      = pbmp->mod.gbmrgb[0].r;
                pbmp->argb2Color[0].bGreen    = pbmp->mod.gbmrgb[0].g;
                pbmp->argb2Color[0].bBlue     = pbmp->mod.gbmrgb[0].b;
                pbmp->argb2Color[0].fcOptions = 0;
                pbmp->argb2Color[1].bRed      = pbmp->mod.gbmrgb[1].r;
                pbmp->argb2Color[1].bGreen    = pbmp->mod.gbmrgb[1].g;
                pbmp->argb2Color[1].bBlue     = pbmp->mod.gbmrgb[1].b;
                pbmp->argb2Color[1].fcOptions = 0;

                /* invert data using long pointer for better performance */
                dataWord = (unsigned long *)(pbmp->mod.pbData);
                for (word = 0; word < numWords; ++word, ++dataWord)
                {
                    *dataWord = ~(*dataWord);
                }
                dataByte = (gbm_u8 *)dataWord;
                for (byte = 0; byte < numBytes; ++byte, ++dataByte)
                {
                    *dataByte = ~(*dataByte);
                }
            }
            return mrc;
        }

        for ( i = 0; i < entries; i++ )
        {
            pbmp->argb2Color[i].bRed      = mod->gbmrgb[i].r;
            pbmp->argb2Color[i].bGreen    = mod->gbmrgb[i].g;
            pbmp->argb2Color[i].bBlue     = mod->gbmrgb[i].b;
            pbmp->argb2Color[i].fcOptions = 0;
        }
    }

    return ModCopy(mod, &(pbmp->mod));
}

/*...e*/
/*...sModMakeHMF:0:*/
/*
I have observed that if I use GpiBitBlt() instead of the GpiWCBitBlt() below,
when the MetaFile is pasted into CUADraw, although CUADraws sizing frame
appears in the middle of the window, the bitmap bits are always drawn at (0,0)
on CUADraws client window. This is why I use GpiWCBitBlt(). Also, in the
PM Programming Reference, under DevOpenDC is a comment suggesting that
GpiBitBlt() should not be used in order to be SAA conforming, presumably there
is a connection here...
*/

MOD_ERR ModMakeHMF(
	HBITMAP hbm,
	HAB hab,
	HMF *phmf
	)
	{
	DEVOPENSTRUC dop = { NULL, "DISPLAY", NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	SIZEL sizl;
	HPS hpsHmf;
	HDC hdcHmf;
	BITMAPINFOHEADER bmp;
	POINTL aptl[4];

	if ( (hdcHmf = DevOpenDC(hab, OD_METAFILE, "*", 5L, (PDEVOPENDATA)&dop, (HDC) NULL)) == (HDC) NULL )
		return MOD_ERR_HDC;

	GpiQueryBitmapParameters(hbm, &bmp);

	sizl.cx = bmp.cx;
	sizl.cy = bmp.cy;
	if ( (hpsHmf = GpiCreatePS(hab, hdcHmf, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
		{
		DevCloseDC(hdcHmf);
		return MOD_ERR_HPS;
		}

	aptl[0].x = 0;
	aptl[0].y = 0;
	aptl[1].x = bmp.cx;
	aptl[1].y = bmp.cy;
	aptl[2].x = 0;
	aptl[2].y = 0;
	aptl[3].x = bmp.cx;
	aptl[3].y = bmp.cy;
	GpiWCBitBlt(hpsHmf, hbm, 4L, aptl, ROP_SRCCOPY, BBO_IGNORE);

	GpiDestroyPS(hpsHmf);
	(*phmf) = DevCloseDC(hdcHmf);

	if ( (*phmf) == DEV_ERROR )
		return MOD_ERR_HMF;

	return MOD_ERR_OK;
	}
/*...e*/
/*...sModErrorString:0:*/
const CHAR * ModErrorString(MOD_ERR rc)
	{
	if ( rc >= MOD_ERR_GBM(0) )
		return gbm_err(rc-MOD_ERR_GBM(0));
	switch ( (int) rc )
		{
		case MOD_ERR_OK:	return "no error";
		case MOD_ERR_MEM:	return "out of memory";
		case MOD_ERR_OPEN:	return "can't open file";
		case MOD_ERR_CREATE:	return "can't create file";
		case MOD_ERR_READ_BPP:	return "bits per pixel is not supported";
		case MOD_ERR_SUPPORT:	return "file format doesn't support this bits per pixel";
		case MOD_ERR_HDC:	return "can't create PM device context resource";
		case MOD_ERR_HPS:	return "can't create PM presentation space resource";
		case MOD_ERR_HMF:	return "can't create PM metafile resource";
		case MOD_ERR_HBITMAP:	return "can't create PM bitmap resource";
		case MOD_ERR_CLIP:	return "coords need to be clipped";
		}
	return "unknown error code";
	}
/*...e*/
