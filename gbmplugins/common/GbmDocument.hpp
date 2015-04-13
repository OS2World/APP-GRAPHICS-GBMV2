
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

#ifndef __CLASS_GBM_DOCUMENT__
    #define __CLASS_GBM_DOCUMENT__

#include "GbmException.hpp"
#include "GbmDocumentPage.hpp"
#include "GbmRenderer.hpp"
#include "GbmAccessor.hpp"
#include "Semaphore.hpp"
#include "Thread.hpp"

// ---------------------------------------------------------

class GbmDocument
{
    public:
        class BackgroundRenderQueue : public Thread
        {
          private:
            BOOL mFinish;

          public:
            struct TASK_ARG
            {
              GbmDocument * pGbmDocument;
              int         * pages;
              int           numPages;
              double        scaleX;
              double        scaleY;
            };

            MutexSemaphore mQueueMutex;

            BackgroundRenderQueue();
            virtual ~BackgroundRenderQueue();

            virtual void run(void * pTask);
            void stop();

            BOOL schedulePages(GbmDocument * pGbmDocument,
                               const int * pPages, int numPages, double scaleX, double scaleY);
        };

    private:
        // --- data definitions ---

        const GbmAccessor &  mGbmAccessor;

        GbmRenderer       ** renderers;
        GbmDocumentPage   ** pages;
        int                  numPages;

        char              *  filename;
        BOOL                 force24bpp;
        int                  gbmFileType;
        GBMFT                formatDescription;

        BackgroundRenderQueue mRenderQueue;
        int                   mNumRenderQueuePages;
        MutexSemaphore        mInterfaceMutex;

        struct RENDERER_INFO
        {
            GbmRenderer             * pRenderer;
            GbmRenderer::ScalerType   scalerType;
            double                    scaleX;
            double                    scaleY;
        };
        struct RENDERER_INFO mFocusRendererInfo;

        // --- functions ---

        GbmDocument();
        GbmDocument(const GbmDocument & doc);
        GbmDocument & operator = ( const GbmDocument & doc );

        void deleteAllPages();

        void readFileFormatDescriptionFromFile() throw ( GbmException );
        int  readNumberOfPagesFromFile() const   throw ( GbmException );

        GbmDocumentPage & getPage(const int pageNumber) throw ( GbmException );

        friend class BackgroundRenderQueue;
        void prepareRenderCache(const int    pageNumber,
                                const double scale_x,
                                const double scale_y);

    public:

        enum MirrorMode { MirrorMode_NONE       = GbmDocumentPage::MirrorMode_NONE,
                          MirrorMode_HORIZONTAL = GbmDocumentPage::MirrorMode_HORIZONTAL,
                          MirrorMode_VERTICAL   = GbmDocumentPage::MirrorMode_VERTICAL,
                          MirrorMode_TRANSPOSE  = GbmDocumentPage::MirrorMode_TRANSPOSE };

        GbmDocument(const GbmAccessor & accessor);
        virtual ~GbmDocument();

        MutexSemaphore & getInterfaceMutex();
        
        BOOL isSupportedFormat(const char * filename,
                               const char * fileExtension) const;

        BOOL hasContent() const;

        void setDocumentFile(const char * filename,
                             const char * fileExtension,
                             const BOOL   allowDelayedDataLoading,
                             const BOOL   force24bpp) throw ( GbmException );
        const char * getDocumentFilename() const;

        const GbmAccessor & getGbmAccessor() const;

        const char * getShortDescription() const;
        const char * getLongDescription()  const;
        const char * getExtensions()       const;

        // page operations

        int getNumberOfPages() const;

        BOOL isPageLoaded(const int pageNumber) const;

        void getPageSize(const int pageNumber,
                               int & pageWidth,
                               int & pageHeight,
                               int & pageColorDepth) throw ( GbmException );

        void rotate(const int pageNumber, const int rotationAngle) throw ( GbmException );
        void mirror(const int pageNumber, const MirrorMode mode)   throw ( GbmException );

        void writeDocumentPageToFile(const char * filename,
                                     const char * options,
                                     const int    pageNumber) throw ( GbmException );

        void setNumberOfBackgroundRenderPages(const int numRenderPages) throw ( GbmException );
        int  getNumberOfBackgroundRenderPages() const;
        
        void resetRenderer(const int pageNumber) throw ( GbmException );

        GbmRenderer & getRenderer(const int pageNumber,
                                  const double scaleX, const double scaleY,
                                  const GbmRenderer::ScalerType scalerType) throw ( GbmException );
};

#endif


