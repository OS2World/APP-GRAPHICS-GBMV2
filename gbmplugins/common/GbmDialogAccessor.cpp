
//***************************************************
// Generalised Bitmap Module C++ adapter for GBM plugins
//
// Requires: GBM.DLL version 1.35 or higher (with multipage support)
//
// Copyright (C) 2006-2008 Heiko Nitzsche
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

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#include <os2.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "GbmDialogAccessor.hpp"

// ---------------------------------------------------------

GbmDialogAccessor::GbmDialogAccessor()
    : moduleName(NULL),
      moduleHandle(NULLHANDLE),
      mIsInternalModule(FALSE),
      fn_GbmFileDlg(NULL),
      fn_GbmFileDlgStatic(NULL),
      fn_GbmDefFileDlgProc(NULL)
{
}

// ---------------------------------------------------------

GbmDialogAccessor::~GbmDialogAccessor()
{
    dispose();
}

// ---------------------------------------------------------

void GbmDialogAccessor::dispose()
{
    if (moduleHandle != 0)
    {
        if (mIsInternalModule)
        {
            DosFreeModule(moduleHandle);
        }
        moduleHandle = 0;
    }
    mIsInternalModule = FALSE;

    delete [] moduleName;
    moduleName = NULL;

    fn_GbmFileDlg        = NULL;
    fn_GbmFileDlgStatic  = NULL;
    fn_GbmDefFileDlgProc = NULL;
}

// ---------------------------------------------------------

const char * GbmDialogAccessor::getModuleName() const
{
    return moduleName;
}

// ---------------------------------------------------------

const char * GbmDialogAccessor::getHelpFileName() const
{
    return "GBMDLG.HLP";
}

// ---------------------------------------------------------

gbm_boolean GbmDialogAccessor::init(const char * dllPath)
{
    if (dllPath == NULL)
    {
        return GBM_FALSE;
    }

    // cleanup first in case of double calls
    dispose();

    // Load GBMDLG.DLL
    UCHAR   loadError[32] = "";
    HMODULE hmod          = NULLHANDLE;

    APIRET rc = DosLoadModule((PSZ)loadError, sizeof(loadError)-1,
                              (PSZ)dllPath, &hmod);
    if (rc == NO_ERROR)
    {
      // mark the module as internally managed
      mIsInternalModule = TRUE;
      return initFromModule(hmod);
    }
    return GBM_FALSE;
}

// ---------------------------------------------------------

gbm_boolean GbmDialogAccessor::init(HMODULE hmod)
{
    // cleanup first in case of double calls
    dispose();
    mIsInternalModule = FALSE;
    return initFromModule(hmod);
}

// ---------------------------------------------------------

gbm_boolean GbmDialogAccessor::initFromModule(HMODULE hmod)
{
     CHAR  foundModuleName[_MAX_PATH] = { 0 };
    ULONG  foundModuleNameLength = sizeof(foundModuleName);
    APIRET rc = NO_ERROR;

    moduleHandle = hmod;

    // get the full path name
    rc = DosQueryModuleName(hmod,
                            foundModuleNameLength,
                            foundModuleName);
    if (rc)
    {
        dispose();
        return GBM_FALSE;
    }

    const int moduleNameLength = strlen(foundModuleName);
    try
    {
        moduleName = new char[moduleNameLength+1];
    }
    catch(...)
    {
        moduleName = NULL;
    }
    if (NULL == moduleName)
    {
        dispose();
        return GBM_FALSE;
    }
    moduleName = new char[moduleNameLength+1];
    strcpy(moduleName, foundModuleName);

    /* as _System exported functions */
    APIRET rcDyn  = DosQueryProcAddr(moduleHandle, 0L, (PSZ)"GbmFileDlg"      , (PFN *) &fn_GbmFileDlg);
    APIRET rcStat = DosQueryProcAddr(moduleHandle, 0L, (PSZ)"GbmFileDlgStatic", (PFN *) &fn_GbmFileDlgStatic);

    if ( ((rcDyn != NO_ERROR) && (rcStat != NO_ERROR)) ||
         ((rcDyn == NO_ERROR) && (rcStat == NO_ERROR)) )
    {
        dispose();
        return FALSE;
    }
    if (rcDyn != NO_ERROR)
    {
      fn_GbmFileDlg = NULL;
    }
    else
    {
      fn_GbmFileDlgStatic = NULL;
    }

    // the dialog window procedure is optional, so don't throw an error
    rc = DosQueryProcAddr(moduleHandle, 0L, (PSZ)"GbmDefFileDlgProc", (PFN *) &fn_GbmDefFileDlgProc);
    if (rc)
    {
        fn_GbmDefFileDlgProc = NULL;
    }
    return GBM_TRUE;
}

// ---------------------------------------------------------

HWND GbmDialogAccessor::GbmFileDlg(HWND hwndP, HWND hwndO, GBMFILEDLG *pgbmfild) const
{
    if (NULL == fn_GbmFileDlg)
    {
        return NULLHANDLE;
    }
    return fn_GbmFileDlg(hwndP, hwndO, pgbmfild);
}

// ---------------------------------------------------------

HWND GbmDialogAccessor::GbmFileDlg(HMODULE hmod, HWND hwndP, HWND hwndO, GBMFILEDLG *pgbmfild) const
{
    if (NULL == fn_GbmFileDlgStatic)
    {
        return NULLHANDLE;
    }
    return fn_GbmFileDlgStatic(hmod, hwndP, hwndO, pgbmfild);
}

// ---------------------------------------------------------

gbm_boolean GbmDialogAccessor::hasPublicGbmDefFileDlgProc() const
{
    return (fn_GbmDefFileDlgProc != NULL) ? GBM_TRUE : GBM_FALSE;
}

MRESULT GbmDialogAccessor::GbmDefFileDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) const
{
    if (fn_GbmDefFileDlgProc != NULL)
    {
        return fn_GbmDefFileDlgProc(hwnd, msg, mp1, mp2);
    }
    return NULL;
}


