
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

#ifndef __CLASS_GBM_DIALOG_ACCESSOR__
    #define __CLASS_GBM_DIALOG_ACCESSOR__

#include "gbm.h"
#include "gbmdlg.h"
#include <os2.h>

// ---------------------------------------------------------

class GbmDialogAccessor
{
    private:

        char    * moduleName;
        HMODULE   moduleHandle;
        BOOL      mIsInternalModule;

        GbmDialogAccessor(const GbmDialogAccessor & accessor);
        GbmDialogAccessor & operator =(const GbmDialogAccessor & accessor);

        gbm_boolean initFromModule(HMODULE hmod);

        // --- Function prototype for DLL functions ---

        typedef HWND    (GBMENTRYP_SYS tfn_GbmFileDlg)       (HWND hwndP, HWND hwndO, GBMFILEDLG *pgbmfild);
        typedef HWND    (GBMENTRYP_SYS tfn_GbmFileDlgStatic) (HMODULE hmod, HWND hwndP, HWND hwndO, GBMFILEDLG *pgbmfild);
        typedef MRESULT (GBMENTRYP_SYS tfn_GbmDefFileDlgProc)(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

        tfn_GbmFileDlg        fn_GbmFileDlg;
        tfn_GbmFileDlgStatic  fn_GbmFileDlgStatic;
        tfn_GbmDefFileDlgProc fn_GbmDefFileDlgProc;

    // ---------------------------------------------------------

    public:

        GbmDialogAccessor();
        virtual ~GbmDialogAccessor();

        // Loads the dll and initializes the accessor
        virtual gbm_boolean init(const char * dllPath);

        // Initializes the accessor from already loaded module
        virtual gbm_boolean init(HMODULE hmod);

        // Unloads the dll and uninitializes the accessor
        virtual void dispose();

        virtual const char * getModuleName()   const;
        virtual const char * getHelpFileName() const;

        // GBMDLG wrapper functions

        // This must be used if the GBM File Dialog is loaded from a DLL
        virtual HWND GbmFileDlg(HWND hwndP, HWND hwndO, GBMFILEDLG *pgbmfild) const;

        // This must be used if the GBM File Dialog is statically linked
        virtual HWND GbmFileDlg(HMODULE hmod, HWND hwndP, HWND hwndO, GBMFILEDLG *pgbmfild) const;

        virtual gbm_boolean hasPublicGbmDefFileDlgProc() const;
        virtual MRESULT     GbmDefFileDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) const;
};

#endif


