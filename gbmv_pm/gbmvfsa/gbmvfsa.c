/*

gbmvfsa.c - GBM View Full Screen Animation

History:
--------
(Heiko Nitzsche)

26-Apr-2006: Fix issue with comma separation between file and options.
             Now the file can have quotes and thus clearly separating
             it from the options.
             On OS/2 command line use: "\"file.ext\",options"

15-Aug-2008: Integrate new GBM types

10-Oct-2008: Changed recommended file specification template to
             "file.ext"\",options   or   "file.ext"\",\""options"
*/

/*...sincludes:0:*/
#define INCL_BASE
#include <os2.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gbm.h"
#include "gbmtool.h"
/*...e*/

static char progname[] = "gbmvfsa";

/*...sfatal:0:*/
static void fatal(const char *fmt, ...)
	{
	va_list	vars;
	char s[256+1];

	va_start(vars, fmt);
	vsprintf(s, fmt, vars);
	va_end(vars);
	fprintf(stderr, "%s: %s\n", progname, s);
	exit(1);
	}
/*...e*/
/*...susage:0:*/
static void usage(void)
	{
	int ft, n_ft;

	fprintf(stderr, "usage: %s [-l] [-p] [-s] [-t] [-P] [-b border] n1 n2 n3 \"fspec\"{\\\",\\\"\"opt\"}\n", progname);
	fprintf(stderr, "flags: -l             loop indefinately\n");
	fprintf(stderr, "       -p             set palette only once, assume same\n");
	fprintf(stderr, "       -s             single step the frames\n");
	fprintf(stderr, "       -t             annotate each frame with its title\n");
	fprintf(stderr, "       -P             display palette bar\n");
	fprintf(stderr, "       -b border      set border colour index (default 0)\n");
	fprintf(stderr, "       n1 n2 n3       for ( f=n1; f<n2; f+=n3 )\n");
	fprintf(stderr, "       fspec            printf(fspec, f);\n");
	fprintf(stderr, "                      filespecs are of the form fn.ext\n");
	fprintf(stderr, "                      ext's are used to deduce desired bitmap file formats\n");

	gbm_init();
	gbm_query_n_filetypes(&n_ft);
	for ( ft = 0; ft < n_ft; ft++ )
		{
		GBMFT gbmft;

		gbm_query_filetype(ft, &gbmft);
		fprintf(stderr, "                      %s when ext in [%s]\n",
			gbmft.short_name, gbmft.extensions);
		}
	gbm_deinit();

	fprintf(stderr, "       opt's          bitmap format specific options\n");
	fprintf(stderr, "   eg: %s 0 100 1 frame%%03d.bmp\n", progname);

	fprintf(stderr, "\n       In case fspec contains a comma or spaces and options\n");
	fprintf(stderr,   "       need to be added, syntax \"fspec\"{\\\",\\\"opt} or \"fspec\"{\\\",\\\"\"opt\"}\n");
	fprintf(stderr,   "       used to clearly separate the file spec from the options.\n");

	exit(1);
	}
/*...e*/
/*...sget_opt_value:0:*/
static int get_opt_value(const char *s, const char *name)
	{
	int v;

	if ( s == NULL )
		fatal("missing %s argument", name);
	if ( !isdigit(s[0]) )
		fatal("bad %s argument", name);
	if ( s[0] == '0' && tolower(s[1]) == 'x' )
		sscanf(s + 2, "%x", &v);
	else
		v = atoi(s);

	return v;
	}
/*...e*/
/*...sget_opt_value_pos:0:*/
static int get_opt_value_pos(const char *s, const char *name)
	{
	int n;

	if ( (n = get_opt_value(s, name)) < 0 )
		fatal("%s should not be negative", name);
	return n;
	}
/*...e*/
/*...smain:0:*/
#define	SCN_W 320
#define	SCN_H 200
static gbm_u8 data[SCN_W*SCN_H];
/*...sread_bitmap_header:0:*/
static gbm_boolean read_bitmap_header(
	const char *fn, const char *opt,
	GBM *gbm, GBMRGB gbmrgb[]
	)
	{
	int ft, fd;

	if ( gbm_guess_filetype(fn, &ft) != GBM_ERR_OK )
		return GBM_FALSE;

	if ( (fd = gbm_io_open(fn, GBM_O_RDONLY)) == -1 )
		return GBM_FALSE;

	if ( gbm_read_header(fn, fd, ft, gbm, opt) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		return GBM_FALSE;
		}

	if ( gbm->bpp != 8 || gbm->w > SCN_W || gbm->h > SCN_H )
		{
		gbm_io_close(fd);
		return GBM_FALSE;
		}

	if ( gbm_read_palette(fd, ft, gbm, gbmrgb) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		return GBM_FALSE;
		}

	gbm_io_close(fd);
	return GBM_TRUE;
	}
/*...e*/
/*...sread_bitmap:0:*/
static gbm_boolean read_bitmap(
	const char *fn, const char *opt,
	GBM *gbm, GBMRGB gbmrgb[]
	)
	{
	int ft, fd;

	if ( gbm_guess_filetype(fn, &ft) != GBM_ERR_OK )
		return GBM_FALSE;

	if ( (fd = gbm_io_open(fn, GBM_O_RDONLY)) == -1 )
		return GBM_FALSE;

	if ( gbm_read_header(fn, fd, ft, gbm, opt) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		return GBM_FALSE;
		}

	if ( gbm->bpp != 8 || gbm->w > SCN_W || gbm->h > SCN_H )
		{
		gbm_io_close(fd);
		return GBM_FALSE;
		}

	if ( gbm_read_palette(fd, ft, gbm, gbmrgb) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		return GBM_FALSE;
		}

	if ( gbm_read_data(fd, ft, gbm, data) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		return GBM_FALSE;
		}

	gbm_io_close(fd);
	return GBM_TRUE;
	}
/*...e*/
/*...sscn:0:*/
static VIOMODEINFO vmiOld;
static BYTE *ptr;

/*...sscn_border:0:*/
static void scn_border(gbm_u8 border)
	{
	VIOOVERSCAN vioos;

	vioos.cb = sizeof(VIOOVERSCAN);
	vioos.type = 1; /* Apparently no #define for this */
	vioos.color = border;
	VioSetState(&vioos, (HVIO) 0);
	}
/*...e*/
/*...sscn_palette:0:*/
static void scn_palette(BYTE *palette, int first, int num)
	{
	BYTE *buf = (BYTE *) palette;
	BYTE *_Seg16 buf1616 = (BYTE *_Seg16) buf;
	VIOCOLORREG viocreg;
	viocreg.cb = sizeof(VIOCOLORREG);
	viocreg.type = 3; /* Apparently no #define for this */
	viocreg.firstcolorreg = first;
	viocreg.numcolorregs = num;

	/* Note: viocreg.colorregaddr should be of type CHAR *_Seg16
	   But it isn't! Hence if I say viocreg.colorregaddr = buf,
	   then the correct thunking will not be applied! */
	memcpy(&viocreg.colorregaddr, (const void *) &buf1616, 4);

	VioSetState(&viocreg, (HVIO) 0);
	}
/*...e*/
/*...sscn_putimage:0:*/
/*
We are being asked to transfer a w*h image to a SCN_W*SCN_H screen.
If image is smaller than screen, then we will centralise it.
*/

static void scn_putimage(int w, int h, const unsigned char *data)
	{
	int stride = ((w+3)&~3);
	int xi = (SCN_W-w)/2;
	int yi = (SCN_H-h)/2;
	int y;
	unsigned char *scn = ptr + ((yi*SCN_W) + xi);
	data += (h-1) * stride;
	for ( y = 0; y < h; y++, data -= stride, scn += SCN_W )
		memcpy(scn, data, w);
		/* Note: memcpy expands inline to REP MOVSD under OS/2
		   using C-Set++, which is about as fast as you can get */
	}
/*...e*/
/*...sscn_init:0:*/
static void scn_init(void)
	{
	VIOMODEINFO vmi;
	VIOPHYSBUF phys;
	unsigned char *_Seg16 ptr1616;
	unsigned char status;

	vmi.cb     = 12;
	vmi.fbType = 3;
	vmi.color  = 8;
	vmi.col    = 40;
	vmi.row    = 25;
	vmi.hres   = SCN_W;
	vmi.vres   = SCN_H;
	VioGetMode(&vmiOld, (HVIO) 0);
	if ( VioSetMode(&vmi, (HVIO) 0) )
		fatal("can't enter 320x200 at 8bpp graphics mode");
	phys.cb = 0x10000;
	phys.pBuf = (PBYTE) 0xa0000;
	if ( VioGetPhysBuf(&phys, 0) )
		fatal("can't get access to physical graphics screen");
	ptr1616 = (unsigned char *_Seg16) ( phys.asel[0] << 16 );
	ptr = (unsigned char *) ptr1616;
	VioScrLock(VP_WAIT, &status, (HVIO) 0);

	/* Set all palette entrys to black */
	{
	static BYTE palette[0x100][3];
	memset(palette, 0, sizeof(palette));
	scn_palette((BYTE *) palette, 0, 0x100);
	}

	/* Set screen to colour 0, which will get set to black */
	memset(ptr, 0, SCN_W*SCN_H);
	}
/*...e*/
/*...sscn_term:0:*/
static void scn_term(void)
	{
	VioScrUnLock((HVIO) 0);
	VioSetMode(&vmiOld, (HVIO) 0);
	}
/*...e*/
/*...e*/
/*...ssetpal:0:*/
static void setpal(const GBMRGB gbmrgb[])
	{
	BYTE palette[0x100][3];
	int i;
	for ( i = 0; i < 0x100; i++ )
		{
		palette[i][0] = (gbmrgb[i].r>>2);
		palette[i][1] = (gbmrgb[i].g>>2);
		palette[i][2] = (gbmrgb[i].b>>2);
		}
	scn_palette((BYTE *) palette, 0, 0x100);
	}
/*...e*/
/*...stitle:0:*/
static char font[][8][8] =
	{
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,0,0,1,0,0,
	0,0,1,0,0,1,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,0,0,1,0,0,
	0,1,1,1,1,1,1,0,
	0,0,1,0,0,1,0,0,
	0,0,1,0,0,1,0,0,
	0,1,1,1,1,1,1,0,
	0,0,1,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,1,0,1,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,1,0,1,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,1,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,0,0,0,1,0,
	0,1,1,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,0,1,1,0,
	0,1,0,0,0,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,1,0,1,0,
	0,1,0,0,0,1,0,0,
	0,0,1,1,1,0,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,1,0,0,0,
	0,0,0,1,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,1,1,0,
	0,1,0,0,1,0,1,0,
	0,1,0,1,0,0,1,0,
	0,1,1,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,1,1,0,0,0,
	0,0,1,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,1,1,0,0,
	0,0,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,1,0,0,0,
	0,0,1,0,1,0,0,0,
	0,1,0,0,1,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,1,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,0,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,0,0,1,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,1,0,1,0,
	0,1,0,1,0,1,1,0,
	0,1,0,1,1,1,1,0,
	0,1,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,1,1,1,1,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,1,0,0,
	0,1,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,1,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,1,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,1,1,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,1,1,1,1,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,0,
	0,0,0,0,0,0,1,0,
	0,0,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,1,0,0,0,
	0,1,1,1,0,0,0,0,
	0,1,0,0,1,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,1,0,
	0,1,1,0,0,1,1,0,
	0,1,0,1,1,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,1,0,
	0,1,1,0,0,0,1,0,
	0,1,0,1,0,0,1,0,
	0,1,0,0,1,0,1,0,
	0,1,0,0,0,1,1,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,1,0,0,1,0,
	0,1,0,0,1,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,1,1,1,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,0,1,0,0,1,0,0,
	0,0,0,1,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,0,0,0,1,0,
	0,1,0,1,1,0,1,0,
	0,0,1,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,1,0,
	0,0,1,0,0,1,0,0,
	0,0,0,1,1,0,0,0,
	0,0,0,1,1,0,0,0,
	0,0,1,0,0,1,0,0,
	0,1,0,0,0,0,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	1,0,0,0,0,0,1,0,
	0,1,0,0,0,1,0,0,
	0,0,1,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,1,1,1,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,1,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,1,1,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,1,1,0,0,0,
	0,1,0,1,0,1,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,

	0,0,0,0,0,0,0,0,
	0,0,0,1,1,1,0,0,
	0,0,1,0,0,0,1,0,
	0,1,1,1,1,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,1,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,0,1,0,0,0,1,0,
	0,0,1,0,0,0,1,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,1,1,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,0,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,0,1,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,1,1,1,0,0,0,
	0,1,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,1,1,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,1,0,0,
	0,0,1,1,1,0,0,0,

	0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,1,1,1,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,0,1,0,0,
	0,0,1,0,0,1,0,0,
	0,0,0,1,1,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,0,1,0,0,0,
	0,0,1,1,0,0,0,0,
	0,0,1,1,0,0,0,0,
	0,0,1,0,1,0,0,0,
	0,0,1,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,1,0,1,0,0,0,
	0,1,0,1,0,1,0,0,
	0,1,0,1,0,1,0,0,
	0,1,0,1,0,1,0,0,
	0,1,0,1,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,1,1,1,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,1,1,1,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,1,1,1,0,0,0,
	0,1,0,0,0,0,0,0,
	0,1,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,0,1,1,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,1,1,1,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,1,1,1,0,0,0,
	0,1,0,0,0,0,0,0,
	0,0,1,1,1,0,0,0,
	0,0,0,0,0,1,0,0,
	0,1,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,1,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,0,1,0,0,0,
	0,0,1,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,1,0,1,0,0,
	0,1,0,1,0,1,0,0,
	0,1,0,1,0,1,0,0,
	0,0,1,0,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,1,0,0,0,
	0,1,0,0,0,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,
	0,0,1,1,1,1,0,0,
	0,0,0,0,0,1,0,0,
	0,0,1,1,1,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,1,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,1,1,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,0,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,0,0,1,1,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,0,0,1,0,0,
	0,0,0,0,1,0,0,0,
	0,0,1,1,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,1,1,0,0,1,0,
	0,1,0,0,1,1,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,1,0,0,0,
	0,0,0,1,0,1,0,0,
	0,0,1,0,0,0,1,0,
	0,0,1,0,0,0,1,0,
	0,0,1,1,1,1,1,0,
	0,0,0,0,0,0,0,0,
	};

static void plot_character(int x, int y, char c)
	{
	int i, j;
	if ( c < ' ' )
		return;
	for ( j = 0; j < 8; j++ )
		for ( i = 0; i < 8; i++ )
			if ( font[c-' '][j][i] )
				ptr[(y+j)*SCN_W+(x+i)] = 0;
			else
				ptr[(y+j)*SCN_W+(x+i)] = 0xff;
	}

static void plot_string(int x, int y, char *s)
	{
	while ( x+8 <= SCN_W && *s != '\0' )
		{
		plot_character(x, y, *s++);
		x += 8;
		}
	}
/*...e*/

int main(int argc, char *argv[])
	{
	GBMTOOL_FILEARG gbmfilearg;
	char fn[GBMTOOL_FILENAME_MAX+1], opt[GBMTOOL_OPTIONS_MAX+1];
	char fn_f[GBMTOOL_FILENAME_MAX+1];
	gbm_boolean	loop = GBM_FALSE, onepal = GBM_FALSE, quit = GBM_FALSE, sstep = GBM_FALSE;
	gbm_boolean title = GBM_FALSE, palbar = GBM_FALSE;
	int i, f, first, last, step, w, h, border = 0;
	GBM gbm; GBMRGB gbmrgb[0x100];

/*...sparse any options:8:*/
for ( i = 1; i < argc; i++ )
	{
	if ( argv[i][0] != '-' )
		break;
	switch ( argv[i][1] )
		{
		case 'l':	loop   = GBM_TRUE;	break;
		case 'p':	onepal = GBM_TRUE;	break;
		case 's':	sstep  = GBM_TRUE;	break;
		case 't':	title  = GBM_TRUE;	break;
		case 'P':	palbar = GBM_TRUE;	break;
		case 'b':
			if ( ++i == argc ) usage();
			border = get_opt_value_pos(argv[i], "border");
			if ( border > 255 )
				fatal("border should be in the range 0 to 255");
			break;
		default:	usage();	break;
		}
	}
/*...e*/
/*...sframes and filenames:8:*/
if ( i == argc )
	usage();
sscanf(argv[i++], "%d", &first);

if ( i == argc )
	usage();
sscanf(argv[i++], "%d", &last);

if ( i == argc )
	usage();
sscanf(argv[i++], "%d", &step);

if ( i == argc )
	usage();

/* Split filename and file options. */
gbmfilearg.argin = argv[i++];
if (strcmp(gbmfilearg.argin, "\"\"") == 0)
{
  usage();
}
if (gbmtool_parse_argument(&gbmfilearg, GBM_FALSE) != GBM_ERR_OK)
{
  fatal("can't parse filename %s", gbmfilearg.argin);
}
strcpy(fn , gbmfilearg.files->filename);
strcpy(opt, gbmfilearg.options);
gbmtool_free_argument(&gbmfilearg);

/*...e*/

	if ( first == last )
		exit(0);

	gbm_init();

	sprintf(fn_f, fn, first);
	if ( !read_bitmap_header(fn_f, opt, &gbm, gbmrgb) )
		fatal("can't read header of first bitmap %s", fn_f);
	if ( gbm.bpp != 8 )
		fatal("%s is not 8bpp", fn_f);
	if ( gbm.w > SCN_W || gbm.h > SCN_H )
		fatal("%s is %dx%d, largest allowed is %dx%d",
			fn_f, gbm.w, gbm.h, SCN_W, SCN_H);
	w = gbm.w; h = gbm.h;

	scn_init();
	scn_border(border);
	memset(ptr, border, SCN_W*SCN_H);

	if ( onepal )
		setpal(gbmrgb);

	do
		for ( f = first; !quit && f < last; f += step )
			{
			sprintf(fn_f, fn, f);
			if ( !read_bitmap(fn_f, opt, &gbm, gbmrgb) )
				{
				scn_term();
				fatal("can't load %s", fn_f);
				}
			if ( gbm.w != w || gbm.h != h )
				{
				scn_term();
				fatal("%s is %dx%d, previous ones were %dx%d",
					fn_f, gbm.w, gbm.h, w, h);
				}
			if ( !onepal )
				setpal(gbmrgb);
			scn_putimage(w, h, data);

			if ( title )
				plot_string(0, SCN_H-8, fn_f);

			if ( palbar )
				{
				int i, j;
				for ( j = 0; j < 5; j++ )
					for ( i = 0; i < 0x100; i++ )
						ptr[(SCN_H-10-j)*SCN_W+i] = (gbm_u8) i;
				}

/*...shandle keyboard interaction:24:*/
{
KBDKEYINFO kki;
gbm_boolean process_char;

kki.chChar = '\0';

if ( sstep )
	{
	do
		KbdCharIn(&kki, IO_WAIT, (HKBD) 0);
	while (	kki.chScan == '\0' || kki.chChar == '\0' );
	process_char = GBM_TRUE;
	}
else
	process_char = ( KbdCharIn(&kki, IO_NOWAIT, (HKBD) 0) == 0 &&
			 kki.chScan != '\0' && kki.chChar != '\0' );

if ( process_char )
	{
	if ( isdigit(kki.chChar) )
		{
		f = last - first;
		f = (f * (kki.chChar-'0'))/10;
		f = (f / step) * step;
		f += first;
		f -= step;
		}
	else
		switch ( kki.chChar )
			{
			case '-':
				if ( f > first )
					f -= 2 * step;
				else
					f = -step;
				break;
			case 's': case 'S':
				sstep = GBM_TRUE;
				break;
			case 'g': case 'G':
				sstep = GBM_FALSE;
				break;
			case 't': case 'T':
				title = !title;
				f -= step;
				break;
			case 'p': case 'P':
				palbar = !palbar;
				f -= step;
				break;
			case 'q': case 'Q':
			case 'x': case 'X':
			case 0x1b: /* Escape */
				quit = GBM_TRUE;
				break;
			}
	}
}
/*...e*/
			}
	while ( loop && !quit );

	scn_term();
	gbm_deinit();

	return 0;
	}
/*...e*/
