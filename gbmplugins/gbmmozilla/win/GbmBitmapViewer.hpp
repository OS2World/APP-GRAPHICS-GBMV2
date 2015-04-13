//********************************************************************************
// Mozilla GBM plug-in: GbmBitmapViewer.hpp (Windows)
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

#include <string>
#include "windefs.h"
#include "GbmDocument.hpp"
#include "GbmRenderer.hpp"
#include "ConfigHandler.hpp"
#include "Thread.hpp"

// ----------------------------

class GbmBitmapViewer
{
    public:
        class PaintThread : public Thread
        {
          public:
            struct TASK_ARG
            {
              HWND              hwnd;
              RECT              rect;
              double            scaleX;
              double            scaleY;
              GbmBitmapViewer * viewer;
            };

          public:
            PaintThread();
            virtual ~PaintThread();

            virtual void run(void * pTask);
            
            BOOL schedulePaint(GbmBitmapViewer * viewer,
                               HWND hwnd, const RECT & rect, double scaleX, double scaleY);
        };

        // --------------------------------------

        GbmBitmapViewer(HMODULE hMod, GbmDocument & document, ConfigHandler & configHandler);
        ~GbmBitmapViewer();

        BOOL setParentWindow(HWND hwnd, BOOL fullMode);
        void setLoadProgress(const int progress, const int progressMax);
        
        void setScalerType(const GbmRenderer::ScalerType scalerType);
        GbmRenderer::ScalerType getScalerType() const;
        
        void setNumberOfBackgroundRenderPages(const int numPages);
        int  getNumberOfBackgroundRenderPages() const;
        
        void draw(HDC hdc, const RECT & rect, double pageScaleX, double pageScaleY);

        BOOL hasBitmap() const;

        int  getNumberOfPages() const;
        void getPageSize(int & width, int & height, int & colorDepth);

        std::wstring getLongDescription() const;

        HWND getClientHwnd() const
        {
           return mHwndClient;
        }

        ConfigHandler & getConfigHandler() const
        {
            return mConfigHandler;
        }
        
        const wchar_t * getScalerName_Simple() const
        {
            return mStringScalerSimple;
        }
        const wchar_t * getScalerName_NearestNeighbor() const
        {
            return mStringScalerNearNeighbor;
        }
        
        LRESULT instanceWindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        
    // ----------------------------

    private:
        // types

        enum ZoomMode { ZoomMode_NONE, ZoomMode_FITWIN, ZoomMode_FITWIDTH, ZoomMode_STRETCH, ZoomMode_FREE };

        typedef struct LANG_LOOKUP_TABLE_DEF
        {
           LANGID language;
           short  resid[10];
        } LANG_LOOKUP_TABLE_ENTRY;

        static const struct LANG_LOOKUP_TABLE_DEF  LANG_LOOKUP_TABLE [];
        static const int    LANG_LOOKUP_TABLE_LENGTH;

        // variables
        BOOL mIsFullMode;

        const LANG_LOOKUP_TABLE_ENTRY * mpLanguage;
        wchar_t mStringUnsupportedFmt[51];
        wchar_t mStringSaveAs[101];
        wchar_t mStringAllFiles[51];
        wchar_t mStringFmtExtError[201];
        wchar_t mStringScalerSimple[11];
        wchar_t mStringScalerNearNeighbor[21];

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

        HWND   mHwndClient;
        HWND   mHwndVScroll, mHwndHScroll, mHwndScrollRect;
        HMENU  mHwndPopupMenu;
        WNDPROC mpOldGlobalWindowProcedure;

        SHORT mHScrollMax, mHScrollPos, mHScrollPage;
        SHORT mVScrollMax, mVScrollPos, mVScrollPage;

        SHORT mCxClient, mCyClient;

        std::wstring mSaveAsFilename;
        std::wstring mSaveAsOptions;

        // functions

        void executeOperations(HWND hwnd, long rotateBy, GbmDocument::MirrorMode mirrorMode);
        BOOL executeMenuOperations(HWND hwnd, SHORT itemId);
        void gotoPageIndex(HWND hwnd, int pageIndex);

        void createChildWindows(HWND hwnd);
        void updateChildWindows();
        void destroyChildWindows();

        void updateScaleFactor(HWND hwnd, BOOL forceWindowRepaint, BOOL skipWindowRepaint);
        void updateWindowSize (HWND hwnd);
        void updateScrollBars (HWND hwnd);
        void updateMenuItems  (HMENU refHwndMenu, SHORT itemId) const;

        BOOL showFileDialog_SaveAs(const HWND     hwnd,
                                   std::wstring & filename,
                                   std::wstring & options);
};

#endif


