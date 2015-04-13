
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

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "GbmDocument.hpp"
#include "gbmmem.h"

// ---------------------------------------------------------

// Should be multiple of 2
//#define DEFAULT_MAX_RENDER_PAGES  2;

// ---------------------------------------------------------

GbmDocument::BackgroundRenderQueue::BackgroundRenderQueue()
  : Thread(150*1024),
    mFinish(FALSE)
{
}

GbmDocument::BackgroundRenderQueue::~BackgroundRenderQueue()
{
}

// ------------------

void GbmDocument::BackgroundRenderQueue::run(void * task)
{
    GbmDocument::BackgroundRenderQueue::TASK_ARG * pTask =
      (GbmDocument::BackgroundRenderQueue::TASK_ARG *) task;

    // init finish state
    mQueueMutex.lock();
    mFinish = FALSE;
    mQueueMutex.unlock();

    if (pTask && pTask->pages && pTask->pGbmDocument)
    {
        for (int i = 0; i < pTask->numPages; ++i)
        {
            // checkif early stop requested
            mQueueMutex.lock();
            if (mFinish)
            {
                mQueueMutex.unlock();
                break;
            }
            mQueueMutex.unlock();

            try
            {
                pTask->pGbmDocument
                     ->prepareRenderCache(pTask->pages[i], pTask->scaleX, pTask->scaleY);
            }
            catch(GbmException &)
            {
                // silently ignore
            }
            catch(...)
            {
                // silently ignore
            }
        }
    }
    if (pTask)
    {
        if (pTask->pages)
        {
            delete [] pTask->pages;
            pTask->pages = NULL;
        }
        delete pTask;
        pTask = NULL;
    }
}

// ------------------

BOOL GbmDocument::BackgroundRenderQueue::schedulePages(
    GbmDocument * pGbmDocument,
    const int * pPages, int numPages, double scaleX, double scaleY)
{
    if ((pGbmDocument == NULL) || (pPages == NULL))
    {
        return FALSE;
    }
    GbmDocument::BackgroundRenderQueue::TASK_ARG * pTask =
      new GbmDocument::BackgroundRenderQueue::TASK_ARG;
    if (pTask == NULL)
    {
        return FALSE;
    }
    pTask->pages = new int[numPages];
    if (pTask->pages == NULL)
    {
        delete pTask;
        return FALSE;
    }
    for (int i = 0; i < numPages; ++i)
    {
        pTask->pages[i] = pPages[i];
    }
    pTask->numPages     = numPages;
    pTask->scaleX       = scaleX;
    pTask->scaleY       = scaleY;
    pTask->pGbmDocument = pGbmDocument;

    if (! scheduleTask(pTask))
    {
        delete [] pTask->pages;
        delete pTask;
        return FALSE;
    }
    return TRUE;
}

// ------------------

void GbmDocument::BackgroundRenderQueue::stop()
{
    MutexGuard interfaceGuard(mGuardMutex);

    mQueueMutex.lock();
    mFinish = TRUE;
    mQueueMutex.unlock();

    Thread::stop();
}

// ---------------------------------------------------------
// ---------------------------------------------------------

static BOOL isEqualLowerCase(const char *s1, const char *s2, size_t n)
{
  const size_t s1len = strlen(s1);
  const size_t s2len = strlen(s2);
  size_t i;

  for (i = 0; (i < n) && (i < s1len) && (i < s2len); i++, s1++, s2++ )
  {
      if ( tolower(*s1) != tolower(*s2) )
      {
          return FALSE;
      }
  }
  return TRUE;
}

// ---------------------------------------------------------

static const char *findWord(const char *str, const char *substr)
{
         char * buf, *s;
         char   strbuf[51] = { 0 };
         size_t len            = strlen(substr);
   const size_t strlength      = strlen(str);
   const BOOL   needsDynBuffer = (strlength > (sizeof(strbuf)-1));

   /* only use dynamic buffer if needed to speedup a bit */
   if (needsDynBuffer)
   {
     buf = (char *) gbmmem_malloc(strlength + 1);
     if (buf == NULL)
     {
       return NULL;
     }
   }
   else
   {
     buf = strbuf;
   }
   for ( s  = strtok(strcpy(buf, str), " \t,");
         s != NULL;
         s  = strtok(NULL, " \t,") )
   {
     if ( isEqualLowerCase(s, substr, len) && s[len] == '\0' )
     {
       int inx = (int)(s - buf);
       if (needsDynBuffer)
       {
         gbmmem_free(buf);
       }
       return str + inx;
     }
   }
   if (needsDynBuffer)
   {
     gbmmem_free(buf);
   }
   return NULL;
}

// ---------------------------------------------------------

// Lookup GBM file type. If extension is specified, use this one
// and ignore the filename.

static int lookupGbmFileType(const GbmAccessor & gbmAccessor, const char * filename, const char * extension)
{
    if (extension != NULL)
    {
        int numFileTypes = -1;

        if (gbmAccessor.Gbm_query_n_filetypes(&numFileTypes) == GBM_ERR_OK)
        {
          int   ft;
          GBMFT gbmft;

          for (ft = 0; ft < numFileTypes; ft++)
          {
            if (gbmAccessor.Gbm_query_filetype(ft, &gbmft) == GBM_ERR_OK)
            {
              if (findWord(gbmft.extensions, extension) != NULL)
              {
                return ft;
              }
            }
          }
        }
    }
    else
    {
        int ft;
        if (gbmAccessor.Gbm_guess_filetype(filename, &ft) == GBM_ERR_OK)
        {
            // additionally check if the header can be decoded
            int       fd = -1;
            GBM_ERR   gbmRc;
            GBM     * pGbm = (GBM *)gbmmem_malloc(sizeof(GBM));
            if (pGbm == NULL)
            {
                return -1;
            }
            if ( (fd = gbmAccessor.Gbm_io_open(filename, GBM_O_RDONLY)) == -1 )
            {
                gbmmem_free(pGbm);
                return -1;
            }
            gbmRc = gbmAccessor.Gbm_read_header(filename, fd, ft, pGbm, "");
            gbmAccessor.Gbm_io_close(fd);
            gbmmem_free(pGbm); pGbm = NULL;

            if (gbmRc == GBM_ERR_OK)
            {
                return ft;
            }
        }

        // last chance is to test load the header
        int numFileTypes = -1;
        if (gbmAccessor.Gbm_query_n_filetypes(&numFileTypes) == GBM_ERR_OK)
        {
          int formatCounter = 0;
          int foundFormat   = -1;

          int fd = -1;
          if ( (fd = gbmAccessor.Gbm_io_open(filename, GBM_O_RDONLY)) == -1 )
          {
              return -1;
          }

          GBM * pGbm = (GBM *)gbmmem_malloc(sizeof(GBM));
          if (pGbm == NULL)
          {
              gbmAccessor.Gbm_io_close(fd);
              return -1;
          }
          for (ft = 0; ft < numFileTypes; ft++)
          {
            if (gbmAccessor.Gbm_read_header(filename, fd, ft, pGbm, "") == GBM_ERR_OK)
            {
                // There might be some formats that overlap, so count the
                // detected format types. If we find more than 1, we have
                // to decode the data to be sure to find the right format.
                formatCounter++;
                foundFormat = ft;
            }
          }
          gbmmem_free(pGbm); pGbm = NULL;
          gbmAccessor.Gbm_io_close(fd);

          if (formatCounter == 0)
          {
              // nothing found
              return -1;
          }

          if (formatCounter == 1)
          {
              // found the format, don't need to decode the data
              return foundFormat;
          }

          for (ft = 0; ft < numFileTypes; ft++)
          {
              // Now try to decode the data and take the first successfully decoded format.
              try
              {
                  GbmDocumentPage testPage(gbmAccessor);

                  // test loading always page 0
                  testPage.loadFromFile(filename, 0, ft, FALSE, FALSE);

                  // no exception -> we found the correct format
                  return ft;
              }
              catch(GbmException &)
              {
              }
              catch(...)
              {
              }
          }
        }
    }

    return -1;
}

// ---------------------------------------------------------
// ---------------------------------------------------------

GbmDocument::GbmDocument(const GbmAccessor & accessor)
    : mGbmAccessor(accessor),
      renderers(NULL),
      pages(NULL),
      numPages(0),
      filename(NULL),
      force24bpp(FALSE),
      gbmFileType(-1),
      mNumRenderQueuePages(0)
{
    memset(&formatDescription, 0, sizeof(GBMFT));

    mFocusRendererInfo.pRenderer  = NULL;
    mFocusRendererInfo.scalerType = GbmRenderer::ScalerType_SIMPLE;
    mFocusRendererInfo.scaleX     = 0.0;
    mFocusRendererInfo.scaleY     = 0.0;

   #ifdef DEFAULT_MAX_RENDER_PAGES
    mNumRenderQueuePages = DEFAULT_MAX_RENDER_PAGES;
   #endif
}

// ---------------------------------------------------------

GbmDocument::~GbmDocument()
{
    mRenderQueue.stop();

    // lock interface after render thread has been stopped because
    // otherwise we may end up in a deadlock (render thread uses same mutex)
    mInterfaceMutex.lock();

    deleteAllPages();

    delete [] this->filename;
    this->filename = NULL;

    delete [] formatDescription.short_name;
    formatDescription.short_name = NULL;

    delete [] formatDescription.long_name;
    formatDescription.long_name = NULL;

    delete [] formatDescription.extensions;
    formatDescription.extensions = NULL;
}

// ---------------------------------------------------------

MutexSemaphore & GbmDocument::getInterfaceMutex()
{
    return mInterfaceMutex;
}

// ---------------------------------------------------------

void GbmDocument::deleteAllPages()
{
    // Note: Requires interface guard in caller.
    //       As this is a private function, it is
    //       taken care of in the callers.
    if (numPages > 0)
    {
        mFocusRendererInfo.pRenderer  = NULL;
        mFocusRendererInfo.scalerType = GbmRenderer::ScalerType_SIMPLE;
        mFocusRendererInfo.scaleX     = 0.0;
        mFocusRendererInfo.scaleY     = 0.0;

        for (int p = 0; p < numPages; p++)
        {
            delete renderers[p];
            delete pages[p];
            renderers[p] = NULL;
            pages[p]     = NULL;
        }
        delete [] renderers;
        delete [] pages;
        renderers = NULL;
        pages     = NULL;
        numPages  = 0;
    }
}

// ---------------------------------------------------------

// Checks if the file contains a supported image format.
//
// If fileExtension is specified, only the filename is parsed
// the extension and then the extension is checked.
// If fileExtension is NULL, the file content is analyzed.

BOOL GbmDocument::isSupportedFormat(const char * filename,
                                    const char * fileExtension) const
{
    // Threadsafe -> no interface guard needed
    if (lookupGbmFileType(mGbmAccessor, filename, fileExtension) == -1)
    {
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------

BOOL GbmDocument::hasContent() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return (numPages > 0);
}

// ---------------------------------------------------------

// Sets the file as active document. The existing document content is replaced.
//
// If fileExtension is specified, only the filename is parsed
// the extension and then the extension is checked.
// If fileExtension is NULL, the file content is analyzed.

void GbmDocument::setDocumentFile(const char * filename,
                                  const char * fileExtension,
                                  const BOOL   allowDelayedDataLoading,
                                  const BOOL   force24bpp) throw ( GbmException )
{
    mRenderQueue.stop();

    // lock interface after render thread has been stopped because
    // otherwise we may end up in a deadlock (render thread uses same mutex)
    MutexGuard interfaceGuard(mInterfaceMutex);

    deleteAllPages();

    delete [] this->filename;
    this->filename = NULL;

    // alloc memory for the filename and copy the filename to the buffer
    this->filename = new char[strlen(filename) + 1];
    strcpy(this->filename, filename);

    this->force24bpp = force24bpp;

    gbmFileType = lookupGbmFileType(mGbmAccessor, filename, fileExtension);
    if (gbmFileType == -1)
    {
        throw GbmException(GBM_ERR_NOT_SUPP, "File format is not supported");
    }

    // forward a thrown exception to the caller
    readFileFormatDescriptionFromFile();

    // forward a thrown exception to the caller
    const int filePages = readNumberOfPagesFromFile();

    // allocate the required number of pages
    try
    {
        renderers = new GbmRenderer*[filePages];
        pages     = new GbmDocumentPage*[filePages];
    }
    catch(...)
    {
    }
    // for compiler that don't throw std::bad_allow (e.g. IBM VAC)
    if ((renderers == NULL) || (pages == NULL))
    {
        deleteAllPages();
        throw GbmException(GBM_ERR_MEM);
    }

    numPages = filePages;

    // allocate page handler
    int allocatedPages = 0;
    try
    {
        for (int p = 0; p < filePages; p++)
        {
            pages[p] = NULL;
            pages[p] = new GbmDocumentPage(mGbmAccessor);
            if (pages[p] == NULL)
            {
                break;
            }
            allocatedPages++;
        }
    }
    catch(...)
    {
    }
    // for compiler that don't throw std::bad_allow (e.g. IBM VAC)
    if (allocatedPages != filePages)
    {
        deleteAllPages();
        throw GbmException(GBM_ERR_MEM);
    }

    // allocate page renderer
    allocatedPages = 0;
    try
    {
        for (int p = 0; p < numPages; p++)
        {
            renderers[p] = NULL;
            renderers[p] = new GbmRenderer();
            if (renderers[p] == NULL)
            {
                break;
            }
            allocatedPages++;
        }
    }
    catch(...)
    {
    }
    // for compiler that don't throw std::bad_allow (e.g. IBM VAC)
    if (allocatedPages != numPages)
    {
        deleteAllPages();
        throw GbmException(GBM_ERR_MEM);
    }

    if (allocatedPages > 0)
    {
        try
        {
            // read at least the first page to verify image data decoding
            pages[0]->loadFromFile(filename,
                                   0,
                                   gbmFileType,
                                   FALSE, // enforce full decoding
                                   force24bpp);

            // read the remaining pages which can use delayed data loading
            for (int p = 1; p < numPages; p++)
            {
                // forward a thrown exception to the caller
                pages[p]->loadFromFile(filename,
                                       p,
                                       gbmFileType,
                                       allowDelayedDataLoading,
                                       force24bpp);
            }
        }
        catch(GbmException & ex)
        {
            deleteAllPages();
            throw ex;
        }
        catch(...)
        {
            deleteAllPages();
            throw GbmException(GBM_ERR_READ);
        }
    }
}

// ---------------------------------------------------------

const char * GbmDocument::getDocumentFilename() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return hasContent() ? filename : "";
}

// ---------------------------------------------------------

void GbmDocument::writeDocumentPageToFile(const char * filename,
                                          const char * options,
                                          const int    pageNumber) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);

    GbmDocumentPage & page(getPage(pageNumber));
    try
    {
        const BOOL needLock = (renderers[pageNumber] != mFocusRendererInfo.pRenderer);
        if (needLock)
        {
            MutexGuard queueGuard(mRenderQueue.mQueueMutex);
            page.writeToFile(filename, options);
        }
        else
        {
            page.writeToFile(filename, options);
        }
    }
    catch(GbmException & ex)
    {
      throw ex;
    }
    catch(...)
    {
      throw GbmException(GBM_ERR_WRITE);
    }
}

// ---------------------------------------------------------


void GbmDocument::getPageSize(const int pageNumber,
                                    int & pageWidth,
                                    int & pageHeight,
                                    int & pageColorDepth) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);

    GbmDocumentPage & page(getPage(pageNumber));
    pageWidth      = page.getWidth();
    pageHeight     = page.getHeight();
    pageColorDepth = page.getColorDepth();
}

// ---------------------------------------------------------

void GbmDocument::rotate(const int pageNumber,
                         const int rotationAngle) throw ( GbmException )
{
    mRenderQueue.stop();

    // lock interface after render thread has been stopped because
    // otherwise we may end up in a deadlock (render thread uses same mutex)
    MutexGuard interfaceGuard(mInterfaceMutex);

    GbmDocumentPage & page(getPage(pageNumber));
    page.rotate(rotationAngle);

    // reset all renderers
    for (int p = 0; p < numPages; p++)
    {
        renderers[p]->setRenderPage(NULL);
    }
    mFocusRendererInfo.pRenderer = NULL;
}

void GbmDocument::mirror(const int pageNumber,
                         const MirrorMode mode) throw ( GbmException )
{
    mRenderQueue.stop();

    // lock interface after render thread has been stopped because
    // otherwise we may end up in a deadlock (render thread uses same mutex)
    MutexGuard interfaceGuard(mInterfaceMutex);

    GbmDocumentPage & page(getPage(pageNumber));
    page.mirror((GbmDocumentPage::MirrorMode) mode);

    // reset all renderers
    for (int p = 0; p < numPages; p++)
    {
        renderers[p]->setRenderPage(NULL);
    }
    mFocusRendererInfo.pRenderer = NULL;
}

// ---------------------------------------------------------

const GbmAccessor & GbmDocument::getGbmAccessor() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return mGbmAccessor;
}

// ---------------------------------------------------------

void GbmDocument::setNumberOfBackgroundRenderPages(const int numRenderPages) throw ( GbmException )
{
    if (numRenderPages < 0)
    {
        throw GbmException(GBM_ERR_BAD_OPTION, "Number of background render pages must be > 0");
    }
    // reconfigure render queue (will clear all jobs)
    MutexGuard interfaceGuard(mInterfaceMutex);
    mNumRenderQueuePages = numRenderPages;
}

int GbmDocument::getNumberOfBackgroundRenderPages() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return mNumRenderQueuePages;
}

// ---------------------------------------------------------

void GbmDocument::resetRenderer(const int pageNumber) throw ( GbmException )
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    if ((pageNumber < 0) || (pageNumber >= numPages))
    {
        throw GbmException(GBM_ERR_READ, "Requested page is not available");
    }

    const BOOL needLock = (renderers[pageNumber] != mFocusRendererInfo.pRenderer);
    if (needLock)
    {
        MutexGuard queueGuard(mRenderQueue.mQueueMutex);
        mFocusRendererInfo.pRenderer = NULL;
        renderers[pageNumber]->setRenderPage(NULL);
    }
    else
    {
        mFocusRendererInfo.pRenderer = NULL;
        renderers[pageNumber]->setRenderPage(NULL);
    }
}

// ---------------------------------------------------------

GbmRenderer & GbmDocument::getRenderer(const int pageNumber,
                                       const double scaleX, const double scaleY,
                                       const GbmRenderer::ScalerType scalerType) throw ( GbmException )
{
    GbmRenderer * pLastFocusRenderer = NULL;
    GbmRenderer * pNewFocusRenderer  = NULL;
    {
        // Keep the lock only as long as we know if we
        // can return the focus renderer. It must be released
        // before the render thread is stopped because otherwise
        // we'll end up in a deadlock.
        MutexGuard interfaceGuard(mInterfaceMutex);

        if ((pageNumber < 0) || (pageNumber >= numPages))
        {
            throw GbmException(GBM_ERR_READ, "Requested page is not available");
        }

        // get the last focus renderer
        pLastFocusRenderer = mFocusRendererInfo.pRenderer;
        pNewFocusRenderer  = renderers[pageNumber];

        if (pLastFocusRenderer == pNewFocusRenderer)
        {
            // check if scale factors or scaler type has changed
            if ((scalerType == mFocusRendererInfo.scalerType) &&
                (scaleX     == mFocusRendererInfo.scaleX)     &&
                (scaleY     == mFocusRendererInfo.scaleY))
            {
               pNewFocusRenderer->setScalerType(scalerType);
               pNewFocusRenderer->prepareRenderCache(scaleX, scaleY);
               return *pNewFocusRenderer;
            }
        }
    }

    // rebuild the background renderer queue
    mRenderQueue.stop();

    // lock interface after render thread has been stopped because
    // otherwise we may end up in a deadlock (render thread uses same mutex)
    MutexGuard interfaceGuard(mInterfaceMutex);

    GbmDocumentPage & page(getPage(pageNumber));
    pNewFocusRenderer->setScalerType(scalerType);
    pNewFocusRenderer->setRenderPage(&page);
    pNewFocusRenderer->prepareRenderCache(scaleX, scaleY);

    mFocusRendererInfo.pRenderer  = pNewFocusRenderer;
    mFocusRendererInfo.scalerType = scalerType;
    mFocusRendererInfo.scaleX     = scaleX;
    mFocusRendererInfo.scaleY     = scaleY;

    // precalculate N pages backwards and forewards from requested page
    const int renderPages = (mNumRenderQueuePages > numPages)
                            ? numPages
                            : mNumRenderQueuePages;
    if (renderPages > 0)
    {
        // schedule focus surrounding pages for background rendering
        int * surroundingPages = new int[renderPages];
        assert(surroundingPages != NULL);

        const int pagesAroundFocus = renderPages/2;
        int lastPage  = pageNumber + pagesAroundFocus;
        int firstPage = lastPage   - (renderPages - 1);

        if (firstPage < 0)
        {
            firstPage = 0;
            lastPage  = renderPages - 1;
        }
        if (lastPage >= numPages)
        {
            lastPage  = numPages - 1;
            firstPage = lastPage - (renderPages - 1);
            if (firstPage < 0) firstPage = 0;
        }

        assert(firstPage >= 0);
        assert(lastPage  < numPages);
        assert(lastPage - firstPage + 1 <= renderPages);

        int p;
        int numSurroundingPages = 0;

        for (int i = 1; i < renderPages; i++)
        {
            // prioritize the rendering of pages
            // - 1 after focus
            // - 1 before focus
            // - 2 after focus
            // - 2 before focus
            // ...

            p = pageNumber + i;
            if (p <= lastPage)
            {
                surroundingPages[numSurroundingPages] = p;
                numSurroundingPages++;
            }

            p = pageNumber - i;
            if (p >= firstPage)
            {
                surroundingPages[numSurroundingPages] = p;
                numSurroundingPages++;
            }
        }

        // free all rendered pages outside the prefetch range
        for (p = 0; p < firstPage; p++)
        {
            renderers[p]->setRenderPage(NULL);
        }
        for (p = lastPage + 1; p < numPages; p++)
        {
            renderers[p]->setRenderPage(NULL);
        }

        if (numSurroundingPages > 0)
        {
           #ifndef NDEBUG
            const BOOL success =
           #endif
            mRenderQueue.schedulePages(this,
                                       surroundingPages,
                                       numSurroundingPages,
                                       scaleX,
                                       scaleY);
            assert(success);
        }
        delete [] surroundingPages;
        surroundingPages = NULL;
    }
    else
    {
        // free previous render cache in case we have no background rendering
        if ((pLastFocusRenderer != NULL) &&
            (pLastFocusRenderer != mFocusRendererInfo.pRenderer))
        {
            pLastFocusRenderer->setRenderPage(NULL);
        }
    }

    // return the focus renderer
    return *(mFocusRendererInfo.pRenderer);
}

// ---------------------------------------------------------

// Forwarder for background renderer thread. Do not use otherwise!!!
void GbmDocument::prepareRenderCache(const int    pageNumber,
                                     const double scale_x,
                                     const double scale_y)
{
    try
    {
        GbmDocumentPage & page(getPage(pageNumber));
        GbmRenderer * pRenderer = renderers[pageNumber];
        pRenderer->setRenderPage(&page);
        pRenderer->setScalerType(mFocusRendererInfo.scalerType);
        pRenderer->prepareRenderCache(scale_x, scale_y);
    }
    catch(GbmException &)
    {
        // silently ignore
    }
}

// ---------------------------------------------------------

GbmDocumentPage & GbmDocument::getPage(const int pageNumber) throw ( GbmException )
{
    // Note: Requires interface guard in caller (this is private).

    if ((pageNumber < 0) || (pageNumber >= numPages))
    {
        throw GbmException(GBM_ERR_READ, "Requested page is not available");
    }

    // check if the page has already been loaded
    if (! pages[pageNumber]->isValid())
    {
        try
        {
            MutexGuard queueGuard(mRenderQueue.mQueueMutex);

            // unregister clear the renderer cache as we replace page data
            renderers[pageNumber]->setRenderPage(NULL);

            // load the page data

            // forward a thrown exception to the caller
            pages[pageNumber]->loadFromFile(filename, pageNumber, gbmFileType, TRUE, force24bpp);
        }
        catch(GbmException & ex)
        {
            throw ex;
        }
        catch(...)
        {
            throw GbmException(GBM_ERR_READ);
        }
    }

    return *pages[pageNumber];
}

// ---------------------------------------------------------

int GbmDocument::getNumberOfPages() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return numPages;
}

// ---------------------------------------------------------

BOOL GbmDocument::isPageLoaded(const int pageNumber) const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if ((pageNumber >= 0) && (pageNumber < numPages))
    {
        // check if the page has already been loaded
        return pages[pageNumber]->isValid();
    }
    return FALSE;
}

// ---------------------------------------------------------

const char * GbmDocument::getShortDescription() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if (hasContent())
    {
        return formatDescription.short_name;
    }
    return "";
}

const char * GbmDocument::getLongDescription() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if (hasContent())
    {
        return formatDescription.long_name;
    }
    return "";
}

const char * GbmDocument::getExtensions() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if (hasContent())
    {
        return formatDescription.extensions;
    }
    return "";
}

// ---------------------------------------------------------

void GbmDocument::readFileFormatDescriptionFromFile() throw ( GbmException )
{
    // Note: Requires interface guard in caller (this is private).

    GBM_ERR gbmRc;

    delete [] formatDescription.short_name;
    formatDescription.short_name = NULL;

    delete [] formatDescription.long_name;
    formatDescription.long_name = NULL;

    delete [] formatDescription.extensions;
    formatDescription.extensions = NULL;

    GBMFT gbmft;
    gbmRc = mGbmAccessor.Gbm_query_filetype(gbmFileType, &gbmft);
    if (gbmRc != GBM_ERR_OK)
    {
        throw GbmException(gbmRc, "Can't extract file format description");
    }

    formatDescription.short_name = new char[strlen(gbmft.short_name) + 1];
    strcpy(formatDescription.short_name, gbmft.short_name);

    formatDescription.long_name = new char[strlen(gbmft.long_name) + 1];
    strcpy(formatDescription.long_name, gbmft.long_name);

    formatDescription.extensions = new char[strlen(gbmft.extensions) + 1];
    strcpy(formatDescription.extensions, gbmft.extensions);
}

// ---------------------------------------------------------

int GbmDocument::readNumberOfPagesFromFile() const throw ( GbmException )
{
    // Note: This function is threadsafe and independent from
    //       class members, so no interface guard is required.

    int     fd = -1;
    GBM_ERR gbmRc;

    if ( (fd = mGbmAccessor.Gbm_io_open(filename, GBM_O_RDONLY)) == -1 )
    {
        throw GbmException(GBM_ERR_READ, "Can't open file");
    }

    /* call gbm_read_imgcount(const char *fn, int fd, int ft, int *pimgcnt) */
    int pageCount = 1;

    gbmRc = mGbmAccessor.Gbm_read_imgcount(filename, fd, gbmFileType, &pageCount);
    if (gbmRc != GBM_ERR_OK)
    {
        mGbmAccessor.Gbm_io_close(fd);
        throw GbmException(gbmRc, "Can't read number of images");
    }
    mGbmAccessor.Gbm_io_close(fd);
    return pageCount;
}

