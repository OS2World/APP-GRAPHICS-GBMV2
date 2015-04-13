//********************************************************************************
// Mozilla GBM plug-in: GbmSaveFileDialog.cpp (Windows)
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
// Minimum supported OS is Windows 2000.
// * If minimum is Windows 2000
//   -> Restriction: No GBM Option field in File Dialog
// * If minimum is Vista
//   -> GBM Option field in File Dialog + new Vista/Win7 File Dialog
//   -> Does not run on Windows 2000 or XP
//
// TODO: Add support for automatic changing the extension when changing the filter
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

#include "GbmSaveFileDialog.hpp"
#include "unicode.hpp"

// Enable support for new File Dialog on Vista and higher
#include "windefs.h"
#if ((NTDDI_VERSION >= 0x06000000) && (_WIN32_IE >= 0x0700))
  #define ENABLE_VISTA_FILE_DIALOG  1
#endif

#ifdef ENABLE_VISTA_FILE_DIALOG
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h> // COM IDL
#include <objbase.h>  // COM
#endif

// ---------------------------------------------------------

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))

// ---------------------------------------------------------

static const int  CID_GBM_OPTIONS_LABEL  = 1; // Label  ID for GBM options label field
static const int  CID_GBM_OPTIONS_EDIT   = 2; // Button ID for GBM options edit field
static const UINT ALL_FILES_FILTER_INDEX = 1;

// ---------------------------------------------------------

static BOOL splitFullPath(const std::wstring & path, std::wstring & folder, std::wstring & filename)
{
   wchar_t drivename[_MAX_DRIVE + 1] = { 0 };
   wchar_t dirname  [_MAX_DIR   + 1] = { 0 };
   wchar_t fname    [_MAX_FNAME + 1] = { 0 };
   wchar_t fext     [_MAX_EXT   + 1] = { 0 };

    _wsplitpath(path.c_str(), drivename, dirname, fname, fext);

    folder = drivename;
    folder.append(dirname);

    filename = fname;
    filename.append(fext);
    return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// check if the output format supports the requested color depth
static BOOL isFileTypeWriteSupported(const GBMFT & gbmft, const int bpp)
{
    int flag = 0;
    switch(bpp)
    {
        case 64: flag = GBM_FT_W64; break;
        case 48: flag = GBM_FT_W48; break;
        case 32: flag = GBM_FT_W32; break;
        case 24: flag = GBM_FT_W24; break;
        case  8: flag = GBM_FT_W8;  break;
        case  4: flag = GBM_FT_W4;  break;
        case  1: flag = GBM_FT_W1;  break;
        default: flag = 0;          break;
    }
    if ((gbmft.flags & flag) == 0)
    {
        return FALSE;
    }
    return TRUE;
}

//----------------------------------------------------------------------------

#ifdef ENABLE_VISTA_FILE_DIALOG

static BOOL createFileDialogFileType(
    const std::wstring & description,
    const std::wstring & extensions,
    COMDLG_FILTERSPEC  & fileType)
{
    fileType.pszName = NULL;
    try
    {
        fileType.pszName = new wchar_t[description.length() + 1];
    }
    catch(...)
    {
        fileType.pszName = NULL;
    }

    fileType.pszSpec = NULL;
    try
    {
        fileType.pszSpec = new wchar_t[extensions.length()  + 1];
    }
    catch(...)
    {
        fileType.pszSpec = NULL;
    }
    if ((fileType.pszName == NULL) ||
        (fileType.pszSpec == NULL))
    {
        delete [] fileType.pszName;
        delete [] fileType.pszSpec;
        return FALSE;
    }
    wcscpy((wchar_t *)(fileType.pszName), description.c_str());
    wcscpy((wchar_t *)(fileType.pszSpec), extensions.c_str());
    return TRUE;
}

//----------------------------------------------------------------------------

static void freeFileDialogFileType(COMDLG_FILTERSPEC & fileType)
{
    delete [] fileType.pszName;
    fileType.pszName = NULL;

    delete [] fileType.pszSpec;
    fileType.pszSpec = NULL;
}

//----------------------------------------------------------------------------

static COMDLG_FILTERSPEC * createFileDialogFileTypesArray(const UINT numElements)
{
    COMDLG_FILTERSPEC * pFileTypes = NULL;
    try
    {
        pFileTypes = new COMDLG_FILTERSPEC[numElements];
    }
    catch(...)
    {
        pFileTypes = NULL;
    }
    if (pFileTypes)
    {
        for (UINT i = 0; i < numElements; ++i)
        {
            pFileTypes[i].pszName = NULL;
            pFileTypes[i].pszSpec = NULL;
        }
    }
    return pFileTypes;
}

//----------------------------------------------------------------------------

static void freeFileDialogFileTypes(
    const UINT numElements, COMDLG_FILTERSPEC ** pFileTypes)
{
    if (pFileTypes)
    {
        if (*pFileTypes)
        {
            for (UINT i = 0; i < numElements; ++i)
            {
                freeFileDialogFileType((*pFileTypes)[i]);
            }
            delete [] *pFileTypes;
            *pFileTypes = NULL;
        }
    }
}

//----------------------------------------------------------------------------

// Build the file types for the new file dialog
static UINT createFileDialogFileTypes(
    const GbmAccessor  & gbmAccessor,
    const std::wstring & stringAllFiles,
    const int            imageBpp,
    COMDLG_FILTERSPEC ** pFileTypes)
{
    // get all supported extensions from GBM
    int n_ft = 0;
    gbmAccessor.Gbm_query_n_filetypes(&n_ft);

    UINT fileTypesLength = (UINT)n_ft;
    for (int ft = 0; ft < n_ft; ft++)
    {
        GBMFT gbmft;
        gbmAccessor.Gbm_query_filetype(ft, &gbmft);

        /* check if the output format supports the requested color depth */
        if (! isFileTypeWriteSupported(gbmft, imageBpp))
        {
            --fileTypesLength;
        }
    }
    
    // n_ft + 1 entries
    ++fileTypesLength;
    *pFileTypes = createFileDialogFileTypesArray(fileTypesLength);
    if (*pFileTypes == NULL)
    {
        return 0;
    }

    COMDLG_FILTERSPEC * pt = *pFileTypes;

    // file types have the format: COMDLG_FILTERSPEC( desc, "*.jpg;*.jpeg" )
    UINT nFilter = 0;

    // Default Format Description
    if (! createFileDialogFileType(stringAllFiles, L"*.*", (*pFileTypes)[0]))
    {
        freeFileDialogFileTypes(fileTypesLength, pFileTypes);
        return FALSE;
    }
    ++nFilter;

    std::wstring wDescription;
    std::wstring wExtensions;
    int ftValid = 0;
    for (int ft = 0; ft < n_ft; ft++)
    {
        GBMFT gbmft;
        gbmAccessor.Gbm_query_filetype(ft, &gbmft);

        /* check if the output format supports the requested color depth */
        if (! isFileTypeWriteSupported(gbmft, imageBpp))
        {
            continue;
        }
        
        // get all extensions
        std::string filterExtensions;
        char * dupExt = _strdup(gbmft.extensions);
        char * extTok = strtok(dupExt, " ");

        while (extTok != NULL)
        {
            BOOL tokenAdded = FALSE;

            filterExtensions.append("*.");
            filterExtensions.append(extTok);
            tokenAdded = TRUE;

            extTok = strtok(NULL, " ");
            if (tokenAdded)
            {
                if (extTok != NULL)
                {
                    filterExtensions.append(1, ';');
                }
            }
        }
        free(dupExt);

        // create the filter entry
        wDescription = string2wstring(gbmft.long_name);
        wExtensions  = string2wstring(filterExtensions);
        if (! createFileDialogFileType(wDescription, wExtensions, (*pFileTypes)[ftValid + 1]))
        {
            freeFileDialogFileTypes(fileTypesLength, pFileTypes);
            return FALSE;
        }
        ++ftValid;
        ++nFilter;
    }
    return nFilter;
}
#endif

//----------------------------------------------------------------------------

// Build the file types for the old file dialog
static UINT createFileDialogFileTypeString(
    const GbmAccessor  & gbmAccessor,
    const std::wstring & stringAllFiles,
    const int            imageBpp,
          std::wstring & formatFilter)
{
    // Build filter string in format: "Source\0*.C;*.CXX\0All\0*.*\0"
    UINT nFilter = 0;
    formatFilter.clear();

    // get all supported extensions from GBM
    int n_ft = 0;
    gbmAccessor.Gbm_query_n_filetypes(&n_ft);

    // Default Format
    formatFilter.reserve(500);
    formatFilter.append(stringAllFiles);
    formatFilter.append(L" (*.*)");
    formatFilter.append(1, 0); // separator
    formatFilter.append(L"*.*");
    formatFilter.append(1, 0); // separator
    ++nFilter;

    std::wstring wDescription;
    std::wstring wExtensions;
    for (int ft = 0; ft < n_ft; ft++)
    {
        GBMFT gbmft;
        gbmAccessor.Gbm_query_filetype(ft, &gbmft);

        /* check if the output format supports the requested color depth */
        if (! isFileTypeWriteSupported(gbmft, imageBpp))
        {
            continue;
        }
        
        // Format description
        wDescription = string2wstring(gbmft.long_name);
        formatFilter.append(wDescription);
        formatFilter.append(L" (");

        // get all extensions
        std::string filterExtensions;
        char * dupExt = _strdup(gbmft.extensions);
        char * extTok = strtok(dupExt, " ");

        while (extTok != NULL)
        {
            BOOL tokenAdded = FALSE;

            filterExtensions.append("*.");
            filterExtensions.append(extTok);
            tokenAdded = TRUE;

            extTok = strtok(NULL, " ");
            if (tokenAdded)
            {
                if (extTok != NULL)
                {
                    filterExtensions.append(1, ';');
                }
            }
        }
        free(dupExt);

        wExtensions = string2wstring(filterExtensions);
        formatFilter.append(wExtensions);
        formatFilter.append(L")");
        formatFilter.append(1, 0); // separator
        formatFilter.append(wExtensions);
        formatFilter.append(1, 0); // separator
        ++nFilter;
    }
    return nFilter;
}

// ---------------------------------------------------------
// ---------------------------------------------------------

#ifdef ENABLE_VISTA_FILE_DIALOG
class DialogEventHandler : public IFileDialogEvents,
                           public IFileDialogControlEvents
{
    public:
        // IUnknown methods
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
        {
            static const QITAB qit[] = {
                QITABENT(DialogEventHandler, IFileDialogEvents),
                QITABENT(DialogEventHandler, IFileDialogControlEvents),
                { 0 }
            };
            return QISearch(this, qit, riid, ppv);
        }

        IFACEMETHODIMP_(ULONG) AddRef()
        {
            return InterlockedIncrement(&_cRef);
        }

        IFACEMETHODIMP_(ULONG) Release()
        {
            long cRef = InterlockedDecrement(&_cRef);
            if (!cRef) delete this;
            return cRef;
        }

        // IFileDialogEvents methods
        IFACEMETHODIMP OnFileOk(IFileDialog * pfd)
        {
            IFileDialogCustomize *pfsdCustomize = NULL;
            if (S_OK == pfd->QueryInterface(IID_IFileDialogCustomize,
                                            reinterpret_cast<void**>(&pfsdCustomize)))
            {
                // get the GBM options from the edit control
                WCHAR * pOptionsText = NULL;
                if (S_OK == pfsdCustomize->GetEditBoxText(CID_GBM_OPTIONS_EDIT, &pOptionsText))
                {
                    mOptionsText = pOptionsText;
                    CoTaskMemFree(pOptionsText); pOptionsText = NULL;
                }
                pfsdCustomize->Release();
            }
            return S_OK;
        }
        IFACEMETHODIMP OnFolderChange(IFileDialog *)                                                { return E_NOTIMPL; }
        IFACEMETHODIMP OnFolderChanging(IFileDialog *, IShellItem *)                                { return E_NOTIMPL; }
        IFACEMETHODIMP OnHelp(IFileDialog *)                                                        { return E_NOTIMPL; }
        IFACEMETHODIMP OnSelectionChange(IFileDialog *)                                             { return E_NOTIMPL; }
        IFACEMETHODIMP OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) { return E_NOTIMPL; }
        IFACEMETHODIMP OnTypeChange(IFileDialog *)                                                  { return E_NOTIMPL; }
        IFACEMETHODIMP OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *)           { return E_NOTIMPL; }

        // IFileDialogControlEvents methods
        IFACEMETHODIMP OnItemSelected      (IFileDialogCustomize *, DWORD, DWORD) { return E_NOTIMPL; }
        IFACEMETHODIMP OnButtonClicked     (IFileDialogCustomize *, DWORD)        { return E_NOTIMPL; }
        IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize *, DWORD, BOOL)  { return E_NOTIMPL; }
        IFACEMETHODIMP OnControlActivating (IFileDialogCustomize *, DWORD)        { return E_NOTIMPL; }

        DialogEventHandler() : _cRef(1) { };

        const std::wstring & getOptionsText() const
        {
            return mOptionsText;
        }

    private:
        virtual ~DialogEventHandler() { };
        long _cRef;
        std::wstring mOptionsText;
};

// Instance creation helper
static HRESULT DialogEventHandler_CreateInstance(REFIID riid, void **ppv)
{
    *ppv = NULL;
    DialogEventHandler *pDialogEventHandler = NULL;
    try
    {
        pDialogEventHandler = new DialogEventHandler();
    }
    catch(...)
    {
        pDialogEventHandler = NULL;
    }
    HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
    if (S_OK == hr)
    {
        hr = pDialogEventHandler->QueryInterface(riid, ppv);
        pDialogEventHandler->Release();
    }
    return hr;
}
#endif

// ---------------------------------------------------------
// ---------------------------------------------------------

GbmSaveFileDialog::GbmSaveFileDialog(const HMODULE        hMod,
                                     const GbmAccessor  & gbmAccessor,
                                     const std::wstring & title,
                                     const std::wstring & titleAllFilesType)
    : mModule(hMod),
      mGbmAccessor(gbmAccessor),
      mTitle(title),
      mTitleAllFiles(titleAllFilesType)
{
}

// ---------------------------------------------------------

GbmSaveFileDialog::~GbmSaveFileDialog()
{
}

//----------------------------------------------------------------------------

BOOL GbmSaveFileDialog::show(const HWND     hwndOwner,
                             const int      imageBpp,
                             std::wstring & chosenFilename,
                             std::wstring & chosenOptions) const
{
    // Extract the filename from the path
    std::wstring wdocFilename;
    std::wstring wdocFoldername;
    if (! chosenFilename.empty())
    {
        if (! splitFullPath(chosenFilename, wdocFoldername, wdocFilename))
        {
            return FALSE;
        }
    }

    std::wstring resultFilename;
    std::wstring resultOptions;
    UINT fileTypeIndex = ALL_FILES_FILTER_INDEX;

    // -----------------------------------------------------------------
    // First try to use the new system file dialog (Vista and above) and
    // fall back to the old one if this fails.
    // -----------------------------------------------------------------
#ifdef ENABLE_VISTA_FILE_DIALOG
    IFileSaveDialog *pSaveDialog = NULL;
    if (S_OK == CoCreateInstance(CLSID_FileSaveDialog,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_IFileSaveDialog,
                                 reinterpret_cast<void**>(&pSaveDialog)))
    {
        // Create an event handling object, and hook it up to the dialog.
        IFileDialogEvents *pSaveDialogEvents = NULL;
        DWORD dwEventsCookie = 0;
        {
            if (S_OK != DialogEventHandler_CreateInstance(IID_IFileDialogEvents,
                                                          reinterpret_cast<void**>(&pSaveDialogEvents)))
            {
                pSaveDialog->Release();
                return FALSE;
            }

            // Hook up the event handler.
            if (S_OK != pSaveDialog->Advise(pSaveDialogEvents, &dwEventsCookie))
            {
                pSaveDialogEvents->Release();
                pSaveDialog->Release();
                return FALSE;
            }
        }

        // add file type filters
        COMDLG_FILTERSPEC * pFileTypes = NULL;
        const UINT nFileTypes = createFileDialogFileTypes(mGbmAccessor,
                                                          mTitleAllFiles.c_str(),
                                                          imageBpp,
                                                          &pFileTypes);
        if (nFileTypes < 1)
        {
            pSaveDialog->Unadvise(dwEventsCookie);
            pSaveDialogEvents->Release();
            pSaveDialog->Release();
            return FALSE;
        }
        if ((S_OK != pSaveDialog->SetFileTypes(nFileTypes, pFileTypes)) ||
            (S_OK != pSaveDialog->SetFileTypeIndex(ALL_FILES_FILTER_INDEX)))
        {
            pSaveDialog->Unadvise(dwEventsCookie);
            pSaveDialogEvents->Release();
            pSaveDialog->Release();
            freeFileDialogFileTypes(nFileTypes, &pFileTypes);
            return FALSE;
        }

        // set title and initial filename
        {
            FILEOPENDIALOGOPTIONS fos;
            if (S_OK != pSaveDialog->GetOptions(&fos))
            {
                pSaveDialog->Unadvise(dwEventsCookie);
                pSaveDialogEvents->Release();
                pSaveDialog->Release();
                freeFileDialogFileTypes(nFileTypes, &pFileTypes);
                return FALSE;
            }
            if ((S_OK != pSaveDialog->SetOptions(fos | FOS_FORCEFILESYSTEM)) ||
                (S_OK != pSaveDialog->SetFileName(wdocFilename.c_str()))     ||
                (S_OK != pSaveDialog->SetTitle(mTitle.c_str())))
            {
                pSaveDialog->Unadvise(dwEventsCookie);
                pSaveDialogEvents->Release();
                pSaveDialog->Release();
                freeFileDialogFileTypes(nFileTypes, &pFileTypes);
                return FALSE;
            }

            if (! wdocFoldername.empty())
            {
                IShellItem * pInitDirItem = NULL;
                if (S_OK == SHCreateItemFromParsingName(wdocFoldername.c_str(),
                                                        NULL,
                                                        IID_IShellItem,
                                                        reinterpret_cast<void**>(&pInitDirItem)))
                {
                    pSaveDialog->SetDefaultFolder(pInitDirItem);
                    pInitDirItem->Release(); pInitDirItem = NULL;
                }
            }
        }

        // add GBM options field
        {
            IFileDialogCustomize * pSaveDialogCustomize = NULL;
            if (S_OK != pSaveDialog->QueryInterface(IID_IFileDialogCustomize,
                                                    reinterpret_cast<void**>(&pSaveDialogCustomize)))
            {
                pSaveDialog->Unadvise(dwEventsCookie);
                pSaveDialogEvents->Release();
                pSaveDialog->Release();
                freeFileDialogFileTypes(nFileTypes, &pFileTypes);
                return FALSE;
            }

            // add a label next to the edit field, so put it in a group.
            const std::wstring wOptionLabelText(L"GBM Parameter:");
            if ((S_OK != pSaveDialogCustomize->StartVisualGroup(CID_GBM_OPTIONS_LABEL, wOptionLabelText.c_str())) ||
                // Add the Edit field:
                (S_OK != pSaveDialogCustomize->AddEditBox(CID_GBM_OPTIONS_EDIT, chosenOptions.c_str())) ||
                (S_OK != pSaveDialogCustomize->EndVisualGroup()))
            {
                pSaveDialog->Unadvise(dwEventsCookie);
                pSaveDialogEvents->Release();
                pSaveDialogCustomize->Release();
                pSaveDialog->Release();
                freeFileDialogFileTypes(nFileTypes, &pFileTypes);
                return FALSE;
            }

            pSaveDialogCustomize->Release(); pSaveDialogCustomize = NULL;
        }

        // Show the dialog
        if (S_OK != pSaveDialog->Show(hwndOwner))
        {
            pSaveDialog->Unadvise(dwEventsCookie);
            pSaveDialogEvents->Release();
            pSaveDialog->Release();
            freeFileDialogFileTypes(nFileTypes, &pFileTypes);
            return FALSE;
        }

        // get the selected file type
        fileTypeIndex = ALL_FILES_FILTER_INDEX;
        if (S_OK != pSaveDialog->GetFileTypeIndex(&fileTypeIndex))
        {
            pSaveDialog->Unadvise(dwEventsCookie);
            pSaveDialogEvents->Release();
            pSaveDialog->Release();
            freeFileDialogFileTypes(nFileTypes, &pFileTypes);
            return FALSE;
        }
        // get the chosen filename
        IShellItem * pResult = NULL;
        if (S_OK != pSaveDialog->GetResult(&pResult))
        {
            pSaveDialog->Unadvise(dwEventsCookie);
            pSaveDialogEvents->Release();
            pSaveDialog->Release();
            freeFileDialogFileTypes(nFileTypes, &pFileTypes);
            return FALSE;
        }
        WCHAR * pszPath = NULL;
        if (S_OK != pResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))
        {
            pResult->Release();
            pSaveDialog->Unadvise(dwEventsCookie);
            pSaveDialogEvents->Release();
            pSaveDialog->Release();
            freeFileDialogFileTypes(nFileTypes, &pFileTypes);
            return FALSE;
        }

        resultFilename = pszPath;
        CoTaskMemFree(pszPath); pszPath = NULL;
        pResult->Release();     pResult = NULL;

        // Get the option field text
        {
            const DialogEventHandler * pSaveDialogEventsImpl =
                dynamic_cast< const DialogEventHandler * >(pSaveDialogEvents);
            if (pSaveDialogEventsImpl)
            {
                resultOptions = pSaveDialogEventsImpl->getOptionsText();
                pSaveDialogEventsImpl = NULL;
            }
        }

        pSaveDialog->Unadvise(dwEventsCookie);
        pSaveDialogEvents->Release();
        pSaveDialog->Release();
        freeFileDialogFileTypes(nFileTypes, &pFileTypes);
    }
    else
#endif
    {
        // ---------------------------
        // Fallback to old file dialog
        // ---------------------------
        std::wstring formatFilter;
        const int nFilter = createFileDialogFileTypeString(mGbmAccessor,
                                                           mTitleAllFiles.c_str(),
                                                           imageBpp,
                                                           formatFilter);
        if (nFilter < 1)
        {
            return FALSE;
        }

        static const size_t MAX_BUFFER_CHARS = _MAX_PATH;
        wchar_t fileNameBuffer[MAX_BUFFER_CHARS + 1] = { 0 };
        OPENFILENAME fileNameStruct = { 0 };

        // set initial fields
        wcsncpy(fileNameBuffer, chosenFilename.c_str(), MIN(chosenFilename.length(), MAX_BUFFER_CHARS));

        fileNameStruct.lStructSize       = sizeof(OPENFILENAME);
        fileNameStruct.hwndOwner         = hwndOwner;
        fileNameStruct.hInstance         = mModule;
        fileNameStruct.lpstrFilter       = formatFilter.c_str();
        fileNameStruct.lpstrCustomFilter = NULL;
        fileNameStruct.nMaxCustFilter    = 0;
        fileNameStruct.nFilterIndex      = ALL_FILES_FILTER_INDEX; // All Files
        fileNameStruct.lpstrFile         = fileNameBuffer;
        fileNameStruct.nMaxFile          = MAX_BUFFER_CHARS;
        fileNameStruct.lpstrFileTitle    = NULL;
        fileNameStruct.nMaxFileTitle     = 0;
        fileNameStruct.lpstrInitialDir   = NULL;
        fileNameStruct.lpstrTitle        = mTitle.c_str();
        fileNameStruct.Flags             = OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN |
                                           OFN_PATHMUSTEXIST   | OFN_LONGNAMES | OFN_ENABLESIZING;

        // Display the dialog and get the file
        if (! GetSaveFileName(&fileNameStruct))
        {
            return FALSE;
        }
        resultFilename = fileNameStruct.lpstrFile;
        fileTypeIndex  = fileNameStruct.nFilterIndex;
        resultOptions.clear(); // no UI field in this mode for this
    }

    // check if the format type can be determined
    if (splitFullPath(resultFilename, wdocFoldername, wdocFilename))
    {
        int ft;
        const std::string cdocFilename(wstring2string(wdocFilename));
        if ((GBM_ERR_OK != mGbmAccessor.Gbm_guess_filetype(cdocFilename.c_str(), &ft)) &&
            (fileTypeIndex > ALL_FILES_FILTER_INDEX))
        {
            // add the first extension from the selected file format
            GBMFT gbmft;
            
            int n_ft = 0;
            mGbmAccessor.Gbm_query_n_filetypes(&n_ft);
            
            BOOL found = FALSE;
            int fileTypeIndexMod = ALL_FILES_FILTER_INDEX;
            for (ft = 0; ft < n_ft; ft++)
            {
                mGbmAccessor.Gbm_query_filetype(ft, &gbmft);
                if (isFileTypeWriteSupported(gbmft, imageBpp))
                {
                    ++fileTypeIndexMod;
                    if (fileTypeIndexMod == fileTypeIndex)
                    {
                        found = TRUE;
                        break;
                    }
                }
            }
            if (found)
            {
                resultFilename.append(1, '.');
                const char * firstExtEnd = strchr(gbmft.extensions, ' ');

                const std::wstring wExt(string2wstring(gbmft.extensions));
                if (firstExtEnd)
                {
                    resultFilename.append(wExt.c_str(), firstExtEnd - gbmft.extensions);
                }
                else
                {
                    resultFilename.append(wExt.c_str());
                }
            }
        }
    }

    chosenFilename = resultFilename;
    chosenOptions  = resultOptions;
    return TRUE;
}

