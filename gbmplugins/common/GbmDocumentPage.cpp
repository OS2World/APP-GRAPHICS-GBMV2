
//***************************************************
// Generalised Bitmap Module C++ adapter for GBM plugins
//
// Requires: GBM.DLL version 1.35 or higher (with multipage support)
//
// Copyright (C) 2006-2010 Heiko Nitzsche
//
//   This software is provided 'as-is', without any express or implied
//   warranty.  In no event will the author be held liable for any damages
//   arising from the use of this software.
//
//   Permission is granted to anyone to use this software for any purpose,
//   including commercial applications, and to alter it and redistribute it
//   freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//   2. Altered source versions must be plainly marked as such, and must not be
//      misrepresented as being the original software.
//   3. This notice may not be removed or altered from any source distribution.
//
//***************************************************

#ifdef __IBMCPP__
  #pragma strings( readonly )
#endif

#if defined(__OS2__)
  #include "os2defs.h"
#elif defined(WIN32)
  #include "windefs.h"
#endif

#include <stdio.h>
#include <string.h>

#include "GbmDocumentPage.hpp"

#include "gbmmem.h"
#include "gbmscale.h"
#include "gbmmir.h"

// --- typedefs ---

/* bitmap header incl. color table */
/* for 8 bit and lower images */
#pragma pack(2)
#if defined(__OS2__)

    typedef struct
    {
       BITMAPINFOHEADER2 bmp2;
       RGB2 argb2Color[0x100];
    } BMP8HEADER;

    /* for above 8 bit images */
    typedef struct
    {
       BITMAPINFOHEADER2 bmp2;
    } BMP24HEADER;

#elif defined(WIN32)

    typedef struct
    {
       BITMAPINFOHEADER bmiHeader;
       RGBQUAD bmiColors[0x100];
    } BMP8HEADER;

    /* for above 8 bit images */
    typedef struct
    {
       BITMAPINFOHEADER bmiHeader;
    } BMP24HEADER;

#endif
#pragma pack()

// ---------------------------------------------------------

GbmDocumentPage::GbmDocumentPage(const GbmAccessor & accessor)
    : mGbmAccessor(accessor),
      pHeader(NULL),
      data(NULL),
      filename(NULL),
      pageNumber(-1),
      gbmFileType(-1),
      requestedAs24bpp(FALSE)
{
    memset(&gbm  , 0, sizeof(GBM));
    memset(gbmrgb, 0, sizeof(gbmrgb));
}

GbmDocumentPage::~GbmDocumentPage()
{
    mInterfaceMutex.lock();
    
    gbmmem_free(data);
    data = NULL;

    delete pHeader;
    pHeader = NULL;

    delete [] filename;
    filename = NULL;
}

// ---------------------------------------------------------

MutexSemaphore & GbmDocumentPage::getInterfaceMutex()
{
    return mInterfaceMutex;
}

// ---------------------------------------------------------

BOOL GbmDocumentPage::expandTo24bpp()
{
    const size_t stride     = ((gbm.w * gbm.bpp + 31)/32) * 4;
    const size_t new_stride = ((gbm.w * 3 + 3) & ~3);
          int y;

    if ( gbm.bpp == 24 )
    {
        return TRUE;
    }

    // allocate new target buffer
    gbm_u8 * data24 = (gbm_u8 *)gbmmem_malloc(new_stride * gbm.h);
    if (data24 == NULL)
    {
        return FALSE;
    }

    for ( y = 0; y < gbm.h; y++ )
    {
        const gbm_u8 *src  = data   + stride * y;
              gbm_u8 *dest = data24 + new_stride * y;

        int x;

        switch ( gbm.bpp )
        {
            case 1:
            {
                gbm_u8 c = 0;

                for ( x = 0; x < gbm.w; x++ )
                {
                    if ( (x & 7) == 0 )
                        c = *src++;
                    else
                        c <<= 1;

                    *dest++ = gbmrgb[c >> 7].b;
                    *dest++ = gbmrgb[c >> 7].g;
                    *dest++ = gbmrgb[c >> 7].r;
                }
            }
            break;

            case 4:
            {
                for ( x = 0; x + 1 < gbm.w; x += 2 )
                {
                    const gbm_u8 c = *src++;

                    *dest++ = gbmrgb[c >> 4].b;
                    *dest++ = gbmrgb[c >> 4].g;
                    *dest++ = gbmrgb[c >> 4].r;
                    *dest++ = gbmrgb[c & 15].b;
                    *dest++ = gbmrgb[c & 15].g;
                    *dest++ = gbmrgb[c & 15].r;
                    }

                if ( x < gbm.w )
                {
                    const gbm_u8 c = *src;

                    *dest++ = gbmrgb[c >> 4].b;
                    *dest++ = gbmrgb[c >> 4].g;
                    *dest++ = gbmrgb[c >> 4].r;
                }
            }
            break;

            case 8:
            {
                for ( x = 0; x < gbm.w; x++ )
                {
                    const gbm_u8 c = *src++;

                    *dest++ = gbmrgb[c].b;
                    *dest++ = gbmrgb[c].g;
                    *dest++ = gbmrgb[c].r;
                }
            }
            break;

            default:
               return FALSE;
        }
    }

    gbmmem_free(data);
    data    = data24;
    gbm.bpp = 24;

    return TRUE;
}

// ---------------------------------------------------------

// Load a single bitmap page

void GbmDocumentPage::loadFromFile(const char * filename,
                                   const int    pageNumber,
                                   const int    gbmFileType,
                                   const BOOL   allowDelayedDataLoading,
                                   const BOOL   to24bpp) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    
    // store filename
    delete [] this->filename;
    this->filename = NULL;
    try
    {
        this->filename = new char[strlen(filename) + 1];
    }
    catch(...)
    {
    }
    if (this->filename == NULL)
    {
        throw GbmException(GBM_ERR_MEM);
    }
    strcpy(this->filename, filename);

    this->pageNumber  = pageNumber;
    this->gbmFileType = gbmFileType;

    requestedAs24bpp = to24bpp;

    // initially we load just the header data if we are allowed
    internLoadFromFile(allowDelayedDataLoading ? FALSE : TRUE);

    // on request of a data pointer, the full file is reloaded
}

// ---------------------------------------------------------

// Write the page into a bitmap file.
// The format is determined from the extension.

void GbmDocumentPage::writeToFile(const char * filename,
                                  const char * options) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);

    int     fd   = -1;
    int     ft   = -1;
    int     flag =  0;
    GBM_ERR rc;
    GBMFT   gbmft = { 0 };

    // check if file format is supported

    rc = mGbmAccessor.Gbm_guess_filetype(filename, &ft);
    if (rc != GBM_ERR_OK)
    {
        throw GbmException(rc, "Can't guess bitmap file format");
    }

    mGbmAccessor.Gbm_query_filetype(ft, &gbmft);
    switch ( gbm.bpp )
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
        throw GbmException(GBM_ERR_NOT_SUPP, "Color depth not supported by bitmap file format");
    }

    // now write the file

    if ( (fd = mGbmAccessor.Gbm_io_create(filename, GBM_O_WRONLY)) == -1 )
    {
        mGbmAccessor.Gbm_io_close(fd);
        throw GbmException(GBM_ERR_WRITE);
    }

    rc = mGbmAccessor.Gbm_write(filename, fd, ft, &gbm, gbmrgb, data, options);
    if (rc != GBM_ERR_OK)
    {
      mGbmAccessor.Gbm_io_close(fd);
      remove(filename);
      throw GbmException(rc, "Can't write file");
    }

    mGbmAccessor.Gbm_io_close(fd);
}

// ---------------------------------------------------------

// Load a single bitmap page

void GbmDocumentPage::internLoadFromFile(const BOOL includeData) throw ( GbmException )
{
    int     ft = gbmFileType;
    int     fd = -1;
    GBM_ERR rc = GBM_ERR_OK;

    // cleanup before reloading to keep the state machine valid
    memset(&gbm  , 0, sizeof(GBM));
    memset(gbmrgb, 0, sizeof(gbmrgb));

    gbmmem_free(data);
    data = NULL;

    delete pHeader;
    pHeader = NULL;

    /* read the bitmap file */

    // default is to load page 0. Old codecs might not support index keyword
    char indexString[20] = { 0 };
    if (pageNumber == 0)
    {
        strcpy(indexString, "");
    }
    else
    {
        sprintf(indexString, "index=%d", pageNumber);
    }

    if (ft == -1)
    {
        rc = mGbmAccessor.Gbm_guess_filetype(filename, &ft);
        if (rc != GBM_ERR_OK)
        {
            throw GbmException(rc, "Can't guess bitmap file format");
        }
    }

    if ( (fd = mGbmAccessor.Gbm_io_open(filename, GBM_O_RDONLY)) == -1 )
    {
        throw GbmException(rc, "Can't open file");
    }

    rc = mGbmAccessor.Gbm_read_header(filename, fd, ft, &gbm, indexString);
    if (rc != GBM_ERR_OK)
    {
        mGbmAccessor.Gbm_io_close(fd);
        throw GbmException(rc, "Can't read header");
    }

    rc = mGbmAccessor.Gbm_read_palette(fd, ft, &gbm, gbmrgb);
    if (rc != GBM_ERR_OK)
    {
        mGbmAccessor.Gbm_io_close(fd);
        throw GbmException(rc, "Can't read palette");
    }

    // check if the data should be read as well
    if (! includeData)
    {
        mGbmAccessor.Gbm_io_close(fd);
        return;
    }

    const size_t dataBytes = ( (((size_t)gbm.w * gbm.bpp + 31)/32) * 4 ) * (size_t)gbm.h;
    data = (gbm_u8 *)gbmmem_malloc(dataBytes);
    if (data == NULL)
    {
        mGbmAccessor.Gbm_io_close(fd);
        throw GbmException(GBM_ERR_MEM);
    }

    rc = mGbmAccessor.Gbm_read_data(fd, ft, &gbm, data);
    if (rc != GBM_ERR_OK)
    {
        gbmmem_free(data);
        data = NULL;
        mGbmAccessor.Gbm_io_close(fd);
        throw GbmException(rc, "Can't read data");
    }

    mGbmAccessor.Gbm_io_close(fd);

    // Do we need to convert the data to 24bpp ?
    if (requestedAs24bpp)
    {
        // convert all color depths to 24 bits per pixel
        if (! expandTo24bpp())
        {
            gbmmem_free(data);
            data = NULL;
            throw GbmException(GBM_ERR_MEM);
        }
    }

    const unsigned long bmpDataStride = ((gbm.w * gbm.bpp + 31)/32) * 4;
    const unsigned long bmpDataLength = bmpDataStride * gbm.h;

    /* create bitmap header */
    if (gbm.bpp <= 8)
    {
       BMP8HEADER * pHeader8 = new BMP8HEADER();

      #if defined(__OS2__)

       pHeader = (BITMAPINFO2 *) pHeader8;

       memset(pHeader8, 0, sizeof(BMP8HEADER));
       pHeader8->bmp2.cbFix         = sizeof(BITMAPINFOHEADER2);
       pHeader8->bmp2.cx            = gbm.w;
       pHeader8->bmp2.cy            = gbm.h;
       pHeader8->bmp2.cPlanes       = 1;               /*  Number of bit planes. */
       pHeader8->bmp2.cBitCount     = (USHORT)gbm.bpp; /*  Number of bits per pel within a plane. */
       pHeader8->bmp2.cclrUsed      = (1 << gbm.bpp);  /*  Number of used color table entries */
       pHeader8->bmp2.ulCompression = BCA_UNCOMP;      /*  Compression scheme used to store the bit map. */
       pHeader8->bmp2.cbImage       = bmpDataLength;   /*  Length of bit-map storage data, in bytes. */

       /* copy color table */
       for (int i = 0; i < 0x100; i++)
       {
           pHeader8->argb2Color[i].bBlue     = gbmrgb[i].b;
           pHeader8->argb2Color[i].bRed      = gbmrgb[i].r;
           pHeader8->argb2Color[i].bGreen    = gbmrgb[i].g;
           pHeader8->argb2Color[i].fcOptions = 0;
       }

      #elif defined(WIN32)

       pHeader = (BITMAPINFO *) pHeader8;

       memset(pHeader8, 0, sizeof(BMP8HEADER));
       pHeader8->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
       pHeader8->bmiHeader.biWidth       = gbm.w;
       pHeader8->bmiHeader.biHeight      = gbm.h;
       pHeader8->bmiHeader.biPlanes      = 1;               /*  Number of bit planes. */
       pHeader8->bmiHeader.biBitCount    = (WORD)gbm.bpp;   /*  Number of bits per pel within a plane. */
       pHeader8->bmiHeader.biClrUsed     = (1 << gbm.bpp);  /*  Number of used color table entries */
       pHeader8->bmiHeader.biCompression = BI_RGB;          /*  Compression scheme used to store the bit map. */
       pHeader8->bmiHeader.biSizeImage   = bmpDataLength;   /*  Length of bit-map storage data, in bytes. */

       /* copy color table */
       for (int i = 0; i < 0x100; i++)
       {
           pHeader8->bmiColors[i].rgbBlue     = gbmrgb[i].b;
           pHeader8->bmiColors[i].rgbRed      = gbmrgb[i].r;
           pHeader8->bmiColors[i].rgbGreen    = gbmrgb[i].g;
           pHeader8->bmiColors[i].rgbReserved = 0;
       }
      #endif
    }
    else
    {
       BMP24HEADER * pHeader24 = new BMP24HEADER();

      #if defined(__OS2__)

       pHeader = (BITMAPINFO2 *) pHeader24;

       memset(pHeader24, 0, sizeof(BMP24HEADER));
       pHeader24->bmp2.cbFix         = sizeof(BITMAPINFOHEADER2);
       pHeader24->bmp2.cx            = gbm.w;
       pHeader24->bmp2.cy            = gbm.h;
       pHeader24->bmp2.cPlanes       = 1;               /*  Number of bit planes. */
       pHeader24->bmp2.cBitCount     = (USHORT)gbm.bpp; /*  Number of bits per pel within a plane. */
       pHeader24->bmp2.ulCompression = BCA_UNCOMP;      /*  Compression scheme used to store the bit map. */
       pHeader24->bmp2.cbImage       = bmpDataLength;   /*  Length of bit-map storage data, in bytes. */

      #elif defined(WIN32)

       pHeader = (BITMAPINFO *) pHeader24;

       memset(pHeader24, 0, sizeof(BMP24HEADER));
       pHeader24->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
       pHeader24->bmiHeader.biWidth       = gbm.w;
       pHeader24->bmiHeader.biHeight      = gbm.h;
       pHeader24->bmiHeader.biPlanes      = 1;               /*  Number of bit planes. */
       pHeader24->bmiHeader.biBitCount    = (WORD)gbm.bpp;   /*  Number of bits per pel within a plane. */
       pHeader24->bmiHeader.biCompression = BI_RGB;          /*  Compression scheme used to store the bit map. */
       pHeader24->bmiHeader.biSizeImage   = bmpDataLength;   /*  Length of bit-map storage data, in bytes. */

      #endif
    }
}

// ---------------------------------------------------------

void GbmDocumentPage::updateBitmapHeader()
{
    if (pHeader != NULL)
    {
        const unsigned long bmpDataStride = ((gbm.w * gbm.bpp + 31)/32) * 4;
        const unsigned long bmpDataLength = bmpDataStride * gbm.h;

        /* update relevant bitmap header data */
      #if defined(__OS2__)

        pHeader->cx        = gbm.w;
        pHeader->cy        = gbm.h;
        pHeader->cBitCount = (USHORT)gbm.bpp; /*  Number of bits per pel within a plane. */
        pHeader->cbImage   = bmpDataLength;   /*  Length of bit-map storage data, in bytes. */

      #elif defined(WIN32)

        pHeader->bmiHeader.biWidth     = gbm.w;
        pHeader->bmiHeader.biHeight    = gbm.h;
        pHeader->bmiHeader.biBitCount  = (USHORT)gbm.bpp; /*  Number of bits per pel within a plane. */
        pHeader->bmiHeader.biSizeImage = bmpDataLength;   /*  Length of bit-map storage data, in bytes. */

      #endif
    }
}

// ---------------------------------------------------------
// ---------------------------------------------------------

void GbmDocumentPage::rotate(const int rotationAngle) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    
    if (data == NULL)
    {
        if (rotationAngle == 0)
        {
            // nothing to do
            return;
        }
        // load the page with the data
        internLoadFromFile(TRUE);
    }

    int rotateBy = rotationAngle;
    while (rotateBy < 0)
    {
        rotateBy += 360;
    }
    while (rotateBy > 360)
    {
        rotateBy -= 360;
    }

    gbm_u8 * rotData = NULL;
    size_t   dataStride = 0;

    switch (rotateBy)
    {
        case 0:
        case 360:
            break;

        case 90:
            dataStride = ((gbm.h * gbm.bpp + 31)/32) * 4;
            rotData = (gbm_u8 *)gbmmem_malloc(dataStride * gbm.w);
            if (rotData != NULL)
            {
                gbm_transpose(&gbm, data, rotData);
                const int tmp = gbm.w;
                gbm.w = gbm.h;
                gbm.h = tmp;

                gbm_ref_vert(&gbm, rotData);
                gbmmem_free(data);
                data = rotData;
            }
            else
            {
                throw GbmException(GBM_ERR_MEM);
            }
            break;

        case 180:
            gbm_ref_vert(&gbm, data);
            gbm_ref_horz(&gbm, data);
            break;

        case 270:
            dataStride = ((gbm.h * gbm.bpp + 31)/32) * 4;
            rotData = (gbm_u8 *)gbmmem_malloc(dataStride * gbm.w);
            if (rotData != NULL)
            {
                gbm_transpose(&gbm, data, rotData);
                const int tmp = gbm.w;
                gbm.w = gbm.h;
                gbm.h = tmp;

                gbm_ref_horz(&gbm, rotData);
                gbmmem_free(data);
                data = rotData;
            }
            else
            {
                throw GbmException(GBM_ERR_MEM);
            }
            break;

        default:
            // others are not supported
            throw GbmException(GBM_ERR_NOT_SUPP, "Rotation angle not any of 0, 90, 180, 270, 360 degrees");
    }

    // update the BITMAPINFO2 parameters
    updateBitmapHeader();
}

// ---------------------------------------------------------

void GbmDocumentPage::mirror(const GbmDocumentPage::MirrorMode mode) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    
    if (data == NULL)
    {
        if (mode == GbmDocumentPage::MirrorMode_NONE)
        {
            // nothing to do
            return;
        }
        // load the page with the data
        internLoadFromFile(TRUE);
    }

    gbm_u8 * rotData = NULL;
    size_t   dataStride = 0;

    switch (mode)
    {
        case GbmDocumentPage::MirrorMode_NONE:
            break;

        case GbmDocumentPage::MirrorMode_HORIZONTAL:
            gbm_ref_horz(&gbm, data);
            break;

        case GbmDocumentPage::MirrorMode_VERTICAL:
            gbm_ref_vert(&gbm, data);
            break;

        case GbmDocumentPage::MirrorMode_TRANSPOSE:
            dataStride = ((gbm.h * gbm.bpp + 31)/32) * 4;
            rotData = (gbm_u8 *)gbmmem_malloc(dataStride * gbm.w);
            if (rotData != NULL)
            {
                gbm_transpose(&gbm, data, rotData);
                const int tmp = gbm.w;
                gbm.w = gbm.h;
                gbm.h = tmp;

                gbmmem_free(data);
                data = rotData;
            }
            else
            {
                throw GbmException(GBM_ERR_MEM);
            }
            break;

        default:
            // others are not supported
            throw GbmException(GBM_ERR_NOT_SUPP, "Unsupported Mirror Mode");
    }

    // update the BITMAPINFO2 parameters
    updateBitmapHeader();
}

// ---------------------------------------------------------
// ---------------------------------------------------------

BOOL GbmDocumentPage::isValid() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return (pageNumber > -1);
}

// ---------------------------------------------------------

int GbmDocumentPage::getWidth()
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (data == NULL)
    {
        try
        {
            // load the page with the data
            internLoadFromFile(TRUE);
        }
        catch(GbmException &)
        {
            return 0;
        }
    }
    return gbm.w;
}

// ---------------------------------------------------------

int GbmDocumentPage::getHeight()
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (data == NULL)
    {
        try
        {
            // load the page with the data
            internLoadFromFile(TRUE);
        }
        catch(GbmException &)
        {
            return 0;
        }
    }
    return gbm.h;
}

// ---------------------------------------------------------

int GbmDocumentPage::getColorDepth() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return gbm.bpp;
}

// ---------------------------------------------------------
// ---------------------------------------------------------

size_t GbmDocumentPage::getBitmapDataRowSize()
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (requestedAs24bpp)
    {
        return ((((size_t)getWidth() * 24 + 31)/32) * 4);
    }
    return ((((size_t)getWidth() * getColorDepth() + 31)/32) * 4);
}

// ---------------------------------------------------------

const BYTE * GbmDocumentPage::getBitmapDataPtr()
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (data == NULL)
    {
        try
        {
            // load the page with the data
            internLoadFromFile(TRUE);
        }
        catch(GbmException &)
        {
            return NULL;
        }
    }
    return (BYTE *)data;
}

// ---------------------------------------------------------

#if defined(__OS2__)
const BITMAPINFO2 * GbmDocumentPage::getBitmapInfoPtr()
#elif defined(WIN32)
const BITMAPINFO * GbmDocumentPage::getBitmapInfoPtr()
#endif
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (data == NULL)
    {
        try
        {
            // load the page with the data
            internLoadFromFile(TRUE);
        }
        catch(GbmException &)
        {
            return NULL;
        }
    }
    return pHeader;
}


