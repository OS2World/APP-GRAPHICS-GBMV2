
//***************************************************
// Lucide GBM plug-in: WGbmDocument.cpp
//
// Copyright (C) 2006-2012 Heiko Nitzsche
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "WGbmDocument.hpp"
#include "ConfigHandler.hpp"

// ---------------------------------------------------------

typedef struct
{
    const char *            name;
    GbmRenderer::ScalerType type;
} SCALER;

static const SCALER SCALER_TABLE[] =
{
    { "simple"         , GbmRenderer::ScalerType_SIMPLE          },
    { "nearestneighbor", GbmRenderer::ScalerType_NEARESTNEIGHBOR },
    { "bilinear"       , GbmRenderer::ScalerType_BILINEAR        },
    { "bell"           , GbmRenderer::ScalerType_BELL            },
    { "bspline"        , GbmRenderer::ScalerType_BSPLINE         },
    { "quadratic"      , GbmRenderer::ScalerType_QUADRATIC       },
    { "gaussian"       , GbmRenderer::ScalerType_QUADRATIC       },
    { "mitchell"       , GbmRenderer::ScalerType_MITCHELL        },
    { "lanczos"        , GbmRenderer::ScalerType_LANCZOS         },
    { "blackman"       , GbmRenderer::ScalerType_BLACKMAN        },
    { "catmullrom"     , GbmRenderer::ScalerType_CATMULLROM      },
    { "kaiser"         , GbmRenderer::ScalerType_KAISER          }
};
static const int SCALER_TABLE_LENGTH = sizeof(SCALER_TABLE)/sizeof(SCALER_TABLE[0]);

// ---------------------------------------------------------

WGbmDocument::WGbmDocument(const GbmAccessor & accessor)
    : gbmDocument(accessor),
      pageRotation(NULL),
      mDefaultScaler(GbmRenderer::ScalerType_SIMPLE),
      mLastRenderedPageNumber(-1)
{
}

// ---------------------------------------------------------

WGbmDocument::~WGbmDocument()
{
    mInterfaceMutex.lock();

    delete [] pageRotation;
    pageRotation = NULL;
}

// ---------------------------------------------------------

MutexSemaphore & WGbmDocument::getInterfaceMutex()
{
    return mInterfaceMutex;
}

// ---------------------------------------------------------

GbmRenderer::ScalerType WGbmDocument::getDefaultScalerType() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return mDefaultScaler;
}

// ---------------------------------------------------------

void WGbmDocument::setConfigurationFile(const char * filename) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (filename == NULL)
    {
        throw GbmException(GBM_ERR_READ, "Configuration file does not exist");
    }

    ConfigHandler configHandler(filename);
    if (configHandler.read())
    {
        const char * scalerName = configHandler.getValueOfKey("scaler");
        if (scalerName != NULL)
        {
            for (int i = 0; i < SCALER_TABLE_LENGTH; i++)
            {
                if (strcmp(SCALER_TABLE[i].name, scalerName) == 0)
                {
                    mDefaultScaler = SCALER_TABLE[i].type;
                    break;
                }
            }
        }

        const char * backgroundRenderPages = configHandler.getValueOfKey("progressive_render_pages");
        if (backgroundRenderPages != NULL)
        {
            int num;
            if (sscanf(backgroundRenderPages, "%i", &num) == 1)
            {
                gbmDocument.setNumberOfBackgroundRenderPages(num);
            }
        }
    }
}

// ---------------------------------------------------------

void WGbmDocument::setDocumentFile(const char * filename) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);

    delete [] pageRotation;
    pageRotation = NULL;

    try
    {
        gbmDocument.setDocumentFile(filename, NULL  /* guess format from file/extension */,
                                              TRUE  /* allow delayed loading of page data */,
                                              FALSE /* render to HPS */);
    }
    catch(GbmException & ex)
    {
        delete [] pageRotation;
        pageRotation = NULL;
        throw ex;
    }

    // allocate the required number of pages
    const int numPages = gbmDocument.getNumberOfPages();

    try
    {
        pageRotation = new int[numPages];
    }
    catch(...)
    {
    }
    if (pageRotation == NULL)
    {
        throw GbmException(GBM_ERR_MEM);
    }

    // init the page array
    memset(pageRotation, 0, numPages * sizeof(int));

    // reset last rendered page
    mLastRenderedPageNumber = -1;
}

// ---------------------------------------------------------

void WGbmDocument::writeDocumentPageToFile(const char * filename,
                                           const int    page) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    gbmDocument.writeDocumentPageToFile(filename, "", page);
}

// ---------------------------------------------------------

void WGbmDocument::setPageRotation(const int page, const int rotation) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);

    if ((page < 0) || (page >= gbmDocument.getNumberOfPages()))
    {
        throw GbmException(GBM_ERR_NOT_SUPP, "Page number out of range");
    }
    if ((pageRotation == NULL) && (rotation != 0))
    {
        throw GbmException(GBM_ERR_NOT_SUPP, "Calling protocol violation. Set document file first");
    }

    // Rotate the page if necessary
    int rotateBy = rotation - pageRotation[page];

    if (rotateBy == 0)
    {
        // nothing to do
        return;
    }
    while (rotateBy < 0)
    {
        rotateBy += 360;
    }
    while (rotateBy > 360)
    {
        rotateBy -= 360;
    }

    // rotate the page
    gbmDocument.rotate(page, rotateBy);

    // update rotation cache
    pageRotation[page] = rotation;
}

// ---------------------------------------------------------

int WGbmDocument::getPageRotation(const int page) const throw ( GbmException )
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if ((page < 0) || (page >= gbmDocument.getNumberOfPages()))
    {
        throw GbmException(GBM_ERR_NOT_SUPP, "Page number out of range");
    }
    if (pageRotation == NULL)
    {
        return 0;
    }
    return pageRotation[page];
}

// ---------------------------------------------------------

const GbmAccessor & WGbmDocument::getGbmAccessor()
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    return gbmDocument.getGbmAccessor();
}

// ---------------------------------------------------------

GbmDocument & WGbmDocument::getGbmDocument()
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    return gbmDocument;
}

// ---------------------------------------------------------

void WGbmDocument::setLastRenderedPage(const int page)
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    mLastRenderedPageNumber = page;
}

// ---------------------------------------------------------

int WGbmDocument::getLastRenderedPage() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return mLastRenderedPageNumber;
}


