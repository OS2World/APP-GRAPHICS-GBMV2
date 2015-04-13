//********************************************************************************
// Mozilla GBM plug-in: GbmBitmapViewer.hpp (OS/2)
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
//********************************************************************************

#ifndef __CLASS_GBM_BITMAP_VIEWER__
    #define __CLASS_GBM_BITMAP_VIEWER__

#include "os2defs.h"
#include "gbmdlg.h"
#include "GbmDocument.hpp"
#include "GbmRenderer.hpp"
#include "ConfigHandler.hpp"
#include "Thread.hpp"

// ----------------------------

// User message to post for pos/size changes of an assigned parent window.
// MP1: SHORT1: X
//      SHORT2: Y
// MP2: SHORT1: Width
//      SHORT2: Height
#define GBM_BITMAP_VIEWER_WM_POSSIZE_PARENT  (WM_USER + 1101)

class GbmBitmapViewer
{
    public:
        class PaintThread : public Thread
        {
          public:
            struct TASK_ARG
            {
              HWND              hwnd;
              RECTL             rect;
              double            scaleX;
              double            scaleY;
              GbmBitmapViewer * viewer;
            };

          public:
            PaintThread();
            virtual ~PaintThread();

            virtual void run(void * pTask);

            BOOL schedulePaint(GbmBitmapViewer * viewer,
                               HWND hwnd, const RECTL & rect, double scaleX, double scaleY);
        };

        // --------------------------------------

        GbmBitmapViewer(HMODULE hMod, GbmDocument & document, ConfigHandler & configHandler);
        ~GbmBitmapViewer();

        void setParentWindow(HWND hwnd, BOOL fullMode);
        void setLoadProgress(const int progress, const int progressMax);

        void setScalerType(const GbmRenderer::ScalerType scalerType);
        GbmRenderer::ScalerType getScalerType() const;

        void setNumberOfBackgroundRenderPages(const int numPages);
        int  getNumberOfBackgroundRenderPages() const;

        void draw(HPS hps, const RECTL & rect, double pageScaleX, double pageScaleY);

        BOOL hasBitmap() const;

        int  getNumberOfPages() const;
        void getPageSize(int & width, int & height, int & colorDepth);

        const char * getLongDescription() const;

        HWND getFrameHwnd() const
        {
            return mHwndFrame;
        }

        ConfigHandler & getConfigHandler() const
        {
            return mConfigHandler;
        }

        const char * getScalerName_Simple() const
        {
            return mStringScalerSimple;
        }
        const char * getScalerName_NearestNeighbor() const
        {
            return mStringScalerNearNeighbor;
        }

        MRESULT instanceWindowProcedure(HWND  hwnd, ULONG  msg, MPARAM mp1, MPARAM mp2);

    // ----------------------------

    private:

        // types

        enum ZoomMode { ZoomMode_NONE, ZoomMode_FITWIN, ZoomMode_FITWIDTH, ZoomMode_STRETCH, ZoomMode_FREE };

        typedef struct LANG_LOOKUP_TABLE_DEF
        {
            const char  * language;
                  short   resid[14];
        } LANG_LOOKUP_TABLE_ENTRY;

        static const struct LANG_LOOKUP_TABLE_DEF  LANG_LOOKUP_TABLE [];
        static const int    LANG_LOOKUP_TABLE_LENGTH;

        // variables
        BOOL mIsFullMode;

        const LANG_LOOKUP_TABLE_ENTRY * mpLanguage;
              CHAR                      mStringSaveAs[50];
              CHAR                      mStringOverrideFile[50];
              CHAR                      mStringUnsupportedFmt[50];
              CHAR                      mStringAllFiles[50];
              CHAR                      mStringFmtExtError[160];
              CHAR                      mHelpFilename[CCHMAXPATH+1];
              CHAR                      mHelpFilenameBase[CCHMAXPATH+1];
              CHAR                      mStringScalerSimple[11];
              CHAR                      mStringScalerNearNeighbor[21];

        CHAR mSaveAsFilename[CCHMAXPATH+1];
        CHAR mSaveAsOptions [L_GBM_OPTIONS+1];

        HMODULE         mDllModule;
        GbmDocument   & mGbmDocument;
        ConfigHandler & mConfigHandler;
        PaintThread     mPaintThread;
        MutexSemaphore  mInterfaceMutex;

        int        mCurrentPopupMenuId;
        int        mCurrentPageIndex;
        double     mScaleX, mScaleY;
        ZoomMode   mZoomMode;
        GbmRenderer::ScalerType mScalerType;

        int  mLoadProgress, mLoadProgressMax;

        HWND mHwndFrame  , mHwndClient;
        HWND mHwndHScroll, mHwndVScroll;
        HWND mHwndMenu   , mHwndPopupMenu;
        HWND mHwndHelp;

        SHORT mHScrollMax, mHScrollPos;
        SHORT mVScrollMax, mVScrollPos;

        SHORT mCxClient, mCyClient;

        // functions

        void executeOperations(HWND hwnd, long rotateBy, GbmDocument::MirrorMode mirrorMode);
        BOOL executeMenuOperations(HWND hwnd, SHORT itemId);
        void gotoPageIndex(HWND hwnd, int pageIndex);

        void createChildWindows(HWND hwnd);
        void destroyChildWindows();

        void updateScaleFactor(HWND hwnd, BOOL forceWindowRepaint, BOOL skipWindowRepaint);
        void updateWindowSize (HWND hwnd);
        void updateScrollBars (HWND hwnd);
        void updateMenuItems  (HWND hwnd, SHORT itemId) const;

        BOOL showFileDialog_SaveAs(HWND   hwnd,
                                   CHAR * filename, size_t filenameSize,
                                   CHAR * options , size_t optionsSize) const;
};

#endif


