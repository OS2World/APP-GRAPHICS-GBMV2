//********************************************************************************
// Mozilla GBM plug-in: GbmBitmapViewer.cpp (OS/2)
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
//   16-Jul-2006: Initial release
//
//   30-Jul-2006: Add FULL mode viewer with a load of features
//                * menus, popup menu, scrollers
//                * variable scaled preview
//                * transformations mirror/transpose/rotate
//                * multipage navigation
//                * save the shown page to any GBM supported bitmap file format
//                * uses GBM File Dialog if found, otherwise OS/2 File Dialog is used
//                * English and German localized with autodetection via LANG variable
//
//                Add EMBED mode viewer with reduced functionality of FULL mode viewer
//                * popup menu
//                * transformations mirror
//                * multipage navigation
//                * save the shown page to any GBM supported bitmap file format
//                * uses GBM File Dialog if found, otherwise OS/2 File Dialog is used
//                * printing of embedded bitmaps is supported on Mozilla/Firefox/SeaMonkey
//                * English and German localized with autodetection via LANG variable
//
//   02-Aug-2006: Add bitmap information dialog box
//
//   08-Sep-2006  1bpp bitmaps were shown inverted (colors swapped)
//
//   21-Sep-2006  * Add help support for the GBM file dialog
//                * Allow all dialogs to be moved outside the plugin window
//                * Improved detection of bitmap file formats
//
//   09-Sep-2007  * 8bpp grayscale and true color images are now scaled with a high
//                  high quality algorithm to improve appearance.
//
//   27-Jan-2008  * Resampling scaler now supports all grayscale bitmaps <=8bpp
//
//   02-Feb-2008  * Move HPS rendering code to common package GbmRenderer
//                  to save memory (now renders directly from cache buffer)
//                  without temporary subrectangle buffer
//                * Add support for configurable scaling filter
//
//   21-Sep-2009  * Add parent position/resize user message posted by NPP_SetWindow
//                  (GBM_BITMAP_VIEWER_WM_POSSIZE_PARENT) to workaround some drawing
//                  issues
//
//   22-Apr-2010  * Fix huge memory consumption when doing background rendering
//                  of multiple pages that have a big size difference.
//
//   18-Aug-2010  * Some restructuring
//                  - separate File Dialog handling
//                  - fix popup menu activation (now system conform)
//                  - fix sometimes not reacting pages switch accelerators
//                * Add support for multithreaded rendering
//
//   10-Oct-2010  * Add preferences dialog box
//
//    1-Nov-2010  Add resampling scaler filters:
//                * blackman, catmullrom, quadratic, gaussian, kaiser
//********************************************************************************

#ifdef __IBMCPP__
  #pragma strings( readonly )
#endif

#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define INCL_WIN
#include "os2defs.h"
#include "GbmBitmapViewer_rc.h"
#include "GbmSaveFileDialog_rc.h"
#include "GbmDocumentPage.hpp"
#include "GbmException.hpp"
#include "GbmBitmapViewer.hpp"
#include "GbmSaveFileDialog.hpp"

// ---------------------------------------------------------

// Keep in sync with Resource file npgbm.rc
static const int GBM_PLUGIN_VERSION = 147;  // 1.47

// Window title for the viewer and the help window
static PCSZ GBMBITMAPVIEWER_Title = (PCSZ)"GBM Bitmap Viewer";
static PCSZ WC_GBMVIEWER          = (PCSZ)"GBMBitmapViewer";

static const int SCROLL_PAGE_INCREMENT_STEPS = 150;

// Keep in sync with GbmBitmapViewer_rc.h and
// GbmBitmapViewer.rc resource IDs !!!

// Index of language specific resource ID into lookup table.
#define RES_RID_GBMVIEWER                             0
#define RES_RID_GBMVIEWER_SMALL_POPUP                 1
#define RES_RID_GBMVIEWER_FULL_POPUP                  2
#define RES_RID_GBMVIEWER_FORMATINFO                  3
#define RES_RID_GBMVIEWER_PREFS                       4
#define RES_RID_GBM_FILEDLG                           5
#define RES_RID_GBMVIEWER_GBMDLG_HELPTABLE            6
#define RES_SID_GBMVIEWER_UNSUPPORTED_FMT             7
#define RES_SID_GBMVIEWER_SAVEAS                      8
#define RES_SID_GBMVIEWER_OVERRIDE_FILE               9
#define RES_SID_GBMVIEWER_STRING_ALL_FILES           10
#define RES_SID_GBMVIEWER_STRING_FMT_EXT_ERROR       11
#define RES_SID_GBMVIEWER_STRING_SCALE_SIMPLE        12
#define RES_SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR  13

const struct GbmBitmapViewer::LANG_LOOKUP_TABLE_DEF GbmBitmapViewer::LANG_LOOKUP_TABLE [] =
{
    { "_EN", { RID_GBMVIEWER_EN,
               RID_GBMVIEWER_SMALL_POPUP_EN,
               RID_GBMVIEWER_FULL_POPUP_EN,
               RID_GBMVIEWER_FORMATINFO_EN,
               RID_GBMVIEWER_PREFS_EN,
               RID_GBM_FILEDLG_EN,
               RID_GBMVIEWER_GBMDLG_HELPTABLE_EN,
               SID_GBMVIEWER_STRING_UNSUPPORTED_FMT_EN,
               SID_GBMVIEWER_STRING_SAVEAS_EN,
               SID_GBMVIEWER_STRING_OVERRIDE_FILE_EN,
               SID_GBMVIEWER_STRING_ALL_FILES_EN,
               SID_GBMVIEWER_STRING_FMT_EXT_ERROR_EN,
               SID_GBMVIEWER_STRING_SCALE_SIMPLE_EN,
               SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR_EN
             }
    },
    { "_DE", { RID_GBMVIEWER_DE,
               RID_GBMVIEWER_SMALL_POPUP_DE,
               RID_GBMVIEWER_FULL_POPUP_DE,
               RID_GBMVIEWER_FORMATINFO_DE,
               RID_GBMVIEWER_PREFS_DE,
               RID_GBM_FILEDLG_DE,
               RID_GBMVIEWER_GBMDLG_HELPTABLE_DE,
               SID_GBMVIEWER_STRING_UNSUPPORTED_FMT_DE,
               SID_GBMVIEWER_STRING_SAVEAS_DE,
               SID_GBMVIEWER_STRING_OVERRIDE_FILE_DE,
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
    const char *            configName;
    const char *            displayName;
    GbmRenderer::ScalerType type;
} SCALER;

static const SCALER SCALER_TABLE[] =
{
    { "simple"         , "Simple"          , GbmRenderer::ScalerType_SIMPLE          },
    { "nearestneighbor", "Nearest Neighbor", GbmRenderer::ScalerType_NEARESTNEIGHBOR },
    { "bilinear"       , "Bilinear"        , GbmRenderer::ScalerType_BILINEAR        },
    { "bell"           , "Bell"            , GbmRenderer::ScalerType_BELL            },
    { "bspline"        , "BSpline"         , GbmRenderer::ScalerType_BSPLINE         },
    { "quadratic"      , "Quadratic"       , GbmRenderer::ScalerType_QUADRATIC       },
    { "gaussian"       , "Gaussian"        , GbmRenderer::ScalerType_GAUSSIAN        },
    { "mitchell"       , "Mitchell"        , GbmRenderer::ScalerType_MITCHELL        },
    { "catmullrom"     , "Catmull/Rom"     , GbmRenderer::ScalerType_CATMULLROM      },
    { "lanczos"        , "Lanczos"         , GbmRenderer::ScalerType_LANCZOS         },
    { "kaiser"         , "Kaiser"          , GbmRenderer::ScalerType_KAISER          },
    { "blackman"       , "Blackman"        , GbmRenderer::ScalerType_BLACKMAN        }
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
static MRESULT APIENTRY GlobalWindowProcedure(HWND   hwnd,
                                              ULONG  msg,
                                              MPARAM mp1,
                                              MPARAM mp2)
{
    GbmBitmapViewer * pViewer = (GbmBitmapViewer *) WinQueryWindowULong(hwnd, QWL_USER);
    if (pViewer != NULL)
    {
        return pViewer->instanceWindowProcedure(hwnd, msg, mp1, mp2);
    }
    return (MRESULT) FALSE;
}

//
// Here is the window procedure for the bitmap info dialog box.
//
static MRESULT APIENTRY GlobalInfoDlgWindowProcedure(HWND   hwnd,
                                                     ULONG  msg,
                                                     MPARAM mp1,
                                                     MPARAM mp2)
{
    switch(msg)
    {
        case WM_INITDLG:
        {
            char numberBuffer[20] = { 0 };

            GbmBitmapViewer * pViewer = (GbmBitmapViewer *) PVOIDFROMMP(mp2);
            if (pViewer == NULL)
            {
                return (MRESULT) FALSE;
            }

            // set the info into the text fields
            WinSetDlgItemText(hwnd, TID_FORMATINFO_FORMAT,
                              (PCSZ)pViewer->getLongDescription());

            if (pViewer->hasBitmap())
            {
                int pageWidth = 0, pageHeight = 0, pageColorDepth = 0;
                pViewer->getPageSize(pageWidth, pageHeight, pageColorDepth);

                sprintf(numberBuffer, "%d", pageWidth);
                WinSetDlgItemText(hwnd, TID_FORMATINFO_WIDTH, (PCSZ)numberBuffer);

                sprintf(numberBuffer, "%d", pageHeight);
                WinSetDlgItemText(hwnd, TID_FORMATINFO_HEIGHT, (PCSZ)numberBuffer);

                sprintf(numberBuffer, "%d", pageColorDepth);
                WinSetDlgItemText(hwnd, TID_FORMATINFO_BPP, (PCSZ)numberBuffer);

                sprintf(numberBuffer, "%d", pViewer->getNumberOfPages());
                WinSetDlgItemText(hwnd, TID_FORMATINFO_PAGES, (PCSZ)numberBuffer);
            }
            else
            {
                static PCSZ valueNotAvailable = (PCSZ)"-";
                WinSetDlgItemText(hwnd, TID_FORMATINFO_WIDTH , valueNotAvailable);
                WinSetDlgItemText(hwnd, TID_FORMATINFO_HEIGHT, valueNotAvailable);
                WinSetDlgItemText(hwnd, TID_FORMATINFO_BPP   , valueNotAvailable);
                WinSetDlgItemText(hwnd, TID_FORMATINFO_PAGES , valueNotAvailable);
            }

            // center the dialog window
            {
                SWP   swpDlg    = { 0 };
                RECTL rclParent = { 0 };

                WinQueryWindowRect(pViewer->getFrameHwnd(), &rclParent);
                WinQueryWindowPos (hwnd, &swpDlg);

                // set the horizontal coordinate of the lower-left corner
                const LONG x = ((LONG) rclParent.xRight - swpDlg.cx) / 2 + swpDlg.x;

                // set vertical coordinate of the lower-left corner
                const LONG y = ((LONG) rclParent.yTop - swpDlg.cy) / 2 + swpDlg.y;

                // move, size, and show the window
                WinSetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_MOVE);
            }
        }
        return (MRESULT) FALSE;

        case WM_COMMAND:
           switch (SHORT1FROMMP(mp1))
           {
              case DID_OK:
                  WinDismissDlg(hwnd, DID_OK);
                  return (MRESULT) 0L;

              default:
                  break;
           }
           break;

        // -------------------------

        default:
            break;
    }

    return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

//
// Here is the window procedure for the preferences dialog box.
//
static MRESULT APIENTRY GlobalPrefsDlgWindowProcedure(HWND   hwnd,
                                                      ULONG  msg,
                                                      MPARAM mp1,
                                                      MPARAM mp2)
{
    switch(msg)
    {
        case WM_INITDLG:
        {
            GbmBitmapViewer * pViewer = (GbmBitmapViewer *) PVOIDFROMMP(mp2);
            if (pViewer == NULL)
            {
                return (MRESULT) FALSE;
            }
            WinSetWindowULong(hwnd, QWL_USER, (ULONG) pViewer);

            // init the dialog fields

            // Fill the combobox
            int i;
            const HWND algoHwnd = WinWindowFromID(hwnd, TID_PREFERENCES_ALGO);
            for (i = 0; i < SCALER_TABLE_LENGTH; ++i)
            {
                switch(i)
                {
                    case 0:
                        WinInsertLboxItem(algoHwnd, LIT_END, pViewer->getScalerName_Simple());
                        break;

                    case 1:
                        WinInsertLboxItem(algoHwnd, LIT_END, pViewer->getScalerName_NearestNeighbor());
                        break;

                    default:
                        WinInsertLboxItem(algoHwnd, LIT_END, SCALER_TABLE[i].displayName);
                        break;
                }
            }
            // Select the current scale type
            const GbmRenderer::ScalerType currScalerType = pViewer->getScalerType();
            for (i = 0; i < SCALER_TABLE_LENGTH; ++i)
            {
                if (SCALER_TABLE[i].type == currScalerType)
                {
                    if (! WinSendMsg(algoHwnd, LM_SELECTITEM,
                                     MPFROMSHORT(i), MPFROMSHORT(TRUE)))
                    {
                        WinSendMsg(algoHwnd, LM_SELECTITEM,
                                   MPFROMSHORT(0), MPFROMSHORT(TRUE));
                    }
                    break;
                }
            }

            // Set the currently configured number of background render pages
            const HWND pagesHwndUpDown = WinWindowFromID(hwnd, TID_PREFERENCES_PAGESUD);
            WinSendMsg(pagesHwndUpDown, SPBM_SETLIMITS      , MPFROMLONG(999), MPFROMLONG(0));
            WinSendMsg(pagesHwndUpDown, SPBM_SETCURRENTVALUE, MPFROMLONG(pViewer->getNumberOfBackgroundRenderPages()), MPFROMLONG(0));

            // center the dialog window
            {
                SWP   swpDlg    = { 0 };
                RECTL rclParent = { 0 };

                WinQueryWindowRect(pViewer->getFrameHwnd(), &rclParent);
                WinQueryWindowPos (hwnd, &swpDlg);

                // set the horizontal coordinate of the lower-left corner
                const LONG x = ((LONG) rclParent.xRight - swpDlg.cx) / 2 + swpDlg.x;

                // set vertical coordinate of the lower-left corner
                const LONG y = ((LONG) rclParent.yTop - swpDlg.cy) / 2 + swpDlg.y;

                // move, size, and show the window
                WinSetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_MOVE);
            }
        }
        return (MRESULT) FALSE;

        case WM_COMMAND:
           switch (SHORT1FROMMP(mp1))
           {
              case DID_OK:
              {
                  // readout the configured settings and program viewer
                  GbmBitmapViewer * pViewer = (GbmBitmapViewer *) WinQueryWindowULong(hwnd, QWL_USER);
                  if (pViewer != NULL)
                  {
                      const HWND pagesHwndUpDown = WinWindowFromID(hwnd, TID_PREFERENCES_PAGESUD);
                      LONG numPages = 0;
                      if (WinSendMsg(pagesHwndUpDown, SPBM_QUERYVALUE,
                                     MPFROMP(&numPages), MPFROM2SHORT(0, SPBQ_UPDATEIFVALID)))
                      {
                          pViewer->setNumberOfBackgroundRenderPages(numPages);
                      }

                      const HWND algoHwnd = WinWindowFromID(hwnd, TID_PREFERENCES_ALGO);
                      const int currLboxItemId = WinQueryLboxSelectedItem(algoHwnd);
                      if (currLboxItemId != LIT_NONE)
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
                                      static const char * msg = "Could not save preferences to file:\n";

                                      // format the error string: gbm_error:\nuser_message
                                      char * pMessage = new char[strlen(msg) + strlen(filename) + 1];
                                      sprintf(pMessage, "%s%s", msg, filename);

                                      WinMessageBox(HWND_DESKTOP,
                                                    hwnd,
                                                    (PCSZ)pMessage,
                                                    (PCSZ)GBMBITMAPVIEWER_Title,
                                                    0, MB_OK | MB_ERROR | MB_MOVEABLE);
                                      delete [] pMessage;
                                  }
                                  else
                                  {
                                      WinMessageBox(HWND_DESKTOP,
                                                    hwnd,
                                                    (PCSZ)"Could not save preferences to file.",
                                                    (PCSZ)GBMBITMAPVIEWER_Title,
                                                    0, MB_OK | MB_ERROR | MB_MOVEABLE);
                                  }
                              }
                          }
                      }
                  }
                  WinDismissDlg(hwnd, DID_OK);
                  return (MRESULT) 0L;
              }

              case DID_CANCEL:
                  WinDismissDlg(hwnd, DID_CANCEL);
                  return (MRESULT) 0L;

              default:
                  break;
           }
           break;

        // -------------------------

        default:
            break;
    }

    return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static VOID EnableMenuItem(HWND hWndMenu, SHORT idMenuItem, BOOL fEnabled)
{
    WinSendMsg(hWndMenu, MM_SETITEMATTR,
           MPFROM2SHORT(idMenuItem, TRUE),
           MPFROM2SHORT(MIA_DISABLED, ( fEnabled ) ? 0 : MIA_DISABLED));
}

static VOID CheckMenuItem(HWND hWndMenu, SHORT idMenuItem, BOOL fChecked)
{
    WinSendMsg(hWndMenu, MM_SETITEMATTR,
           MPFROM2SHORT(idMenuItem, TRUE),
           MPFROM2SHORT(MIA_CHECKED, ( fChecked ) ? MIA_CHECKED : 0));
}

//----------------------------------------------------------------------------

static void SetScrollBar(HWND hwndScroll, SHORT pos, SHORT min, SHORT max)
{
     /* Set the scroll-bar value and range. */
     WinSendMsg(hwndScroll, SBM_SETSCROLLBAR,
                MPFROM2SHORT(pos, 0),
                MPFROM2SHORT(min, max));
}

static void SetScrollBarPos(HWND hwndScroll, SHORT pos)
{
     /* Set the vertical scroll-bar value. */
     WinSendMsg(hwndScroll, SBM_SETPOS, MPFROMSHORT(pos), (MPARAM) NULL);
}

static void SetScrollThumbSize(HWND hwndScroll, SHORT visible, SHORT total)
{
     /* Set the vertical scroll-bar value. */
     WinSendMsg(hwndScroll, SBM_SETTHUMBSIZE, MPFROM2SHORT(visible, total), (MPARAM) NULL);
}

// ---------------------------------------------------------

static BOOL CheckFileExistsAndQueryOverride(HWND hwnd, const CHAR * filename, const CHAR * message)
{
  struct stat buf;

  if (stat((const char *)filename, &buf) != -1)
  {
    if (MBID_YES != WinMessageBox(HWND_DESKTOP,
                                 hwnd,
                                 (PCSZ)message,
                                 (PCSZ)GBMBITMAPVIEWER_Title,
                                 0, MB_YESNO | MB_DEFBUTTON2 | MB_QUERY | MB_MOVEABLE))
    {
      return FALSE;
    }
  }
  return TRUE;
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
      mHwndFrame(0),
      mHwndClient(0),
      mHwndHScroll(0),
      mHwndVScroll(0),
      mHwndMenu(0),
      mHwndPopupMenu(0),
      mHwndHelp(0),
      mHScrollMax(0),
      mHScrollPos(0),
      mVScrollMax(0),
      mVScrollPos(0),
      mCxClient(0),
      mCyClient(0)
{
    // check system language and lookup the matching resource set
    const char * systemLanguage = getenv("LANG");
    if (systemLanguage)
    {
        const int systemLanguageLen = strlen(systemLanguage);

        char * systemLanguageUpperCase = NULL;
        try
        {
            systemLanguageUpperCase = new char[systemLanguageLen + 1];
        }
        catch(...)
        {
            systemLanguageUpperCase = NULL;
        }
        if (systemLanguageUpperCase)
        {
            memset(systemLanguageUpperCase, 0, systemLanguageLen + 1);
            for (int c = 0; c < systemLanguageLen; c++)
            {
                systemLanguageUpperCase[c] = toupper(systemLanguage[c]);
            }
            for (int i = 0; i < LANG_LOOKUP_TABLE_LENGTH; i++)
            {
                if (strstr(systemLanguageUpperCase, LANG_LOOKUP_TABLE[i].language) != NULL)
                {
                    mpLanguage = &LANG_LOOKUP_TABLE[i];
                    break;
                }
            }
            delete [] systemLanguageUpperCase;
            systemLanguageUpperCase = NULL;
        }
    }
    // if the system language was not found, keep English as default

    // init string buffers
    memset(mSaveAsFilename          , 0, sizeof(mSaveAsFilename));
    memset(mSaveAsOptions           , 0, sizeof(mSaveAsOptions));
    memset(mStringUnsupportedFmt    , 0, sizeof(mStringUnsupportedFmt));
    memset(mStringSaveAs            , 0, sizeof(mStringSaveAs));
    memset(mStringOverrideFile      , 0, sizeof(mStringOverrideFile));
    memset(mStringAllFiles          , 0, sizeof(mStringAllFiles));
    memset(mStringFmtExtError       , 0, sizeof(mStringFmtExtError));
    memset(mStringScalerSimple      , 0, sizeof(mStringScalerSimple));
    memset(mStringScalerNearNeighbor, 0, sizeof(mStringScalerNearNeighbor));

    memset(mHelpFilename    , 0, sizeof(mHelpFilename));
    memset(mHelpFilenameBase, 0, sizeof(mHelpFilenameBase));

    // make and store the help file name
    char fullName[_MAX_PATH+1] = { 0 };
    if (DosQueryModuleName(mDllModule, sizeof(fullName)-1, fullName) == NO_ERROR)
    {
        char drive[_MAX_DRIVE+1] = { 0 };
        char dir  [_MAX_DIR+1]   = { 0 };
        char fname[_MAX_FNAME+1] = { 0 };

        _splitpath(fullName, drive, dir, fname, NULL);

        if (strlen(fname) + strlen(mpLanguage->language) < sizeof(fname))
        {
            strcat(fname, mpLanguage->language+1);
            _makepath (fullName, drive, dir, fname, "HLP");

            if (strlen(fullName) < sizeof(mHelpFilename))
            {
                strcpy(mHelpFilename, fullName);

                _makepath(fullName, NULL, NULL, fname, "HLP");
                strcpy(mHelpFilenameBase, fullName);
            }
        }
    }

    // configure the viewer
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

void GbmBitmapViewer::setParentWindow(HWND hwnd, BOOL fullMode)
{
    // stop paint thread
    mPaintThread.stop();
    MutexGuard interfaceGuard(mInterfaceMutex);

    destroyChildWindows();
    mIsFullMode = FALSE;

    if (hwnd == 0)
    {
        return;
    }

    const HAB hab = WinQueryAnchorBlock(hwnd);

    // init string buffers
    memset(mSaveAsFilename          , 0, sizeof(mSaveAsFilename));
    memset(mSaveAsOptions           , 0, sizeof(mSaveAsOptions));
    memset(mStringUnsupportedFmt    , 0, sizeof(mStringUnsupportedFmt));
    memset(mStringSaveAs            , 0, sizeof(mStringSaveAs));
    memset(mStringOverrideFile      , 0, sizeof(mStringOverrideFile));
    memset(mStringAllFiles          , 0, sizeof(mStringAllFiles));
    memset(mStringFmtExtError       , 0, sizeof(mStringFmtExtError));
    memset(mStringScalerSimple      , 0, sizeof(mStringScalerSimple));
    memset(mStringScalerNearNeighbor, 0, sizeof(mStringScalerNearNeighbor));

    // load language specific strings
    WinLoadString(hab, mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_UNSUPPORTED_FMT],
                  sizeof(mStringUnsupportedFmt), (PSZ)mStringUnsupportedFmt);

    WinLoadString(hab, mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_SAVEAS],
                  sizeof(mStringSaveAs), (PSZ)mStringSaveAs);

    WinLoadString(hab, mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_OVERRIDE_FILE],
                  sizeof(mStringOverrideFile), (PSZ)mStringOverrideFile);

    WinLoadString(hab, mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_STRING_ALL_FILES],
                  sizeof(mStringAllFiles), (PSZ)mStringAllFiles);

    WinLoadString(hab, mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_STRING_FMT_EXT_ERROR],
                  sizeof(mStringFmtExtError), (PSZ)mStringFmtExtError);

    WinLoadString(hab, mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_STRING_SCALE_SIMPLE],
                  sizeof(mStringScalerSimple), (PSZ)mStringScalerSimple);

    WinLoadString(hab, mDllModule, mpLanguage->resid[RES_SID_GBMVIEWER_STRING_SCALE_NEARNEIGHBOR],
                  sizeof(mStringScalerNearNeighbor), (PSZ)mStringScalerNearNeighbor);

    // configure the viewer either for FULL or EMBEDDED mode
    if (fullMode)
    {
        mCurrentPopupMenuId = RES_RID_GBMVIEWER_FULL_POPUP;
        mZoomMode = GbmBitmapViewer::ZoomMode_NONE;
    }
    else
    {
        mCurrentPopupMenuId = RES_RID_GBMVIEWER_SMALL_POPUP;
        mZoomMode = GbmBitmapViewer::ZoomMode_STRETCH;
    }

    mIsFullMode = fullMode;

    // build popup menu and child windows
    createChildWindows(hwnd);

    // register the instance
    WinSetWindowULong(mHwndClient, QWL_USER, (ULONG) this);

    // set initial scrollbar settings
    updateScrollBars(mHwndClient);
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
        WinInvalidateRect(mHwndClient, 0, FALSE);
        WinUpdateWindow(mHwndClient);
    }
}

// ---------------------------------------------------------

void GbmBitmapViewer::setScalerType(const GbmRenderer::ScalerType scalerType)
{
    // stop paint thread before locking interface
    mPaintThread.stop();
    MutexGuard interfaceGuard(mInterfaceMutex);

    mScalerType = scalerType;
    if (mHwndClient)
    {
        WinInvalidateRect(mHwndClient, 0, TRUE);
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
        WinInvalidateRect(mHwndClient, 0, TRUE);
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

void GbmBitmapViewer::draw(HPS hps, const RECTL & rect, double pageScaleX, double pageScaleY)
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    MutexGuard documentGuard(mGbmDocument.getInterfaceMutex());
    if (! hasBitmap())
    {
        POINTL startPoint = { rect.xLeft , rect.yBottom };
        POINTL endPoint   = { rect.xRight, rect.yTop };

        if ((mLoadProgress    > 0) &&
            (mLoadProgressMax > 0) &&
            (mLoadProgress    <= mLoadProgressMax))
        {
            endPoint.y = (LONG)((float)(rect.yTop) * mLoadProgress / mLoadProgressMax);
            GpiSetColor(hps, CLR_DARKGRAY);
            GpiMove    (hps, &startPoint);
            GpiBox     (hps, DRO_OUTLINEFILL, &endPoint, 0L, 0L);

            startPoint.y = endPoint.y;
            endPoint.y   = rect.yTop;

            GpiSetColor(hps, CLR_PALEGRAY);
            GpiMove    (hps, &startPoint);
            GpiBox     (hps, DRO_OUTLINEFILL, &endPoint, 0L, 0L);
        }
        else
        {
            GpiSetColor(hps, CLR_PALEGRAY);
            GpiMove    (hps, &startPoint);
            GpiBox     (hps, DRO_OUTLINEFILL, &endPoint, 0L, 0L);
        }
        return;
    }


    // get the current mouse pointer before changing it to hourglas
    const BOOL isCurrentPageDeferred = ! mGbmDocument.isPageLoaded(mCurrentPageIndex);

    // get the current mouse pointer before changing it to hourglas
    HPOINTER hptrOld = 0;
    if (isCurrentPageDeferred)
    {
        hptrOld = WinQueryPointer(HWND_DESKTOP);
    }
    try
    {
        if (isCurrentPageDeferred)
        {
            /* Get the wait mouse pointer. */
            HPOINTER hptrWait = WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE);

            /* Set the mouse pointer to the wait pointer. */
            WinSetPointer(HWND_DESKTOP, hptrWait);
        }

        // we show so far always page 0
        GbmRenderer & renderer(mGbmDocument.getRenderer(mCurrentPageIndex,
                                                        pageScaleX, pageScaleY,
                                                        mScalerType));

        // check if render rect is larger than the actual bitmap data
        int pageWidthI = 0, pageHeightI = 0, pageColorDepthI = 0;
        getPageSize(pageWidthI, pageHeightI, pageColorDepthI);

        const int pageWidth  = (int)(pageScaleX * (double)pageWidthI);
        const int pageHeight = (int)(pageScaleY * (double)pageHeightI);

        LONG yOffset = rect.yTop - rect.yBottom - pageHeight;
        if (yOffset < 0)
        {
            yOffset = 0;
        }

        RECTL renderRectl = rect;

        if (renderRectl.xRight - 1 - renderRectl.xLeft > pageWidth)
        {
            renderRectl.xRight = pageWidth + renderRectl.xLeft;

            // draw only the visible area not filled by the bitmap right beside
            POINTL unusedAreaStartPoint = { renderRectl.xRight, renderRectl.yBottom };
            POINTL unusedAreaEndPoint   = { rect.xRight       , renderRectl.yTop };

            GpiSetColor(hps, CLR_DARKGRAY);
            GpiMove    (hps, &unusedAreaStartPoint);
            GpiBox     (hps, DRO_OUTLINEFILL, &unusedAreaEndPoint, 0L, 0L);
        }
        if (renderRectl.yTop - 1 - renderRectl.yBottom > pageHeight)
        {
            renderRectl.yTop = pageHeight + renderRectl.yBottom;

            // draw only the visible area not filled by the bitmap right beside
            POINTL unusedAreaStartPoint = { rect.xLeft , renderRectl.yBottom + yOffset - 1 };
            POINTL unusedAreaEndPoint   = { rect.xRight, rect.yBottom };

            GpiSetColor(hps, CLR_DARKGRAY);
            GpiMove    (hps, &unusedAreaStartPoint);
            GpiBox     (hps, DRO_OUTLINEFILL, &unusedAreaEndPoint, 0L, 0L);
        }

        // calculate the source rectangle to be extracted from the bitmap
        const int src_x      = mHScrollPos;
        const int src_y      = mVScrollPos;
        const int src_width  = (int) (renderRectl.xRight - renderRectl.xLeft);
        const int src_height = (int) (renderRectl.yTop   - renderRectl.yBottom);

        // correct render area and move the bitmap to be shown at the top rather than the bottom
        renderRectl.yBottom += yOffset;
        renderRectl.yTop    += yOffset - 1;
        renderRectl.xRight  -= 1;

        renderer.renderToHPS(     src_x,      src_y,
                              src_width, src_height,
                             pageScaleX, pageScaleY, renderRectl, hps);
    }
    catch(...)
    {
        POINTL startPoint = { rect.xLeft , rect.yBottom };
        POINTL endPoint   = { rect.xRight, rect.yTop };

        GpiSetColor(hps, CLR_PALEGRAY);
        GpiMove    (hps, &startPoint);
        GpiBox     (hps, DRO_OUTLINEFILL, &endPoint, 0L, 0L);
    }
    if (isCurrentPageDeferred)
    {
        // restore old mouse pointer
        WinSetPointer(HWND_DESKTOP, hptrOld);
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

const char * GbmBitmapViewer::getLongDescription() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    MutexGuard documentGuard(mGbmDocument.getInterfaceMutex());
    if (hasBitmap())
    {
        return mGbmDocument.getLongDescription();
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
    HPOINTER hptrOld = WinQueryPointer(HWND_DESKTOP);

    try
    {
        /* Get the wait mouse pointer. */
        HPOINTER hptrWait = WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE);

        /* Set the mouse pointer to the wait pointer. */
        WinSetPointer(HWND_DESKTOP, hptrWait);

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
    WinSetPointer(HWND_DESKTOP, hptrOld);

    WinInvalidateRect(hwnd, 0, TRUE);
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
            if (showFileDialog_SaveAs(hwnd,
                                      mSaveAsFilename, sizeof(mSaveAsFilename),
                                      mSaveAsOptions , sizeof(mSaveAsOptions)))
            {
                if (CheckFileExistsAndQueryOverride(hwnd, mSaveAsFilename,
                                                          mStringOverrideFile))
                {
                    // get the current mouse pointer before changing it to hourglas
                    HPOINTER hptrOld = WinQueryPointer(HWND_DESKTOP);

                    /* Get the wait mouse pointer. */
                    HPOINTER hptrWait = WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE);

                    /* Set the mouse pointer to the wait pointer. */
                    WinSetPointer(HWND_DESKTOP, hptrWait);

                    // Write file
                    try
                    {
                        int ft;
                        if (GBM_ERR_OK == mGbmDocument.getGbmAccessor().Gbm_guess_filetype(
                                              mSaveAsFilename, &ft))
                        {
                            mGbmDocument.writeDocumentPageToFile((const char *)mSaveAsFilename,
                                                                 (const char *)mSaveAsOptions,
                                                                 mCurrentPageIndex);
                        }
                        else
                        {
                            WinMessageBox(HWND_DESKTOP,
                                          hwnd,
                                          (PCSZ)mStringFmtExtError,
                                          (PCSZ)GBMBITMAPVIEWER_Title,
                                          0, MB_OK | MB_ERROR | MB_MOVEABLE);
                        }
                    }
                    catch(GbmException & ex)
                    {
                        // restore old mouse pointer
                        WinSetPointer(HWND_DESKTOP, hptrOld);

                        const GbmAccessor & gbmAccessor(mGbmDocument.getGbmAccessor());

                        // extract error message
                        const char * msg = ex.getErrorMessage();
                        if (strlen(msg) == 0)
                        {
                            const char * gbmErrorMsg = gbmAccessor.Gbm_err(ex.getErrorId());

                            WinMessageBox(HWND_DESKTOP,
                                          hwnd,
                                          (PCSZ)gbmErrorMsg,
                                          (PCSZ)GBMBITMAPVIEWER_Title,
                                          0, MB_OK | MB_ERROR | MB_MOVEABLE);
                        }
                        else
                        {
                            const char * gbmErrorMsg = gbmAccessor.Gbm_err(ex.getErrorId());

                            // format the error string: gbm_error:\nuser_message
                            char * pMessage = new char[strlen(gbmErrorMsg) + strlen(msg) + 3 + 1];
                            sprintf(pMessage, "%s:\n%s", gbmErrorMsg, msg);

                            WinMessageBox(HWND_DESKTOP,
                                          hwnd,
                                          (PCSZ)pMessage,
                                          (PCSZ)GBMBITMAPVIEWER_Title,
                                          0, MB_OK | MB_ERROR | MB_MOVEABLE);
                            delete [] pMessage;
                        }
                    }
                    // restore old mouse pointer
                    WinSetPointer(HWND_DESKTOP, hptrOld);
                }
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
            WinDlgBox(HWND_DESKTOP,
                      hwnd,
                      GlobalPrefsDlgWindowProcedure,
                      mDllModule,
                      mpLanguage->resid[RES_RID_GBMVIEWER_PREFS],
                      this);
            return TRUE;

        // -------------------------
        // Help menu
        // -------------------------

        case MID_HELP_ABOUT:
        {
            const char * info = "GBM plugin (version %d.%d), using GBM.DLL %d.%d\n(c) 2006-2012, Heiko Nitzsche";
            const int infoLen = strlen(info);

            char * pBuf = new char[infoLen + 10 + 1];
            if (pBuf != NULL)
            {
                const GbmAccessor & gbmAccessor(mGbmDocument.getGbmAccessor());
                sprintf(pBuf, info, GBM_PLUGIN_VERSION / 100, GBM_PLUGIN_VERSION % 100,
                                    gbmAccessor.Gbm_version() / 100, gbmAccessor.Gbm_version() % 100);
                WinMessageBox(HWND_DESKTOP,
                              hwnd,
                              (PCSZ)pBuf,
                              (PCSZ)GBMBITMAPVIEWER_Title,
                              0, MB_OK | MB_INFORMATION | MB_MOVEABLE);
                delete [] pBuf;
                pBuf = NULL;
            }
            return TRUE;
        }

        case MID_HELP_IMAGEINFO:
            WinDlgBox(HWND_DESKTOP,
                      hwnd,
                      GlobalInfoDlgWindowProcedure,
                      mDllModule,
                      mpLanguage->resid[RES_RID_GBMVIEWER_FORMATINFO],
                      this);
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
            WinInvalidateRect(hwnd, 0, TRUE);
        }
    }
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::updateScaleFactor(HWND hwnd, BOOL forceWindowRepaint, BOOL skipWindowRepaint)
{
    const double oldScaleX = mScaleX;
    const double oldScaleY = mScaleY;

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
        WinInvalidateRect(hwnd, 0, TRUE);
    }
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::createChildWindows(HWND hwnd)
{
    if (mIsFullMode)
    {
        // create the embedded frame window with menu
        ULONG flFrameFlags = FCF_MENU       | FCF_ACCELTABLE |
                             FCF_VERTSCROLL | FCF_HORZSCROLL |
                             FCF_NOBYTEALIGN;

        WinRegisterClass(WinQueryAnchorBlock(hwnd),
                         WC_GBMVIEWER,
                         (PFNWP) GlobalWindowProcedure,
                         0L,
                         4L);

        mHwndFrame = WinCreateStdWindow(
            hwnd,                                 /* Parent window handle              */
            WS_VISIBLE,                           /* Style of frame window             */
            &flFrameFlags,                        /* Pointer to control data           */
            WC_GBMVIEWER,                         /* Client window class name          */
            GBMBITMAPVIEWER_Title,                /* Title bar text                    */
            0L,                                   /* Style of client window            */
            mDllModule,                           /* Module handle for resources       */
            mpLanguage->resid[RES_RID_GBMVIEWER], /* ID of resources                   */
            &mHwndClient);                        /* Pointer to client window handle   */

        WinSetWindowULong(mHwndClient, QWL_USER, (ULONG) this);

        mHwndHScroll   = WinWindowFromID(mHwndFrame, FID_HORZSCROLL);
        mHwndVScroll   = WinWindowFromID(mHwndFrame, FID_VERTSCROLL);
        mHwndMenu      = WinWindowFromID(mHwndFrame, FID_MENU);
        mHwndPopupMenu = WinLoadMenu(mHwndClient, mDllModule, mpLanguage->resid[mCurrentPopupMenuId]);
    }
    else
    {
        // create the embedded frame window with menu
        ULONG flFrameFlags = FCF_NOBYTEALIGN | FCF_ACCELTABLE;

        WinRegisterClass(WinQueryAnchorBlock(hwnd),
                         WC_GBMVIEWER,
                         (PFNWP) GlobalWindowProcedure,
                         0L,
                         4L);

        mHwndFrame = WinCreateStdWindow(
            hwnd,                                 /* Parent window handle              */
            WS_VISIBLE,                           /* Style of frame window             */
            &flFrameFlags,                        /* Pointer to control data           */
            WC_GBMVIEWER,                         /* Client window class name          */
            GBMBITMAPVIEWER_Title,                /* Title bar text                    */
            0L,                                   /* Style of client window            */
            mDllModule,                           /* Module handle for resources       */
            mpLanguage->resid[RES_RID_GBMVIEWER], /* ID of resources                   */
            &mHwndClient);                        /* Pointer to client window handle   */

        mHwndHScroll   = 0;
        mHwndVScroll   = 0;
        mHwndPopupMenu = WinLoadMenu(mHwndClient, mDllModule, mpLanguage->resid[mCurrentPopupMenuId]);
        mHwndMenu      = mHwndPopupMenu;
    }

    /*
     * Create help instance for GBM file dialog context help.
     */
    mHwndHelp = NULLHANDLE;
    if (strlen(mHelpFilename) > 0)
    {
        HELPINIT helpinit;
        memset(&helpinit, 0, sizeof(HELPINIT));

        /* Setup the help initialization structure */
        helpinit.cb                       = sizeof(HELPINIT);
        helpinit.ulReturnCode             = 0L;
        helpinit.pszTutorialName          = (PSZ)NULL;

        /* Help table in plugin resource */
        helpinit.phtHelpTable             = (HELPTABLE *) (0xffff0000L |
                                                           mpLanguage->resid[RES_RID_GBMVIEWER_GBMDLG_HELPTABLE]);
        helpinit.hmodHelpTableModule      = mDllModule;

        /* Default action bar and accelerators */
        helpinit.hmodAccelActionBarModule = NULLHANDLE;
        helpinit.idAccelTable             = 0;
        helpinit.idActionBar              = 0;
        helpinit.pszHelpWindowTitle       = (PSZ) GBMBITMAPVIEWER_Title;
        helpinit.fShowPanelId             = CMIC_HIDE_PANEL_ID;

        const HAB hab = WinQueryAnchorBlock(hwnd);

        // check if the help file exists
        struct stat buf;
        if (stat(mHelpFilename, &buf) == 0)
        {
            helpinit.pszHelpLibraryName = (PSZ) mHelpFilename;

            /* Create and associate the help instance */
            mHwndHelp = WinCreateHelpInstance(hab, &helpinit);

            // if creation of help instance fails, we can still continue without help
        }
        else
        {
            // try to search for the help file via HELP variable paths
            UCHAR foundName[CCHMAXPATH+1] = { 0 };

            if (NO_ERROR == DosSearchPath(SEARCH_ENVIRONMENT | SEARCH_IGNORENETERRS,
                                          (PSZ)"HELP",
                                          (PSZ)mHelpFilenameBase,
                                          (PBYTE)foundName,
                                          sizeof(foundName)))
            {
                helpinit.pszHelpLibraryName = (PSZ) mHelpFilenameBase;

                /* Create and associate the help instance */
                mHwndHelp = WinCreateHelpInstance(hab, &helpinit);

                // if creation of help instance fails, we can still continue without help
            }
        }
    }
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::destroyChildWindows()
{
    if (mHwndHelp != NULLHANDLE)
    {
        WinDestroyHelpInstance(mHwndHelp);
        mHwndHelp = NULLHANDLE;
    }
    if (mIsFullMode && (mHwndPopupMenu != 0))
    {
        WinDestroyWindow(mHwndPopupMenu);
    }
    if (mHwndFrame != 0)
    {
        WinDestroyWindow(mHwndFrame);
        mHwndFrame = 0;
    }
    mHwndClient    = 0;
    mHwndHScroll   = 0;
    mHwndVScroll   = 0;
    mHScrollMax    = 0;
    mHScrollPos    = 0;
    mVScrollMax    = 0;
    mVScrollPos    = 0;
    mCxClient      = 0;
    mCyClient      = 0;
    mHwndMenu      = 0;
    mHwndPopupMenu = 0;
    mCurrentPopupMenuId = -1;
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::updateWindowSize(HWND hwnd)
{
    /* get window size */
    RECTL rect = { 0, 0, 0, 0 };
    WinQueryWindowRect(hwnd, &rect);

    mCxClient = rect.xRight - rect.xLeft;
    mCyClient = rect.yTop   - rect.yBottom;
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

                pageWidth  = (int)((double)pageWidth  * mScaleX);
                pageHeight = (int)((double)pageHeight * mScaleY);

                mHScrollMax = MAX(0, pageWidth - mCxClient);
                mHScrollPos = MIN(mHScrollPos, mHScrollMax);

                mVScrollMax = MAX(0, pageHeight - mCyClient);
                mVScrollPos = MIN(mVScrollPos, mVScrollMax);

                SetScrollBar(mHwndHScroll, mHScrollPos, 0, mHScrollMax);
                SetScrollBar(mHwndVScroll, mVScrollPos, 0, mVScrollMax);

                const SHORT visibleH = (pageWidth  > mCxClient) ? mCxClient : pageWidth;
                const SHORT visibleV = (pageHeight > mCyClient) ? mCyClient : pageHeight;

                SetScrollThumbSize(mHwndHScroll, visibleH, pageWidth);
                SetScrollThumbSize(mHwndVScroll, visibleV, pageHeight);

                WinEnableWindow(mHwndHScroll, mHScrollMax ? TRUE : FALSE);
                WinEnableWindow(mHwndVScroll, mVScrollMax ? TRUE : FALSE);
            }
            catch(GbmException &)
            {
            }
        }
        else
        {
            WinEnableWindow(mHwndHScroll, FALSE);
            WinEnableWindow(mHwndVScroll, FALSE);
        }
    }
}

//----------------------------------------------------------------------------

void GbmBitmapViewer::updateMenuItems(HWND hwnd, SHORT itemId) const
{
    const BOOL hasData  = hasBitmap();
    const int  numPages = mGbmDocument.getNumberOfPages();

    // work either on the frame menu or if visible the popup menu (same menu style)
    const HWND refHwndMenu = WinIsWindowVisible(mHwndPopupMenu) ? mHwndPopupMenu : mHwndMenu;

    switch(itemId)
    {
        case MID_FILE:
            EnableMenuItem(refHwndMenu, MID_FILE_SAVE_AS, hasData);
            break;

        // -------------------------

        case MID_BITMAP:
            EnableMenuItem(refHwndMenu, MID_BITMAP_REF_HORZ , hasData);
            EnableMenuItem(refHwndMenu, MID_BITMAP_REF_VERT , hasData);
            EnableMenuItem(refHwndMenu, MID_BITMAP_TRANSPOSE, hasData);
            EnableMenuItem(refHwndMenu, MID_BITMAP_ROT_P90  , hasData);
            EnableMenuItem(refHwndMenu, MID_BITMAP_ROT_M90  , hasData);
            break;

        // -------------------------

        case MID_VIEW:
            // Page submenu
            EnableMenuItem(refHwndMenu, MID_VIEW_PAGE_FIRST, mCurrentPageIndex > 0);
            EnableMenuItem(refHwndMenu, MID_VIEW_PAGE_PREV , mCurrentPageIndex > 0);
            EnableMenuItem(refHwndMenu, MID_VIEW_PAGE_NEXT , mCurrentPageIndex < numPages-1);
            EnableMenuItem(refHwndMenu, MID_VIEW_PAGE_LAST , mCurrentPageIndex < numPages-1);

            // Zoom menu entries
            EnableMenuItem(refHwndMenu, MID_VIEW_ZOOM_DOWN    , hasData);
            EnableMenuItem(refHwndMenu, MID_VIEW_ZOOM_UP      , hasData);
            EnableMenuItem(refHwndMenu, MID_VIEW_ZOOM_NONE    , hasData);
            EnableMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIN  , hasData);
            EnableMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIDTH, hasData);

            switch(mZoomMode)
            {
                case GbmBitmapViewer::ZoomMode_NONE:
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_NONE    , TRUE);
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;

                case GbmBitmapViewer::ZoomMode_FITWIN:
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIN  , TRUE);
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;

                case GbmBitmapViewer::ZoomMode_FITWIDTH:
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIDTH, TRUE);
                    break;

                default: // ZoomMode_FREE
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItem(refHwndMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;
            }
            break;

        // -------------------------

        case MID_HELP:
            // Help submenu
            EnableMenuItem(refHwndMenu, MID_HELP_IMAGEINFO, hasData);
            break;

        // -------------------------

        case RES_RID_GBMVIEWER_SMALL_POPUP:
            EnableMenuItem(mHwndPopupMenu, MID_FILE_SAVE_AS    , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_BITMAP_REF_HORZ , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_BITMAP_REF_VERT , hasData);

            // Page submenu
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE      , numPages > 1);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE_FIRST, mCurrentPageIndex > 0);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE_PREV , mCurrentPageIndex > 0);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE_NEXT , mCurrentPageIndex < numPages-1);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE_LAST , mCurrentPageIndex < numPages-1);

            // Help submenu
            EnableMenuItem(mHwndPopupMenu, MID_HELP_IMAGEINFO, hasData);
            break;

        // -------------------------

        case RES_RID_GBMVIEWER_FULL_POPUP:
            EnableMenuItem(mHwndPopupMenu, MID_FILE_SAVE_AS    , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_BITMAP_REF_HORZ , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_BITMAP_REF_VERT , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_BITMAP_TRANSPOSE, hasData);
            EnableMenuItem(mHwndPopupMenu, MID_BITMAP_ROT_P90  , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_BITMAP_ROT_M90  , hasData);

            // Page submenu
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE      , numPages > 1);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE_FIRST, mCurrentPageIndex > 0);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE_PREV , mCurrentPageIndex > 0);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE_NEXT , mCurrentPageIndex < numPages-1);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_PAGE_LAST , mCurrentPageIndex < numPages-1);

            // Zoom menu entries
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_DOWN    , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_UP      , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , hasData);
            EnableMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, hasData);

            switch(mZoomMode)
            {
                case GbmBitmapViewer::ZoomMode_NONE:
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , TRUE);
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;

                case GbmBitmapViewer::ZoomMode_FITWIN:
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , TRUE);
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;

                case GbmBitmapViewer::ZoomMode_FITWIDTH:
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, TRUE);
                    break;

                default: // ZoomMode_FREE
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_NONE    , FALSE);
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIN  , FALSE);
                    CheckMenuItem(mHwndPopupMenu, MID_VIEW_ZOOM_FITWIDTH, FALSE);
                    break;
            }

            // Help submenu
            EnableMenuItem(mHwndPopupMenu, MID_HELP_IMAGEINFO, hasData);
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
MRESULT GbmBitmapViewer::instanceWindowProcedure(HWND   hwnd,
                                                 ULONG  msg,
                                                 MPARAM mp1,
                                                 MPARAM mp2)
{
    switch(msg)
    {
        // private message posted by NPP_SetWindow() to resize
        // the frame after the plugin window has been resized
        case GBM_BITMAP_VIEWER_WM_POSSIZE_PARENT:
            WinSetWindowPos(mHwndFrame, 0,
                            SHORT1FROMMP(mp1), SHORT2FROMMP(mp1),
                            SHORT1FROMMP(mp2), SHORT2FROMMP(mp2),
                            SWP_MOVE | SWP_SIZE);
            return 0;

        // -------------------------

        case WM_SIZE:
        {
            // stop paint thread before locking interface
            mPaintThread.stop();
            MutexGuard interfaceGuard(mInterfaceMutex);

            updateWindowSize(hwnd);
            updateScaleFactor(hwnd, FALSE, FALSE);
            updateScrollBars(hwnd);
            return 0;
        }

        // -------------------------

        case WM_HSCROLL:
            if (mIsFullMode)
            {
                SHORT hScrollInc = 0;

                // stop paint thread before locking interface
                mPaintThread.stop();
                {
                    MutexGuard interfaceGuard(mInterfaceMutex);

                    // set line increment to SCROLL_PAGE_INCREMENT_STEPS steps of the image width
                    int pageWidth = 0, pageHeight = 0, pageColorDepth = 0;
                    getPageSize(pageWidth, pageHeight, pageColorDepth);

                    SHORT scrollbar_line_increment = pageWidth / SCROLL_PAGE_INCREMENT_STEPS;
                    scrollbar_line_increment = MAX(1, scrollbar_line_increment);

                    switch(SHORT2FROMMP(mp2))
                    {
                        case SB_LINELEFT:
                            hScrollInc = -scrollbar_line_increment;
                            break;

                        case SB_LINERIGHT:
                            hScrollInc = scrollbar_line_increment;
                            break;

                        case SB_PAGELEFT:
                            hScrollInc = MIN(-scrollbar_line_increment, -mCxClient);
                            break;

                        case SB_PAGERIGHT:
                            hScrollInc = MAX( scrollbar_line_increment, mCxClient);
                            break;

                        case SB_SLIDERTRACK:
                            hScrollInc = SHORT1FROMMP(mp2) - mHScrollPos;
                            break;

                        default:
                            break;
                    }
                    hScrollInc = MAX(-mHScrollPos, MIN(hScrollInc, mHScrollMax - mHScrollPos));
                    if (hScrollInc != 0)
                    {
                        mHScrollPos += hScrollInc;
                    }
                }
                if (hScrollInc != 0)
                {
                    WinScrollWindow(hwnd, -hScrollInc, 0, NULL, NULL, NULL, NULL, SW_INVALIDATERGN);
                    SetScrollBarPos(mHwndHScroll, mHScrollPos);
                    WinUpdateWindow(hwnd);
                }
                return 0;
            }
            break;

        // -------------------------

        case WM_VSCROLL:
            if (mIsFullMode)
            {
                SHORT vScrollInc = 0;

                // stop paint thread before locking interface
                mPaintThread.stop();
                {
                    MutexGuard interfaceGuard(mInterfaceMutex);

                    // set line increment to SCROLL_PAGE_INCREMENT_STEPS steps of the image width
                    int pageWidth = 0, pageHeight = 0, pageColorDepth = 0;
                    getPageSize(pageWidth, pageHeight, pageColorDepth);

                    SHORT scrollbar_line_increment = pageHeight / SCROLL_PAGE_INCREMENT_STEPS;
                    scrollbar_line_increment = MAX(1, scrollbar_line_increment);

                    switch(SHORT2FROMMP(mp2))
                    {
                        case SB_LINEUP:
                            vScrollInc = -scrollbar_line_increment;
                            break;

                        case SB_LINEDOWN:
                            vScrollInc = scrollbar_line_increment;
                            break;

                        case SB_PAGEUP:
                            vScrollInc = MIN(-scrollbar_line_increment, -mCyClient);
                            break;

                        case SB_PAGEDOWN:
                            vScrollInc = MAX( scrollbar_line_increment, mCyClient);
                            break;

                        case SB_SLIDERTRACK:
                            vScrollInc = SHORT1FROMMP(mp2) - mVScrollPos;
                            break;

                        default:
                            break;
                    }
                    vScrollInc = MAX(-mVScrollPos, MIN(vScrollInc, mVScrollMax - mVScrollPos));
                    if (vScrollInc != 0)
                    {
                        mVScrollPos += vScrollInc;
                    }
                }
                if (vScrollInc != 0)
                {
                    WinScrollWindow(hwnd, 0, vScrollInc, NULL, NULL, NULL, NULL, SW_INVALIDATERGN);
                    SetScrollBarPos(mHwndVScroll, mVScrollPos);
                    WinUpdateWindow(hwnd);
                }
                return 0;
            }
            break;

        // -------------------------

        case WM_CHAR:
            if (mIsFullMode)
            {
                const USHORT fsflags = SHORT1FROMMP(mp1);
                if (     ((fsflags & KC_KEYUP) == 0)
                      && ((fsflags & KC_CTRL)  == 0)
                      &&  (fsflags & KC_VIRTUALKEY)  )
                {

                    // forward keyboard messages to the scrollers and let them do the processing
                    switch(SHORT2FROMMP(mp2))
                    {
                        case VK_LEFT:
                        case VK_RIGHT:
                            if (mHwndHScroll)
                            {
                                return WinSendMsg(mHwndHScroll, msg, mp1, mp2);
                            }
                            break;

                        case VK_UP:
                        case VK_DOWN:
                        case VK_PAGEUP:
                        case VK_PAGEDOWN:
                            if (mHwndVScroll)
                            {
                                return WinSendMsg(mHwndVScroll, msg, mp1, mp2);
                            }
                            break;
                    }
                }
            }
            break;

        // -------------------------

        case WM_ERASEBACKGROUND:
            // suppress erasing the background
            return 0;

        // -------------------------

        case WM_PAINT:
        {
            mPaintThread.stop();
            {
                MutexGuard interfaceGuard(mInterfaceMutex);
                updateScaleFactor(hwnd, FALSE, TRUE);
                updateScrollBars(hwnd);
            }

            /* get window size */
            RECTL rect;
            WinQueryWindowRect(hwnd, &rect);

            RECTL invalidRect;
            HPS hps = WinBeginPaint(hwnd, NULL, &invalidRect);

            if (hasBitmap())
            {
                // Forward the paint request to the paint thread to decouple
                // the rendering from the message queue (shared across plugins).
                if (! mPaintThread.schedulePaint(this, hwnd, rect, mScaleX, mScaleY))
                {
                    // Draw directly as fallback
                    draw(hps, rect, mScaleX, mScaleY);
                }
            }
            else
            {
                // Draw directly as this just paint the progress rectangle
                draw(hps, rect, mScaleX, mScaleY);
            }

            WinEndPaint(hps);
            return 0;
        }

        // -------------------------

        case WM_INITMENU:
            updateMenuItems(hwnd, SHORT1FROMMP(mp1));
            return 0;

        // -------------------------

        case WM_CONTEXTMENU:
            // Update the menu item of the small popup window manually as it
            // has no submenus. Thus there is no WM_INITMENU send.
            updateMenuItems(hwnd, mCurrentPopupMenuId);
            WinPopupMenu(hwnd,
                         hwnd,
                         mHwndPopupMenu,
                         SHORT1FROMMP(mp1) /* x */,
                         SHORT2FROMMP(mp1) /* y */,
                         0,
                         PU_HCONSTRAIN |
                         PU_VCONSTRAIN |
                         PU_NONE       |
                         PU_KEYBOARD   | PU_MOUSEBUTTON1);
            return 0;

        // -------------------------

        case WM_COMMAND:
            if (executeMenuOperations(hwnd, SHORT1FROMMP(mp1) /* itemId */))
            {
                return 0;
            }
            break;

        default:
            break;
    }

    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

//----------------------------------------------------------------------------

/*
 * Show either GBM File Dialog if gbmdlg.dll is available or
 * use as fallback the standard OS/2 file dialog.
 */
BOOL GbmBitmapViewer::showFileDialog_SaveAs(HWND   hwnd,
                                            CHAR * filename, size_t filenameSize,
                                            CHAR * options , size_t optionsSize) const
{
    GbmSaveFileDialog saveFileDlg(mDllModule,
                                  mHwndHelp,
                                  mpLanguage->resid[RES_RID_GBM_FILEDLG],
                                  mGbmDocument.getGbmAccessor(),
                                  mStringSaveAs,
                                  mStringAllFiles);
    if (saveFileDlg.show(hwnd, filename, filenameSize,
                               options , optionsSize))
    {
        return TRUE;
    }
    memset(filename, 0, filenameSize);
    memset(options , 0, optionsSize);

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
    HWND hwnd, const RECTL & rect, double scaleX, double scaleY)
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
            const HPS hps = WinGetPS(pTask->hwnd);
            pTask->viewer->draw(hps, pTask->rect,
                                     pTask->scaleX,
                                     pTask->scaleY);
            WinReleasePS(hps);
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


