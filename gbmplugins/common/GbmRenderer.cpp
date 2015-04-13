
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

  // direct access macros for the bitmap header
  #define BMPHEADER_WIDTH(bmpHeader)      ((bmpHeader).bmp2.cx)
  #define BMPHEADER_HEIGHT(bmpHeader)     ((bmpHeader).bmp2.cy)
  #define BMPHEADER_BITCOUNT(bmpHeader)   ((bmpHeader).bmp2.cBitCount)
  #define BMPHEADER_CLRUSED(bmpHeader)    ((bmpHeader).bmp2.cclrUsed)
  #define BMPHEADER_IMGSIZE(bmpHeader)    ((bmpHeader).bmp2.cbImage)
  #define BMPHEADER_COLORS(bmpHeader)     ((bmpHeader).argb2Color)
  #define BMPHEADER_COLOR_R(bmpHeader, i) ((bmpHeader).argb2Color[i].bRed)
  #define BMPHEADER_COLOR_G(bmpHeader, i) ((bmpHeader).argb2Color[i].bGreen)
  #define BMPHEADER_COLOR_B(bmpHeader, i) ((bmpHeader).argb2Color[i].bBlue)

  // direct access macros for the RGB2 struct
  #define RGB_COLOR_R(rgba) ((rgba).bRed)
  #define RGB_COLOR_G(rgba) ((rgba).bGreen)
  #define RGB_COLOR_B(rgba) ((rgba).bBlue)

  // direct access macros for the RECTL struct
  #define RECT_LEFT(rectl)   ((rectl).xLeft)
  #define RECT_RIGHT(rectl)  ((rectl).xRight)
  #define RECT_BOTTOM(rectl) ((rectl).yBottom)
  #define RECT_TOP(rectl)    ((rectl).yTop)

#elif defined(WIN32)
  #include "windefs.h"

  // direct access macros for the bitmap header
  #define BMPHEADER_WIDTH(bmpHeader)      ((bmpHeader).bmiHeader.biWidth)
  #define BMPHEADER_HEIGHT(bmpHeader)     ((bmpHeader).bmiHeader.biHeight)
  #define BMPHEADER_BITCOUNT(bmpHeader)   ((bmpHeader).bmiHeader.biBitCount)
  #define BMPHEADER_CLRUSED(bmpHeader)    ((bmpHeader).bmiHeader.biClrUsed)
  #define BMPHEADER_IMGSIZE(bmpHeader)    ((bmpHeader).bmiHeader.biSizeImage)
  #define BMPHEADER_COLORS(bmpHeader)     ((bmpHeader).bmiColors)
  #define BMPHEADER_COLOR_R(bmpHeader, i) ((bmpHeader).bmiColors[i].rgbRed)
  #define BMPHEADER_COLOR_G(bmpHeader, i) ((bmpHeader).bmiColors[i].rgbGreen)
  #define BMPHEADER_COLOR_B(bmpHeader, i) ((bmpHeader).bmiColors[i].rgbBlue)

  // direct access macros for the RGBQUAD struct
  #define RGB_COLOR_R(rgba) ((rgba).rgbRed)
  #define RGB_COLOR_G(rgba) ((rgba).rgbGreen)
  #define RGB_COLOR_B(rgba) ((rgba).rgbBlue)

  // direct access macros for the RECTL struct
  #define RECT_LEFT(rect)   ((rect).left)
  #define RECT_RIGHT(rect)  ((rect).right)
  #define RECT_BOTTOM(rect) ((rect).bottom)
  #define RECT_TOP(rect)    ((rect).top)

#endif

#include <string.h>
#include <stdlib.h>

#include "GbmRenderer.hpp"

#include "gbm.h"
#include "gbmmem.h"
#include "gbmscale.h"
#include "gbmmir.h"
#include "gbmrect.h"

// ---------------------------------------------------------

GbmRenderer::GbmRenderer()
    : renderPage(NULL),
      unscaledWidth(0),
      unscaledHeight(0),
      unscaledBpp(0),
      mScaleFilter(GbmRenderer::ScalerType_SIMPLE)
{
}

// ---------------------------------------------------------

GbmRenderer::~GbmRenderer()
{
    mInterfaceMutex.lock();
    renderPage = NULL;
}

// ---------------------------------------------------------

void GbmRenderer::setRenderPage(GbmDocumentPage * page)
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (page != renderPage)
    {
        if (renderPage)
        {
            MutexGuard pageGuard(renderPage->getInterfaceMutex());
            // clear old render cache
            cacheDescription.clear();
            renderPage = page;
        }
        else
        {
            // clear old render cache
            cacheDescription.clear();
            renderPage = page;
        }
    }
}

GbmDocumentPage * GbmRenderer::getRenderPage() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return renderPage;
}

// ---------------------------------------------------------

void GbmRenderer::setScalerType(const GbmRenderer::ScalerType filter)
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (mScaleFilter != filter)
    {
       // clear old render cache to force recalculation
       cacheDescription.clear();
    }
    mScaleFilter = filter;
}

// ---------------------------------------------------------

GbmRenderer::ScalerType GbmRenderer::getScalerType() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return mScaleFilter;
}

// ---------------------------------------------------------

/**
 * @param src_x         x coordinate of upper left corner
 * @param src_y         y coordinate of upper left corner
 * @param src_width     width of rectangle to render
 * @param src_height    height of rectangle to render
 * @param scale_x       scale specified as pixels per point (width)
 * @param scale_y       scale specified as pixels per point (height)
 * @param rect          RECTL structure, defines render area on HPS
 * @param hps           handle of presentation space to render into
 *
 * 1. Scale the document to match the specified pixels per point
 * 2. Render the rectangle given by the upper left corner at
 *    (src_x, src_y) and src_width and src_height.
 **/

#if defined(__OS2__)
void GbmRenderer::renderToHPS(long   src_x    , long   src_y,
                              long   src_width, long   src_height,
                              double scale_x  , double scale_y,
                              const  RECTL  & render_area,
                              const  HPS      hps) throw ( GbmException )
#elif defined(WIN32)
void GbmRenderer::renderToHDC(long   src_x    , long   src_y,
                              long   src_width, long   src_height,
                              double scale_x  , double scale_y,
                              const  RECT  & render_area,
                              const  HDC     hdc) throw ( GbmException )
#endif
{
   #if defined(__OS2__)
    if (hps == NULLHANDLE)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Missing HPS handle");
    }
   #elif defined(WIN32)
    if (hdc == (HDC)0)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Missing HDC handle");
    }
   #endif
    if ((src_y < 0) || (src_x < 0) || (src_width <= 0) || (src_height <= 0))
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Bad render options");
    }

    MutexGuard interfaceGuard(mInterfaceMutex);
    if (renderPage == NULL)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "No render page set");
    }
    MutexGuard pageGuard(renderPage->getInterfaceMutex());
    
    BMPHEADER bmpHeader;
    GBMRGB    gbmrgb[0x100] = { 0 };
    BOOL      isGray        = FALSE;
    copyBmpHeaderAndGetPaletteAsGBMRGB(&bmpHeader, gbmrgb, &isGray);

    // update the render cache based on scale factors
    prepareRenderCache(scale_x, scale_y);

    BMPHEADER_WIDTH(bmpHeader)    = cacheDescription.width;
    BMPHEADER_HEIGHT(bmpHeader)   = cacheDescription.height;
    BMPHEADER_BITCOUNT(bmpHeader) = cacheDescription.bpp;
    BMPHEADER_CLRUSED(bmpHeader)  = cacheDescription.bpp > 8 ? 0 : (1 << cacheDescription.bpp);
    BMPHEADER_IMGSIZE(bmpHeader)  = (((cacheDescription.width * cacheDescription.bpp + 31)/32) * 4)
                                    * cacheDescription.height;

    // update the bitmap header and the color palette to be returned
    copyGBMRGB2BmpHeader(cacheDescription.gbmrgb, &bmpHeader);

    // protect against buffer overrun due to rounding errors
    if (src_x + src_width > cacheDescription.width)
    {
        src_width = cacheDescription.width - src_x;
    }
    if (src_y + src_height > cacheDescription.height)
    {
        src_height = cacheDescription.height - src_y;
    }

    // Draw the bitmap
    // ===============

   #if defined(__OS2__)

    // set forground/background color if we render a 1bpp bitmap
    if (BMPHEADER_BITCOUNT(bmpHeader) == 1)
    {
        const RGB2 * pRGBA = bmpHeader.argb2Color;

        const LONG colorFg = (pRGBA[0].bRed << 16) + (pRGBA[0].bGreen << 8) + pRGBA[0].bBlue;
        const LONG colorBg = (pRGBA[1].bRed << 16) + (pRGBA[1].bGreen << 8) + pRGBA[1].bBlue;

        LONG colorTable[2] = { colorBg, colorFg };

        GpiCreateLogColorTable(hps, 0, LCOLF_CONSECRGB, 0, 2, (PLONG) colorTable);
        /* if something goes wrong when creating color table, use the closest color */
        GpiSetBackColor(hps, GpiQueryColorIndex(hps, 0, colorBg));
        GpiSetColor    (hps, GpiQueryColorIndex(hps, 0, colorFg));
    }

    RECTL aptlPoints[2] = { 0 };
    aptlPoints[0] = render_area;

    RECT_LEFT  (aptlPoints[1]) = src_x;
    RECT_BOTTOM(aptlPoints[1]) = cacheDescription.height - (src_y + src_height);
    RECT_RIGHT (aptlPoints[1]) = RECT_LEFT  (aptlPoints[1]) + src_width;
    RECT_TOP   (aptlPoints[1]) = RECT_BOTTOM(aptlPoints[1]) + src_height;

    if (GpiDrawBits(hps,
                    (PBYTE) cacheDescription.getDataPtr(),
                    (PBITMAPINFO2)(&bmpHeader),
                    4L,
                    (PPOINTL) aptlPoints,
                    ROP_SRCCOPY,
                    BBO_IGNORE ) == GPI_ERROR)
    {
        throw GbmException(GBM_ERR_MEM);
    }

   #elif defined(WIN32)

    SetStretchBltMode(hdc, STRETCH_DELETESCANS);
    if (StretchDIBits(hdc,
                      RECT_LEFT(render_area), RECT_TOP(render_area),
                      RECT_RIGHT(render_area)  - RECT_LEFT(render_area),
                      RECT_BOTTOM(render_area) - RECT_TOP(render_area),
                      src_x, src_y,
                      src_width,
                      src_height,
                      cacheDescription.getDataPtr(),
                      (const BITMAPINFO *)(&bmpHeader),
                      DIB_RGB_COLORS,
                      SRCCOPY) == GDI_ERROR)
    {
        throw GbmException(GBM_ERR_MEM);
    }
   #endif
}

// ---------------------------------------------------------

/**
 * @param src_x         x coordinate of upper left corner
 * @param src_y         y coordinate of upper left corner
 * @param src_width     width of rectangle to render
 * @param src_height    height of rectangle to render
 * @param scale_x       scale specified as pixels per point (width)
 * @param scale_y       scale specified as pixels per point (height)
 *
 * @return the required buffer size in bytes
 **/

size_t GbmRenderer::calcRenderBufferSize(long     src_x    , long   src_y,
                                         long     src_width, long   src_height,
                                         double   scale_x  , double scale_y) const throw ( GbmException )
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if (renderPage == NULL)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "No render page set");
    }
    if ((scale_x < 0.0) || (scale_y < 0.0) ||
        (src_y < 0)     || (src_x < 0)     || (src_width <= 0) || (src_height <= 0))
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Bad render options");
    }
    MutexGuard pageGuard(renderPage->getInterfaceMutex());
    
    // calc visible rectangle of the drawing buffer
    GBM renderGbm;

    // Needs additional scaling ?
    if ((scale_x != 1.0) || (scale_y != 1.0))
    {
        BMPHEADER bmpHeader;
        GBMRGB    gbmrgb[0x100] = { 0 };
        BOOL      isGray        = FALSE;
        copyBmpHeaderAndGetPaletteAsGBMRGB(&bmpHeader, gbmrgb, &isGray);

        renderGbm.w   = (int) ((scale_x * ((double) (renderPage->getWidth() ))) + 0.5);
        renderGbm.h   = (int) ((scale_y * ((double) (renderPage->getHeight()))) + 0.5);
        renderGbm.bpp = (isGray && (mScaleFilter != ScalerType_SIMPLE)) ? 8 : renderPage->getColorDepth();
    }
    else
    {
        renderGbm.w   = renderPage->getWidth();
        renderGbm.h   = renderPage->getHeight();
        renderGbm.bpp = renderPage->getColorDepth();
    }

    // protect against buffer overrun due to rounding errors
    if (src_x + src_width > renderGbm.w)
    {
        src_width = renderGbm.w - src_x;
    }
    if (src_y + src_height > renderGbm.h)
    {
        src_height = renderGbm.h - src_y;
    }

    const size_t stride  = (((size_t)src_width * renderGbm.bpp + 31)/32) * 4;
    const size_t dataLen = stride * src_height;

    return dataLen;
}

// ---------------------------------------------------------

/**
 * Scale the document to match the specified pixels per point
 *
 * @param scale_x       scale specified as pixels per point (width)
 * @param scale_y       scale specified as pixels per point (height)
 **/
void GbmRenderer::prepareRenderCache(const double scale_x,
                                     const double scale_y) throw (GbmException)
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (renderPage == NULL)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "No render page set");
    }
    if ((scale_x < 0.0) || (scale_y < 0.0))
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Bad render options");
    }
    MutexGuard pageGuard(renderPage->getInterfaceMutex());
    
    BMPHEADER bmpHeader;
    GBMRGB    gbmrgb[0x100] = { 0 };
    BOOL      isGray        = FALSE;
    copyBmpHeaderAndGetPaletteAsGBMRGB(&bmpHeader, gbmrgb, &isGray);

    // Check if any important render parameters (scaling, page size) have changed.
    if ((scale_x                     != cacheDescription.scale_x) ||
        (scale_y                     != cacheDescription.scale_y) ||
        (renderPage->getColorDepth() != unscaledBpp)              ||
        (renderPage->getWidth()      != unscaledWidth)            ||
        (renderPage->getHeight()     != unscaledHeight))
    {
        // force full recalculation of target bitmap
        cacheDescription.clear();
        unscaledBpp    = renderPage->getColorDepth();
        unscaledWidth  = renderPage->getWidth();
        unscaledHeight = renderPage->getHeight();
    }

          int      copySrcWidth  = cacheDescription.width;
          int      copySrcHeight = cacheDescription.height;
          size_t   copySrcStride = cacheDescription.rowStride;
          int      copySrcBpp    = cacheDescription.bpp;
    const gbm_u8 * copySrcData   = cacheDescription.getDataPtr();
          gbm_u8 * scaledData    = NULL;

    if (copySrcData == NULL)
    {
        copySrcWidth  = renderPage->getWidth();
        copySrcHeight = renderPage->getHeight();
        copySrcStride = renderPage->getBitmapDataRowSize();
        copySrcBpp    = renderPage->getColorDepth();
        copySrcData   = (const gbm_u8 *) renderPage->getBitmapDataPtr();

        // Needs additional scaling ?
        if ((scale_x != 1.0) || (scale_y != 1.0))
        {
            const int    scaledWidth   = (int) ((scale_x * ((double) (copySrcWidth ))) + 0.5);
            const int    scaledHeight  = (int) ((scale_y * ((double) (copySrcHeight))) + 0.5);
            const int    scaledBpp     = (isGray && (mScaleFilter != ScalerType_SIMPLE)) ? 8 : copySrcBpp;
            const size_t scaledStride  = (((size_t)scaledWidth * scaledBpp + 31)/32) * 4;
            const size_t scaledDataLen = scaledStride * scaledHeight;

            scaledData = (gbm_u8 *)gbmmem_malloc(scaledDataLen);
            if (scaledData != NULL)
            {
                GBM_ERR gbmrc = GBM_ERR_OK;

                // scale to requested size
                if ((mScaleFilter != ScalerType_SIMPLE) &&
                    (isGray || (renderPage->getColorDepth() > 8)))
                {
                    // map to GBM scale filter (1:1 int)
                    const GBM_SCALE_FILTER gbmScaleFilter = (GBM_SCALE_FILTER) mScaleFilter;

                    if (isGray)
                    {
                        gbmrc = gbm_quality_scale_gray(copySrcData, copySrcWidth  , copySrcHeight,
                                                       renderPage->getColorDepth(), gbmrgb,
                                                       scaledData , scaledWidth   , scaledHeight,
                                                       gbmrgb     , gbmScaleFilter);
                    }
                    else
                    {
                        gbmrc = gbm_quality_scale_bgra(copySrcData, copySrcWidth  , copySrcHeight,
                                                       scaledData , scaledWidth   , scaledHeight,
                                                       renderPage->getColorDepth(), gbmScaleFilter);
                    }
                }
                else
                {
                    gbmrc = gbm_simple_scale(copySrcData, copySrcWidth , copySrcHeight,
                                             scaledData , scaledWidth  , scaledHeight,
                                             renderPage->getColorDepth());
                }
                if (gbmrc == GBM_ERR_OK)
                {
                    // set the copy source to the scaled data
                    copySrcWidth  = scaledWidth;
                    copySrcHeight = scaledHeight;
                    copySrcStride = scaledStride;
                    copySrcBpp    = scaledBpp;
                    copySrcData   = scaledData;

                    // update local bitmap header
                    BMPHEADER_WIDTH   (bmpHeader) = scaledWidth;
                    BMPHEADER_HEIGHT  (bmpHeader) = scaledHeight;
                    BMPHEADER_BITCOUNT(bmpHeader) = scaledBpp;
                    BMPHEADER_CLRUSED (bmpHeader) = scaledBpp > 8 ? 0 : (1 << scaledBpp);
                    BMPHEADER_IMGSIZE (bmpHeader) = (((scaledWidth * scaledBpp + 31)/32) * 4) * scaledHeight;
                }
                else
                {
                    gbmmem_free(scaledData);
                    scaledData = NULL;
                    throw GbmException(gbmrc);
                }
            }
            else
            {
                /* In case we had a problem with scaling (e.g. out of memory), */
                /* render nothing (or later show an error icon?).              */
                throw GbmException(GBM_ERR_MEM);
            }
        }

        // For black & white bitmaps we need special data ordering to draw the bitmap
        // correctly in PM presentation space.
        if (isGray && (copySrcBpp == 1) && (gbmrgb[0].r != 255))
        {
            // swap color table entries
            const GBMRGB rgb(gbmrgb[0]);
            gbmrgb[0] = gbmrgb[1];
            gbmrgb[1] = rgb;

            const size_t dataStride1 = (((size_t)copySrcWidth * copySrcBpp + 31)/32) * 4;
            const size_t dataLen1    = dataStride1 * copySrcHeight;

            const size_t numWords = dataLen1 / sizeof(unsigned long *);
            const size_t numBytes = dataLen1 - (numWords * sizeof(unsigned long *));

            if (scaledData == NULL)
            {
                // allocate a new buffer as no scaling happened
                scaledData = (gbm_u8 *)gbmmem_malloc(dataLen1);
                if (scaledData == NULL)
                {
                    throw GbmException(GBM_ERR_MEM);
                }

                // invert and copy data using long pointer for better performance
                const unsigned long * dataSrcWord = (const unsigned long *)copySrcData;
                      unsigned long * dataDstWord = (      unsigned long *)scaledData;
                for (size_t word = 0; word < numWords; ++word, ++dataSrcWord, ++dataDstWord)
                {
                    *dataDstWord = ~(*dataSrcWord);
                }
                const gbm_u8 * dataSrcByte = (const gbm_u8 *)dataSrcWord;
                      gbm_u8 * dataDstByte = (      gbm_u8 *)dataDstWord;
                for (size_t byte = 0; byte < numBytes; ++byte, ++dataSrcByte, ++dataDstByte)
                {
                    *dataDstByte = ~(*dataSrcByte);
                }

                // update data copy source
                copySrcData = scaledData;
            }
            else
            {
                // invert data using long pointer for better performance
                unsigned long * dataWord = (unsigned long *)scaledData;
                for (size_t word = 0; word < numWords; ++word, ++dataWord)
                {
                    *dataWord = ~(*dataWord);
                }
                gbm_u8 * dataByte = (gbm_u8 *)dataWord;
                for (size_t byte = 0; byte < numBytes; ++byte, ++dataByte)
                {
                    *dataByte = ~(*dataByte);
                }
            }
        }

        cacheDescription.width     = copySrcWidth;
        cacheDescription.height    = copySrcHeight;
        cacheDescription.rowStride = copySrcStride;
        cacheDescription.bpp       = copySrcBpp;
        cacheDescription.scale_x   = scale_x;
        cacheDescription.scale_y   = scale_y;
        memcpy(cacheDescription.gbmrgb, gbmrgb, sizeof(cacheDescription.gbmrgb));

        if (copySrcData == scaledData)
        {
            // this data can be freed as we own it
            cacheDescription.setData((gbm_u8 *) scaledData);
        }
        else
        {
            // just store the reference which will not be freed
            cacheDescription.setConstData(copySrcData);

            // Don't free the scaledData buffer if it points
            // to the same address as the cache.
            gbmmem_free(scaledData);
            scaledData = NULL;
        }
    }
}

// ---------------------------------------------------------

/**
 * @param src_x         x coordinate of upper left corner
 * @param src_y         y coordinate of upper left corner
 * @param src_width     width of rectangle to render
 * @param src_height    height of rectangle to render
 * @param scale_x       scale specified as pixels per point (width)
 * @param scale_y       scale specified as pixels per point (height)
 * @param renderBuffer  render buffer (4 byte aligned row size according
 *                      to src_width and page colour depth of page set)
 * @param bmpHeader     the bitmapinfo for direct use by GPI
 *
 * 1. Scale the document to match the specified pixels per point
 * 2. Render the rectangle given by the upper left corner at
 *    (src_x, src_y) and src_width and src_height.
 **/

void GbmRenderer::renderToBuffer(long        src_x    , long   src_y,
                                 long        src_width, long   src_height,
                                 double      scale_x  , double scale_y,
                                 unsigned char * renderBuffer,
                                 BMPHEADER * bmpHeader) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (renderPage == NULL)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "No render page set");
    }
    if (bmpHeader == NULL)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Missing bitmap header");
    }
    if ((src_y < 0) || (src_x < 0) || (src_width <= 0) || (src_height <= 0))
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Bad render options");
    }
    MutexGuard pageGuard(renderPage->getInterfaceMutex());

    GBMRGB gbmrgb[0x100] = { 0 };
    BOOL   isGray        = FALSE;
    copyBmpHeaderAndGetPaletteAsGBMRGB(bmpHeader, gbmrgb, &isGray);

    prepareRenderCache(scale_x, scale_y);

    // update the bitmap header and the color palette to be returned
    copyGBMRGB2BmpHeader(cacheDescription.gbmrgb, bmpHeader);

    // copy visible rectangle to the drawing buffer
    GBM renderGbm;
    renderGbm.w   = cacheDescription.width;
    renderGbm.h   = cacheDescription.height;
    renderGbm.bpp = cacheDescription.bpp;

    // protect against buffer overrun due to rounding errors
    if (src_x + src_width > renderGbm.w)
    {
        src_width = renderGbm.w - src_x;
    }
    if (src_y + src_height > renderGbm.h)
    {
        src_height = renderGbm.h - src_y;
    }

    gbm_subrectangle(&renderGbm,
                      src_x, renderGbm.h-(src_y+src_height), src_width, src_height,
                      cacheDescription.getDataPtr(), (gbm_u8 *)renderBuffer);

    BMPHEADER_WIDTH   (*bmpHeader) = src_width;
    BMPHEADER_HEIGHT  (*bmpHeader) = src_height;
    BMPHEADER_BITCOUNT(*bmpHeader) = renderGbm.bpp;
    BMPHEADER_CLRUSED (*bmpHeader) = renderGbm.bpp > 8 ? 0 : (1 << renderGbm.bpp);
    BMPHEADER_IMGSIZE (*bmpHeader) = (((src_width * renderGbm.bpp + 31)/32) * 4) * src_height;
}

// ---------------------------------------------------------

/**
 * @param src_x           x coordinate of upper left corner
 * @param src_y           y coordinate of upper left corner
 * @param src_width       width of rectangle to render
 * @param src_height      height of rectangle to render
 * @param scale_x         scale specified as pixels per point (width)
 * @param scale_y         scale specified as pixels per point (height)
 * @param renderBuffer24  render buffer (4 byte aligned row size according
 *                        to src_width and 24bpp colour depth)
 * @param bmpHeader       the bitmapinfo for direct use by GPI
 *
 * 1. Scale the document to match the specified pixels per point
 * 2. Render the rectangle given by the upper left corner at
 *    (src_x, src_y) and src_width and src_height.
 **/

void GbmRenderer::renderToBuffer24bpp(long            src_x    , long   src_y,
                                      long            src_width, long   src_height,
                                      double          scale_x  , double scale_y,
                                      unsigned char * renderBuffer24,
                                      BMPHEADER     * bmpHeader) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (renderPage == NULL)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "No render page set");
    }
    MutexGuard pageGuard(renderPage->getInterfaceMutex());
    
    // if the source is already 24bpp, just forward to direct render function
    if (renderPage->getColorDepth() == 24)
    {
        renderToBuffer(src_x         , src_y,
                       src_width     , src_height,
                       scale_x       , scale_y,
                       renderBuffer24, bmpHeader);
        return;
    }

    if ((scale_x < 0.0) || (scale_y < 0.0) ||
        (src_y < 0)     || (src_x < 0)     || (src_width <= 0) || (src_height <= 0))
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Bad render options");
    }

    const size_t scaledDataLength = calcRenderBufferSize(src_x    , src_y,
                                                         src_width, src_height,
                                                         scale_x  , scale_y);
    gbm_u8 * scaledData = (gbm_u8 *)gbmmem_malloc(scaledDataLength);
    if (scaledData != NULL)
    {
        // first prepare the bitmap as usual but then expand it to 24bpp
        try
        {
            renderToBuffer(src_x     , src_y,
                           src_width , src_height,
                           scale_x   , scale_y,
                           scaledData, bmpHeader);

            expandTo24bpp(bmpHeader , src_width, src_height,
                          (gbm_u8 *)scaledData, (gbm_u8 *)renderBuffer24);

            /* update relevant bitmap header data */
            const unsigned long bmpDataStride = (((unsigned long)src_width * 24 + 31)/32) * 4;
            const unsigned long bmpDataLength = bmpDataStride * src_height;

            BMPHEADER_BITCOUNT(*bmpHeader) = 24;            /*  Number of bits per pel within a plane. */
            BMPHEADER_CLRUSED (*bmpHeader) = 0;             /*  Number of used colors in the palette */
            BMPHEADER_IMGSIZE (*bmpHeader) = bmpDataLength; /*  Length of bit-map storage data, in bytes. */
        }
        catch(GbmException & ex)
        {
            gbmmem_free(scaledData);
            scaledData = NULL;
            throw ex;
        }
        gbmmem_free(scaledData);
        scaledData = NULL;
        // DONE
    }
    else
    {
        /* In case we had a problem with scaling (e.g. out of memory), */
        /* render nothing (or later show an error icon?).              */
        throw GbmException(GBM_ERR_MEM);
    }
}

// ---------------------------------------------------------

void GbmRenderer::expandTo24bpp(const BMPHEADER * bmpHeader,
                                const long src_w, const long src_h,
                                const gbm_u8 *data, gbm_u8 * data24) const throw ( GbmException )
{
    if ((data == NULL) || (data24 == NULL) || (bmpHeader == NULL) ||
        (src_w < 1) || (src_h < 1))
    {
        throw GbmException(GBM_ERR_BAD_ARG);
    }
    if (BMPHEADER_BITCOUNT(*bmpHeader) >= 24)
    {
        throw GbmException(GBM_ERR_BAD_ARG);
    }

   #if defined(__OS2__)
    const RGB2    * pRGB = BMPHEADER_COLORS(*bmpHeader);
   #elif defined(WIN32)
    const RGBQUAD * pRGB = BMPHEADER_COLORS(*bmpHeader);
   #endif
    const size_t stride     = (((size_t)src_w * BMPHEADER_BITCOUNT(*bmpHeader) + 31)/32) * 4;
    const size_t new_stride = (((size_t)src_w * 3 + 3) & ~3);

    const gbm_u8 *src  = data;
          gbm_u8 *dest = data24;

    int  x, y;
    gbm_u8 c = 0;

    switch (BMPHEADER_BITCOUNT(*bmpHeader))
    {
        case 1:
        {
            for ( y = 0; y < src_h; y++ )
            {
                const gbm_u8 *srcp  = src;
                      gbm_u8 *destp = dest;
                for ( x = 0; x < src_w; x++ )
                {
                    if ( (x & 7) == 0 )
                    {
                        c = *srcp++;
                    }
                    else
                    {
                        c <<= 1;
                    }
                    *destp++ = RGB_COLOR_B( pRGB[c >> 7] );
                    *destp++ = RGB_COLOR_G( pRGB[c >> 7] );
                    *destp++ = RGB_COLOR_R( pRGB[c >> 7] );
                }
                src  += stride;
                dest += new_stride;
            }
        }
        break;

        case 4:
        {
            for ( y = 0; y < src_h; y++ )
            {
                const gbm_u8 *srcp  = src;
                      gbm_u8 *destp = dest;
                for ( x = 0; x + 1 < src_w; x += 2 )
                {
                    c       = *srcp++;
                    *destp++ = RGB_COLOR_B( pRGB[c >> 4] );
                    *destp++ = RGB_COLOR_G( pRGB[c >> 4] );
                    *destp++ = RGB_COLOR_R( pRGB[c >> 4] );
                    *destp++ = RGB_COLOR_B( pRGB[c & 15] );
                    *destp++ = RGB_COLOR_G( pRGB[c & 15] );
                    *destp++ = RGB_COLOR_R( pRGB[c & 15] );
                }

                if ( x < src_w )
                {
                    c       = *srcp;
                    *destp++ = RGB_COLOR_B( pRGB[c >> 4] );
                    *destp++ = RGB_COLOR_G( pRGB[c >> 4] );
                    *destp++ = RGB_COLOR_R( pRGB[c >> 4] );
                }
                src  += stride;
                dest += new_stride;
            }
        }
        break;

        case 8:
        {
            for ( y = 0; y < src_h; y++ )
            {
                const gbm_u8 *srcp  = src;
                      gbm_u8 *destp = dest;
                for ( x = 0; x < src_w; x++ )
                {
                    c       = *srcp++;
                    *destp++ = RGB_COLOR_B( pRGB[c] );
                    *destp++ = RGB_COLOR_G( pRGB[c] );
                    *destp++ = RGB_COLOR_R( pRGB[c] );
                }
                src  += stride;
                dest += new_stride;
            }
        }
        break;

        default:
           throw GbmException(GBM_ERR_NOT_SUPP);
    }
}

/* ---------------------------- */

void GbmRenderer::copyBmpHeaderAndGetPaletteAsGBMRGB(BMPHEADER *bmpHeader,
                                                     GBMRGB    *gbmrgb,
                                                     BOOL      *isGrayscale)
                                                         const throw ( GbmException )
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    MutexGuard pageGuard(renderPage->getInterfaceMutex());
    
   #if defined(__OS2__)
    const BITMAPINFO2 * bmpInfo = renderPage->getBitmapInfoPtr();
   #elif defined(WIN32)
    const BITMAPINFO * bmpInfo = renderPage->getBitmapInfoPtr();
   #endif
    if (bmpInfo == NULL)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Render page has no bitmap header");
    }

    memset(gbmrgb, 0, sizeof(GBMRGB) * 0x100);

    if (renderPage->getColorDepth() > 8)
    {
        *isGrayscale = FALSE;
        memset(bmpHeader, 0, sizeof(BMPHEADER));
       #if defined(__OS2__)
        memcpy(bmpHeader, bmpInfo, sizeof(BITMAPINFOHEADER2));
       #elif defined(WIN32)
        memcpy(bmpHeader, bmpInfo, sizeof(BITMAPINFOHEADER));
       #endif
    }
    else
    {
        memcpy(bmpHeader, bmpInfo, sizeof(BMPHEADER));

        const int palette_entries = (renderPage->getColorDepth() > 8)
                                    ? 0 : 1 << renderPage->getColorDepth();
        *isGrayscale = TRUE;

        for (int i = 0; i < palette_entries; i++)
        {
            GBMRGB * pRGB = &(gbmrgb[i]);

            pRGB->r = BMPHEADER_COLOR_R(*bmpHeader, i);
            pRGB->g = BMPHEADER_COLOR_G(*bmpHeader, i);
            pRGB->b = BMPHEADER_COLOR_B(*bmpHeader, i);

            if ((pRGB->r != pRGB->g) ||
                (pRGB->r != pRGB->b) ||
                (pRGB->g != pRGB->b))
            {
                *isGrayscale = FALSE;
            }
        }
    }
}

/* ---------------------------- */

void GbmRenderer::getPaletteAsGBMRGB(GBMRGB *gbmrgb,
                                     BOOL   *isGrayscale) const throw (GbmException)
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    MutexGuard pageGuard(renderPage->getInterfaceMutex());
    
    const BMPHEADER * bmpHeader = (const BMPHEADER *) renderPage->getBitmapInfoPtr();
    if (bmpHeader == NULL)
    {
        throw GbmException(GBM_ERR_BAD_ARG, "Render page has no bitmap header");
    }

    memset(gbmrgb, 0, sizeof(GBMRGB) * 0x100);

    if (renderPage->getColorDepth() > 8)
    {
        *isGrayscale = FALSE;
    }
    else
    {
        const int palette_entries = (renderPage->getColorDepth() > 8)
                                    ? 0 : 1 << renderPage->getColorDepth();
        *isGrayscale = TRUE;

        for (int i = 0; i < palette_entries; i++)
        {
            GBMRGB * pRGB = &(gbmrgb[i]);

            pRGB->r = BMPHEADER_COLOR_R(*bmpHeader, i);
            pRGB->g = BMPHEADER_COLOR_G(*bmpHeader, i);
            pRGB->b = BMPHEADER_COLOR_B(*bmpHeader, i);

            if ((pRGB->r != pRGB->g) ||
                (pRGB->r != pRGB->b) ||
                (pRGB->g != pRGB->b))
            {
                *isGrayscale = FALSE;
            }
        }
    }
}

/* ---------------------------- */

void GbmRenderer::copyGBMRGB2BmpHeader(const GBMRGB    * gbmrgb,
                                             BMPHEADER * bmpHeader) const
{
    for (int i = 0; i < 256; i++)
    {
        BMPHEADER_COLOR_R(*bmpHeader, i) = gbmrgb[i].r;
        BMPHEADER_COLOR_G(*bmpHeader, i) = gbmrgb[i].g;
        BMPHEADER_COLOR_B(*bmpHeader, i) = gbmrgb[i].b;
    }
}


