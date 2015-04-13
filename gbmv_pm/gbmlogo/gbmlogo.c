/*

gbmlogo.c - Make VRAM files for sending to MAKELOGO

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
#include "gbmtrunc.h"
#include "gbmerr.h"
#include "gbmht.h"
#include "gbmtool.h"
/*...e*/

static char progname[] = "gbmlogo";

/*...ssizes:0:*/
#define	SCN_W 640
#define	SCN_H 480
#define	LOGO_W SCN_W
#define	LOGO_H ((SCN_H*5)/6)
/*...e*/
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

	fprintf(stderr, "usage: %s [-e] [-hN] \"filename.ext\"{\\\",\\\"\"opt\"}\n", progname);
	fprintf(stderr, "flags: -e             error-diffuse to RGBI\n");
	fprintf(stderr, "       -hN            halftone to RGBI\n");
	fprintf(stderr, "                      N is a halftoning algorithm number (default 0)\n");
	fprintf(stderr, "                      -e and -h can't be used together\n");
	fprintf(stderr, "       filename.ext   %dx%d bitmap\n", LOGO_W, LOGO_H);
	fprintf(stderr, "                      ext's are used to deduce desired bitmap file format\n");

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

	fprintf(stderr, "\n       In case the filename contains a comma or spaces and options\n");
    fprintf(stderr,   "       need to be added, syntax \"fn.ext\"{\\\",\\\"opt} or \"fn.ext\"{\\\",\\\"\"opt\"}\n");
	fprintf(stderr,   "       used to clearly separate the filename from the options.\n");

	exit(1);
	}
/*...e*/
/*...smain:0:*/
static char errbuf[100+1];

/*...sread_bitmap:0:*/
static char *read_bitmap(
	const char *fn, const char *opt,
	GBM *gbm, GBMRGB gbmrgb[], gbm_u8 **data
	)
	{
	int ft, fd, stride;
	GBM_ERR rc;

	if ( gbm_guess_filetype(fn, &ft) != GBM_ERR_OK )
		{
		sprintf(errbuf, "can't guess filetype of %s", fn);
		return errbuf;
		}

	if ( (fd = gbm_io_open(fn, GBM_O_RDONLY)) == -1 )
		{
		sprintf(errbuf, "can't open %s for reading", fn);
		return "can't open bitmap file";
		}

	if ( (rc = gbm_read_header(fn, fd, ft, gbm, opt)) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		sprintf(errbuf, "can't read header of %s: %s", fn, gbm_err(rc));
		return errbuf;
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
		     sprintf(errbuf, "%d bpp file is not supported", gbm->bpp);
		  }
		}

	if ( (rc = gbm_read_palette(fd, ft, gbm, gbmrgb)) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		sprintf(errbuf, "can't read palette of %s: %s", fn, gbm_err(rc));
		return errbuf;
		}

	stride = (gbm->w*gbm->bpp+31)/32*4;
	if ( ((*data) = malloc(stride*gbm->h)) == NULL )
		{
		gbm_io_close(fd);
		sprintf(errbuf, "can't allocate memory to hold data from %s", fn);
		return errbuf;
		}

	if ( (rc = gbm_read_data(fd, ft, gbm, (*data))) != GBM_ERR_OK )
		{
		gbm_io_close(fd);
		sprintf(errbuf, "can't read data from %s", fn);
		return errbuf;
		}

	gbm_io_close(fd);
	return NULL;
	}
/*...e*/
/*...sto_24_bit:0:*/
static gbm_boolean to_24_bit(GBM *gbm, GBMRGB *gbmrgb, BYTE **ppbData)
	{
	int stride = ((gbm -> w * gbm -> bpp + 31)/32) * 4;
	int new_stride = ((gbm -> w * 3 + 3) & ~3);
	int bytes, y;
	gbm_u8 *pbDataNew;

	if ( gbm->bpp == 24 )
		return GBM_TRUE;

	bytes = new_stride * gbm -> h;
	if ( (pbDataNew = malloc(bytes)) == NULL )
		return GBM_FALSE;

	for ( y = 0; y < gbm -> h; y++ )
		{
		gbm_u8 *src = *ppbData + y * stride;
		gbm_u8 *dest = pbDataNew + y * new_stride;
		int x;

		switch ( gbm -> bpp )
			{
/*...s1:24:*/
case 1:
	{
	gbm_u8	c = 0;

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
/*...e*/
/*...s4:24:*/
case 4:
	for ( x = 0; x + 1 < gbm -> w; x += 2 )
		{
		gbm_u8	c = *src++;

		*dest++ = gbmrgb [c >> 4].b;
		*dest++ = gbmrgb [c >> 4].g;
		*dest++ = gbmrgb [c >> 4].r;
		*dest++ = gbmrgb [c & 15].b;
		*dest++ = gbmrgb [c & 15].g;
		*dest++ = gbmrgb [c & 15].r;
		}

	if ( x < gbm -> w )
		{
		gbm_u8	c = *src;

		*dest++ = gbmrgb [c >> 4].b;
		*dest++ = gbmrgb [c >> 4].g;
		*dest++ = gbmrgb [c >> 4].r;
		}
	break;
/*...e*/
/*...s8:24:*/
case 8:
	for ( x = 0; x < gbm -> w; x++ )
		{
		gbm_u8	c = *src++;

		*dest++ = gbmrgb [c].b;
		*dest++ = gbmrgb [c].g;
		*dest++ = gbmrgb [c].r;
		}
	break;
/*...e*/
			}
		}
	free(*ppbData);
	*ppbData = pbDataNew;
	gbm->bpp = 24;
	return GBM_TRUE;
	}
/*...e*/
/*...sgenerate_file:0:*/
/* data points to 480*5/6 lines, each of 640 4 bit pixels.
   We are interested in the plane'th bits of each nibble. */

static char *generate_file(const char *fn, int plane, const gbm_u8 *data)
	{
	FILE *fp;
	int x, y;
	gbm_u8 planemask = (0x11<<plane), sidemask, *p, *q;

	if ( (fp = fopen(fn, "w")) == NULL )
		{
		sprintf(errbuf, "can't create %s", fn);
		return errbuf;
		}

	if ( (p = q = malloc(LOGO_W*SCN_H/8)) == NULL )
		{
		fclose(fp); remove(fn);
		return "out of memory";
		}

	memset(p, 0, LOGO_W*SCN_H/8);

	data += (LOGO_H-1) * (LOGO_W/2);
	for ( y = 0; y < LOGO_H; y++, data -= LOGO_W/2, q += LOGO_W/8 )
		{
		for ( x = 0, sidemask = 0xf0; x < LOGO_W; x++, sidemask ^= 0xff )
			if ( data[x>>1] & sidemask & planemask )
				q[x>>3] |= (0x80>>(x&7));
		}

	for ( y = 0; y < 0x9600; y += 0x10 )
		{
		fprintf(fp, "%%%%%08x ", 0xa0000+y);
		for ( x = 0; x < 0x10; x += 2 )
			fprintf(fp, " %02x%02x", p[y+x+1], p[y+x]);
		fprintf(fp, "\n");
		}
	fclose(fp);
	return NULL;
	}
/*...e*/

int main(int argc, char *argv[])
	{
	GBMTOOL_FILEARG gbmfilearg;
	char fn[GBMTOOL_FILENAME_MAX+1], opt[GBMTOOL_OPTIONS_MAX+1];
	char *err;
	gbm_boolean	errdiff = GBM_FALSE, halftone = GBM_FALSE;
	int htmode = 0, plane, i;
	GBM gbm; GBMRGB gbmrgb[0x100]; gbm_u8 *data;

/*...sparse any options:8:*/
for ( i = 1; i < argc; i++ )
	{
	if ( argv[i][0] != '-' )
		break;
	switch ( argv[i][1] )
		{
		case 'e':	errdiff = GBM_TRUE;
				break;
		case 'h':	halftone = GBM_TRUE;
				if ( argv[i][2] != '\0' && isdigit(argv[i][2]) )
					htmode = argv[i][2] - '0';
				break;
		default:	usage();	break;
		}
	}

if ( errdiff && halftone )
	fatal("error-diffusion and halftoning can't both be done at once");

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

	gbm_init();

	if ( (err = read_bitmap(fn, opt, &gbm, gbmrgb, &data)) != NULL )
		fatal(err);

	if ( gbm.w != LOGO_W || gbm.h != LOGO_H )
		fatal("%s is %dx%d not %dx%d, as required", fn, gbm.w, gbm.h, LOGO_W, LOGO_H);

	if ( !to_24_bit(&gbm, gbmrgb, &data) )
		fatal("can't expand %s to 24 bpp, prior to mapping to RGBI", fn);

/*...smap down to RGBI \40\same as VGA palette\41\:8:*/
if ( errdiff )
	{
	gbm_errdiff_pal_VGA(gbmrgb);
	if ( !gbm_errdiff_VGA(&gbm, data, data) )
		fatal("can't error-diffuse to RGBI, out of memory");
	}
else if ( halftone )
	{
	gbm_ht_pal_VGA(gbmrgb);
	switch ( htmode )
		{
		default:
		case 0: gbm_ht_VGA_3x3(&gbm, data, data); break;
		case 1: gbm_ht_VGA_2x2(&gbm, data, data); break;
		}
	}
else
	{
	gbm_trunc_pal_VGA(gbmrgb);
	gbm_trunc_VGA(&gbm, data, data);
	}
gbm.bpp = 4;
/*...e*/
/*...swrite out the IBGR planes:8:*/
/* Palette is ordered IBGR, vram files are B G R I */

for ( plane = 0; plane < 4; plane++ )
	{
	char fn_o[20+1];
	int oplane = 0;
	sprintf(fn_o, "vram%d.dat", plane);
	switch ( plane )
		{
		case 0:	oplane = 2; break;
		case 1:	oplane = 1; break;
		case 2:	oplane = 0; break;
		case 3:	oplane = 3; break;
		}
	if ( (err = generate_file(fn_o, oplane, data)) != NULL )
		{
		while ( --plane >= 0 )
			{
			sprintf(fn_o, "vram%d.dat", plane);
			remove(fn_o);
			}
		fatal(err);
		}
	}
/*...e*/

	gbm_deinit();

	return 0;
	}
/*...e*/
