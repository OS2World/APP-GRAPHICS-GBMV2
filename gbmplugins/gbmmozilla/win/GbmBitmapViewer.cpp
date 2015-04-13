//********************************************************************************
// Mozilla GBM plug-in: GbmBitmapViewer.cpp (Windows)
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
//
// Requires: GBM.DLL version 1.73 or higher (with multipage support)
//
//********************************************************************************
//
// History:
//
//   28-Jul-2010: Initial Windows version (derived from OS/2 version 1.40)
//   10-Oct-2010  Add preferences dialog box
//    1-Nov-2010  Add resampling scaler filters:
//                * blackman, catmullrom, quadratic, gaussian, kaiser
//
//********************************************************************************

// This module is fully Unicode compliant (except access to GBM functions).
#define UNICODE

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "windefs.h"
#include <windowsx.h>
#include <commctrl.h>
#include "npgbmrc.h"

#include "GbmDocumentPage.hpp"
#include "GbmException.hpp"
#include "GbmBitmapViewer.hpp"
#include "GbmSaveFileDialog.hpp"
#include "unicode.hpp"

// ---------------------------------------------------------

// Keep in sync with Resource file npgbm.rc
static const int GBM_PLUGIN_VERSION = 147;  // 1.47

// Window title for the viewer
static const wchar_t * GBMBITMAPVIEWER_Title = L"GBM Bitmap Viewer";

static const int WND_ID_SCROLLBAR_RECT = 1;
static const int WND_ID_SCROLLBAR_VERT = 2;
static const int WND_ID_SCROLLBAR_HORZ = 3;

static const int SCROLL_PAGE_INCREMENT_STEPS = 150;

// Keep in sync with npgbm.rc and npgbmrc.h resource IDs !!!

// Index of language specific resource ID into lookup table.
#define RES_RID_GBMVIEWER_SMALL_POPUP                0
#define RES_RID_GBMVIEWER_FULL_POPUP                 1
#define RES_RID_GBMVIEWER_FORMATINFO                 2
#define RES_RID_GBMVIEWER_PREFS                      3
#define RES_SID_GBMVIEWER_UNSUPPORTED_FMT            4
#define RES_SID_GBMVIEWER_SAVEAS                     5
#define RES_SID_GBMVIEWER_STRING_ALL_FILES           6
#define RES_SID_GBMVIEWER_STRING_FMT_EXT_ERROR       7
#define RES_SID_GBMVIEWER_STRING_SCALE_SIMPLE        8
#define RES_SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR  9

// Keep in sync with npgbm.rc and npgbmrc.h resource IDs !!!
const struct GbmBitmapViewer::LANG_LOOKUP_TABLE_DEF GbmBitmapViewer::LANG_LOOKUP_TABLE [] =
{
    { 0x09, { RID_GBMVIEWER_SMALL_POPUP_EN,
              RID_GBMVIEWER_FULL_POPUP_EN,
              RID_GBMVIEWER_FORMATINFO_EN,
              RID_GBMVIEWER_PREFS_EN,
              SID_GBMVIEWER_STRING_UNSUPPORTED_FMT_EN,
              SID_GBMVIEWER_STRING_SAVEAS_EN,
              SID_GBMVIEWER_STRING_ALL_FILES_EN,
              SID_GBMVIEWER_STRING_FMT_EXT_ERROR_EN,
              SID_GBMVIEWER_STRING_SCALE_SIMPLE_EN,
              SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR_EN
            }
    },
    { 0x07, { RID_GBMVIEWER_SMALL_POPUP_DE,
              RID_GBMVIEWER_FULL_POPUP_DE,
              RID_GBMVIEWER_FORMATINFO_DE,
              RID_GBMVIEWER_PREFS_DE,
              SID_GBMVIEWER_STRING_UNSUPPORTED_FMT_DE,
              SID_GBMVIEWER_STRING_SAVEAS_DE,
              SID_GBMVIEWER_STRING_ALL_FILES_DE,
              SID_GBMVIEWER_STRING_FMT_EXT_ERROR_DE,
              SID_GBMVIEWER_STRING_SCALE_SIMPLE_DE,
              SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR_DE
             }
    }
};
const int GbmBitmapViewer::LANG_LOOKUP_TABLE_LENGTH =
    sizeof(GbmBitmapViewer::LANG_LOOKUP_TABLE) / sizeof(GbmBitmapViewer::LANG_LOOKUP_TABLE[0]);

// ---------------------------------------------------------

typedef struct
{
    const char    *         configName;
    const wchar_t *         displayName;
    GbmRenderer::ScalerType type;
} SCALER;

static const SCALER SCALER_TABLE[] =
{
    { "simple"         , L"Simple"          , GbmRenderer::ScalerType_SIMPLE          },
    { "nearestneighbor", L"Nearest Neighbor", GbmRenderer::ScalerType_NEARESTNEIGHBOR },
    { "bilinear"       , L"Bilinear"        , GbmRenderer::ScalerType_BILINEAR        },
    { "bell"           , L"Bell"            , GbmRenderer::ScalerType_BELL            },
    { "bspline"        , L"BSpline"         , GbmRenderer::ScalerType_BSPLINE         },
    { "quadratic"      , L"Quadratic"       , GbmRenderer::ScalerType_QUADRATIC       },
    { "gaussian"       , L"Gaussian"        , GbmRenderer::ScalerType_GAUSSIAN        },
    { "mitchell"       , L"Mitchell"        , GbmRenderer::ScalerType_MITCHELL        },
    { "catmullrom"     , L"Catmull/Rom"     , GbmRenderer::ScalerType_CATMULLROM      },
    { "lanczos"        , L"Lanczos"         , GbmRenderer::ScalerType_LANCZOS         },
    { "kaiser"         , L"Kaiser"          , GbmRenderer::ScalerType_KAISER          },
    { "blackman"       , L"Blackman"        , GbmRenderer::ScalerType_BLACKMAN        }
};
static const int SCALER_TABLE_LENGTH = sizeof(SCALER_TABLE)/sizeof(SCALER_TABLE[0]);

// ---------------------------------------------------------
// ---------------------------------------------------------

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))

// ---------------------------------------------------------
// ---------------------------------------------------------

//
// Here is the window procedure for the client window.
//
static LRESULT CALLBACK GlobalWindowProcedure(HWND   hwnd,
                                              UINT   msg,
                                              WPARAM wparam,
                                              LPARAM lparam)
{
    GbmBitmapViewer * pViewer = (GbmBitmapViewer *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pViewer != NULL)
    {
        return pViewer->instanceWindowProcedure(hwnd, msg, wparam, lparam);
    }
    return 0;
}

//
// Here is the window procedure for the bitmap info dialog box.
//
static INT_PTR CALLBACK GlobalInfoDlgWindowProcedure(HWND   hwnd,
                                                     UINT   msg,
                                                     WPARAM wparam,
                                                     LPARAM lparam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            static const size_t MAX_BUFFER_CHARS = 20;
            wchar_t numberBuffer[MAX_BUFFER_CHARS + 1] = { 0 };

            GbmBitmapViewer * pViewer = (GbmBitmapViewer *) lparam;
            if (pViewer == NULL)
            {
                return FALSE;
            }

            // set the info into the text fields
            SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_FORMAT),
                          pViewer->getLongDescription().c_str());

            if (pViewer->hasBitmap())
            {
                int pageWidth = 0, pageHeight = 0, pageColorDepth = 0;
                pViewer->getPageSize(pageWidth, pageHeight, pageColorDepth);

                swprintf(numberBuffer, MAX_BUFFER_CHARS, L"%d", pageWidth);
                SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_WIDTH), numberBuffer);

                swprintf(numberBuffer, MAX_BUFFER_CHARS, L"%d", pageHeight);
                SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_HEIGHT), numberBuffer);

                swprintf(numberBuffer, MAX_BUFFER_CHARS, L"%d", pageColorDepth);
                SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_BPP), numberBuffer);

                swprintf(numberBuffer, MAX_BUFFER_CHARS, L"%d", pViewer->getNumberOfPages());
                SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_PAGES), numberBuffer);
            }
            else
            {
                static const wchar_t * valueNotAvailable = L"-";
                SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_WIDTH) , valueNotAvailable);
                SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_HEIGHT), valueNotAvailable);
                SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_BPP)   , valueNotAvailable);
                SetWindowText(GetDlgItem(hwnd, TID_FORMATINFO_PAGES) , valueNotAvailable);
            }

            // center the dialog window
            {
                RECT rectParent = { 0 };
                RECT rectDialog = { 0 };

                const HWND parentHwnd = GetParent(pViewer->getClientHwnd());
                GetWindowRect(parentHwnd ? parentHwnd : pViewer->getClientHwnd(), &rectParent);
                GetWindowRect(hwnd, &rectDialog);

                // set the horizontal coordinate of the lower-left corner
                const int w = rectDialog.right - rectDialog.left;
                const int x = ((rectParent.right - rectParent.left) - w) / 2 + rectParent.left;

                // set vertical coordinate of the lower-left corner
                const int h = rectDialog.bottom - rectDialog.top;
                const int y = ((rectParent.bottom - rectParent.top) - h) / 2 + rectParent.top;

                // move, size, and show the window
                MoveWindow(hwnd, x, y, w, h, FALSE);
            }
        }
        return TRUE;

        case WM_COMMAND:
           switch (LOWORD(wparam))
           {
              case IDOK:
                  EndDialog(hwnd, IDOK);
                  return TRUE;

              case IDCANCEL:
                  EndDialog(hwnd, IDCANCEL);
                  return TRUE;

              default:
                  break;
           }
           break;

        // -------------------------

        default:
            break;
    }
    return FALSE;
}

//
// Here is the window procedure for the preferences dialog box.
//
static INT_PTR CALLBACK GlobalPrefsDlgWindowProcedure(HWND   hwnd,
                                                      UINT   msg,
                                                      WPARAM wparam,
                                                      LPARAM lparam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            GbmBitmapViewer * pViewer = (GbmBitmapViewer *) lparam;
            if (pViewer == NULL)
            {
                return FALSE;
            }
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pViewer);

            // init the dialog fields

            // Fill the combobox
            const HWND algoHwnd = GetDlgItem(hwnd, TID_PREFERENCES_ALGO);
            for (int i = 0; i < SCALER_TABLE_LENGTH; ++i)
            {
                switch(i)
                {
                    case 0:
                        ComboBox_AddString(algoHwnd, pViewer->getScalerName_Simple());
                        break;

                    case 1:
                        ComboBox_AddString(algoHwnd, pViewer->getScalerName_NearestNeighbor());
                        break;

                    default:
                        ComboBox_AddString(algoHwnd, SCALER_TABLE[i].displayName);
                        break;
                }
            }
            // Select the current scale type
            const GbmRenderer::ScalerType currScalerType = pViewer->getScalerType();
            for (int i = 0; i < SCALER_TABLE_LENGTH; ++i)
            {
                if (SCALER_TABLE[i].type == currScalerType)
                {
                    if (CB_ERR == ComboBox_SetCurSel(algoHwnd, i))
                    {
                        ComboBox_SetCurSel(algoHwnd, 0);
                    }
                    break;
                }
            }

            // Set the currently configured number of background render pages
            const HWND pagesHwnd       = GetDlgItem(hwnd, TID_PREFERENCES_PAGES);
            const HWND pagesHwndUpDown = GetDlgItem(hwnd, TID_PREFERENCES_PAGESUD);
            SendMessage(pagesHwndUpDown, UDM_SETBUDDY, (WPARAM)pagesHwnd, 0);
            SendMessage(pagesHwndUpDown, UDM_SETBASE,  (WPARAM)10, (LPARAM)0);
            SendMessage(pagesHwndUpDown, UDM_SETRANGE, (WPARAM) 0, MAKELPARAM(999, 0));
            SendMessage(pagesHwndUpDown, UDM_SETPOS,   (WPARAM) 0, (LPARAM)(pViewer->getNumberOfBackgroundRenderPages()));

            // center the dialog window
            {
                RECT rectParent = { 0 };
                RECT rectDialog = { 0 };

                const HWND parentHwnd = GetParent(pViewer->getClientHwnd());
                GetWindowRect(parentHwnd ? parentHwnd : pViewer->getClientHwnd(), &rectParent);
                GetWindowRect(hwnd, &rectDialog);

                // set the horizontal coordinate of the lower-left corner
                const int w = rectDialog.right - rectDialog.left;
                const int x = ((rectParent.right - rectParent.left) - w) / 2 + rectParent.left;

                // set vertical coordinate of the lower-left corner
                const int h = rectDialog.bottom - rectDialog.top;
                const int y = ((rectParent.bottom - rectParent.top) - h) / 2 + rectParent.top;

                // move, size, and show the window
                MoveWindow(hwnd, x, y, w, h, FALSE);
            }
        }
        return TRUE;

        case WM_COMMAND:
           switch (LOWORD(wparam))
           {
              case IDOK:
              {
                  // readout the configured settings and program viewer
                  GbmBitmapViewer * pViewer = (GbmBitmapViewer *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
                  if (pViewer != NULL)
                  {
                      const HWND pagesHwndUpDown = GetDlgItem(hwnd, TID_PREFERENCES_PAGESUD);
                      const LRESULT pagesRc = SendMessage(pagesHwndUpDown, UDM_GETPOS, (WPARAM)0, (LPARAM)0);
                      if (HIWORD(pagesRc) == 0)
                      {
                          pViewer->setNumberOfBackgroundRenderPages(LOWORD(pagesRc));
                      }

                      const HWND algoHwnd = GetDlgItem(hwnd, TID_PREFERENCES_ALGO);
                      const int currLboxItemId = ComboBox_GetCurSel(algoHwnd);
                      if (currLboxItemId != CB_ERR)
                      {
                          assert((currLboxItemId >= 0) && (currLboxItemId < SCALER_TABLE_LENGTH));
                          pViewer->setScalerType(SCALER_TABLE[currLboxItemId].type);
                      }

                      // check if user settings were changed and save them to config file
                      {
                          BOOL hasError   = FALSE;
                          BOOL hasChanged = FALSE;

                          // set the scaler in config file
                          ConfigHandler & configHandler(pViewer->getConfigHandler());

                          const GbmRenderer::ScalerType scalerType = pViewer->getScalerType();
                          for (int i = 0; i < SCALER_TABLE_LENGTH; i++)
                          {
                              if (scalerType == SCALER_TABLE[i].type)
                              {
                                  const char * scalerValue = configHandler.getValueOfKey("scaler");
                                  if ((scalerValue != NULL) && (strcmp(scalerValue, SCALER_TABLE[i].configName) != NULL))
                                  {
                                      hasError   = ! configHandler.append("scaler", SCALER_TABLE[i].configName);
                                      hasChanged = TRUE;
                                  }
                                  break;
                              }
                          }
                          // set the background render pages in config file
                          if (! hasError)
                          {
                              char numberBuffer[21] = { 0 };
                              sprintf(numberBuffer, "%i", pViewer->getNumberOfBackgroundRenderPages());

                              const char * pagesValue = configHandler.getValueOfKey("progressive_render_pages");
                              if ((pagesValue != NULL) && (strcmp(pagesValue, numberBuffer) != NULL))
                              {
                                  hasError   = ! configHandler.append("progressive_render_pages", numberBuffer);
                                  hasChanged = TRUE;
                              }
                          }
                          // finally save/update the config file on the disk
                          if (hasChanged)
                          {
                              if (! configHandler.save())
                              {
                                  const char * filename = configHandler.getFilename();
                                  if (filename)
                                  {
                                      std::wstring msg(L"Could not save preferences to file:\n");
                                      msg.append(string2wstring(filename));
                                      MessageBox(hwnd, msg.c_str(),
                                                 GBMBITMAPVIEWER_Title,
                                                 MB_OK | MB_ICONERROR | MB_APPLMODAL);
                                  }
                                  else
                                  {
                                      MessageBox(hwnd, L"Could not save preferences to file.",
                                                 GBMBITMAPVIEWER_Title,
                                                 MB_OK | MB_ICONERROR | MB_APPLMODAL);
                                  }
                              }
                          }
                      }
                  }
                  EndDialog(hwnd, IDOK);
                  return TRUE;
              }

              case IDCANCEL:
                  EndDialog(hwnd, IDCANCEL);
                  return TRUE;

              default:
                  break;
           }
           break;

        // -------------------------

        default:
            break;
    }
    return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static VOID EnableMenuItemId(HMENU hWndMenu, SHORT idMenuItem, BOOL fEnabled)
{
    EnableMenuItem(hWndMenu, idMenuItem, fEnabled ? MF_ENABLED : MF_GRAYED);
}

static VOID CheckMenuItemId(HMENU hWndMenu, SHORT idMenuItem, BOOL fChecked)
{
    CheckMenuItem(hWndMenu, idMenuItem, fChecked ? MF_CHECKED : MF_UNCHECKED);
}

//----------------------------------------------------------------------------

static VOID ScrollBarEnable(HWND hwndScroller, BOOL fEnabled)
{
    EnableScrollBar(hwndScroller, SB_CTL,
                    fEnabled ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
}

static void SetScrollBar(HWND hwndScroller, SHORT pos, SHORT min, SHORT max)
{
     /* Set the scroll-bar value and range. */
     SCROLLINFO scrollInfo;
     scrollInfo.cbSize = sizeof(SCROLLINFO);
     scrollInfo.fMask  = SIF_POS | SIF_RANGE;
     scrollInfo.nMin   = min;
     scrollInfo.nMax   = max;
     scrollInfo.nPos   = pos;

     SetScrollInfo(hwndScroller, SB_CTL, &scrollInfo, TRUE);
}

static void SetScrollBarPos(HWND hwndScroller, SHORT pos)
{
     /* Set the scroll-bar value. */
     SetScrollPos(hwndScroller, SB_CTL, pos, TRUE);
}

static void SetScrollThumbSize(HWND hwndScroller, SHORT visible)
{
     /* Set the scroll-bar thumb size value. */
     SCROLLINFO scrollInfo;
     scrollInfo.cbSize = sizeof(SCROLLINFO);
     scrollInfo.fMask  = SIF_PAGE;
     scrollInfo.nPage  = visible;

     SetScrollInfo(hwndScroller, SB_CTL, &scrollInfo, TRUE);
}

// ---------------------------------------------------------
// ---------------------------------------------------------

GbmBitmapViewer::GbmBitmapViewer(HMODULE         hMod,
                                 GbmDocument   & document,
                                 ConfigHandler & configHandler)
    : mIsFullMode(FALSE),
      mpLanguage(&LANG_LOOKUP_TABLE[0]), // English
      mDllModule(hMod),
      mGbmDocument(document),
      mConfigHandler(configHandler),
      mCurrentPopupMenuId(-1),
      mCurrentPageIndex(0),
      mScaleX(1.0),
      mScaleY(1.0),
      mZoomMode(GbmBitmapViewer::ZoomMode_NONE),
      mScalerType(GbmRenderer::ScalerType_SIMPLE),
      mLoadProgress(0),
      mLoadProgressMax(0),
      mHwndClient(0),
      mHwndScrollRect(0),
      mHwndVScroll(0),
      mHwndHScroll(0),
      mHwndPopupMenu(0),
      mpOldGlobalWindowProcedure(NULL),
      mHScrollMax(0),
      mHScrollPos(0),
      mHScrollPage(0),
      mVScrollMax(0),
      mVScrollPos(0),
      mVScrollPage(0),
      mCxClient(0),
      mCyClient(0)
{
    // check system language and lookup the matching resource set
   #if (WINVER >= 0x0500)
    const LANGID primaryLangId = PRIMARYLANGID(GetUserDefaultUILanguage());
   #else
    const LANGID primaryLangId = PRIMARYLANGID(GetUserDefaultLangID());
   #endif

    for (int i = 0; i < LANG_LOOKUP_TABLE_LENGTH; i++)
    {
        if (primaryLangId == LANG_LOOKUP_TABLE[i].language)
        {
            mpLanguage = &LANG_LOOKUP_TABLE[i];
            break;
        }
    }
    // if the system language was not found, keep English as default

    // load language specific strings
    memset(mStringUnsupportedFmt    , 0, sizeof(mStringUnsupportedFmt));
    memset(mStringSaveAs            , 0, sizeof(mStringSaveAs));
    memset(mStringAllFiles          , 0, sizeof(mStringAllFiles));
    memset(mStringFmtExtError       , 0, sizeof(mStringFmtExtError));
    memset(mStringScalerSimple      , 0, sizeof(mStringScalerSimple));
    memset(mStringScalerNearNeighbor, 0, sizeof(mStringScalerNearNeighbor));

    LoadString(mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_UNSUPPORTED_FMT],
               mStringUnsupportedFmt, sizeof(mStringUnsupportedFmt)/sizeof(mStringUnsupportedFmt[0]));
    LoadString(mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_SAVEAS],
               mStringSaveAs, sizeof(mStringSaveAs)/sizeof(mStringSaveAs[0]));
    LoadString(mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_STRING_ALL_FILES],
               mStringAllFiles, sizeof(mStringAllFiles)/sizeof(mStringAllFiles[0]));
    LoadString(mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_STRING_FMT_EXT_ERROR],
               mStringFmtExtError, sizeof(mStringFmtExtError)/sizeof(mStringFmtExtError[0]));
    LoadString(mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_STRING_SCALE_SIMPLE],
               mStringScalerSimple, sizeof(mStringScalerSimple)/sizeof(mStringScalerSimple[0]));
    LoadString(mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR],
               mStringScalerNearNeighbor, sizeof(mStringScalerNearNeighbor)/sizeof(mStringScalerNearNeighbor[0]));

    // read the config file if it exists and configure the viewer
    {
        const char * scalerName = mConfigHandler.getValueOfKey("scaler");
        if (scalerName != NULL)
        {
            for (int i = 0; i < SCALER_TABLE_LENGTH; i++)
            {
                if (strcmp(SCALER_TABLE[i].configName, scalerName) == 0)
                {
                    mScalerType = SCALER_TABLE[i].type;
                    break;
                }
            }
        }
        const char * backgroundRenderPages = mConfigHandler.getValueOfKey("progressive_render_pages");
        if (backgroundRenderPages != NULL)
        {
            int num;
            if (sscanf(backgroundRenderPages, "%i", &num) == 1)
            {
                mGbmDocument.setNumberOfBackgroundRenderPages(num);
            }
        }
    }
}

// ---------------------------------------------------------

GbmBitmapViewer::~GbmBitmapViewer()
{
    setParentWindow(0, FALSE);
}

// ---------------------------------------------------------

BOOL GbmBitmapViewer::setParentWindow(HWND hwnd, BOOL fullMode)
{
    // stop paint thread before locking interface
    mPaintThread.stop();
    MutexGuard interfaceGuard(mInterfaceMutex);

    if ((mHwndClient != 0) && (mpOldGlobalWindowProcedure != NULL))
    {
        SubclassWindow(mHwndClient, mpOldGlobalWindowProcedure);
        mpOldGlobalWindowProcedure = NULL;
    }

    destroyChildWindows();
    mHwndClient = 0;
    mIsFullMode = FALSE;

    mSaveAsFilename.clear();
    mSaveAsOptions.clear();

    if (hwnd == 0)
    {
        return FALSE;
    }

    // configure the viewer either for FULL or EMBEDDED mode
    if (fullMode)
    {
        mCurrentPopupMenuId = RES_RID_GBMVIEWER_FULL_POPUP;
        mZoomMode           = GbmBitmapViewer::ZoomMode_NONE;
    }
    else
    {
        mCurrentPopupMenuId = RES_RID_GBMVIEWER_SMALL_POPUP;
        mZoomMode           = GbmBitmapViewer::ZoomMode_STRETCH;
    }

    mIsFullMode = fullMode;
    mHwndClient = hwnd;

    // build popup menu and child windows
    createChildWindows(hwnd);

    // subclass parent window
    SetWindowLongPtr(mHwndClient, GWLP_USERDATA, (LONG_PTR)this);
    mpOldGlobalWindowProcedure = SubclassWindow(mHwndClient, GlobalWindowProcedure);

    // modify window styles
    SetWindowLong(mHwndClient, GWL_STYLE,
                  GetWindowLong(mHwndClient, GWL_STYLE) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // set initial scrollbar settings
    updateScrollBars(mHwndClient);

    return TRUE;
}

// ---------------------------------------------------------

void GbmBitmapViewer::setLoadProgress(const int progress, const int progressMax)
{
    // stop paint thread before locking interface
    mPaintThread.stop();
    {
        MutexGuard interfaceGuard(mInterfaceMutex);

        mLoadProgress    = (progress    < 0) ? 0 : progress;
        mLoadProgressMax = (progressMax < 0) ? 0 : progressMax;

        if (mLoadProgress > mLoadProgressMax)
        {
            mLoadProgress = mLoadProgressMax;
        }
    }
    if (mHwndClient)
    {
        InvalidateRect(mHwndClient, 0, FALSE);
        UpdateWindow(mHwndClient);
    }
}

// ---------------------------------------------------------

void GbmBitmapViewer::setScalerType(const GbmRenderer::ScalerType scalerType)
{
    // stop paint thread before locking interface
    mPaintThread.stop();
    MutexGuard interfaceGuard(mInterfaceMutex);
    if (mScalerType != scalerType)
    {
        mScalerType = scalerType;
        if (mHwndClient)
        {
            InvalidateRect(mHwndClient, 0, TRUE);
        }
    }
}

GbmRenderer::ScalerType GbmBitmapViewer::getScalerType() const
{
    return mScalerType;
}

// ---------------------------------------------------------

void GbmBitmapViewer::setNumberOfBackgroundRenderPages(const int numPages)
{
    // stop paint thread before locking interface
    mPaintThread.stop();
    MutexGuard interfaceGuard(mInterfaceMutex);
    try
    {
        mGbmDocument.setNumberOfBackgroundRenderPages(numPages);
    }
    catch(GbmException &)
    {
        // silently ignore, continue with defaults
    }
    if (mHwndClient)
    {
        InvalidateRect(mHwndClient, 0, TRUE);
    }
}

int GbmBitmapViewer::getNumberOfBackgroundRenderPages() const
{
    return mGbmDocument.getNumberOfBackgroundRenderPages();
}

// ---------------------------------------------------------

BOOL GbmBitmapViewer::hasBitmap() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return (mGbmDocument.getNumberOfPages() < 1) ? FALSE : TRUE;
}

// ---------------------------------------------------------

void GbmBitmapViewer::draw(HDC hdc, const RECT & rect, double pageScaleX, double pageScaleY)
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    MutexGuard documentGuard(mGbmDocument.getInterfaceMutex());
    if (! hasBitmap())
    {
        RECT fillRect = rect;

        if ((mLoadProgress    > 0) &&
            (mLoadProgressMax > 0) &&
            (mLoadProgress    <= mLoadProgressMax))
        {
            fillRect.top = rect.top + rect.bottom -
                           (LONG)((float)(rect.bottom) * mLoadProgress / mLoadProgressMax);
            FillRect(hdc, &fillRect, GetStockBrush(DKGRAY_BRUSH));
        }
        else
        {
            FillRect(hdc, &fillRect, GetStockBrush(GRAY_BRUSH));
        }
        return;
    }

    try
    {
        GbmRenderer & renderer(mGbmDocument.getRenderer(mCurrentPageIndex,
                                                        pageScaleX, pageScaleY,
                                                        mScalerType));

        // check if render rect is larger than the actual bitmap data
        int pageWidthI = 0, pageHeightI = 0, pageColorDepthI = 0;
        getPageSize(pageWidthI, pageHeightI, pageColorDepthI);

        const int pageWidth  = (int)(pageScaleX * (double)pageWidthI);
        const int pageHeight = (int)(pageScaleY * (double)pageHeightI);

        RECT renderRect = rect;

        if (renderRect.right - renderRect.left > pageWidth)
        {
            renderRect.right = pageWidth + renderRect.left;

            // draw only the visible area not filled by the bitmap right beside
            RECT unusedArea;
            unusedArea.left   = renderRect.right;
            unusedArea.top    = rect.top;
            unusedArea.right  = rect.right;
            unusedArea.bottom = rect.bottom;
            FillRect(hdc, &unusedArea, GetStockBrush(DKGRAY_BRUSH));
        }
        if (renderRect.bottom - renderRect.top > pageHeight)
        {
            renderRect.bottom = pageHeight + renderRect.top;

            // draw only the visible area not filled by the bitmap right beside
            RECT unusedArea;
            unusedArea.left   = rect.left;
            unusedArea.top    = renderRect.bottom;
            unusedArea.right  = rect.right;
            unusedArea.bottom = rect.bottom;
            FillRect(hdc, &unusedArea, GetStockBrush(DKGRAY_BRUSH));
        }

        // calculate the source rectangle to be extracted from the bitmap
        const int src_width  = (int) (renderRect.right  - renderRect.left);
        const int src_height = (int) (renderRect.bottom - renderRect.top);
        const int src_x      = 0;
        const int src_y      = pageHeight - src_height;

        renderer.renderToHDC(     src_x,      src_y,
                              src_width, src_height,
                             pageScaleX, pageScaleY, renderRect, hdc);
    }
    catch(...)
    {
        RECT fillRect = rect;
        FillRect(hdc, &fillRect, GetStockBrush(GRAY_BRUSH));
    }
}

//----------------------------------------------------------------------------

int GbmBitmapViewer::getNumberOfPages() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return mGbmDocument.getNumberOfPages();
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::getPageSize(int & width, int & height, int & colorDepth)
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    try
    {
        width = 0; height = 0; colorDepth = 0;
        mGbmDocument.getPageSize(mCurrentPageIndex, width, height, colorDepth);
    }
    catch(GbmException &)
    {
    }
}

//----------------------------------------------------------------------------

std::wstring GbmBitmapViewer::getLongDescription() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    MutexGuard documentGuard(mGbmDocument.getInterfaceMutex());
    if (hasBitmap())
    {
        return string2wstring(mGbmDocument.getLongDescription());
    }
    return mStringUnsupportedFmt;
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::executeOperations(HWND hwnd, long rotateBy, GbmDocument::MirrorMode mirrorMode)
{
    // stop paint thread before locking interface
    mPaintThread.stop();
    MutexGuard interfaceGuard(mInterfaceMutex);
    MutexGuard documentGuard(mGbmDocument.getInterfaceMutex());

    // get the current mouse pointer before changing it to hourglas
    HCURSOR hptrOld = GetCursor();

    try
    {
        /* Get the wait mouse pointer. */
        HCURSOR hptrWait = LoadCursor(NULL, IDC_WAIT);

        /* Set the mouse pointer to the wait pointer. */
        SetCursor(hptrWait);

        // Reset the renderer as it cannot detect these changes in all cases
        // because the size of the bitmap and the page index might not change.
        mGbmDocument.resetRenderer(mCurrentPageIndex);

        // Set the render page rotation angle
        // -> we rotate the original page data as this is lossless and most memory conservative
        mGbmDocument.rotate(mCurrentPageIndex, rotateBy);

        // Set the render page mirror mode
        // -> we mirror the original page data as this is lossless and most memory conservative
        mGbmDocument.mirror(mCurrentPageIndex, mirrorMode);
    }
    catch(...)
    {
    }
    // restore old mouse pointer
    SetCursor(hptrOld);

    InvalidateRect(hwnd, 0, TRUE);
}

//----------------------------------------------------------------------------

BOOL GbmBitmapViewer::executeMenuOperations(HWND hwnd, SHORT itemId)
{
    switch(itemId)
    {
        // -------------------------
        // File menu
        // -------------------------

        case MID_FILE_SAVE_AS:
            if (showFileDialog_SaveAs(hwnd, mSaveAsFilename, mSaveAsOptions))
            {
                // get the current mouse pointer before changing it to hourglas
                HCURSOR hptrOld = GetCursor();

                /* Get the wait mouse pointer. */
                HCURSOR hptrWait = LoadCursor(NULL, IDC_WAIT);

                /* Set the mouse pointer to the wait pointer. */
                SetCursor(hptrWait);

                // Write file
                try
                {
                    int ft;
                    const std::string cSaveAsFilename(wstring2string(mSaveAsFilename));
                    const std::string cSaveAsOptions(wstring2string(mSaveAsOptions));

                    if (GBM_ERR_OK == mGbmDocument.getGbmAccessor().Gbm_guess_filetype(
                                          cSaveAsFilename.c_str(), &ft))
                    {
                        mGbmDocument.writeDocumentPageToFile(cSaveAsFilename.c_str(),
                                                             cSaveAsOptions.c_str(),
                                                             mCurrentPageIndex);
                    }
                    else
                    {
                        MessageBox(hwnd, mStringFmtExtError, GBMBITMAPVIEWER_Title,
                                   MB_OK | MB_ICONERROR | MB_APPLMODAL);
                    }
                }
                catch(GbmException & ex)
                {
                    const GbmAccessor & gbmAccessor(mGbmDocument.getGbmAccessor());

                    // extract error message
                    std::wstring gbmErrorMsg(string2wstring(gbmAccessor.Gbm_err(ex.getErrorId())));
                    const std::wstring msg(string2wstring(ex.getErrorMessage()));
                    if (msg.empty())
                    {
                        MessageBox(hwnd, gbmErrorMsg.c_str(), GBMBITMAPVIEWER_Title,
                                   MB_OK | MB_ICONERROR | MB_APPLMODAL);
                    }
                    else
                    {
                        // format the error string: gbm_error:\nuser_message
                        gbmErrorMsg.append(L"\n");
                        gbmErrorMsg.append(msg);
                        MessageBox(hwnd, gbmErrorMsg.c_str(), GBMBITMAPVIEWER_Title,
                                   MB_OK | MB_ICONERROR | MB_APPLMODAL);
                    }
                }
                // restore old mouse pointer
                SetCursor(hptrOld);
            }
            return TRUE;

        // -------------------------
        // Bitmap menu
        // -------------------------

        case MID_BITMAP_REF_HORZ:
            executeOperations(hwnd, 0, GbmDocument::MirrorMode_HORIZONTAL);
            return TRUE;

        case MID_BITMAP_REF_VERT:
            executeOperations(hwnd, 0, GbmDocument::MirrorMode_VERTICAL);
            return TRUE;

        case MID_BITMAP_TRANSPOSE:
            if (mIsFullMode)
            {
                executeOperations(hwnd, 0, GbmDocument::MirrorMode_TRANSPOSE);
                return TRUE;
            }
            break;

        case MID_BITMAP_ROT_P90:
            if (mIsFullMode)
            {
                executeOperations(hwnd, 90, GbmDocument::MirrorMode_NONE);
                return TRUE;
            }
            break;

        case MID_BITMAP_ROT_M90:
            if (mIsFullMode)
            {
                executeOperations(hwnd, 270, GbmDocument::MirrorMode_NONE);
                return TRUE;
            }
            break;

        // -------------------------
        // View menu
        // -------------------------

        case MID_VIEW_PAGE_FIRST:
            gotoPageIndex(hwnd, 0);
            return TRUE;

        case MID_VIEW_PAGE_PREV:
            gotoPageIndex(hwnd, mCurrentPageIndex-1);
            return TRUE;

        case MID_VIEW_PAGE_NEXT:
            gotoPageIndex(hwnd, mCurrentPageIndex+1);
            return TRUE;

        case MID_VIEW_PAGE_LAST:
            gotoPageIndex(hwnd, mGbmDocument.getNumberOfPages()-1);
            return TRUE;

        // -------------------------

        case MID_VIEW_ZOOM_DOWN:
        {
            // stop paint thread before locking interface
            mPaintThread.stop();
            MutexGuard interfaceGuard(mInterfaceMutex);
            if (mIsFullMode)
            {
                mZoomMode = GbmBitmapViewer::ZoomMode_FREE;
                mScaleX   = MAX(0.1, mScaleX/1.1);
                mScaleY   = MAX(0.1, mScaleY/1.1);
                updateScaleFactor(hwnd, TRUE, FALSE);
                return TRUE;
            }
            break;
        }

        case MID_VIEW_ZOOM_UP:
        {
            // stop paint thread before locking interface
            mPaintThread.stop();
            MutexGuard interfaceGuard(mInterfaceMutex);
            if (mIsFullMode)
            {
                mZoomMode = GbmBitmapViewer::ZoomMode_FREE;
                mScaleX   = MIN(100, mScaleX*1.1);
                mScaleY   = MIN(100, mScaleY*1.1);
                updateScaleFactor(hwnd, TRUE, FALSE);
                return TRUE;
            }
            break;
        }

        case MID_VIEW_ZOOM_NONE:
        {
            // stop paint thread before locking interface
            mPaintThread.stop();
            MutexGuard interfaceGuard(mInterfaceMutex);
            if (mIsFullMode)
            {
                mZoomMode = GbmBitmapViewer::ZoomMode_NONE;
                updateScaleFactor(hwnd, TRUE, FALSE);
                return TRUE;
            }
            break;
        }

        case MID_VIEW_ZOOM_FITWIN:
        {
            // stop paint thread before locking interface
            mPaintThread.stop();
            MutexGuard interfaceGuard(mInterfaceMutex);
            if (mIsFullMode)
            {
                mZoomMode = GbmBitmapViewer::ZoomMode_FITWIN;
                updateScaleFactor(hwnd, TRUE, FALSE);
                return TRUE;
            }
            break;
        }

        case MID_VIEW_ZOOM_FITWIDTH:
        {
            // stop paint thread before locking interface
            mPaintThread.stop();
            MutexGuard interfaceGuard(mInterfaceMutex);
            if (mIsFullMode)
            {
                mZoomMode = GbmBitmapViewer::ZoomMode_FITWIDTH;
                updateScaleFactor(hwnd, TRUE, FALSE);
                return TRUE;
            }
            break;
        }

        case MID_VIEW_PREFERENCES:
            DialogBoxParam(mDllModule,
                           MAKEINTRESOURCE(mpLanguage->resid[RES_RID_GBMVIEWER_PREFS]),
                           hwnd,
                           GlobalPrefsDlgWindowProcedure,
                           (LPARAM)this);
            return TRUE;

        // -------------------------
        // Help menu
        // -------------------------

        case MID_HELP_ABOUT:
        {
            static const wchar_t * info = L"GBM plugin (version %d.%d), using GBM.DLL %d.%d\n(c) 2006-2012, Heiko Nitzsche";
            static const size_t infoLen = wcslen(info);
            static const size_t maxBufferChars = infoLen + 10;

            wchar_t * pBuf = NULL;
            try
            {
                pBuf = new wchar_t[maxBufferChars + 1];
            }
            catch(...)
            {
                pBuf = NULL;
            }
            if (pBuf != NULL)
            {
                const GbmAccessor & gbmAccessor(mGbmDocument.getGbmAccessor());
                swprintf(pBuf, maxBufferChars, info,
                          GBM_PLUGIN_VERSION / 100, GBM_PLUGIN_VERSION % 100,
                            gbmAccessor.Gbm_version() / 100, gbmAccessor.Gbm_version() % 100);

                MessageBox(hwnd, pBuf, GBMBITMAPVIEWER_Title,
                           MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);

                delete [] pBuf;
                pBuf = NULL;
            }
            return TRUE;
        }

        case MID_HELP_IMAGEINFO:
            DialogBoxParam(mDllModule,
                           MAKEINTRESOURCE(mpLanguage->resid[RES_RID_GBMVIEWER_FORMATINFO]),
                           hwnd,
                           GlobalInfoDlgWindowProcedure,
                           (LPARAM)this);
            return TRUE;
    }

    // item unprocessed
    return FALSE;
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::gotoPageIndex(HWND hwnd, int pageIndex)
{
    // stop paint thread before locking interface
    mPaintThread.stop();
    MutexGuard interfaceGuard(mInterfaceMutex);

    if (pageIndex != mCurrentPageIndex)
    {
        const int oldPageIndex = mCurrentPageIndex;

        mCurrentPageIndex = MAX(pageIndex, 0);
        mCurrentPageIndex = MIN(mCurrentPageIndex, mGbmDocument.getNumberOfPages()-1);

        if (mCurrentPageIndex != oldPageIndex)
        {
            InvalidateRect(hwnd, 0, TRUE);
        }
    }
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::updateScaleFactor(HWND hwnd, BOOL forceWindowRepaint, BOOL skipWindowRepaint)
{
    const double oldScaleX = mScaleX;
    const double oldScaleY = mScaleY;

    updateWindowSize(hwnd);

    switch(mZoomMode)
    {
        case GbmBitmapViewer::ZoomMode_NONE:
            mScaleX = 1.0;
            mScaleY = mScaleX;
            break;

        case GbmBitmapViewer::ZoomMode_FITWIN:
        {
            int pageWidth = 0, pageHeight = 0, pageColorDepth = 0;
            getPageSize(pageWidth, pageHeight, pageColorDepth);
            const double scaleX  = (pageWidth  > 0) ? ((double)mCxClient)/pageWidth  : 1.0;
            const double scaleY  = (pageHeight > 0) ? ((double)mCyClient)/pageHeight : 1.0;
            mScaleX = MIN(scaleX, scaleY);
            mScaleY = mScaleX;
            break;
        }

        case GbmBitmapViewer::ZoomMode_FITWIDTH:
        {
            int pageWidth = 0, pageHeight = 0, pageColorDepth = 0;
            getPageSize(pageWidth, pageHeight, pageColorDepth);
            mScaleX = (pageWidth > 0) ? ((double)mCxClient)/pageWidth  : 1.0;
            mScaleY = mScaleX;
            break;
        }

        case GbmBitmapViewer::ZoomMode_STRETCH:
        {
            int pageWidth = 0, pageHeight = 0, pageColorDepth = 0;
            getPageSize(pageWidth, pageHeight, pageColorDepth);
            mScaleX = (pageWidth  > 0) ? ((double)mCxClient)/pageWidth  : 1.0;
            mScaleY = (pageHeight > 0) ? ((double)mCyClient)/pageHeight : 1.0;
            break;
        }

        default: // ZoomMode_FREE
            break;
    }

    if ((! skipWindowRepaint) &&
        (forceWindowRepaint || (mScaleX != oldScaleX) || (mScaleY != oldScaleY)))
    {
        InvalidateRect(hwnd, 0, TRUE);
   }
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::createChildWindows(HWND hwnd)
{
    mHwndPopupMenu = LoadMenu(mDllModule, MAKEINTRESOURCE(mpLanguage->resid[mCurrentPopupMenuId]));

    if (! mIsFullMode)
    {
        return;
    }

    // create the client windows
    RECT parentRect = { 0 };
    GetClientRect(hwnd, &parentRect);

    mHwndScrollRect = CreateWindow(
        L"STATIC",                               /* Client window class name    */
        NULL,                                    /* Title text                  */
        WS_CHILD | SS_WHITERECT,                 /* Style of window             */
        parentRect.right,                        /* X-Position                  */
        parentRect.bottom,                       /* Y-Position                  */
        0,                                       /* Width                       */
        0,                                       /* Height                      */
        hwnd,                                    /* Parent window handle        */
        (HMENU) WND_ID_SCROLLBAR_RECT,           /* Class Frame menu            */
        mDllModule,                              /* Module handle for resources */
        NULL);

    // create the scrollbars
    mHwndVScroll = CreateWindow(
        L"SCROLLBAR",                            /* Client window class name    */
        NULL,                                    /* Title text                  */
        WS_CHILD | SBS_VERT,                     /* Style of window             */
        parentRect.right,                        /* X-Position                  */
        parentRect.top,                          /* Y-Position                  */
        0,                                       /* Width                       */
        parentRect.bottom - parentRect.top + 1,  /* Height                      */
        hwnd,                                    /* Parent window handle        */
        (HMENU) WND_ID_SCROLLBAR_VERT,           /* Class Frame menu            */
        mDllModule,                              /* Module handle for resources */
        NULL);

    mHwndHScroll = CreateWindow(
        L"SCROLLBAR",                            /* Client window class name    */
        NULL,                                    /* Title text                  */
        WS_CHILD | SBS_HORZ,                     /* Style of window             */
        parentRect.left,                         /* X-Position                  */
        parentRect.bottom,                       /* Y-Position                  */
        parentRect.right - parentRect.left + 1,  /* Width                       */
        0,                                       /* Height                      */
        hwnd,                                    /* Parent window handle        */
        (HMENU) WND_ID_SCROLLBAR_HORZ,           /* Class Frame menu            */
        mDllModule,                              /* Module handle for resources */
        NULL);
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::updateChildWindows()
{
    if (!mIsFullMode)
    {
        // child windows only in full mode
        return;
    }

    // Update the layout of child windows
    RECT parentRect = { 0 };
    GetClientRect(mHwndClient, &parentRect);

    int vertScrollBarWidth  = 0;
    int horzScrollBarHeight = 0;
    bool scrollRectVisible  = false;

    if (IsWindowVisible(mHwndVScroll))
    {
        vertScrollBarWidth += GetSystemMetrics(SM_CXVSCROLL);
        scrollRectVisible = true;
    }
    if (IsWindowVisible(mHwndHScroll))
    {
        horzScrollBarHeight += GetSystemMetrics(SM_CYHSCROLL);
        scrollRectVisible = true;
    }

    // reposition the small rectangle between the scroll bars
    MoveWindow(mHwndScrollRect,
               parentRect.right  - vertScrollBarWidth,
               parentRect.bottom - horzScrollBarHeight,
               vertScrollBarWidth,
               horzScrollBarHeight,
               TRUE);
    ShowWindow(mHwndScrollRect, scrollRectVisible ? SW_SHOW : SW_HIDE);

    // reposition the vertical scroll bar
    MoveWindow(mHwndVScroll,
               parentRect.right - vertScrollBarWidth,
               parentRect.top,
               vertScrollBarWidth,
               parentRect.bottom - parentRect.top + 1 - horzScrollBarHeight,
               TRUE);

    // reposition the horizontal scroll bar
    MoveWindow(mHwndHScroll,
               parentRect.left,
               parentRect.bottom - horzScrollBarHeight,
               parentRect.right - parentRect.left + 1 - vertScrollBarWidth,
               horzScrollBarHeight,
               TRUE);
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::destroyChildWindows()
{
    if (mHwndPopupMenu != 0)
    {
       DestroyMenu(mHwndPopupMenu);
       mHwndPopupMenu = 0;
       mCurrentPopupMenuId = -1;
    }
    if (IsWindow(mHwndScrollRect))
    {
       DestroyWindow(mHwndScrollRect);
    }
    if (IsWindow(mHwndVScroll))
    {
       DestroyWindow(mHwndVScroll);
    }
    if (IsWindow(mHwndHScroll))
    {
       DestroyWindow(mHwndHScroll);
    }
    mCxClient = 0;
    mCyClient = 0;

    mHwndScrollRect = 0;
    mHwndVScroll    = 0;
    mHwndHScroll    = 0;

    mHScrollMax  = 0;
    mHScrollPos  = 0;
    mVScrollMax  = 0;
    mVScrollPos  = 0;
    mHScrollPage = 0;
    mVScrollPage = 0;
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::updateWindowSize(HWND hwnd)
{
    /* get window size */
    RECT rect = { 0, 0, 0, 0 };
    GetClientRect(hwnd, &rect);

    int vertScrollBarWidth  = 0;
    int horzScrollBarHeight = 0;

    if (mIsFullMode)
    {
        if (IsWindowVisible(mHwndVScroll))
        {
            vertScrollBarWidth += GetSystemMetrics(SM_CXVSCROLL);
        }
        if (IsWindowVisible(mHwndHScroll))
        {
            horzScrollBarHeight += GetSystemMetrics(SM_CYHSCROLL);
        }
    }
    mCxClient = (SHORT)(rect.right  - rect.left + 1 - vertScrollBarWidth);
    mCyClient = (SHORT)(rect.bottom - rect.top  + 1 - horzScrollBarHeight);
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::updateScrollBars(HWND hwnd)
{
    updateWindowSize(hwnd);

    if (mIsFullMode)
    {
        if (hasBitmap())
        {
            try
            {
                int pageWidth = 0, pageHeight = 0, pageColorDepth = 0;
                getPageSize(pageWidth, pageHeight, pageColorDepth);

                pageWidth  = (int)(mScaleX * pageWidth);
                pageHeight = (int)(mScaleY * pageHeight);

                mHScrollMax = (SHORT)(MAX(0, pageWidth - 1));
                mVScrollMax = (SHORT)(MAX(0, pageHeight - 1));

                mHScrollPage = (pageWidth  > mCxClient) ? mCxClient : (SHORT)pageWidth;
                mVScrollPage = (pageHeight > mCyClient) ? mCyClient : (SHORT)pageHeight;

                // MaxScrollPos = MaxRangeValue - (PageSize - 1)
                const SHORT scrollHMax = mHScrollMax - (mHScrollPage - 1);
                mHScrollPos = MIN(MAX(0, scrollHMax), mHScrollPos);
                mHScrollPos = MAX(0, mHScrollPos);
                SetScrollBar(mHwndHScroll, mHScrollPos, 0, mHScrollMax);
                SetScrollThumbSize(mHwndHScroll, mHScrollPage);
                ScrollBarEnable(mHwndHScroll, mHScrollMax ? TRUE : FALSE);
                ShowScrollBar(mHwndHScroll, SB_CTL, pageWidth > mCxClient);

                // MaxScrollPos = MaxRangeValue - (PageSize - 1)
                const SHORT scrollVMax = mVScrollMax - (mVScrollPage - 1);
                mVScrollPos = MIN(MAX(0, scrollVMax), mVScrollPos);
                mVScrollPos = MAX(0, mVScrollPos);
                SetScrollBar(mHwndVScroll, mVScrollPos, 0, mVScrollMax);
                SetScrollThumbSize(mHwndVScroll, mVScrollPage);
                ScrollBarEnable(mHwndVScroll, mVScrollMax ? TRUE : FALSE);
                ShowScrollBar(mHwndVScroll, SB_CTL, pageHeight > mCyClient);
            }
            catch(GbmException &)
            {
            }
        }
        else
        {
            ScrollBarEnable(mHwndHScroll, FALSE);
            ScrollBarEnable(mHwndVScroll, FALSE);
            ShowScrollBar(mHwndHScroll, SB_CTL, FALSE);
            ShowScrollBar(mHwndVScroll, SB_CTL, FALSE);
        }
        updateChildWindows();
    }
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::updateMenuItems(HMENU refHwndMenu, SHORT itemId) const
{
    const BOOL hasData  = hasBitmap();
    const int  numPages = mGbmDocument.getNumberOfPages();

    switch(itemId)
    {
       case RES_RID_GBMVIEWER_SMALL_POPUP:
            EnableMenuItemId(mHwndPopupMenu, MID_FILE_SAVE_AS    , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_BITMAP_REF_HORZ , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_BITMAP_REF_VERT , hasData);

            // Page submenu
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_PAGE_FIRST, mCurrentPageIndex > 0);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_PAGE_PREV , mCurrentPageIndex > 0);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_PAGE_NEXT , mCurrentPageIndex < numPages-1);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_PAGE_LAST , mCurrentPageIndex < numPages-1);

            // Help submenu
            EnableMenuItemId(mHwndPopupMenu, MID_HELP_IMAGEINFO, hasData);
            break;

        // -------------------------

        case RES_RID_GBMVIEWER_FULL_POPUP:
            EnableMenuItemId(mHwndPopupMenu, MID_FILE_SAVE_AS    , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_BITMAP_REF_HORZ , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_BITMAP_REF_VERT , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_BITMAP_TRANSPOSE, hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_BITMAP_ROT_P90  , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_BITMAP_ROT_M90  , hasData);

            // Page submenu
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_PAGE_FIRST, mCurrentPageIndex > 0);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_PAGE_PREV , mCurrentPageIndex > 0);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_PAGE_NEXT , mCurrentPageIndex < numPages-1);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_PAGE_LAST , mCurrentPageIndex < numPages-1);

            // Zoom menu entries
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_DOWN    , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_UP      , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , hasData);
            EnableMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, hasData);

            switch(mZoomMode)
            {
                case GbmBitmapViewer::ZoomMode_NONE:
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , TRUE);
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;

                case GbmBitmapViewer::ZoomMode_FITWIN:
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , TRUE);
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;

                case GbmBitmapViewer::ZoomMode_FITWIDTH:
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, TRUE);
                    break;

                default: // ZoomMode_FREE
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItemId(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;
            }

            // Help submenu
            EnableMenuItemId(mHwndPopupMenu, MID_HELP_IMAGEINFO, hasData);
            break;

        // -------------------------

        default:
            break;
    }
}

//----------------------------------------------------------------------------

//
// Here is the window procedure for the client window.
//
LRESULT GbmBitmapViewer::instanceWindowProcedure(HWND hwnd,
                                                 UINT msg,
                                                 WPARAM wparam,
                                                 LPARAM lparam)
{
    #define CONTROL_KEY_PRESSED  0x8000

    switch(msg)
    {
        case WM_SIZE:
        {
            // first let the default handler do the resizing
            const LRESULT result = DefWindowProc(hwnd, msg, wparam, lparam);

            // stop paint thread before locking interface
            mPaintThread.stop();
            MutexGuard interfaceGuard(mInterfaceMutex);

            updateChildWindows();
            updateScaleFactor(hwnd, FALSE, FALSE);
            updateScrollBars(hwnd);
            return result;
        }

        // -------------------------

        case WM_HSCROLL:
            if (mIsFullMode)
            {
                // stop paint thread before locking interface
                mPaintThread.stop();
                {
                    MutexGuard interfaceGuard(mInterfaceMutex);

                    // set line increment to SCROLL_PAGE_INCREMENT_STEPS steps of the image width
                    SHORT scrollbar_line_increment = mHScrollMax / SCROLL_PAGE_INCREMENT_STEPS;
                    scrollbar_line_increment = MAX(1, scrollbar_line_increment);

                    const SHORT scrollHMax = mHScrollMax - (mHScrollPage - 1);
                    assert(scrollHMax >= 0);

                    switch(LOWORD(wparam))
                    {
                        case SB_LINELEFT:
                            mHScrollPos = MAX(0, mHScrollPos - scrollbar_line_increment);
                            break;

                        case SB_LINERIGHT:
                            mHScrollPos = MIN(scrollHMax, mHScrollPos + scrollbar_line_increment);
                            break;

                        case SB_PAGELEFT:
                            mHScrollPos = MAX(0, mHScrollPos - mHScrollPage);
                            break;

                        case SB_PAGERIGHT:
                            mHScrollPos = MIN(scrollHMax, mHScrollPos + mHScrollPage);
                            break;

                        case SB_LEFT:
                            mHScrollPos = 0;
                            break;

                        case SB_RIGHT:
                            mHScrollPos = scrollHMax;
                            break;

                        case SB_THUMBPOSITION:
                        case SB_THUMBTRACK:
                            mHScrollPos = HIWORD(wparam);
                            break;
                    }
                }
                SetScrollBarPos(mHwndHScroll, mHScrollPos);
                InvalidateRect(hwnd, 0, TRUE);
                UpdateWindow(hwnd);
                return 0;
            }
            break;

        // -------------------------

        case WM_VSCROLL:
            if (mIsFullMode)
            {
                // stop paint thread before locking interface
                mPaintThread.stop();
                {
                    MutexGuard interfaceGuard(mInterfaceMutex);

                    // set line increment to SCROLL_PAGE_INCREMENT_STEPS steps of the image height
                    SHORT scrollbar_line_increment = mVScrollMax / SCROLL_PAGE_INCREMENT_STEPS;
                    scrollbar_line_increment = MAX(1, scrollbar_line_increment);

                    const SHORT scrollHMax = mHScrollMax - (mHScrollPage - 1);
                    assert(scrollHMax >= 0);
                    const SHORT scrollVMax = mVScrollMax - (mVScrollPage - 1);
                    assert(scrollVMax >= 0);

                    switch(LOWORD(wparam))
                    {
                        case SB_LINEUP:
                            mVScrollPos = MAX(0, mVScrollPos - scrollbar_line_increment);
                            break;

                        case SB_LINEDOWN:
                            mVScrollPos = MIN(scrollVMax, mVScrollPos + scrollbar_line_increment);
                            break;

                        case SB_PAGEUP:
                            mVScrollPos = MAX(0, mVScrollPos - mVScrollPage);
                            break;

                        case SB_PAGEDOWN:
                            mVScrollPos = MIN(scrollVMax, mVScrollPos + mVScrollPage);
                            break;

                        case SB_TOP:
                            mHScrollPos = 0;
                            mVScrollPos = 0;
                            break;

                        case SB_BOTTOM:
                            mHScrollPos = scrollHMax;
                            mVScrollPos = scrollVMax;
                            break;

                        case SB_THUMBPOSITION:
                        case SB_THUMBTRACK:
                            mVScrollPos = HIWORD(wparam);
                            break;
                    }
                }
                SetScrollBarPos(mHwndHScroll, mHScrollPos);
                SetScrollBarPos(mHwndVScroll, mVScrollPos);
                InvalidateRect(hwnd, 0, TRUE);
                UpdateWindow(hwnd);
                return 0;
            }
            break;

        // -------------------------

        case WM_MOUSEWHEEL:
            if (mIsFullMode)
            {
                // stop paint thread before locking interface
                mPaintThread.stop();
                {
                    MutexGuard interfaceGuard(mInterfaceMutex);

                    const SHORT zDelta = GET_WHEEL_DELTA_WPARAM(wparam);
                    SHORT scrollbar_increment = 1;

                    // Pagewise scrolling?
                    const SHORT nVirtKey = GetKeyState(VK_CONTROL);
                    if (nVirtKey & CONTROL_KEY_PRESSED)
                    {
                        // page increment
                        scrollbar_increment = mVScrollPage;
                    }
                    else
                    {
                        // line increment to SCROLL_PAGE_INCREMENT_STEPS steps of the image height
                        scrollbar_increment = mVScrollMax / SCROLL_PAGE_INCREMENT_STEPS;
                        scrollbar_increment = MAX(1, scrollbar_increment);
                    }

                    scrollbar_increment *= zDelta / WHEEL_DELTA;
                    mVScrollPos -= scrollbar_increment;

                    // MaxScrollPos = MaxRangeValue - (PageSize - 1)
                    const SHORT scrollVMax = mVScrollMax - (mVScrollPage - 1);
                    mVScrollPos = MIN(MAX(0, scrollVMax), mVScrollPos);
                    mVScrollPos = MAX(0, mVScrollPos);
                }
                SetScrollBarPos(mHwndVScroll, mVScrollPos);
                InvalidateRect(hwnd, 0, TRUE);
                UpdateWindow(hwnd);
                return 0;
            }
            break;

        // -------------------------

        case WM_KEYDOWN:
        {
            const SHORT nVirtKey = GetKeyState(VK_CONTROL);
            if (nVirtKey & CONTROL_KEY_PRESSED)
            {
                switch((LONG)wparam)
                {
                    case VK_HOME: // MID_VIEW_PAGE_FIRST
                        if (executeMenuOperations(hwnd, MID_VIEW_PAGE_FIRST))
                        {
                            return 0;
                        }
                        break;

                    case VK_SUBTRACT: // MID_VIEW_PAGE_PREV
                    case VK_OEM_MINUS:
                        if (executeMenuOperations(hwnd, MID_VIEW_PAGE_PREV))
                        {
                            return 0;
                        }
                        break;

                    case VK_ADD: // MID_VIEW_PAGE_NEXT
                    case VK_OEM_PLUS:
                        if (executeMenuOperations(hwnd, MID_VIEW_PAGE_NEXT))
                        {
                            return 0;
                        }
                        break;

                    case VK_END: // MID_VIEW_PAGE_LAST
                        if (executeMenuOperations(hwnd, MID_VIEW_PAGE_LAST))
                        {
                            return 0;
                        }
                        break;

                    case '1': // MID_VIEW_ZOOM_NONE
                    case VK_NUMPAD1:
                        if (executeMenuOperations(hwnd, MID_VIEW_ZOOM_NONE))
                        {
                            return 0;
                        }
                        break;

                    case '2': // MID_VIEW_ZOOM_FITWIN
                    case VK_NUMPAD2:
                        if (executeMenuOperations(hwnd, MID_VIEW_ZOOM_FITWIN))
                        {
                            return 0;
                        }
                        break;

                    case '3': // MID_VIEW_ZOOM_FITWIDTH
                    case VK_NUMPAD3:
                        if (executeMenuOperations(hwnd, MID_VIEW_ZOOM_FITWIDTH))
                        {
                            return 0;
                        }
                        break;

                    case 'A': // MID_FILE_SAVE_AS
                        if (executeMenuOperations(hwnd, MID_FILE_SAVE_AS))
                        {
                            return 0;
                        }
                        break;

                    case 'H': // MID_BITMAP_REF_HORZ
                        if (executeMenuOperations(hwnd, MID_BITMAP_REF_HORZ))
                        {
                            return 0;
                        }
                        break;

                    case 'V': // MID_BITMAP_REF_VERT
                        if (executeMenuOperations(hwnd, MID_BITMAP_REF_VERT))
                        {
                            return 0;
                        }
                        break;

                    case 'D': // MID_BITMAP_TRANSPOSE
                        if (executeMenuOperations(hwnd, MID_BITMAP_TRANSPOSE))
                        {
                            return 0;
                        }
                        break;

                    case 'R': // MID_BITMAP_ROT_P90
                        if (executeMenuOperations(hwnd, MID_BITMAP_ROT_P90))
                        {
                            return 0;
                        }
                        break;

                    case 'L': // MID_BITMAP_ROT_M90
                        if (executeMenuOperations(hwnd, MID_BITMAP_ROT_M90))
                        {
                            return 0;
                        }
                        break;
                }
            }
            else
            {
                switch((LONG)wparam)
                {
                    case VK_LEFT:
                    case VK_RIGHT:
                        if (IsWindow(mHwndHScroll))
                        {
                            // forward keyboard messages to the scrollers and let them do the processing
                            return SendMessage(mHwndHScroll, msg, wparam, lparam);
                        }
                        break;

                    case VK_UP:
                    case VK_DOWN:
                    case VK_PRIOR:
                    case VK_NEXT:
                    case VK_HOME:
                    case VK_END:
                        if (IsWindow(mHwndVScroll))
                        {
                            // forward keyboard messages to the scrollers and let them do the processing
                            return SendMessage(mHwndVScroll, msg, wparam, lparam);
                        }
                        break;

                    case VK_SUBTRACT: // MID_VIEW_ZOOM_DOWN
                    case VK_OEM_MINUS:
                        if (executeMenuOperations(hwnd, MID_VIEW_ZOOM_DOWN))
                        {
                            return 0;
                        }
                        break;

                    case VK_ADD: // MID_VIEW_ZOOM_UP
                    case VK_OEM_PLUS:
                        if (executeMenuOperations(hwnd, MID_VIEW_ZOOM_UP))
                        {
                            return 0;
                        }
                        break;
                }
            }
            break;
        }

        // -------------------------

        case WM_ERASEBKGND:
            // stop paint thread before locking interface
            mPaintThread.stop();
            {
                MutexGuard interfaceGuard(mInterfaceMutex);
                updateScaleFactor(hwnd, FALSE, TRUE);
                updateScrollBars(hwnd);
            }
            // suppress erasing the background by handling the message silently
            return 0;

        // -------------------------

        case WM_PAINT:
        {
            mPaintThread.stop();

            // move the output origin by the scroller positions ...
            // ... and increase the render height and width
            RECT rect = { 0, 0, 0, 0 };
            rect.left   = -mHScrollPos;
            rect.top    = -mVScrollPos;
            rect.right  = mCxClient + mHScrollPos - 1;
            rect.bottom = mCyClient + mVScrollPos - 1;

            PAINTSTRUCT paintStruct;
            HDC hdc = BeginPaint(hwnd, &paintStruct);

            if (hasBitmap())
            {
                // Forward the paint request to the paint thread to decouple
                // the rendering from the message queue (shared across plugins).
                if (! mPaintThread.schedulePaint(this, hwnd, rect, mScaleX, mScaleY))
                {
                    // Draw directly as fallback
                    draw(hdc, rect, mScaleX, mScaleY);
                }
            }
            else
            {
                // Draw directly as this just paint the progress rectangle
                draw(hdc, rect, mScaleX, mScaleY);
            }
            EndPaint(hwnd, &paintStruct);
            return 0;
        }

        // -------------------------

        case WM_INITMENUPOPUP:
            if ((HMENU)wparam == GetSubMenu(mHwndPopupMenu, 0))
            {
                updateMenuItems(mHwndPopupMenu, mCurrentPopupMenuId);
                return 0;
            }
            break;

        // -------------------------

        case WM_CONTEXTMENU:
            TrackPopupMenu(GetSubMenu(mHwndPopupMenu, 0),
                           GetSystemMetrics(SM_MENUDROPALIGNMENT) | TPM_LEFTBUTTON,
                           GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), 0,
                           hwnd, NULL);
            return 0;

        // -------------------------

        case WM_COMMAND:
            if (executeMenuOperations(hwnd, LOWORD(wparam) /* itemId */))
            {
                return 0;
            }
            break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

//----------------------------------------------------------------------------

BOOL GbmBitmapViewer::showFileDialog_SaveAs(const HWND     hwnd,
                                            std::wstring & filename,
                                            std::wstring & options)
{
    if (hasBitmap())
    {
        GbmSaveFileDialog saveFileDialog(mDllModule,
                                         mGbmDocument.getGbmAccessor(),
                                         mStringSaveAs,
                                         mStringAllFiles);
        int w = 0, h = 0, bpp = 0;
        getPageSize(w, h, bpp);
        if (saveFileDialog.show(hwnd, bpp, filename, options))
        {
            return TRUE;
        }
    }
    filename.clear();
    options.clear();
    return FALSE;
}

// ---------------------------------------------------------
// ---------------------------------------------------------

GbmBitmapViewer::PaintThread::PaintThread()
    : Thread(300*1024)
{
}

// ------------------

GbmBitmapViewer::PaintThread::~PaintThread()
{
}

// ------------------

BOOL GbmBitmapViewer::PaintThread::schedulePaint(
    GbmBitmapViewer * viewer,
    HWND hwnd, const RECT & rect, double scaleX, double scaleY)
{
    GbmBitmapViewer::PaintThread::TASK_ARG * pTask =
      new GbmBitmapViewer::PaintThread::TASK_ARG;
    if (pTask == NULL)
    {
        return FALSE;
    }
    pTask->hwnd   = hwnd;
    pTask->viewer = viewer;
    pTask->rect   = rect;
    pTask->scaleX = scaleX;
    pTask->scaleY = scaleY;

    if (! scheduleTask(pTask))
    {
        delete pTask;
        return FALSE;
    }
    return TRUE;
}

// ------------------

void GbmBitmapViewer::PaintThread::run(void * task)
{
    GbmBitmapViewer::PaintThread::TASK_ARG * pTask =
      (GbmBitmapViewer::PaintThread::TASK_ARG *) task;

    if (pTask && pTask->hwnd && pTask->viewer)
    {
        try
        {
            const HDC hdc = GetDC(pTask->hwnd);
            pTask->viewer->draw(hdc, pTask->rect,
                                     pTask->scaleX,
                                     pTask->scaleY);
            ReleaseDC(pTask->hwnd, hdc);
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
    delete pTask;
    pTask = NULL;
}

