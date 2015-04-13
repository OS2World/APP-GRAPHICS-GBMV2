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
// Date    : 28-Oct-2012
// Version : 1.47
// Requires: GBM.DLL version 1.73 or higher (with multipage support)
//
//********************************************************************************
//
// History:
//
//   16-Jul-2006: Initial release
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
//                Now supports dynamic lookup of GBM.DLL either via LIBPATH
//                or look into the directory where npgbm.dll is located.
//
//   16-Aug-2006: * Add bitmap information dialog box
//                * Fix for issue that GBM.DLL and GBMDLG.DLL are not found when
//                  LIBPATHSTRICT (or RUN!) is used. Thanks to Rich Walsh for the background
//                  info about the root cause.
//                * Add bldlevel info
//
//   21-Sep-2006: * Improved detection of bitmap file formats
//                * Enable support for XPM format
//
//   09-Sep-2007  * 8bpp grayscale and true color images are now scaled with
//                  a high quality algorithm to improve appearance.
//
//   19-Sep-2007  * Fixed resampling scaler rounding issues
//
//   27-Jan-2008  * Resampling scaler now supports all grayscale bitmaps <=8bpp
//
//   08-Feb-2008  * Move HPS rendering code to common package GbmRenderer
//                  to save memory (now renders directly from cache buffer)
//                  without temporary subrectangle buffer
//                * Allocate memory from high memory for bitmap data to
//                  stretch limit for out-of-memory errors
//                  (requires kernel with high memory support)
//
//   03-Jun-2008  * Wrong cache preparation for 1bpp bitmaps when simple scaler is active
//                  (no upconversion to 8bpp as for resampling scaler required)
//
//   19-Jun-2008  * New user configurable background rendering (option progressive_render_pages)
//                  -> Improves continous scrolling/page switching performance, especially when
//                     using high quality scaler. On dual-core systems the performance gain
//                     will be much higher than on single-core systems.
//
//   28-Aug-2008  * Add support for JPEG2000
//
//   05-Sep-2008  * Fix wrong extension to codec assignment for JPEG2000
//
//   27-Sep-2008  * Enforce struct padding to 4 byte for compiler independent compatibility
//                * Integrate modified GbmAccessor & GbmDialogAccessor
//
//   21-Sep-2009  * Fix grab focus issue in NPP_SetWindow
//                * Rework error handling in NPP_SetWindow
//                * Add JBIG support (1bpp, 8bpp)
//
//   22-Apr-2010  * Fix huge memory consumption when doing background rendering
//                  of multiple pages that have a big size difference.
//
//   30-Aug-2010  * Multithreaded rendering
//                * NPGBM now contains static version of GBMDLG,
//                  language dependent helpfiles can now be loaded.
//
//   16-Oct-2010  * Add preferences dialog box
//                * Update RAW format list
//
//********************************************************************************

#ifdef __IBMCPP__
  #pragma strings( readonly )
#endif

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define GBMFILEDLG_STATIC 1
#include "os2defs.h"
#include "ConfigHandler.hpp"
#include "GbmAccessor.hpp"
#include "GbmDocument.hpp"
#include "GbmBitmapViewer.hpp"
#include "gbm.h"
#include "gbmdlg.h"

#ifndef _NPAPI_H_
#include "npapi.h"
#endif

// ---------------------------------------------------------

// minimum required GBM.DLL version is 1.73
#define MIN_GBM_VERSION  173 // 1.73

// ---------------------------------------------------------

//
// Instance state information about the plugin.
//
typedef struct _PluginInstance PluginInstance;
typedef struct _PluginInstance
{
    NPWindow        * fWindow;
    HWND              hWnd;
    uint16            fMode;
    GbmDocument     * pDocument;
    GbmBitmapViewer * pViewer;
    int               gbmTypeIndex;
    int               progress;
    int               progressMax;
    PluginInstance  * pNext;
} PluginInstance;

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
static char            fullCfgPath[_MAX_PATH+1] = { 0 };
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

        char fullDllName[_MAX_PATH+1] = { 0 };
        char drive[_MAX_DRIVE+1]      = { 0 };
        char dir[_MAX_DIR+1]          = { 0 };
        char fname[_MAX_FNAME+1]      = { 0 };

        if (DosQueryModuleName(DLLInstance, sizeof(fullDllName)-1, fullDllName) != 0)
        {
            return 0;
        }
        _splitpath(fullDllName, drive, dir, fname, NULL);
        _makepath (fullCfgPath, drive, dir, fname, "cfg");
        _makepath (fullDllName, drive, dir, "gbm", "dll");

        // try to load GBM.DLL via absolute path of NPGBM.DLL (plugin DLL)
        if (GBM_FALSE == gbmAccessor.init(fullDllName))
        {
            if (GBM_FALSE == gbmAccessor.init("GBM"))
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

        // Create the configuration file handler
        configHandler = NULL;
        try
        {
            configHandler = new ConfigHandler(fullCfgPath);
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

// ---------------------------------------------------------

//
// This plugin will always be linked statically.
// Thus only the DLL init/term for this mode is implemented.
//

// ---------------------------------------------------------

#if defined(__IBMCPP__) || defined(__GNUC__)

/* _CRT_init is the C run-time environment initialization function.         */
/* It will return 0 to indicate success and -1 to indicate failure.         */
extern "C" int _CRT_init(void);

/* _CRT_term is the C run-time environment termination function.            */
/* It only needs to be called when the C run-time functions are statically  */
/* linked.                                                                  */
extern "C" void _CRT_term(void);

/* __ctordtorInit is the C++ run-time environment initialization function.  */
/* It is required to initialize static objects and destructors              */
extern "C" void __ctordtorInit(void);

/* __ctordtorTerm is the C++ run-time environment termination function.  */
/* It is required to destroy static objects and destructors              */
extern "C" void __ctordtorTerm(void);

/* _DLL_InitTerm() - called by the loader for DLL initialization/termination  */
/* This function must return a non-zero value if successful and a zero value  */
/* if unsuccessful.                                                           */

extern "C" unsigned long _System _DLL_InitTerm( unsigned long hModule, unsigned long termination )
{
   DLLInstance = (HMODULE) hModule;

   switch(termination)
   {
       case 0:
          if (_CRT_init() == -1)
          {
              return 0;
          }
          __ctordtorInit();

          // init our DLL
          if (libInitialize(0) == 0)
          {
              return 0;
          }
          break;

      case 1:
          // terminate our DLL
          if (libInitialize(1) == 0)
          {
              return 0;
          }
          __ctordtorTerm();
          _CRT_term();
          break;

      default:
          return 0;
   }

   /* Indicate success.  Non-zero means success!!! */
   return 1;
}

#endif // #if defined(__IBMCPP__) || defined(__GNUC__)

// -------------------------------------------------------
// -------------------------------------------------------

#ifdef __WATCOM_CPLUSPLUS__

extern "C" unsigned _System LibMain( unsigned hmod, unsigned termination )
{
    DLLInstance = (HMODULE) hmod;

    if ( termination )
    {
        // terminate our DLL
        if (libInitialize(1) == 0)
        {
            return 0;
        }
    }
    else
    {
        /* DLL is attaching to process */

        // init our DLL
        if (libInitialize(0) == 0)
        {
            return 0;
        }
    }
    return( 1 );
}
#endif // ifdef __WATCOM_CPLUSPLUS__

//----------------------------------------------------------------------------
// NPP_Initialize:
//----------------------------------------------------------------------------
NPError NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}

//----------------------------------------------------------------------------
// NPP_Shutdown:
//----------------------------------------------------------------------------
void NPP_Shutdown(void)
{
}

//----------------------------------------------------------------------------
// NPP_New:
//----------------------------------------------------------------------------
NPError NP_LOADDS NPP_New(NPMIMEType    pluginType,
                          NPP           instance,
                          uint16        mode,
                          int16         argc,
                          char        * argn[],
                          char        * argv[],
                          NPSavedData * saved)
{
    if (instance == NULL)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    // lookup the file extension valid for the mime type
    int gbmTypeIndex = -1;

    if (pluginType != NULL)
    {
        for (int index = 0; index < LOOKUP_TABLE_LENGTH; index++)
        {
            if (strcmp(LOOKUP_TABLE[index].mimeType, pluginType) == 0)
            {
                gbmTypeIndex = index;
                break;
            }
        }
    }

    // don't try to catch an unknown type even though it might work in some cases
    if (gbmTypeIndex == -1)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));
    PluginInstance* This = (PluginInstance*) instance->pdata;

    if (This == NULL)
    {
        return NPERR_OUT_OF_MEMORY_ERROR;
    }

    //
    // *Developers*: Initialize fields of your plugin
    // instance data here.  If the NPSavedData is non-
    // NULL, you can use that data (returned by you from
    // NPP_Destroy to set up the new plugin instance.
    //

    This->fWindow      = 0;
    // mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h)
    This->fMode        = mode;
    This->hWnd         = 0;
    This->pNext        = NULL;

    This->gbmTypeIndex = gbmTypeIndex;
    This->progress     = 0;
    This->progressMax  = 0;

    This->pDocument    = NULL;
    This->pViewer      = NULL;

    try
    {
        // create the document handler
        This->pDocument = new GbmDocument(gbmAccessor);
        if (This->pDocument == NULL)
        {
            return NPERR_OUT_OF_MEMORY_ERROR;
        }

        // create the window of the mini viewer (client of the provided root window)
        This->pViewer = new GbmBitmapViewer(DLLInstance, *(This->pDocument), *configHandler);
        if (This->pViewer == NULL)
        {
            delete This->pDocument;
            This->pDocument = NULL;
            return NPERR_OUT_OF_MEMORY_ERROR;
        }
    }
    catch(...)
    {
        delete This->pViewer;
        This->pViewer = NULL;

        delete This->pDocument;
        This->pDocument = NULL;

        return NPERR_OUT_OF_MEMORY_ERROR;
    }

    return NPERR_NO_ERROR;
}


//-----------------------------------------------------------------------------
// NPP_Destroy:
//-----------------------------------------------------------------------------
NPError NP_LOADDS NPP_Destroy(NPP instance, NPSavedData** save)
{
    if (instance == 0)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    PluginInstance* This = (PluginInstance*) instance->pdata;

    //
    // *Developers*: If desired, call NP_MemAlloc to create a
    // NPSavedDate structure containing any state information
    // that you want restored if this plugin instance is later
    // recreated.
    //

    if (This != NULL)
    {
        // cleanup

        delete This->pViewer;
        This->pViewer = NULL;

        delete This->pDocument;
        This->pDocument = NULL;

        This->gbmTypeIndex = -1;
        This->progress     = 0;
        This->progressMax  = 0;

        NPN_MemFree(instance->pdata);
        instance->pdata = 0;
    }

    return NPERR_NO_ERROR;
}


//----------------------------------------------------------------------------
// NPP_SetWindow:
//----------------------------------------------------------------------------
NPError NP_LOADDS NPP_SetWindow(NPP instance, NPWindow* window)
{
    if (!instance)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    PluginInstance* This = (PluginInstance*) instance->pdata;

    if ((!window) || (!This))
    {
        return NPERR_GENERIC_ERROR;
    }

    // if the window handle hasn't changed, then it's size is about
    // to change (but it hasn't yet);  post a private message to the
    // client to resize the frame to the new dimensions
    if ((This->hWnd) && (This->hWnd == (HWND)window->window))
    {
        const HWND hwndFrame = This->pViewer->getFrameHwnd();
        WinPostMsg(WinWindowFromID(hwndFrame, FID_CLIENT),
                   GBM_BITMAP_VIEWER_WM_POSSIZE_PARENT,
                   MPFROM2SHORT(0, 0),
                   MPFROM2SHORT(window->width, window->height));
        return NPERR_NO_ERROR;
    }

    // remember the new window info
    This->fWindow = window;
    This->hWnd    = (HWND)window->window;

    // set the viewer's parent window (or destroy it if hWnd is 0)
    This->pViewer->setParentWindow(This->hWnd, (This->fMode == NP_FULL) ? TRUE : FALSE);

    if (This->hWnd)
    {
        // set the frame to its initial size and position
        const HWND hwndFrame = This->pViewer->getFrameHwnd();
        WinSetWindowPos(hwndFrame, 0, 0, 0, window->width, window->height, SWP_MOVE | SWP_SIZE);
    }

    return NPERR_NO_ERROR;
}


//----------------------------------------------------------------------------
// NPP_NewStream:
//----------------------------------------------------------------------------
NPError NP_LOADDS NPP_NewStream(NPP          instance,
                                NPMIMEType   type,
                                NPStream   * stream,
                                NPBool       seekable,
                                uint16     * stype)
{
    if (instance == 0)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    PluginInstance* This = (PluginInstance*) instance->pdata;

    // ask for the whole file
    // (we might add streaming later by writing our own GBM I/O functions)
    // So far we use the streaming calls to display the loading progress.
    *stype = NP_ASFILE;

    This->progress    = 0;
    This->progressMax = stream->end;

    This->pViewer->setLoadProgress(This->progress, This->progressMax);

    return NPERR_NO_ERROR;
}


//
// *Developers*:
// These next 2 functions are directly relevant in a plug-in which handles the
// data in a streaming manner.  If you want zero bytes because no buffer space
// is YET available, return 0.  As long as the stream has not been written
// to the plugin, Navigator will continue trying to send bytes.  If the plugin
// doesn't want them, just return some large number from NPP_WriteReady(), and
// ignore them in NPP_Write().  For a NP_ASFILE stream, they are still called
// but can safely be ignored using this strategy.
//

//----------------------------------------------------------------------------
// NPP_WriteReady:
//----------------------------------------------------------------------------
int32 NP_LOADDS NPP_WriteReady(NPP instance, NPStream *stream)
{
   // If we are reading from a file in NP_ASFILE mode, we can take any size
   // stream in our write call (since we ignore it).
   static const int32 STREAMBUFSIZE = 0X0FFFFFFF;

   return STREAMBUFSIZE;   // Number of bytes ready to accept in NPP_Write()
}


//----------------------------------------------------------------------------
// NPP_Write:
//----------------------------------------------------------------------------
int32 NP_LOADDS NPP_Write(NPP instance, NPStream *stream,
                          int32 offset, int32     len   , void *buffer)
{
    if (instance != 0)
    {
        PluginInstance* This = (PluginInstance*) instance->pdata;

        This->progress += len;

        This->pViewer->setLoadProgress(This->progress, This->progressMax);
    }

    return len;     // The number of bytes accepted.  Return a
                    // negative number here if, e.g., there was an error
                    // during plugin operation and you want to abort the
                    // stream
}


//----------------------------------------------------------------------------
// NPP_DestroyStream:
//----------------------------------------------------------------------------
NPError NP_LOADDS NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
    if (instance == 0)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    return NPERR_NO_ERROR;
}


//----------------------------------------------------------------------------
// NPP_StreamAsFile:
//----------------------------------------------------------------------------
void NP_LOADDS NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
    if (instance == 0)
    {
        return;
    }

    const PluginInstance* This = (const PluginInstance*) instance->pdata;

    try
    {
        if (This->gbmTypeIndex == -1)
        {
            // set bitmap for use in drawing and printing (lookup format via filename)
            This->pDocument->setDocumentFile(fname, NULL,
                                                    FALSE /* force immediate loading of page data */,
                                                    FALSE /* allow also non 24bpp */);
        }
        else
        {
            try
            {
                // set bitmap for use in drawing and printing (lookup format via provided extension)
                This->pDocument->setDocumentFile(fname, LOOKUP_TABLE[This->gbmTypeIndex].extension,
                                                        FALSE /* force immediate loading of page data */,
                                                        FALSE /* allow also non 24bpp */);
            }
            catch(GbmException & ex)
            {
                // fallback to dynamic analysis

                // set bitmap for use in drawing and printing (lookup format via filename)
                This->pDocument->setDocumentFile(fname, NULL,
                                                        FALSE /* force immediate loading of page data */,
                                                        FALSE /* allow also non 24bpp */);
            }
        }
    }
    catch(...)
    {
    }

    // invalidate window to ensure a redraw
    WinInvalidateRect(This->hWnd, 0, TRUE);
}


//----------------------------------------------------------------------------
// NPP_Print:
//----------------------------------------------------------------------------
void NP_LOADDS NPP_Print(NPP instance, NPPrint* printInfo)
{
    if (printInfo == 0)   // trap invalid parm
    {
        return;
    }

    if (instance != 0)
    {
        const PluginInstance* This = (const PluginInstance*) instance->pdata;

        if (printInfo->mode == NP_FULL)
        {
            //
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
            //
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

            // get Presentation Space and save it
            const HPS  hps    = (HPS)platformPrint;
            const LONG saveID = GpiSavePS(hps);

            /* draw using common drawing routine */

            // Netscape and Mozilla use different coordinate systems and
            // thus we have to use different placement algorithms.

            int plugin_major  , plugin_minor;
            int netscape_major, netscape_minor;

            NPN_Version(&plugin_major, &plugin_minor, &netscape_major, &netscape_minor);

            // Mozilla based browser?
            if (netscape_minor > 11 /* New browsers report 16, old Netscape 4.61 reports 11 */)
            {
                SIZEL siz = { 0,0 };
                GpiQueryPS(hps, &siz);

                // calc target rectangle
                RECTL printRect;
                printRect.xLeft   = printWindow->x;
                printRect.yBottom = siz.cy - (printWindow->y + printWindow->height);
                printRect.xRight  = printRect.xLeft   + printWindow->width;
                printRect.yTop    = printRect.yBottom + printWindow->height;

                // get the original bitmap size of the viewer */
                const double cxDraw = printRect.xRight - printRect.xLeft;
                const double cyDraw = printRect.yTop   - printRect.yBottom;

                int pageWidth      = 0;
                int pageHeight     = 0;
                int pageColorDepth = 0;
                This->pViewer->getPageSize(pageWidth, pageHeight, pageColorDepth);

                const double scaleX = (pageWidth  > 0) ? cxDraw/pageWidth  : 1.0;
                const double scaleY = (pageHeight > 0) ? cyDraw/pageHeight : 1.0;

                // draw full visible window
                This->pViewer->draw(hps, printRect, scaleX, scaleY);
            }
#if 0 // Not yet finished!!!
            else
            {
                // Older Netscape (2.x to 4.x) seem to have a different coordinate system.
                // The printWindow is obviously in TWIPS rather than PELS (Mozilla) which
                // makes it difficult to output our bitmap.

                // Open issue: How can the printWindow TWIPS coordinates be converted to PELS?

                SIZEL siz = { 0,0 };
                GpiQueryPS(hps, &siz);
                //GpiQueryPageViewport

                // calc target rectangle
                RECTL printRect;
                printRect.xLeft   = printWindow->x;
                printRect.yBottom = printWindow->y;
                printRect.xRight  = printRect.xLeft   + printWindow->width;
                printRect.yTop    = printRect.yBottom + printWindow->height;

                GpiConvert(hps, CVTC_MODEL, CVTC_WORLD, 2, (PPOINTL)&printRect);

                SIZEL sizlsize = { 0, 0 };
                GpiSetPS(hps, &sizlsize, PU_PELS | GPIF_DEFAULT);

                // get the original bitmap size of the viewer */
                const double cxDraw = printRect.xRight - printRect.xLeft;
                const double cyDraw = printRect.yTop   - printRect.yBottom;

                const int pageWidth  = This->pViewer->getPageWidth();
                const int pageHeight = This->pViewer->getPageHeight();

                const double scaleX = (pageWidth  > 0) ? cxDraw/pageWidth  : 1.0;
                const double scaleY = (pageHeight > 0) ? cyDraw/pageHeight : 1.0;

                // draw full visible window
                This->pViewer->draw(hps, printRect, scaleX, scaleY);
            }
#endif
            GpiRestorePS(hps, saveID);
        }
    }
}


//----------------------------------------------------------------------------
// NPP_HandleEvent:
// Mac-only.
//----------------------------------------------------------------------------
int16 NP_LOADDS NPP_HandleEvent(NPP instance, void* event)
{
    NPBool eventHandled = FALSE;
    if (instance == 0)
    {
        return eventHandled;
    }

    //PluginInstance* This = (PluginInstance*) instance->pdata;

    //
    // *Developers*: The "event" passed in is a Macintosh
    // EventRecord*.  The event.what field can be any of the
    // normal Mac event types, or one of the following additional
    // types defined in npapi.h: getFocusEvent, loseFocusEvent,
    // adjustCursorEvent.  The focus events inform your plugin
    // that it will become, or is no longer, the recepient of
    // key events.  If your plugin doesn't want to receive key
    // events, return false when passed at getFocusEvent.  The
    // adjustCursorEvent is passed repeatedly when the mouse is
    // over your plugin; if your plugin doesn't want to set the
    // cursor, return false.  Handle the standard Mac events as
    // normal.  The return value for all standard events is currently
    // ignored except for the key event: for key events, only return
    // true if your plugin has handled that particular key event.
    //

    return eventHandled;
}


