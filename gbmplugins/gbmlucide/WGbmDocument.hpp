
//***************************************************
// Lucide GBM plug-in: WGbmDocument.hpp
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

#ifndef __CLASS_W_GBM_DOCUMENT__
    #define __CLASS_W_GBM_DOCUMENT__

#include "gbm.h"

#include "GbmException.hpp"
#include "GbmDocument.hpp"
#include "GbmRenderer.hpp"
#include "Semaphore.hpp"

// ---------------------------------------------------------

class WGbmDocument
{
    private:

        WGbmDocument();
        WGbmDocument(const WGbmDocument & doc);
        WGbmDocument & operator = ( const WGbmDocument & doc );

        // --- data definitions ---

        GbmDocument   gbmDocument;
        int         * pageRotation;
        GbmRenderer::ScalerType mDefaultScaler;

        int mLastRenderedPageNumber;

        MutexSemaphore mInterfaceMutex;

    public:

        WGbmDocument(const GbmAccessor & accessor);
        virtual ~WGbmDocument();

        MutexSemaphore & getInterfaceMutex();

        void setConfigurationFile(const char * filename) throw ( GbmException );

        void setDocumentFile(const char * filename) throw ( GbmException );
        void writeDocumentPageToFile(const char * filename,
                                     const int    page) throw ( GbmException );

        void setPageRotation(const int page, const int rotation) throw ( GbmException );
        int  getPageRotation(const int page) const throw ( GbmException );

        const GbmAccessor & getGbmAccessor();
              GbmDocument & getGbmDocument();

        GbmRenderer::ScalerType getDefaultScalerType() const;

        void setLastRenderedPage(const int page);
        int  getLastRenderedPage() const;
};

#endif


