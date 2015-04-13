//********************************************************************************
// Mozilla GBM plug-in: GbmSaveFileDialog.hpp (Windows)
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

#ifndef __CLASS_GBM_FILE_SAVE_DIALOG__
    #define __CLASS_GBM_FILE_SAVE_DIALOG__

#include <string>
#include "GbmAccessor.hpp"

// ----------------------------

class GbmSaveFileDialog
{
    public:

        GbmSaveFileDialog(const HMODULE        hMod,
                          const GbmAccessor  & gbmAccessor,
                          const std::wstring & title,
                          const std::wstring & titleAllFilesType);
        ~GbmSaveFileDialog();

        // functions
        BOOL show(const HWND     hwndOwner,
                  const int      imageBpp,
                  std::wstring & chosenFilename,
                  std::wstring & chosenOptions) const;

    // ----------------------------

    private:

        const std::wstring mTitle;
        const std::wstring mTitleAllFiles;

        const HMODULE       mModule;
        const GbmAccessor & mGbmAccessor;
};

#endif


