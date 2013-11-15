/*

gbmcreate.c - Creates a test bitmap file

*/

/*...sincludes:0:*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
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
#include "gbm.h"


static char progname[] = "gbmcreate";

static void fatal(const char *fmt, ...)
{
    va_list    vars;
    char s[256+1];

    va_start(vars, fmt);
    vsprintf(s, fmt, vars);
    va_end(vars);
    fprintf(stderr, "%s: %s\n", progname, s);
    exit(1);
}

static void usage(void)
{
    int ft, n_ft;

    fprintf(stderr, "usage: %s w h bpp fn.ext{,opt}\n", progname);
    fprintf(stderr, "       w             width\n");
    fprintf(stderr, "       h             height\n");
    fprintf(stderr, "       fn.ext{,opt}  output filename (with any format specific options)\n");
    fprintf(stderr, "                     ext's are used to deduce desired bitmap file formats\n");

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

    exit(1);
}

/* ------------------------------ */

int main(int argc, char *argv[])
{
    char     fn_dst[500+1], *opt_dst;
    int      fd, ft_dst, stride, flag;
    GBM_ERR  rc;
    GBMFT    gbmft;
    GBM      gbm;
    GBMRGB   gbmrgb[0x100];
    gbm_u8  *data, *data_tmp;
    int      x, y, z;

    if ( argc < 5 )
        usage();

    sscanf(argv[1], "%d", &gbm.w);
    sscanf(argv[2], "%d", &gbm.h);
    sscanf(argv[3], "%d", &gbm.bpp);

    strcpy(fn_dst, argv[4]);

    if ( (opt_dst = strchr(fn_dst, ',')) != NULL )
        *opt_dst++ = '\0';
    else
        opt_dst = "";

    gbm_init();

    if ( gbm_guess_filetype(fn_dst, &ft_dst) != GBM_ERR_OK )
        fatal("can't guess bitmap file format for %s", fn_dst);

    gbm_query_filetype(ft_dst, &gbmft);
    switch ( gbm.bpp )
    {
        case 64: flag = GBM_FT_W48; break;
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
        fatal("output bitmap format %s does not support writing %d bpp data",
            gbmft.short_name, gbm.bpp);
    }

    stride = ( ((gbm.w * gbm.bpp + 31)/32) * 4 );
    if ( (data = malloc((size_t) (stride * gbm.h))) == NULL )
    {
        fatal("out of memory allocating %d bytes for input bitmap", stride * gbm.h);
    }

    /* create the bitmap */
    memset(data, 0, stride * gbm.h);

    data_tmp = data;
    switch(gbm.bpp)
    {
       case 1:
       case 4:
       case 8:
          for (y = 0; y < gbm.h; y++)
          {
             z = 0;
             for (x = 0; x < stride; x++)
             {
                data_tmp[x] = z++;
                if (z > 255) z = 0;
             }
             data_tmp += stride;
          }
          break;

       case 24:
          for (y = 0; y < gbm.h; y++)
          {
             z = 0;
             for (x = 0; x < stride; x += 3)
             {
                data_tmp[x]   = z;
                data_tmp[x+1] = z;
                data_tmp[x+2] = z++;
                if (z > 255) z = 0;
             }
             data_tmp += stride;
          }
          break;

       case 32:
          for (y = 0; y < gbm.h; y++)
          {
             z = 0;
             for (x = 0; x < stride; x += 4)
             {
                data_tmp[x]   = z;
                data_tmp[x+1] = z;
                data_tmp[x+2] = z;
                data_tmp[x+3] = z++;
                if (z > 255) z = 0;
             }
             data_tmp += stride;
          }
          break;

       case 48:
          for (y = 0; y < gbm.h; y++)
          {
             z = 0;
             for (x = 0; x < stride; x += 6)
             {
                data_tmp[x]   = 0;
                data_tmp[x+1] = z;
                data_tmp[x+2] = 0;
                data_tmp[x+3] = z;
                data_tmp[x+4] = 0;
                data_tmp[x+5] = z++;
                if (z > 255) z = 0;
             }
             data_tmp += stride;
          }
          break;

       case 64:
          #define SCALE(x) ((((gbm_u16) x) * ((1L << 16) - 1)) / 255)
          for (y = 0; y < gbm.h; y++)
          {
             z = 0;
             for (x = 0; x < stride; x += 8)
             {
                data_tmp[x]   = 0;
                data_tmp[x+1] = z;
                data_tmp[x+2] = 0;
                data_tmp[x+3] = z;
                data_tmp[x+4] = 0;
                data_tmp[x+5] = z;
                data_tmp[x+6] = 0;
                data_tmp[x+7] = z++;
                if (z > 255) z = 0;
             }
             data_tmp += stride;
          }
          break;

       default:
          fatal("unsupported color depth %d", gbm.bpp);
    }

    /* create the bitmap */
    if (gbm.bpp <= 8)
    {
       int increment = 0, value;

       switch(gbm.bpp)
       {
          case 1:
             increment = 255;
             break;

          case 2:
             increment = 85;
             break;

          case 4:
             increment = 17;
             break;

          case 8:
             increment = 1;
             break;

          default:
             free(data);
             fatal("unsupported bpp");
       }

       value = 0;
       for (y = 0; y < (1 << gbm.bpp); y++)
       {
          gbmrgb[y].r =
          gbmrgb[y].g =
          gbmrgb[y].b = value;
          value += increment;
       }
    }

    if ( (fd = gbm_io_create(fn_dst, GBM_O_WRONLY)) == -1 )
    {
        free(data);
        fatal("can't create %s", fn_dst);
    }

    if ( (rc = gbm_write(fn_dst, fd, ft_dst, &gbm, gbmrgb, data, opt_dst)) != GBM_ERR_OK )
    {
        free(data);
        gbm_io_close(fd);
        remove(fn_dst);
        fatal("can't write %s: %s", fn_dst, gbm_err(rc));
    }

    free(data);

    gbm_io_close(fd);
    gbm_deinit();

    return 0;
}


