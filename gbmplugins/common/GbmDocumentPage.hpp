
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

#ifndef __CLASS_GBM_DOCUMENT_PAGE__
    #define __CLASS_GBM_DOCUMENT_PAGE__

#if defined(__OS2__)
  #include "os2defs.h"
#elif defined(WIN32)
  #include "windefs.h"
#endif

#include "GbmAccessor.hpp"
#include "GbmException.hpp"
#include "Semaphore.hpp"

// ---------------------------------------------------------

class GbmDocumentPage
{
    public:

        enum MirrorMode { MirrorMode_NONE = 0,
                          MirrorMode_HORIZONTAL,
                          MirrorMode_VERTICAL,
                          MirrorMode_TRANSPOSE };

        GbmDocumentPage(const GbmAccessor & accessor);
        virtual ~GbmDocumentPage();

        void loadFromFile(const char * filename,
                          const int    pageNumber,
                          const int    gbmFileType,
                          const BOOL   allowDelayedDataLoading,
                          const BOOL   to24bpp) throw ( GbmException );

        void writeToFile(const char * filename,
                         const char * options) throw ( GbmException );

        BOOL isValid() const;

        int getWidth();
        int getHeight();
        int getColorDepth() const;

        void rotate(const int rotationAngle) throw ( GbmException );
        void mirror(const MirrorMode mode)   throw ( GbmException );

        MutexSemaphore & getInterfaceMutex();
              size_t        getBitmapDataRowSize();
        const BYTE        * getBitmapDataPtr();
      #if defined(__OS2__)
        const BITMAPINFO2 * getBitmapInfoPtr();
      #elif defined(WIN32)
        const BITMAPINFO * getBitmapInfoPtr();
      #endif

    // -----------------------------

    private:

        const GbmAccessor & mGbmAccessor;
        MutexSemaphore      mInterfaceMutex;

      #if defined(__OS2__)
        BITMAPINFO2 * pHeader;
      #elif defined(WIN32)
        BITMAPINFO  * pHeader;
      #endif
        gbm_u8      * data;
        char        * filename;
        int           pageNumber;
        int           gbmFileType;
        BOOL          requestedAs24bpp;
        GBM           gbm;
        GBMRGB        gbmrgb[0x100];

        GbmDocumentPage();
        GbmDocumentPage(const GbmDocumentPage & page);
        GbmDocumentPage & operator = ( const GbmDocumentPage & ex );

        void internLoadFromFile(const BOOL includeData) throw ( GbmException );

        BOOL expandTo24bpp();
        void updateBitmapHeader();

};

#endif


