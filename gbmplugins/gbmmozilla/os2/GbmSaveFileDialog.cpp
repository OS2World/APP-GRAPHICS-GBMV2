//********************************************************************************
// Mozilla GBM plug-in: GbmSaveFileDialog.cpp (OS/2)
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

#ifdef __IBMCPP__
  #pragma strings( readonly )
#endif

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INCL_WIN
#include "os2defs.h"
#include "GbmSaveFileDialog_rc.h"
#include "GbmSaveFileDialog.hpp"
#include "gbmmem.h"
#include "gbmscale.h"

// ---------------------------------------------------------

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))

// ---------------------------------------------------------

#define L_GBM_OPTIONS  500
#pragma pack(4)
typedef struct _GBMFILEDLG
{
    FILEDLG fild;
    CHAR    szOptions[L_GBM_OPTIONS+1];
} GBMFILEDLG;
#pragma pack()

// ---------------------------------------------------------

static void UpdateBitmapPreview(const GbmAccessor & gbmAccessor,
                                const HWND          dlgHwnd,
                                const char *        fileName,
                                const char *        options);

static BOOL CreateScaledSystemBitmap(const GbmAccessor & gbmAccessor,
                                     const HAB   hab,
                                     const int   fd     , const int ft,
                                     const GBM * gbm_src, const int dst_w, const int dst_h,
                                     HBITMAP   * bmpHandle);

static BOOL Expand1To24bit(GBM *gbm, const GBMRGB *gbmrgb, gbm_u8 **data);

static BOOL ReplaceFilenameExtension(const GbmAccessor & gbmAccessor,
                                     const char        * filename,
                                     const char        * filetype,
                                           char       ** pNewFilename);

// ---------------------------------------------------------

//
// Here is the window procedure for subclassing the GBM file dialog box.
//
static MRESULT APIENTRY GlobalWindowProcedure(HWND   hwnd,
                                              ULONG  msg,
                                              MPARAM mp1,
                                              MPARAM mp2)
{
    const FILEDLG * fild = (const FILEDLG *) WinQueryWindowULong(hwnd, QWL_USER);
    if (fild != NULL)
    {
        GbmSaveFileDialog * pObj = (GbmSaveFileDialog *) fild->ulUser;
        return pObj->instanceWindowProcedure(hwnd, msg, mp1, mp2);
    }
    return (MRESULT) FALSE;
}

// ---------------------------------------------------------

static BOOL processGbmFileDlg(HWND   hwndDlg , GBMFILEDLG * gbmfild,
                              CHAR * filename, size_t       filenameSize,
                              CHAR * options , size_t       optionsSize)
{
    if (gbmfild->fild.lReturn == DID_OK)
    {
        memset(filename, 0, filenameSize);
        memset(options , 0, optionsSize);

        strncpy((char *)filename, gbmfild->fild.szFullFile,
                MIN(filenameSize-1, strlen(gbmfild->fild.szFullFile)));
        strncpy((char *)options, gbmfild->szOptions,
                MIN(optionsSize-1, strlen(gbmfild->szOptions)));

        return TRUE;
    }
    return FALSE;
}

// ---------------------------------------------------------

GbmSaveFileDialog::GbmSaveFileDialog(const HMODULE       hModResources,
                                     const HWND          hwndHelp,
                                     const SHORT         dialogResourceId,
                                     const GbmAccessor & gbmAccessor,
                                     const CHAR        * title,
                                     const CHAR        * titleAllFilesType)
    : mGbmAccessor(gbmAccessor),
      mModule(hModResources),
      mHwndHelp(hwndHelp),
      mDialogResourceId(dialogResourceId),
      mTitle(NULL),
      mTitleAllFilesType(NULL)
{
    if (title)
    {
        const size_t titleLen = strlen(title);
        try
        {
            mTitle = new CHAR[titleLen + 1];
        }
        catch(...)
        {
            mTitle = NULL;
        }
        if (mTitle)
        {
            memset((char *)mTitle, 0, titleLen + 1);
            strcpy((char *)mTitle, title);
        }
    }
    if (titleAllFilesType)
    {
        const size_t titleLen = strlen(titleAllFilesType);
        try
        {
            mTitleAllFilesType = new CHAR[titleLen + 1];
        }
        catch(...)
        {
            mTitleAllFilesType = NULL;
        }
        if (mTitleAllFilesType)
        {
            memset((char *)mTitleAllFilesType, 0, titleLen + 1);
            strcpy((char *)mTitleAllFilesType, titleAllFilesType);
        }
    }
}

// ---------------------------------------------------------

GbmSaveFileDialog::~GbmSaveFileDialog()
{
    delete [] mTitle;
    mTitle = NULL;
}

//----------------------------------------------------------------------------

//
// Here is the subclass window procedure for the GBM file dialog.
//
MRESULT GbmSaveFileDialog::instanceWindowProcedure(HWND   hwnd,
                                                   ULONG  msg,
                                                   MPARAM mp1,
                                                   MPARAM mp2) const
{
   #define L_FN  CCHMAXPATH

    switch(msg)
    {
        case WM_INITDLG:
        {
           MRESULT mr = WinDefFileDlgProc(hwnd, msg, mp1, mp2);
           GBMFILEDLG *pgbmfild = (GBMFILEDLG *) WinQueryWindowULong(hwnd, QWL_USER);
           FILEDLG *pfild = &(pgbmfild->fild);

           WinSendDlgItemMsg(hwnd, DID_GBM_OPTIONS_ED, EM_SETTEXTLIMIT, MPFROMSHORT(L_GBM_OPTIONS), NULL);
           WinSetDlgItemText(hwnd, DID_GBM_OPTIONS_ED, (PSZ)(pgbmfild->szOptions));
           /* Init query changed message for options entry field (focus change handling) */
           WinSendDlgItemMsg(hwnd, DID_GBM_OPTIONS_ED, EM_QUERYCHANGED, (MPARAM)0, (MPARAM)0);

           WinSetDlgItemText(hwnd, DID_PREVIEW_FORMAT_ED, (PSZ)"");
           WinSetDlgItemText(hwnd, DID_PREVIEW_WIDTH_ED , (PSZ)"");
           WinSetDlgItemText(hwnd, DID_PREVIEW_HEIGHT_ED, (PSZ)"");
           WinSetDlgItemText(hwnd, DID_PREVIEW_DEPTH_ED , (PSZ)"");
           WinSetDlgItemText(hwnd, DID_PREVIEW_PAGES_ED , (PSZ)"");

           WinSendDlgItemMsg(hwnd, DID_PREVIEW_RECT, VM_SETITEM,
                             MPFROM2SHORT(1,1), MPFROMLONG(NULLHANDLE));

           if (pfild->pszTitle != NULL)
           {
               WinSetWindowText(hwnd, pfild->pszTitle);
           }
           return mr;
        }

        // ------------------------------------------

        case WM_COMMAND:
           switch (SHORT1FROMMP(mp1))
           {
              case DID_OK:
              case DID_CANCEL:
              {
                 GBMFILEDLG *pgbmfild = (GBMFILEDLG *) WinQueryWindowULong(hwnd, QWL_USER);

                 WinQueryDlgItemText(hwnd, DID_GBM_OPTIONS_ED, L_GBM_OPTIONS, (PSZ)(pgbmfild->szOptions));

                 /* delete bitmap set */
                 {
                    HBITMAP oldBitmap = (HBITMAP) WinSendDlgItemMsg(hwnd, DID_PREVIEW_RECT, VM_QUERYITEM,
                                                                    MPFROM2SHORT(1,1), MPFROMLONG(NULL));
                    if (oldBitmap)
                    {
                       WinSendDlgItemMsg(hwnd, DID_PREVIEW_RECT, VM_SETITEM,
                                         MPFROM2SHORT(1,1), MPFROMLONG(NULLHANDLE));
                       GpiDeleteBitmap(oldBitmap);
                       oldBitmap = NULLHANDLE;
                    }
                 }

                 MRESULT mr = WinDefFileDlgProc(hwnd, msg, mp1, mp2);

                 /* free the type list */
                 delete [] pgbmfild->fild.papszITypeList;
                 delete [] pgbmfild->fild.pszIType;
                 pgbmfild->fild.papszITypeList = NULL;
                 pgbmfild->fild.pszIType       = NULL;

                 return mr;
              }
           }
           break;

        // ------------------------------------------

        case WM_FOCUSCHANGE: /* update preview when options have been changed */
        {
          const HWND hwndSender = HWNDFROMMP(mp1);
          const HWND hwndField  = WinWindowFromID(hwnd, DID_GBM_OPTIONS_ED);

          if (hwndSender == hwndField)
          {
             const MRESULT mr = WinDefFileDlgProc(hwnd, msg, mp1, mp2);

             /* update preview if window changed looses focus and options field has changed */
             if (LONGFROMMR(WinSendMsg(hwndField, EM_QUERYCHANGED, (MPARAM)0, (MPARAM)0)))
             {
                const HWND hwndListbox = WinWindowFromID(hwnd, DID_FILES_LB);
                const LONG index = LONGFROMMR(WinSendMsg(hwndListbox, LM_QUERYSELECTION,
                                                                      MPFROMLONG(LIT_FIRST),
                                                                      (MPARAM)0));
                if (index >= 0)
                {
                   /* deselecting and reselecting the file entry triggers preview update */
                   WinSendMsg(hwndListbox, LM_SELECTITEM, MPFROMSHORT(index), MPFROMSHORT(FALSE));
                   WinSendMsg(hwndListbox, LM_SELECTITEM, MPFROMSHORT(index), MPFROMSHORT(TRUE));
                }
             }
             return ( mr );
          }
        }
        break;

        // ------------------------------------------

        case WM_CONTROL:
          switch (SHORT1FROMMP(mp1))
          {
             case DID_FILES_LB:
                if (SHORT2FROMMP(mp1) == LN_SELECT)
                {
                   const MRESULT mr = WinDefFileDlgProc(hwnd, msg, mp1, mp2);

                   const HWND lbHwnd = HWNDFROMMP(mp2);
                   const LONG index  = WinQueryLboxSelectedItem(lbHwnd);

                   char itemText[L_FN+1] = { 0 };

                   WinSetDlgItemText(hwnd, DID_PREVIEW_FORMAT_ED, (PSZ)"");
                   WinSetDlgItemText(hwnd, DID_PREVIEW_WIDTH_ED , (PSZ)"");
                   WinSetDlgItemText(hwnd, DID_PREVIEW_HEIGHT_ED, (PSZ)"");
                   WinSetDlgItemText(hwnd, DID_PREVIEW_DEPTH_ED , (PSZ)"");
                   WinSetDlgItemText(hwnd, DID_PREVIEW_PAGES_ED , (PSZ)"");

                   /* delete bitmap set */
                   {
                      HBITMAP oldBitmap = (HBITMAP) WinSendDlgItemMsg(hwnd, DID_PREVIEW_RECT, VM_QUERYITEM,
                                                                      MPFROM2SHORT(1,1), MPFROMLONG(NULL));
                      if (oldBitmap)
                      {
                         WinSendDlgItemMsg(hwnd, DID_PREVIEW_RECT, VM_SETITEM,
                                           MPFROM2SHORT(1,1), MPFROMLONG(NULLHANDLE));
                         GpiDeleteBitmap(oldBitmap);
                         oldBitmap = NULLHANDLE;
                      }
                   }

                   if (index >= 0)
                   {
                      int ft;
                      GBMFILEDLG *pgbmfild = (GBMFILEDLG *) WinQueryWindowULong(hwnd, QWL_USER);

                      WinQueryDlgItemText(hwnd, DID_GBM_OPTIONS_ED, L_GBM_OPTIONS, (PSZ)(pgbmfild->szOptions));
                      WinQueryLboxItemText(lbHwnd, (SHORT) index, itemText, L_FN);
                      WinSetDlgItemText(hwnd, DID_FILENAME_ED, (PSZ)itemText);

                      /* check file type */
                      if (mGbmAccessor.Gbm_guess_filetype(itemText, &ft) == GBM_ERR_OK)
                      {
                         char options[L_GBM_OPTIONS+1] = { 0 };

                         WinQueryDlgItemText(hwnd, DID_GBM_OPTIONS_ED, L_GBM_OPTIONS, (PSZ)options);
                         UpdateBitmapPreview(mGbmAccessor, hwnd, itemText, options);
                      }
                   }
                   return mr;
                }
                break;

            case DID_FILTER_CB:
                if (SHORT2FROMMP(mp1) == LN_SELECT)
                {
                   const MRESULT mr  = WinDefFileDlgProc(hwnd, msg, mp1, mp2);
                   const HWND lbHwnd = HWNDFROMMP(mp2);
                   const LONG index  = WinQueryLboxSelectedItem(lbHwnd);

                   /* try to sync the filename extension to the selcted type filter */

                   if (index >= 0)
                   {
                      char *fnText       = NULL;
                      LONG  fnTextLength = WinQueryDlgItemTextLength(hwnd, DID_FILENAME_ED);

                      if (fnTextLength > 0)
                      {
                         try
                         {
                             fnText = new char[fnTextLength+1];
                         }
                         catch(...)
                         {
                             fnText = NULL;
                         }
                         if (fnText != NULL)
                         {
                            char   ftText[100+1] = { 0 };
                            char * pNewFilename  = NULL;

                            WinQueryLboxItemText(lbHwnd, (SHORT) index, ftText, sizeof(ftText));
                            WinQueryDlgItemText(hwnd, DID_FILENAME_ED, fnTextLength+1, (PSZ)fnText);

                            if (ReplaceFilenameExtension(mGbmAccessor, fnText, ftText, &pNewFilename))
                            {
                               WinSetDlgItemText(hwnd, DID_FILENAME_ED, (PSZ)pNewFilename);
                               delete [] pNewFilename;

                               /* clean preview area */
                               WinSetDlgItemText(hwnd, DID_PREVIEW_FORMAT_ED, (PSZ)"");
                               WinSetDlgItemText(hwnd, DID_PREVIEW_WIDTH_ED , (PSZ)"");
                               WinSetDlgItemText(hwnd, DID_PREVIEW_HEIGHT_ED, (PSZ)"");
                               WinSetDlgItemText(hwnd, DID_PREVIEW_DEPTH_ED , (PSZ)"");
                               WinSetDlgItemText(hwnd, DID_PREVIEW_PAGES_ED , (PSZ)"");

                               /* delete bitmap set */
                               {
                                  HBITMAP oldBitmap = (HBITMAP) WinSendDlgItemMsg(hwnd, DID_PREVIEW_RECT, VM_QUERYITEM,
                                                                                  MPFROM2SHORT(1,1), MPFROMLONG(NULL));
                                  if (oldBitmap)
                                  {
                                     WinSendDlgItemMsg(hwnd, DID_PREVIEW_RECT, VM_SETITEM,
                                                       MPFROM2SHORT(1,1), MPFROMLONG(NULLHANDLE));
                                     GpiDeleteBitmap(oldBitmap);
                                     oldBitmap = NULLHANDLE;
                                  }
                               }

                               /* remove the file selection in the listbox */
                               WinSendDlgItemMsg(hwnd, DID_FILES_LB, LM_SELECTITEM,
                                                 MPFROMSHORT(LIT_NONE), MPFROMSHORT(FALSE));
                            }
                            delete [] fnText;
                         }
                      }
                   }
                   return mr;
                }
                break;
          }
          break;

        // ------------------------------------------

        /*
         * To be listed, the file must match the user specified filename (if present)
         * and the filetype specification.
         */
        case FDM_FILTER:
        {
           CHAR szFn [L_FN+1] = { 0 }, szFnOut [L_FN+1] = { 0 };
           WinQueryDlgItemText(hwnd, DID_FILENAME_ED, sizeof(szFn), (PSZ)szFn);

           /* user has specified a filter himself */
           if ( strlen(szFn) != 0 )
           {
              if ( DosEditName(1, (PCH) mp1, (PSZ)szFn, (PSZ)szFnOut, sizeof(szFnOut)) == 0 )
              {
                 if ( stricmp(szFn, szFnOut) &&
                      stricmp((char *) mp1, szFnOut) )
                 {
                    return ( (MRESULT) FALSE );
                 }
              }
           }

           /* filter based on file type */
           {
              HWND hwndFt = WinWindowFromID(hwnd, DID_FILTER_CB);
              CHAR szFt [100+1] = { 0 };
              int ft = -1, n_ft = 0, guess_ft = -1;
              SHORT sInx;

              if ( (sInx = SHORT1FROMMR(WinSendMsg(hwndFt, LM_QUERYSELECTION, NULL, NULL))) != -1 )
              {
                 WinSendMsg(hwndFt, LM_QUERYITEMTEXT, MPFROM2SHORT(sInx, sizeof(szFt)), MPFROMP(szFt));
              }
              else
              {
                 WinQueryWindowText(hwndFt, sizeof(szFt), (PSZ)szFt);
              }

              /* Look up type name in GBM supported file types */
              mGbmAccessor.Gbm_query_n_filetypes(&n_ft);
              for ( ft = 0; ft < n_ft; ft++ )
              {
                 GBMFT gbmft;
                 mGbmAccessor.Gbm_query_filetype(ft, &gbmft);
                 if ( !strcmp((const char*)szFt, gbmft.long_name) )
                 {
                    break;
                 }
              }

              if ( ft < n_ft )
              {
                 /* Must not be <All Files> or <All GBM supported files> */
                 if ( mGbmAccessor.Gbm_guess_filetype((char *) mp1, &guess_ft) != GBM_ERR_OK ||
                      guess_ft != ft )
                 {
                    return ( (MRESULT) FALSE );
                 }
              }
              else if ( !strcmp((const char*)szFt, mTitleAllFilesType) )
              {
                 if ( mGbmAccessor.Gbm_guess_filetype((char *) mp1, &guess_ft) != GBM_ERR_OK ||
                      guess_ft == -1 )
                 {
                    return ( (MRESULT) FALSE );
                 }
              }
           }

           return ( (MRESULT) TRUE );
        }

        // ------------------------------------------

        case WM_ACTIVATE:
            if (mHwndHelp)
            {
                const HWND hwndActivated = HWNDFROMMP(mp2);
                if (hwndActivated == hwnd)
                {
                    const BOOL activated = SHORT1FROMMP(mp1); // usactive
                    if (activated)
                    {
                        WinAssociateHelpInstance(mHwndHelp, hwnd);
                    }
                    else
                    {
                        WinAssociateHelpInstance(NULLHANDLE, hwnd);
                    }
                }
            }
            break;

        // ------------------------------------------

        case WM_HELP:
            if (mHwndHelp)
            {
                WinSendMsg(mHwndHelp, HM_DISPLAY_HELP, MPFROMSHORT(HID_GBM_FILEDLG),
                                                       MPFROMSHORT(HM_RESOURCEID));
                return ((MRESULT) TRUE);
            }
            break;

        default:
            break;
    }

    return WinDefFileDlgProc(hwnd, msg, mp1, mp2);
}

//----------------------------------------------------------------------------

// Build the file types for the file dialog
static BOOL createFileDialogFileTypeString(
    const GbmAccessor & gbmAccessor,
          CHAR        * stringAllSupportedFiles,
          FILEDLG     & fild)
{
    int n_ft = 0;
    gbmAccessor.Gbm_query_n_filetypes(&n_ft);

    CHAR **apsz = NULL;
    try
    {
        apsz = new CHAR*[n_ft + 2];
    }
    catch(...)
    {
        apsz = NULL;
    }
    if (NULL == apsz)
    {
       return FALSE;
    }

    int ft;
    for (ft = 0; ft < n_ft; ft++ )
    {
       GBMFT gbmft;
       gbmAccessor.Gbm_query_filetype(ft, &gbmft);
       apsz[ft] = gbmft.long_name;
    }
    // Add "all supported types"
    apsz[n_ft++] = stringAllSupportedFiles;
    apsz[n_ft]   = NULL;

    fild.papszITypeList = (PAPSZ) apsz;
    fild.pszIType       = NULL;

    /* try to find a reasonable initial type selection */
    if (fild.fl & FDS_SAVEAS_DIALOG)
    {
        /* try to find the initial type selection based on the filename */
        if (fild.szFullFile != NULL)
        {
            int guess_ft;
            if (gbmAccessor.Gbm_guess_filetype(fild.szFullFile, &guess_ft) == GBM_ERR_OK)
            {
               GBMFT gbmft;
               gbmAccessor.Gbm_query_filetype(guess_ft, &gbmft);

               try
               {
                   fild.pszIType = (PSZ) new CHAR[strlen(gbmft.long_name) + 1];
               }
               catch(...)
               {
                   fild.pszIType = NULL;
               }
               if (fild.pszIType)
               {
                   strcpy((char *)(fild.pszIType), gbmft.long_name);
               }
            }
        }
    }
    if (fild.pszIType == NULL)
    {
        try
        {
            fild.pszIType = (PSZ) new CHAR[strlen(stringAllSupportedFiles) + 1];
        }
        catch(...)
        {
            fild.pszIType = NULL;
        }
        if (fild.pszIType)
        {
            strcpy((char *)(fild.pszIType), stringAllSupportedFiles);
        }
    }
    return TRUE;
}

//----------------------------------------------------------------------------

/*
 * Show either GBM File Dialog if gbmdlg.dll is available or
 * use as fallback the standard OS/2 file dialog.
 */
BOOL GbmSaveFileDialog::show(const HWND hwndOwner,
                             CHAR * filename, size_t filenameSize,
                             CHAR * options , size_t optionsSize) const
{
    // use the GBM File Dialog to provide the user with more options if available
    GBMFILEDLG gbmfild;

    memset(&gbmfild, 0, sizeof(GBMFILEDLG));
    gbmfild.fild.cbSize   = sizeof(FILEDLG);
    gbmfild.fild.fl       = FDS_CENTER | FDS_ENABLEFILELB | FDS_SAVEAS_DIALOG;
    gbmfild.fild.pszTitle = (PSZ)(mTitle ? mTitle : "Save as...");

    // set initial fields
    strncpy(gbmfild.fild.szFullFile, (const char *)filename, MIN(filenameSize-1, sizeof(gbmfild.fild.szFullFile)-1));
    strncpy(gbmfild.szOptions      , (const char *)options , MIN(optionsSize-1 , sizeof(gbmfild.szOptions)-1));

    BOOL errorDetected = FALSE;

    if (mModule != NULLHANDLE)
    {
        // Do we have the help manager available?
        // Is this a bugfixed GBM file dialog that has a public dialog window procedure?
        if (mHwndHelp != NULLHANDLE)
        {
            // add help button and make the dialog modeless so that we can subclass it
            gbmfild.fild.fl |= FDS_HELPBUTTON;
        }

        // configure custom template based file dialog
        gbmfild.fild.hMod     = mModule;
        gbmfild.fild.usDlgId  = mDialogResourceId;
        gbmfild.fild.fl      |= FDS_CUSTOM;

        // Put the class instance in the ulUser field to access it from the
        // global subclass window procedure.
        gbmfild.fild.ulUser = (ULONG) this;

        // Set our local window procedure. It will forward messages to the GBM file dialog.
        gbmfild.fild.pfnDlgProc = (PFNWP) GlobalWindowProcedure;

        // build the list of supported file types
        if (createFileDialogFileTypeString(mGbmAccessor,
                                           mTitleAllFilesType,
                                           gbmfild.fild))
        {
            // create the GBM file dialog
            const HWND hwndDlg = WinFileDlg(HWND_DESKTOP, hwndOwner, (FILEDLG *)(&gbmfild));
            if (hwndDlg)
            {
               return processGbmFileDlg( hwndDlg , &gbmfild,
                                         filename,  filenameSize,
                                         options ,  optionsSize);
            }
            else
            {
               // GBM file dialog not created
               errorDetected = TRUE;
            }
        }
        else
        {
           errorDetected = TRUE;
        }
    }
    else
    {
        // no GbmDlg accessor available
        errorDetected = TRUE;
    }

    /* Fallback to standard OS/2 File Dialog */
    if (errorDetected)
    {
        memset(&gbmfild, 0, sizeof(GBMFILEDLG));
        gbmfild.fild.cbSize   = sizeof(FILEDLG);
        gbmfild.fild.fl       = FDS_CENTER | FDS_ENABLEFILELB | FDS_SAVEAS_DIALOG;
        gbmfild.fild.pszTitle = (PSZ)(mTitle ? mTitle : "Save as...");

        // set initial fields
        strncpy(gbmfild.fild.szFullFile, (const char *)filename, MIN(filenameSize-1, sizeof(gbmfild.fild.szFullFile)-1));
        strncpy(gbmfild.szOptions      , (const char *)options , MIN(optionsSize-1 , sizeof(gbmfild.szOptions)-1));

        // Display the dialog and get the file
        const HWND hwndDlg = WinFileDlg(HWND_DESKTOP, hwndOwner, &(gbmfild.fild));
        if (hwndDlg)
        {
            return processGbmFileDlg( hwndDlg , &gbmfild,
                                      filename,  filenameSize,
                                      options ,  optionsSize);
        }
        else
        {
            WinMessageBox(HWND_DESKTOP,
                          hwndOwner,
                          (PCSZ)"File dialog not available.",
                          (PCSZ)"GBM Bitmap Viewer",
                          0, MB_OK | MB_ERROR | MB_MOVEABLE);
        }
    }

    return FALSE;
}

// -------------------------------------------------------
// --- This is a clone of the original GBM File Dialog ---
// --- Slightly modified for multi-language support.   ---
// -------------------------------------------------------

static void UpdateBitmapPreview(const GbmAccessor & gbmAccessor,
                                const HWND          dlgHwnd,
                                const char *        fileName,
                                const char *        options)
{
   /* check file type */
   int ft = 0;
   if (gbmAccessor.Gbm_guess_filetype(fileName, &ft) == GBM_ERR_OK)
   {
      char  fullFileName[2*CCHMAXPATH+1] = { 0 };

      const GBMFILEDLG *pgbmfild = (GBMFILEDLG *) WinQueryWindowULong(dlgHwnd, QWL_USER);
      const FILEDLG    *pfild    = &(pgbmfild -> fild);
      const CHAR       *pPath    = pfild->szFullFile;

      /* create fully qualified filename */
      strcpy(fullFileName, pPath);
      strcat(fullFileName, fileName);

      GBMFT gbmft;
      gbmAccessor.Gbm_query_filetype(ft, &gbmft);

      /* get bitmap header */
      int fd = gbmAccessor.Gbm_io_open(fullFileName, GBM_O_RDONLY);
      if (fd != -1)
      {
         int numPages = 1;
         char ext_options[L_GBM_OPTIONS+10+1] = { 0 };
         char  buffer[50] = { 0 };
         GBM   gbm;

         if (gbmAccessor.Gbm_read_imgcount(fileName, fd, ft, &numPages) == GBM_ERR_OK)
         {
            sprintf(buffer, "%d", numPages);
            WinSetDlgItemText(dlgHwnd, DID_PREVIEW_PAGES_ED, (PSZ)buffer);
         }
         else
         {
            WinSetDlgItemText(dlgHwnd, DID_PREVIEW_PAGES_ED, (PSZ)"");
         }

         sprintf(ext_options, "ext_bpp %s", options);
         if (gbmAccessor.Gbm_read_header(fullFileName, fd, ft, &gbm, ext_options) == GBM_ERR_OK)
         {
            const int ext_bpp = gbm.bpp;

            WinSetDlgItemText(dlgHwnd, DID_PREVIEW_FORMAT_ED, (PSZ)(gbmft.short_name));

            sprintf(buffer, "%d", gbm.w);
            WinSetDlgItemText(dlgHwnd, DID_PREVIEW_WIDTH_ED, (PSZ)buffer);

            sprintf(buffer, "%d", gbm.h);
            WinSetDlgItemText(dlgHwnd, DID_PREVIEW_HEIGHT_ED, (PSZ)buffer);

            /* We asked for extended color depth for info field, but OS/2 only supports up to 24 bpp.
             * So we must reopen the file and use the backward compatible mode which automatically
             * scales down to 24 bpp.
             */
            if (ext_bpp > 24)
            {
               if (gbmAccessor.Gbm_read_header(fullFileName, fd, ft, &gbm, options) == GBM_ERR_OK)
               {
                  /* if user entered "ext_bpp" */
                  if (gbm.bpp > 24)
                  {
                     const char * extbpp = "ext_bpp";
                     strcpy(ext_options, options);

                     /* strip "ext_bpp" if specified by the user, so that the preview can still be shown */
                     char * bufptr = strstr(ext_options, extbpp);
                     while((bufptr != NULL) && (bufptr != extbpp))
                     {
                        /* override "ext_bpp" with white spaces */
                        memset(bufptr, ' ', 7);
                        bufptr = strstr(ext_options, extbpp);
                     }

                     if (gbmAccessor.Gbm_read_header(fullFileName, fd, ft, &gbm, ext_options) != GBM_ERR_OK)
                     {
                        gbmAccessor.Gbm_io_close(fd);
                        WinSetDlgItemText(dlgHwnd, DID_PREVIEW_PAGES_ED, (PSZ)"");
                        return;
                     }

                     sprintf(buffer, "%d", ext_bpp);
                     WinSetDlgItemText(dlgHwnd, DID_PREVIEW_DEPTH_ED, (PSZ)buffer);
                  }
                  else
                  {
                     sprintf(buffer, "%d (%d)", gbm.bpp, ext_bpp);
                     WinSetDlgItemText(dlgHwnd, DID_PREVIEW_DEPTH_ED, (PSZ)buffer);
                  }
               }
               else
               {
                  gbmAccessor.Gbm_io_close(fd);
                  WinSetDlgItemText(dlgHwnd, DID_PREVIEW_PAGES_ED, (PSZ)"");
                  return;
               }
            }
            else
            {
               sprintf(buffer, "%d", gbm.bpp);
               WinSetDlgItemText(dlgHwnd, DID_PREVIEW_DEPTH_ED, (PSZ)buffer);
            }

            /* load bitmap for preview and scale down */

            /* set to preview */
            MRESULT mr = WinSendDlgItemMsg(dlgHwnd, DID_PREVIEW_RECT,
                                           VM_QUERYMETRICS, (MPARAM) VMA_ITEMSIZE, (MPARAM) 0);
            USHORT rect_w = SHORT1FROMMR(mr);
            USHORT rect_h = SHORT2FROMMR(mr);

            /* create system bitmap */
            {
               int dst_w = gbm.w;
               int dst_h = gbm.h;

               if (rect_h < gbm.h)
               {
                  dst_w = (gbm.w * rect_h) / gbm.h;
                  dst_h = rect_h;
               }

               /* check if we need to scale down based on the width */
               if (dst_w > rect_w)
               {
                  dst_w = rect_w;
                  dst_h = (gbm.h * rect_w) / gbm.w;
               }

               if ((dst_w > 0 && dst_w <= rect_w) && (dst_h > 0 && dst_h <= rect_h))
               {
                  HBITMAP bmpHandle = NULLHANDLE;
                  if (CreateScaledSystemBitmap(gbmAccessor,
                                               WinQueryAnchorBlock(dlgHwnd),
                                               fd, ft, &gbm,
                                               dst_w, dst_h,
                                               &bmpHandle))
                  {
                      /* set to preview */
                      WinSendDlgItemMsg(dlgHwnd, DID_PREVIEW_RECT, VM_SETITEM,
                                        MPFROM2SHORT(1,1), MPFROMLONG(bmpHandle));
                  }
               }
            }
         }
         else
         {
            WinSetDlgItemText(dlgHwnd, DID_PREVIEW_PAGES_ED, (PSZ)"");
         }

         gbmAccessor.Gbm_io_close(fd);
      }
   }
}

/********************************/

static BOOL CreateScaledSystemBitmap(const GbmAccessor & gbmAccessor,
                                     const HAB   hab,
                                     const int   fd     , const int ft,
                                     const GBM * gbm_src, const int dst_w, const int dst_h,
                                     HBITMAP   * bmpHandle)
{
   /* bitmap header incl. color table */
   /* for above 8 bit images */
   #pragma pack(2)
   typedef struct
   {
       BITMAPINFOHEADER2 bmp2;
   } BMP24HEADER;
   #pragma pack()

   GBM gbm = *gbm_src;
   GBMRGB gbmrgb[0x100] = { { 0, 0, 0 } };

   const unsigned long dataDstLength = ((((unsigned long)dst_w * 24 + 31) / 32) * 4) * dst_h;
   gbm_u8 * dataDst = NULL;

   BMP24HEADER header24BMP;

   /* init returned handle */
   *bmpHandle = NULLHANDLE;

   if (gbm.bpp <= 8)
   {
      /* read color palette */
      if (gbmAccessor.Gbm_read_palette(fd, ft, &gbm, gbmrgb) != GBM_ERR_OK)
      {
         return FALSE;
      }
   }

   /* read bitmap data */
   {
      GBM_ERR rc;
      const unsigned long dataSrcLength = ((((unsigned long)gbm.w * gbm.bpp + 31) / 32) * 4) * gbm.h;

      gbm_u8 * dataSrc = (gbm_u8 *) gbmmem_malloc(dataSrcLength);
      if (dataSrc == NULL)
      {
         return FALSE;
      }

      if (gbmAccessor.Gbm_read_data(fd, ft, &gbm, dataSrc) != GBM_ERR_OK)
      {
         gbmmem_free(dataSrc);
         return FALSE;
      }

      /* expand to true color bitmap if necessary */
      if (! Expand1To24bit(&gbm, gbmrgb, &dataSrc))
      {
        gbmmem_free(dataSrc);
        return FALSE;
      }

      /* scale down to target size */
      dataDst = (gbm_u8 *) gbmmem_malloc(dataDstLength);
      if (dataDst == NULL)
      {
         gbmmem_free(dataSrc);
         return FALSE;
      }

      rc = gbm_quality_scale_bgra(dataSrc, gbm.w, gbm.h,
                                  dataDst, dst_w, dst_h,
                                  gbm.bpp,
                                  GBM_SCALE_FILTER_MITCHELL);
      if (rc != GBM_ERR_OK)
      {
         gbmmem_free(dataSrc);
         gbmmem_free(dataDst);
         return FALSE;
      }

      gbmmem_free(dataSrc);
      gbm.w = dst_w;
      gbm.h = dst_h;
   }

   /* create bitmap header */
   {
      memset( &header24BMP, 0, sizeof(BMP24HEADER));
      header24BMP.bmp2.cbFix         = sizeof(BITMAPINFOHEADER2);
      header24BMP.bmp2.cx            = gbm.w;
      header24BMP.bmp2.cy            = gbm.h;
      header24BMP.bmp2.cPlanes       = 1;             /*  Number of bit planes. */
      header24BMP.bmp2.cBitCount     = gbm.bpp;       /*  Number of bits per pel within a plane. */
      header24BMP.bmp2.ulCompression = BCA_UNCOMP;    /*  Compression scheme used to store the bit map. */
      header24BMP.bmp2.cbImage       = dataDstLength; /*  Length of bit-map storage data, in bytes. */
   }

   /* create system bitmap */
   {
      HDC hdc;
      HPS hps;
      SIZEL sizl;

      BITMAPINFO2 *bmpInfo;

      bmpInfo = (PBITMAPINFO2) &header24BMP;

      /* create a memory context */
      if ( (hdc = DevOpenDC(hab, OD_MEMORY, (PSZ)"*", 0L, (PDEVOPENDATA) NULL, (HDC) NULL)) == (HDC) NULL )
      {
         gbmmem_free(dataDst);
         return FALSE;
      }

      sizl.cx = bmpInfo->cx;
      sizl.cy = bmpInfo->cy;

      /* create presentation space */
      if ( (hps = GpiCreatePS(hab, hdc, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) == (HPS) NULL )
      {
         DevCloseDC(hdc);
         gbmmem_free(dataDst);
         return FALSE;
      }

      /* create the bitmap */
      *bmpHandle = GpiCreateBitmap( hps
                                    , (PBITMAPINFOHEADER2) bmpInfo
                                    , CBM_INIT
                                    , (PBYTE) dataDst
                                    , bmpInfo );

      /* destroy memory context */
      GpiSetBitmap(hps, (HBITMAP) NULL);
      GpiDestroyPS(hps);
      DevCloseDC(hdc);
   }
   gbmmem_free(dataDst);

   return TRUE;
}

/********************************/

/** Expand to 24bpp */
static BOOL Expand1To24bit(GBM *gbm, const GBMRGB *gbmrgb, gbm_u8 **data)
{
  unsigned long stride = (((unsigned long)gbm->w * gbm->bpp + 31)/32) * 4;
  unsigned long new_stride = (((unsigned long)gbm->w * 3 + 3) & ~3);
  unsigned long bytes;
  int y;
  gbm_u8 *new_data;

  if (gbm->bpp == 24)
  {
    return TRUE;
  }
  if ((gbm->bpp != 1) && (gbm->bpp != 4) && (gbm->bpp != 8))
  {
    return FALSE;
  }

  bytes = new_stride * gbm->h;
  if ( (new_data = (gbm_u8 *)gbmmem_malloc(bytes)) == NULL )
  {
    return FALSE;
  }

  for (y = 0; y < gbm->h; y++)
  {
    gbm_u8  *src = *data + y * stride;
    gbm_u8  *dest = new_data + y * new_stride;
    gbm_u8  c = 0;
    int     x;

    switch ( gbm -> bpp )
    {
      case 1:
      {
        for (x = 0; x < gbm->w; x++)
        {
          if ((x & 7) == 0)
          {
            c = *src++;
          }
          else
          {
            c <<= 1;
          }

          *dest++ = gbmrgb[c >> 7].b;
          *dest++ = gbmrgb[c >> 7].g;
          *dest++ = gbmrgb[c >> 7].r;
        }
      }
      break;

      case 4:
        for ( x = 0; x + 1 < gbm -> w; x += 2 )
        {
          c = *src++;

          *dest++ = gbmrgb [c >> 4].b;
          *dest++ = gbmrgb [c >> 4].g;
          *dest++ = gbmrgb [c >> 4].r;
          *dest++ = gbmrgb [c & 15].b;
          *dest++ = gbmrgb [c & 15].g;
          *dest++ = gbmrgb [c & 15].r;
        }

        if ( x < gbm -> w )
        {
          c = *src;

          *dest++ = gbmrgb [c >> 4].b;
          *dest++ = gbmrgb [c >> 4].g;
          *dest++ = gbmrgb [c >> 4].r;
        }
        break;

      case 8:
        for ( x = 0; x < gbm -> w; x++ )
        {
          c = *src++;

          *dest++ = gbmrgb [c].b;
          *dest++ = gbmrgb [c].g;
          *dest++ = gbmrgb [c].r;
        }
        break;
    }
  }
  gbmmem_free(*data);
  *data = new_data;
  gbm->bpp = 24;

  return TRUE;
}

/********************************/

/* Replace filename extension by one of the specified type */
static BOOL ReplaceFilenameExtension(const GbmAccessor & gbmAccessor,
                                     const char        * filename,
                                     const char        * filetype,
                                           char       ** pNewFilename)
{
   int ft, n_ft;

   gbmAccessor.Gbm_query_n_filetypes(&n_ft);
   for ( ft = 0; ft < n_ft; ft++ )
   {
      GBMFT gbmft;
      gbmAccessor.Gbm_query_filetype(ft, &gbmft);

      if (strcmp(filetype, gbmft.long_name) == 0)
      {
        char * extTok = NULL;
        char * dupExt = strdup(gbmft.extensions);

        extTok = strtok(dupExt, " ");
        if (extTok != NULL)
        {
          char fullDllName[_MAX_PATH+1] = { 0 };
          char drive[_MAX_DRIVE+1]      = { 0 };
          char dir  [_MAX_DIR+1]        = { 0 };
          char fname[_MAX_FNAME+1]      = { 0 };
          char ext  [_MAX_EXT]          = { 0 };

          strcpy(fullDllName, filename);

          /* extract the file extension */
          _splitpath(fullDllName, drive, dir, fname, ext);

          /* set the new extension */
          _makepath(fullDllName, drive, dir, fname, extTok);

          *pNewFilename = NULL;
          try
          {
              *pNewFilename = new char[strlen(fullDllName)+1];
          }
          catch(...)
          {
              *pNewFilename = NULL;
          }
          if (*pNewFilename != NULL)
          {
            strcpy(*pNewFilename, fullDllName);
            free(dupExt);
            return TRUE;
          }
        }
        free(dupExt);
        break;
      }
   }
   return FALSE;
}



