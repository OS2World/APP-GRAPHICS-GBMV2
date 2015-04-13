//********************************************************************************
// Mozilla GBM plug-in: npgbm.cpp
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
// Date    : 26-Feb-2012
// Version : 1.46
// Requires: GBM.DLL version 1.73 or higher (with multipage support)
//
//********************************************************************************
//
// History:
//
//   28-Jul-2010: Initial Windows version (derived from OS/2 version 1.40)
//   16-Oct-2010: Add preferences dialog box
//                Update RAW format list
//
//********************************************************************************

// This module is Unicode compliant (except access to GBM functions).
#define UNICODE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "windefs.h"
#include <commctrl.h>
#include "npgbm.h"
#include "unicode.hpp"

#include "ConfigHandler.hpp"
#include "GbmAccessor.hpp"
#include "GbmDocument.hpp"
#include "GbmBitmapViewer.hpp"
#include "gbm.h"

#ifndef _NPAPI_H_
#include "npapi.h"
#endif

// ---------------------------------------------------------

// minimum required GBM.DLL version is 1.73
#define MIN_GBM_VERSION  173 // 1.73

// ---------------------------------------------------------

typedef struct LOOKUP_TABLE_DEF
{
    const char * mimeType;
    const char * extension;
} LOOKUP_TABLE_ENTRY;

// Keep in sync with npgbm.rc mime type entries !!!

static const struct LOOKUP_TABLE_DEF LOOKUP_TABLE [] =
{
    { "image/bmp"               , "BMP" },
    { "image/x-bmp"             , "BMP" },
    { "image/x-windows-bmp"     , "BMP" },
    { "image/gif"               , "GIF" },
    { "image/pcx"               , "PCX" },
    { "image/x-pcx"             , "PCX" },
    { "image/tif"               , "TIF" },
    { "image/tiff"              , "TIF" },
    { "image/x-tiff"            , "TIF" },
    { "image/tga"               , "TGA" },
    { "image/x-tga"             , "TGA" },
    { "image/iff"               , "IFF" },
    { "image/x-iff"             , "IFF" },
    { "image/vid"               , "VID" },
    { "image/x-portable-bitmap" , "PBM" },
    { "image/x-portable-graymap", "PGM" },
    { "image/x-portable-greymap", "PGM" },
    { "image/x-portable-pixmap" , "PPM" },
    { "image/x-portable-anymap" , "PNM" },
    { "image/png"               , "PNG" },
    { "image/x-png"             , "PNG" },
    { "image/kps"               , "KPS" },
    { "image/x-xbm"             , "XBM" },
    { "image/xbm"               , "XBM" },
    { "image/x-xbitmap"         , "XBM" },
    { "image/xpm"               , "XPM" },
    { "image/x-xpm"             , "XPM" },
    { "image/x-xpixmap"         , "XPM" },
    { "image/spr"               , "SPR" },
    { "image/pseg"              , "PSE" },
    { "image/img"               , "IMG" },
    { "image/cvp"               , "CVP" },
    { "image/jpeg"              , "JPG" },
    { "image/pjpeg"             , "JPG" },
    { "image/jp2"               , "JP2" },
    { "image/x-jp2"             , "JP2" },
    { "image/jpeg2000"          , "JP2" },
    { "image/j2k"               , "J2K" },
    { "image/x-j2k"             , "J2K" },
    { "image/jpt"               , "JPT" },
    { "image/x-jpt"             , "JPT" },
    { "image/jbig"              , "JBG" },
    { "image/x-jbig"            , "JBG" },
    /* RAW formats */
    { "image/x-adobe-dng"       , "DNG" },
    { "image/x-hasselblad-3fr"  , "3FR" },
    { "image/x-sony-arw"        , "ARW" },
    { "image/x-casio-bay"       , "BAY" },
    { "image/x-phaseone-cap"    , "CAP" },
    { "image/x-canon-crw"       , "CRW" },
    { "image/x-canon-cr2"       , "CR2" },
    { "image/x-kodak-dcr"       , "DCR" },
    { "image/x-kodak-dcs"       , "DCS" },
    { "image/x-kodak-drf"       , "DRF" },
    { "image/x-epson-erf"       , "ERF" },
    { "image/x-kodak-kdc"       , "KDC" },
    { "image/x-minolta-mdc"     , "MDC" },
    { "image/x-raw"             , "MEF" },
    { "image/x-leaf-mos"        , "MOS" },
    { "image/x-minolta-mrw"     , "MRW" },
    { "image/x-nikon-nef"       , "NEF" },
    { "image/x-nikon-nrw"       , "NRW" },
    { "image/x-olympus-orf"     , "ORF" },
    { "image/x-pentax-pef"      , "PEF" },
    { "image/x-fuji-raf"        , "RAF" },
    { "image/x-panasonic-raw"   , "RAW" },
    { "image/x-leica-raw"       , "RAW" },
    { "image/x-panasonic-rw2"   , "RW2" },
    { "image/x-leica-rwl"       , "RWL" },
    { "image/x-sony-srf"        , "SRF" },
    { "image/x-sony-sr2"        , "SR2" }
};
static const int LOOKUP_TABLE_LENGTH = sizeof(LOOKUP_TABLE) /
                                       sizeof(LOOKUP_TABLE[0]);

// ---------------------------------------------------------

static HMODULE         DLLInstance              = 0;
static wchar_t         fullCfgPath[_MAX_PATH+1] = { 0 };
static GbmAccessor     gbmAccessor;
static ConfigHandler * configHandler = NULL;

// ---------------------------------------------------------

// Compiler independent library initialize function

static unsigned libInitialize(unsigned termination)
{
    if ( termination )
    {
        delete configHandler; configHandler = NULL;
        gbmAccessor.dispose();
        memset(fullCfgPath, 0, sizeof(fullCfgPath));
    }
    else
    {
        /* DLL is attaching to process */

        wchar_t fullDllName[_MAX_PATH+1] = { 0 };
        wchar_t drive[_MAX_DRIVE+1]      = { 0 };
        wchar_t dir[_MAX_DIR+1]          = { 0 };
        wchar_t fname[_MAX_FNAME+1]      = { 0 };
        {
            DWORD rc = GetModuleFileName(DLLInstance, fullDllName, _MAX_PATH);
            if ((rc >= _MAX_PATH) &&
                (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
            {
                return 0;
            }
        }

        _wsplitpath(fullDllName, drive, dir, fname , NULL);
        _wmakepath (fullCfgPath, drive, dir, fname , L"cfg");
        _wmakepath (fullDllName, drive, dir, L"gbm", L"dll");

        // try to load GBM.DLL via absolute path of NPGBM.DLL (plugin DLL)
        if (GBM_FALSE == gbmAccessor.init(fullDllName))
        {
            // HN: Disabled to protect against DLL hijacking attacks.
            //     No general search of GBM.DLL.
            //if (GBM_FALSE == gbmAccessor.init(L"GBM"))
            {
                memset(fullCfgPath, 0, sizeof(fullCfgPath));
                return 0;
            }
        }
        // check minimum required GBM.DLL version
        if (gbmAccessor.Gbm_version() < MIN_GBM_VERSION)
        {
            gbmAccessor.dispose();
            memset(fullCfgPath, 0, sizeof(fullCfgPath));
            return 0;
        }

        // Init common controls library
        INITCOMMONCONTROLSEX commCtrlsEx;
        commCtrlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
        commCtrlsEx.dwICC  = ICC_UPDOWN_CLASS;
        if (! InitCommonControlsEx(&commCtrlsEx))
        {
            gbmAccessor.dispose();
            memset(fullCfgPath, 0, sizeof(fullCfgPath));
            return 0;
        }

        // Create the configuration file handler
        configHandler = NULL;
        try
        {
            const std::string cFullCfgPath(wstring2string(fullCfgPath));
            configHandler = new ConfigHandler(cFullCfgPath.c_str());
        }
        catch(...)
        {
        }
        if (configHandler == NULL)
        {
            gbmAccessor.dispose();
            memset(fullCfgPath, 0, sizeof(fullCfgPath));
            return 0;
        }
        configHandler->read();
    }
    return 1;
}

// -------------------------------------------------------

#ifdef __WATCOM_CPLUSPLUS__
extern "C" BOOL APIENTRY LibMain(HANDLE hModule,
                                 DWORD  ul_reason_for_call,
                                 LPVOID lpReserved)
{
#endif // #ifdef __WATCOM_CPLUSPLUS__

#ifdef _MSC_VER  // Microsoft Compiler
extern "C" BOOL APIENTRY DllMain(HANDLE hModule,
                                 DWORD  ul_reason_for_call,
                                 LPVOID lpReserved)
{
#endif // #ifdef _MSC_VER

    DLLInstance = (HMODULE) hModule;

    switch( ul_reason_for_call )
    {
        case DLL_PROCESS_ATTACH:
            // init our DLL
            if (libInitialize(0) == 0)
            {
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            // terminate our DLL
            if (libInitialize(1) == 0)
            {
                return FALSE;
            }
            break;

        default:
            break;
    }
    return TRUE;
}


// -----------------------------------
// general initialization and shutdown
// -----------------------------------
NPError NS_PluginInitialize()
{
    return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}

// ----------------------------------------------------------
// construction and destruction of our plugin instance object
// ----------------------------------------------------------
nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
    if (NULL == aCreateDataStruct)
    {
      return NULL;
    }
    GbmPluginInstance * gbmPlugin = new GbmPluginInstance(*aCreateDataStruct);
    return gbmPlugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
    delete (GbmPluginInstance *)aPlugin;
}

// --------------------------------------
// GbmPluginInstance class implementation
// --------------------------------------

GbmPluginInstance::GbmPluginInstance(const nsPluginCreateData & createData)
  : nsPluginInstanceBase(),
    mInstance(createData.instance),
    mInitialized(FALSE),
    mpWindow(NULL),
    mHwnd(0L),
    mMode(createData.mode), // mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h)
    mpDocument(NULL),
    mpViewer(NULL),
    mGbmTypeIndex(-1),
    mProgress(0),
    mProgressMax(0)
{
    // lookup the file extension valid for the mime type
    if (createData.type != NULL)
    {
        for (int index = 0; index < LOOKUP_TABLE_LENGTH; index++)
        {
            if (strcmp(LOOKUP_TABLE[index].mimeType, createData.type) == 0)
            {
                mGbmTypeIndex = index;
                break;
            }
        }
    }
}

// ---------------------------------------------
GbmPluginInstance::~GbmPluginInstance()
{
    shut();
}

// ---------------------------------------------
NPBool GbmPluginInstance::init(NPWindow* aWindow)
{
    if (mInitialized)
    {
        return TRUE;
    }

    // initialize
    try
    {
        // create the document handler
        mpDocument = new GbmDocument(gbmAccessor);
        if (mpDocument == NULL)
        {
            return FALSE;
        }
        // create the window of the mini viewer (client of the provided root window)
        mpViewer = new GbmBitmapViewer(DLLInstance, *mpDocument, *configHandler);
        if (mpViewer == NULL)
        {
            delete mpDocument;
            mpDocument = NULL;
            return FALSE;
        }

        // remember the new window info
        mpWindow = aWindow;
        mHwnd    = (HWND)(mpWindow->window);

        // set the viewer's parent window (or destroy it if hWnd is 0)
        mpViewer->setParentWindow(mHwnd, (mMode == NP_FULL) ? TRUE : FALSE);
        InvalidateRect(mHwnd, 0, TRUE);
    }
    catch(...)
    {
        delete mpViewer;
        mpViewer = NULL;

        delete mpDocument;
        mpDocument = NULL;

        return FALSE;
    }

    mInitialized = TRUE;
    return TRUE;
}

// ---------------------------------------------
void GbmPluginInstance::shut()
{
    if (! mInitialized)
    {
        return;
    }

    // cleanup and set to uninitialized
    mInitialized = FALSE;

    delete mpViewer;
    mpViewer = NULL;

    delete mpDocument;
    mpDocument = NULL;

    mGbmTypeIndex = -1;
    mProgress     =  0;
    mProgressMax  =  0;
}

// ---------------------------------------------
NPBool GbmPluginInstance::isInitialized()
{
  return mInitialized;
}

// ---------------------------------------------
NPError GbmPluginInstance::SetWindow(NPWindow* pNPWindow)
{
    if ((! pNPWindow) || (! mInitialized))
    {
        return NPERR_GENERIC_ERROR;
    }
    if (pNPWindow->type != NPWindowTypeWindow)
    {
        return NPERR_GENERIC_ERROR;
    }
    if (pNPWindow->window == 0)
    {
        // window goes away
        mpWindow = pNPWindow;
        mHwnd    = (HWND)(pNPWindow->window);

        // destroy viewer window handler
        if (! mpViewer->setParentWindow(mHwnd, (mMode == NP_FULL) ? TRUE : FALSE))
        {
            return NPERR_GENERIC_ERROR;
        }
    }
    else
    {
        // window resized? -> nothing to do as the viewer is subclassed
    }
    return NPERR_NO_ERROR;
}

// ---------------------------------------------
NPError GbmPluginInstance::NewStream(NPMIMEType   type,
                                     NPStream   * stream,
                                     NPBool       seekable,
                                     uint16     * stype)
{
    // ask for the whole file
    // (we might add streaming later by writing our own GBM I/O functions)
    // So far we use the streaming calls to display the loading progress.
    *stype = NP_ASFILE;

    mProgress    = 0;
    mProgressMax = stream->end;

    mpViewer->setLoadProgress(mProgress, mProgressMax);

    return NPERR_NO_ERROR;
}

// ---------------------------------------------
NPError GbmPluginInstance::DestroyStream(NPStream * stream, NPError reason)
{
    return NPERR_NO_ERROR;
}

// ---------------------------------------------
void GbmPluginInstance::StreamAsFile(NPStream * stream, const char * fname)
{
    try
    {
        if (mGbmTypeIndex == -1)
        {
            // set bitmap for use in drawing and printing (lookup format via filename)
            mpDocument->setDocumentFile(fname, NULL,
                                        FALSE /* force immediate loading of page data */,
                                        FALSE /* allow also non 24bpp */);
        }
        else
        {
            try
            {
                // set bitmap for use in drawing and printing (lookup format via provided extension)
                mpDocument->setDocumentFile(fname, LOOKUP_TABLE[mGbmTypeIndex].extension,
                                            FALSE /* force immediate loading of page data */,
                                            FALSE /* allow also non 24bpp */);
            }
            catch(GbmException &)
            {
                // fallback to dynamic analysis

                // set bitmap for use in drawing and printing (lookup format via filename)
                mpDocument->setDocumentFile(fname, NULL,
                                            FALSE /* force immediate loading of page data */,
                                            FALSE /* allow also non 24bpp */);
            }
        }
    }
    catch(...)
    {
    }

    // invalidate window to ensure a redraw
    InvalidateRect(mHwnd, 0, TRUE);
}

// *Developers*:
// These next 2 functions are directly relevant in a plug-in which handles the
// data in a streaming manner.  If you want zero bytes because no buffer space
// is YET available, return 0.  As long as the stream has not been written
// to the plugin, Navigator will continue trying to send bytes.  If the plugin
// doesn't want them, just return some large number from NPP_WriteReady(), and
// ignore them in NPP_Write().  For a NP_ASFILE stream, they are still called
// but can safely be ignored using this strategy.

int32 GbmPluginInstance::WriteReady(NPStream * stream)
{
   // If we are reading from a file in NP_ASFILE mode, we can take any size
   // stream in our write call (since we ignore it).
   static const int32 STREAMBUFSIZE = 0X0FFFFFFF;
   return STREAMBUFSIZE;   // Number of bytes ready to accept in NPP_Write()
}

int32 GbmPluginInstance::Write(NPStream * stream, int32 offset, int32 len, void * buffer)
{
    mProgress += len;
    mpViewer->setLoadProgress(mProgress, mProgressMax);

    return len;     // The number of bytes accepted.  Return a
                    // negative number here if, e.g., there was an error
                    // during plugin operation and you want to abort the
                    // stream
}

// ---------------------------------------------

void GbmPluginInstance::Print(NPPrint* printInfo)
{
    if (printInfo == 0)   // trap invalid parm
    {
        return;
    }
    if (printInfo->mode == NP_FULL)
    {
        // *Developers*: If your plugin would like to take over
        // printing completely when it is in full-screen mode,
        // set printInfo->pluginPrinted to TRUE and print your
        // plugin as you see fit.  If your plugin wants Netscape
        // to handle printing in this case, set printInfo->pluginPrinted
        // to FALSE (the default) and do nothing.  If you do want
        // to handle printing yourself, printOne is true if the
        // print button (as opposed to the print menu) was clicked.
        // On the Macintosh, platformPrint is a THPrint; on Windows,
        // platformPrint is a structure (defined in npapi.h) containing
        // the printer name, port, etc.
        //
        //void* platformPrint = printInfo->print.fullPrint.platformPrint;
        //NPBool printOne = printInfo->print.fullPrint.printOne;

        printInfo->print.fullPrint.pluginPrinted = FALSE; // Do the default
    }
    else    // If not fullscreen, we must be embedded
    {
        // *Developers*: If your plugin is embedded, or is full-screen
        // but you returned false in pluginPrinted above, NPP_Print
        // will be called with mode == NP_EMBED.  The NPWindow
        // in the printInfo gives the location and dimensions of
        // the embedded plugin on the printed page.  On the Macintosh,
        // platformPrint is the printer port; on Windows, platformPrint
        // is the handle to the printing device context.
        //
        const NPWindow* printWindow = &(printInfo->print.embedPrint.window);
        const void* platformPrint = printInfo->print.embedPrint.platformPrint;

        // Heiko: The original SDK code did not work and moved the elements totally
        //        outside the visible area. Thus it is changed (and simplified).
        //        Though there seems to be an incompatibility between Mozilla
        //        old Netscape 4.61. The draw position for Netscape is different
        //        than in Mozilla. This print code is written for Mozilla based browsers.

        // get Device Context and save it
        const HDC hdc    = (HDC)platformPrint;
        const int saveID = SaveDC(hdc);

        // convert coordinates
        POINT points[2] = { { printWindow->x , printWindow->y },
                            { printWindow->x + printWindow->width, printWindow->y + printWindow->height } };
        if (! DPtoLP(hdc, points, 2))
        {
            return;
        }

        /* draw using common drawing routine */

        // calc target rectangle
        RECT printRect;
        printRect.left   = points[0].x;
        printRect.top    = points[0].y;
        printRect.right  = points[1].x;
        printRect.bottom = points[1].y;

        // get the original bitmap size of the viewer */
        const double cxDraw = printRect.right  - printRect.left;
        const double cyDraw = printRect.bottom - printRect.top;

        int pageWidth      = 0;
        int pageHeight     = 0;
        int pageColorDepth = 0;
        mpViewer->getPageSize(pageWidth, pageHeight, pageColorDepth);

        const double scaleX = (pageWidth  > 0) ? cxDraw/pageWidth  : 1.0;
        const double scaleY = (pageHeight > 0) ? cyDraw/pageHeight : 1.0;

        // draw full visible window
        mpViewer->draw(hdc, printRect, scaleX, scaleY);

        RestoreDC(hdc, saveID);
    }
}

