
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

#ifndef __CLASS_GBM_RENDERER__
    #define __CLASS_GBM_RENDERER__

#include "gbm.h"
#include "gbmmem.h"
#include "gbmscale.h"

#include "GbmException.hpp"
#include "GbmDocumentPage.hpp"
#include "Semaphore.hpp"

// ---------------------------------------------------------

class GbmRenderer
{
    public:

        enum ScalerType { ScalerType_SIMPLE          = 0,
                          ScalerType_NEARESTNEIGHBOR = GBM_SCALE_FILTER_NEARESTNEIGHBOR,
                          ScalerType_BILINEAR        = GBM_SCALE_FILTER_BILINEAR,
                          ScalerType_BELL            = GBM_SCALE_FILTER_BELL,
                          ScalerType_BSPLINE         = GBM_SCALE_FILTER_BSPLINE,
                          ScalerType_QUADRATIC       = GBM_SCALE_FILTER_QUADRATIC,
                          ScalerType_GAUSSIAN        = GBM_SCALE_FILTER_GAUSSIAN,
                          ScalerType_MITCHELL        = GBM_SCALE_FILTER_MITCHELL,
                          ScalerType_LANCZOS         = GBM_SCALE_FILTER_LANCZOS,
                          ScalerType_BLACKMAN        = GBM_SCALE_FILTER_BLACKMAN,
                          ScalerType_KAISER          = GBM_SCALE_FILTER_KAISER,
                          ScalerType_CATMULLROM      = GBM_SCALE_FILTER_CATMULLROM };

        #pragma pack(2)
        #if defined(__OS2__)
            typedef struct
            {
               BITMAPINFOHEADER2 bmp2;
               RGB2 argb2Color[0x100]; // unused for bitmaps >8bpp
            } BMPHEADER;
        #elif defined(WIN32)
            typedef struct
            {
               BITMAPINFOHEADER bmiHeader;
               RGBQUAD bmiColors[0x100]; // unused for bitmaps >8bpp
            } BMPHEADER;
        #endif
        #pragma pack()
        
        // -------------------

        GbmRenderer();
        virtual ~GbmRenderer();

        ScalerType getScalerType() const;

       #if defined(__OS2__)
        void renderToHPS(long   src_x    , long   src_y,
                         long   src_width, long   src_height,
                         double scale_x  , double scale_y,
                         const  RECTL  & render_area,
                         const  HPS      hps) throw ( GbmException );
        #elif defined(WIN32)
        void renderToHDC(long   src_x    , long   src_y,
                         long   src_width, long   src_height,
                         double scale_x  , double scale_y,
                         const  RECT   & render_area,
                         const  HDC      hdc) throw ( GbmException );
        #endif
                         
        size_t calcRenderBufferSize(long     src_x    , long   src_y,
                                    long     src_width, long   src_height,
                                    double   scale_x  , double scale_y) const throw ( GbmException );

        void renderToBuffer(long            src_x    , long   src_y,
                            long            src_width, long   src_height,
                            double          scale_x  , double scale_y,
                            unsigned char * renderBuffer,
                            BMPHEADER     * bmpHeader) throw ( GbmException );

        void renderToBuffer24bpp(long            src_x    , long   src_y,
                                 long            src_width, long   src_height,
                                 double          scale_x  , double scale_y,
                                 unsigned char * renderBuffer24,
                                 BMPHEADER     * bmpHeader) throw ( GbmException );

    // ---------------------------------

    private:

        GbmRenderer(const GbmRenderer & page);
        GbmRenderer & operator = ( const GbmRenderer & r );

        void expandTo24bpp(const BMPHEADER * bmpHeader,
                           const long src_w, const long src_h,
                           const gbm_u8 *data, gbm_u8 * data24) const throw ( GbmException );

        void copyBmpHeaderAndGetPaletteAsGBMRGB(BMPHEADER *bmpHeader,
                                                GBMRGB    *gbmrgb,
                                                BOOL      *isGrayscale8bpp)
                                                            const throw ( GbmException );

        void getPaletteAsGBMRGB(GBMRGB *gbmrgb,
                                BOOL   *isGrayscale) const throw ( GbmException );

        void copyGBMRGB2BmpHeader(const GBMRGB    * gbmrgb,
                                        BMPHEADER * bmpHeader) const;

        // -------------------

        class CacheDescription
        {
            private:
                const gbm_u8 * constData;
                      gbm_u8 * data;

            public:
                int     width;
                int     height;
                size_t  rowStride;
                int     bpp;
                GBMRGB  gbmrgb[0x100];
                double  scale_x, scale_y;

                CacheDescription()
                    : constData(NULL),
                      data(NULL),
                      width(0),
                      height(0),
                      rowStride(0),
                      bpp(0),
                      scale_x(1.0),
                      scale_y(1.0)
                {
                    memset(gbmrgb, 0, sizeof(gbmrgb));
                }

                virtual ~CacheDescription()
                {
                    clear();
                }

                void setConstData(const gbm_u8 * const_data)
                {
                    constData = const_data;
                    gbmmem_free(data);
                    data = NULL;
                }
                void setData(gbm_u8 * data)
                {
                    constData = NULL;
                    gbmmem_free(this->data);
                    this->data = data;
                }
                BOOL isConstData() const
                {
                    return (constData != NULL);
                }
                const gbm_u8 * getDataPtr() const
                {
                    return isConstData() ? constData : data;
                }

                void clear()
                {
                    gbmmem_free(data);
                    data      = NULL;
                    constData = NULL;
                    width     = 0;
                    height    = 0;
                    rowStride = 0;
                    bpp       = 0;
                    memset(gbmrgb, 0, sizeof(gbmrgb));
                    scale_x   = 1.0;
                    scale_y   = 1.0;
                }
        };

        GbmDocumentPage  * renderPage;
        CacheDescription   cacheDescription;
        MutexSemaphore     mInterfaceMutex;

        int unscaledWidth;
        int unscaledHeight;
        int unscaledBpp;

        ScalerType mScaleFilter;

        // --- friends ---

        friend class GbmDocument;

        void prepareRenderCache(const double scale_x,
                                const double scale_y) throw ( GbmException );

        void setRenderPage(GbmDocumentPage * page);
        GbmDocumentPage * getRenderPage() const;

        void setScalerType(const ScalerType filter);
};

#endif


