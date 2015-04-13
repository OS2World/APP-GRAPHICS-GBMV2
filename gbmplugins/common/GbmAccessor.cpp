
//***************************************************
// Generalised Bitmap Module C++ adapter for GBM plugins
//
// Requires: GBM.DLL version 1.35 or higher (with multipage support)
//
// Copyright (C) 2006-2009 Heiko Nitzsche
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

#if defined(__OS2__)
  #define INCL_DOS
  #define INCL_DOSERRORS
  #include <os2.h>
#elif defined(WIN32)
  #define UNICODE
  #include <string>
  #include <string.h>
  #include <windows.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "GbmAccessor.hpp"

// ---------------------------------------------------------

GbmAccessor::GbmAccessor()
    : moduleName(NULL),
      moduleHandle(0),
      mIsInternalModule(FALSE),
      fn_Gbm_init(NULL),
      fn_Gbm_deinit(NULL),
      fn_Gbm_version(NULL),
      fn_Gbm_err(NULL),
      fn_Gbm_io_open(NULL),
      fn_Gbm_io_create(NULL),
      fn_Gbm_io_close(NULL),
      fn_Gbm_io_lseek(NULL),
      fn_Gbm_io_read(NULL),
      fn_Gbm_io_write(NULL),
      fn_Gbm_io_setup(NULL),
      fn_Gbm_restore_io_setup(NULL),
      fn_Gbm_query_n_filetypes(NULL),
      fn_Gbm_guess_filetype(NULL),
      fn_Gbm_query_filetype(NULL),
      fn_Gbm_read_header(NULL),
      fn_Gbm_read_palette(NULL),
      fn_Gbm_read_data(NULL),
      fn_Gbm_write(NULL)
{
}

// ---------------------------------------------------------

GbmAccessor::~GbmAccessor()
{
    dispose();
}

// ---------------------------------------------------------

#if defined(__OS2__)
const char * GbmAccessor::getModuleName() const
#elif defined(WIN32)
const wchar_t * GbmAccessor::getModuleName() const
#endif
{
    return moduleName;
}

// ---------------------------------------------------------

void GbmAccessor::dispose()
{
    if (moduleHandle != 0)
    {
        if (fn_Gbm_deinit != NULL)
        {
            fn_Gbm_deinit();
        }
        if (mIsInternalModule)
        {
          #if defined(__OS2__)
            DosFreeModule(moduleHandle);
          #elif defined(WIN32)
            FreeLibrary(moduleHandle);
          #endif
        }
        moduleHandle = 0;
    }
    mIsInternalModule = FALSE;

    delete [] moduleName;
    moduleName = NULL;

    fn_Gbm_version           = NULL;
    fn_Gbm_init              = NULL;
    fn_Gbm_deinit            = NULL;
    fn_Gbm_io_open           = NULL;
    fn_Gbm_io_create         = NULL;
    fn_Gbm_io_close          = NULL;
    fn_Gbm_io_lseek          = NULL;
    fn_Gbm_io_read           = NULL;
    fn_Gbm_io_write          = NULL;
    fn_Gbm_io_setup          = NULL;
    fn_Gbm_restore_io_setup  = NULL;
    fn_Gbm_query_n_filetypes = NULL;
    fn_Gbm_guess_filetype    = NULL;
    fn_Gbm_query_filetype    = NULL;
    fn_Gbm_read_header       = NULL;
    fn_Gbm_read_palette      = NULL;
    fn_Gbm_read_data         = NULL;
    fn_Gbm_write             = NULL;
    fn_Gbm_err               = NULL;
}

// ---------------------------------------------------------

#if defined(__OS2__)
gbm_boolean GbmAccessor::init(const char * dllPath)
#elif defined(WIN32)
gbm_boolean GbmAccessor::init(const wchar_t * dllPath)
#endif
{
    if (dllPath == NULL)
    {
        return GBM_FALSE;
    }

    // cleanup first in case of double calls
    dispose();

    // load GBM.DLL

    /* load the module */
    #if defined(__OS2__) // ----------------------------------------------
    {
      UCHAR   loadError[32] = "";
      HMODULE hmod = NULLHANDLE;

      APIRET rc = DosLoadModule((PSZ)loadError, sizeof(loadError)-1,
                                (PSZ)dllPath, &hmod);
      if (rc != NO_ERROR)
      {
        return GBM_FALSE;
      }
      // mark the module as internally managed
      mIsInternalModule = TRUE;
      if (! initFromModule(hmod))
      {
        return GBM_FALSE;
      }
    }
    #elif defined(WIN32) // ----------------------------------------------
    {
      HMODULE hmod = LoadLibrary(dllPath);
      if (hmod == 0)
      {
        return GBM_FALSE;
      }
      // mark the module as internally managed
      mIsInternalModule = TRUE;
      if (! initFromModule(hmod))
      {
        return GBM_FALSE;
      }
    }
    #else
     return GBM_FALSE;
    #endif

    return GBM_TRUE;
}

// ---------------------------------------------------------

gbm_boolean GbmAccessor::init(HMODULE hmod)
{
    // cleanup first in case of double calls
    dispose();
    mIsInternalModule = FALSE;
    return initFromModule(hmod);
}

// ---------------------------------------------------------

gbm_boolean GbmAccessor::initFromModule(HMODULE hmod)
{
    moduleHandle = hmod;

    #if defined(__OS2__) // ----------------------------------------------
    {
         CHAR  foundModuleName[_MAX_PATH] = { 0 };
        ULONG  foundModuleNameLength = sizeof(foundModuleName);
        APIRET rc = NO_ERROR;

        // get the full path name
        rc = DosQueryModuleName(moduleHandle,
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
            moduleName = new char[moduleNameLength + 1];
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
        strcpy(moduleName, foundModuleName);

        // as _System exported functions
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_version", (PFN *) &fn_Gbm_version);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_init"   , (PFN *) &fn_Gbm_init);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_deinit" , (PFN *) &fn_Gbm_deinit);

        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_io_open"  , (PFN *) &fn_Gbm_io_open);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_io_create", (PFN *) &fn_Gbm_io_create);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_io_close" , (PFN *) &fn_Gbm_io_close);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_io_lseek" , (PFN *) &fn_Gbm_io_lseek);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_io_read"  , (PFN *) &fn_Gbm_io_read);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_io_write" , (PFN *) &fn_Gbm_io_write);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"gbm_io_setup" , (PFN *) &fn_Gbm_io_setup);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"gbm_restore_io_setup" , (PFN *) &fn_Gbm_restore_io_setup);

        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_query_n_filetypes", (PFN *) &fn_Gbm_query_n_filetypes);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_guess_filetype"   , (PFN *) &fn_Gbm_guess_filetype);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_query_filetype"   , (PFN *) &fn_Gbm_query_filetype);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_read_header"      , (PFN *) &fn_Gbm_read_header);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_read_palette"     , (PFN *) &fn_Gbm_read_palette);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_read_data"        , (PFN *) &fn_Gbm_read_data);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_write"            , (PFN *) &fn_Gbm_write);
        rc |= DosQueryProcAddr(moduleHandle, 0L, (PSZ)"Gbm_err"              , (PFN *) &fn_Gbm_err);

        if (rc)
        {
            dispose();
            return GBM_FALSE;
        }

        // optional functions that have default operation
        rc = DosQueryProcAddr(moduleHandle, 0L, (PSZ)"gbm_read_imgcount", (PFN *) &fn_Gbm_read_imgcount);
        if (rc)
        {
            fn_Gbm_read_imgcount = NULL;
        }
    }
    #elif defined(WIN32) // ----------------------------------------------
    {
        wchar_t foundModuleName[_MAX_PATH + 1] = { 0 };
        DWORD foundModuleNameLength = _MAX_PATH;
        DWORD rc = 0;

        // get the full path name
        rc = GetModuleFileName(moduleHandle,
                               foundModuleName,
                               foundModuleNameLength);
        if ((rc >= foundModuleNameLength) &&
            (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        {
          dispose();
          return GBM_FALSE;
        }
        foundModuleName[rc] = 0;
        if (rc < 1)
        {
          dispose();
          return GBM_FALSE;
        }

        const size_t moduleNameLength = wcslen(foundModuleName);
        try
        {
            moduleName = new wchar_t[moduleNameLength + 1];
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
        wcscpy(moduleName, foundModuleName);

        // as __stdcall exported functions
#if defined(_MSC_VER) && defined(_M_X64)
        fn_Gbm_version = (tfn_Gbm_version) GetProcAddress(moduleHandle, "gbm_version");
        fn_Gbm_init    = (tfn_Gbm_init)    GetProcAddress(moduleHandle, "gbm_init");
        fn_Gbm_deinit  = (tfn_Gbm_deinit)  GetProcAddress(moduleHandle, "gbm_deinit");

        fn_Gbm_io_open   = (tfn_Gbm_io_open)   GetProcAddress(moduleHandle, "gbm_io_open");
        fn_Gbm_io_create = (tfn_Gbm_io_create) GetProcAddress(moduleHandle, "gbm_io_create");
        fn_Gbm_io_close  = (tfn_Gbm_io_close)  GetProcAddress(moduleHandle, "gbm_io_close");
        fn_Gbm_io_lseek  = (tfn_Gbm_io_lseek)  GetProcAddress(moduleHandle, "gbm_io_lseek");
        fn_Gbm_io_read   = (tfn_Gbm_io_read)   GetProcAddress(moduleHandle, "gbm_io_read");
        fn_Gbm_io_write  = (tfn_Gbm_io_write)  GetProcAddress(moduleHandle, "gbm_io_write");
        fn_Gbm_io_setup  = (tfn_Gbm_io_setup)  GetProcAddress(moduleHandle, "gbm_io_setup");
        fn_Gbm_restore_io_setup = (tfn_Gbm_restore_io_setup)  GetProcAddress(moduleHandle, "gbm_restore_io_setup");

        fn_Gbm_query_n_filetypes = (tfn_Gbm_query_n_filetypes) GetProcAddress(moduleHandle, "gbm_query_n_filetypes");
        fn_Gbm_guess_filetype    = (tfn_Gbm_guess_filetype)    GetProcAddress(moduleHandle, "gbm_guess_filetype");
        fn_Gbm_query_filetype    = (tfn_Gbm_query_filetype)    GetProcAddress(moduleHandle, "gbm_query_filetype");
        fn_Gbm_read_header       = (tfn_Gbm_read_header)       GetProcAddress(moduleHandle, "gbm_read_header");
        fn_Gbm_read_palette      = (tfn_Gbm_read_palette)      GetProcAddress(moduleHandle, "gbm_read_palette");
        fn_Gbm_read_data         = (tfn_Gbm_read_data)         GetProcAddress(moduleHandle, "gbm_read_data");
        fn_Gbm_write             = (tfn_Gbm_write)             GetProcAddress(moduleHandle, "gbm_write");
        fn_Gbm_err               = (tfn_Gbm_err)               GetProcAddress(moduleHandle, "gbm_err");
#else
        fn_Gbm_version = (tfn_Gbm_version) GetProcAddress(moduleHandle, "_gbm_version@0");
        fn_Gbm_init    = (tfn_Gbm_init)    GetProcAddress(moduleHandle, "_gbm_init@0");
        fn_Gbm_deinit  = (tfn_Gbm_deinit)  GetProcAddress(moduleHandle, "_gbm_deinit@0");

        fn_Gbm_io_open   = (tfn_Gbm_io_open)   GetProcAddress(moduleHandle, "_gbm_io_open@8");
        fn_Gbm_io_create = (tfn_Gbm_io_create) GetProcAddress(moduleHandle, "_gbm_io_create@8");
        fn_Gbm_io_close  = (tfn_Gbm_io_close)  GetProcAddress(moduleHandle, "_gbm_io_close@4");
        fn_Gbm_io_lseek  = (tfn_Gbm_io_lseek)  GetProcAddress(moduleHandle, "_gbm_io_lseek@12");
        fn_Gbm_io_read   = (tfn_Gbm_io_read)   GetProcAddress(moduleHandle, "_gbm_io_read@12");
        fn_Gbm_io_write  = (tfn_Gbm_io_write)  GetProcAddress(moduleHandle, "_gbm_io_write@12");
        fn_Gbm_io_setup  = (tfn_Gbm_io_setup)  GetProcAddress(moduleHandle, "_gbm_io_setup@24");
        fn_Gbm_restore_io_setup = (tfn_Gbm_restore_io_setup)  GetProcAddress(moduleHandle, "_gbm_restore_io_setup@0");

        fn_Gbm_query_n_filetypes = (tfn_Gbm_query_n_filetypes) GetProcAddress(moduleHandle, "_gbm_query_n_filetypes@4");
        fn_Gbm_guess_filetype    = (tfn_Gbm_guess_filetype)    GetProcAddress(moduleHandle, "_gbm_guess_filetype@8");
        fn_Gbm_query_filetype    = (tfn_Gbm_query_filetype)    GetProcAddress(moduleHandle, "_gbm_query_filetype@8");
        fn_Gbm_read_header       = (tfn_Gbm_read_header)       GetProcAddress(moduleHandle, "_gbm_read_header@20");
        fn_Gbm_read_palette      = (tfn_Gbm_read_palette)      GetProcAddress(moduleHandle, "_gbm_read_palette@16");
        fn_Gbm_read_data         = (tfn_Gbm_read_data)         GetProcAddress(moduleHandle, "_gbm_read_data@16");
        fn_Gbm_write             = (tfn_Gbm_write)             GetProcAddress(moduleHandle, "_gbm_write@28");
        fn_Gbm_err               = (tfn_Gbm_err)               GetProcAddress(moduleHandle, "_gbm_err@4");
#endif

        if ((fn_Gbm_version           == NULL) ||
            (fn_Gbm_init              == NULL) ||
            (fn_Gbm_deinit            == NULL) ||
            (fn_Gbm_io_open           == NULL) ||
            (fn_Gbm_io_create         == NULL) ||
            (fn_Gbm_io_close          == NULL) ||
            (fn_Gbm_io_lseek          == NULL) ||
            (fn_Gbm_io_read           == NULL) ||
            (fn_Gbm_io_write          == NULL) ||
            (fn_Gbm_io_setup          == NULL) ||
            (fn_Gbm_restore_io_setup  == NULL) ||
            (fn_Gbm_query_n_filetypes == NULL) ||
            (fn_Gbm_guess_filetype    == NULL) ||
            (fn_Gbm_query_filetype    == NULL) ||
            (fn_Gbm_read_header       == NULL) ||
            (fn_Gbm_read_palette      == NULL) ||
            (fn_Gbm_read_data         == NULL) ||
            (fn_Gbm_write             == NULL) ||
            (fn_Gbm_err               == NULL))
        {
            dispose();
            return GBM_FALSE;
        }

        // optional functions that have default operation
#if defined(_MSC_VER) && defined(_M_X64)
        fn_Gbm_read_imgcount = (tfn_Gbm_read_imgcount) GetProcAddress(moduleHandle, "gbm_read_imgcount");
#else
        fn_Gbm_read_imgcount = (tfn_Gbm_read_imgcount) GetProcAddress(moduleHandle, "_gbm_read_imgcount@16");
#endif
    }
    #endif

    fn_Gbm_init();
    return GBM_TRUE;
}

// ---------------------------------------------------------

int GbmAccessor::Gbm_version() const
{
    assert(fn_Gbm_version != NULL);
    return fn_Gbm_version();
}

// ---------------------------------------------------------

int GbmAccessor::Gbm_io_open(const char *fn, int mode) const
{
    assert(fn_Gbm_io_open != NULL);
    return fn_Gbm_io_open(fn, mode);
}

// ---------------------------------------------------------

int GbmAccessor::Gbm_io_create(const char *fn, int mode) const
{
    assert(fn_Gbm_io_create != NULL);
    return fn_Gbm_io_create(fn, mode);
}

// ---------------------------------------------------------

void GbmAccessor::Gbm_io_close(int fd) const
{
    assert(fn_Gbm_io_close != NULL);
    fn_Gbm_io_close(fd);
}

// ---------------------------------------------------------

long GbmAccessor::Gbm_io_lseek(int fd, long  pos, int whence) const
{
    assert(fn_Gbm_io_lseek != NULL);
    return fn_Gbm_io_lseek(fd, pos, whence);
}

// ---------------------------------------------------------

int GbmAccessor::Gbm_io_read(int fd, void *buf, int len) const
{
    assert(fn_Gbm_io_read != NULL);
    return fn_Gbm_io_read(fd, buf, len);
}

// ---------------------------------------------------------

int  GbmAccessor::Gbm_io_write(int fd, const void *buf, int len) const
{
    assert(fn_Gbm_io_write != NULL);
    return fn_Gbm_io_write(fd, buf, len);
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_io_setup(
    int  (GBMENTRYP open  )(const char *fn, int mode),
    int  (GBMENTRYP create)(const char *fn, int mode),
    void (GBMENTRYP close )(int fd),
    long (GBMENTRYP lseek )(int fd, long pos, int whence),
    int  (GBMENTRYP read  )(int fd, void *buf, int len),
    int  (GBMENTRYP write )(int fd, const void *buf, int len))
{
    assert(fn_Gbm_io_setup != NULL);
    return fn_Gbm_io_setup(open, create, close, lseek, read, write);
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_restore_io_setup()
{
    assert(fn_Gbm_restore_io_setup != NULL);
    return fn_Gbm_restore_io_setup();
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_query_n_filetypes(int *n_ft) const
{
    assert(fn_Gbm_query_n_filetypes != NULL);
    return fn_Gbm_query_n_filetypes(n_ft);
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_guess_filetype(const char *fn, int *ft) const
{
    assert(fn_Gbm_guess_filetype != NULL);
    return fn_Gbm_guess_filetype(fn, ft);
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_query_filetype(int ft, GBMFT *gbmft) const
{
    assert(fn_Gbm_query_filetype != NULL);
    return fn_Gbm_query_filetype(ft, gbmft);
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_read_imgcount(const char *fn, int fd, int ft, int *pimgcnt) const
{
    if (fn_Gbm_read_imgcount != NULL)
    {
        return fn_Gbm_read_imgcount(fn, fd, ft, pimgcnt);
    }
    return 1; // simulate old behaviour (GBM versions before 1.35)
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_read_header(const char *fn, int fd, int ft, GBM *gbm, const char *opt) const
{
    assert(fn_Gbm_read_header != NULL);
    return fn_Gbm_read_header(fn, fd, ft, gbm, opt);
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_read_palette(int fd, int ft, GBM *gbm, GBMRGB *gbmrgb) const
{
    assert(fn_Gbm_read_palette != NULL);
    return fn_Gbm_read_palette(fd, ft, gbm, gbmrgb);
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_read_data(int fd, int ft, GBM *gbm, gbm_u8 *data) const
{
    assert(fn_Gbm_read_data != NULL);
    return fn_Gbm_read_data(fd, ft, gbm, data);
}

// ---------------------------------------------------------

GBM_ERR GbmAccessor::Gbm_write(const char *fn, int fd, int ft, const GBM *gbm, const GBMRGB *gbmrgb, const gbm_u8 *data, const char *opt) const
{
    assert(fn_Gbm_write != NULL);
    return fn_Gbm_write(fn, fd, ft, gbm, gbmrgb, data, opt);
}

// ---------------------------------------------------------

const char * GbmAccessor::Gbm_err(GBM_ERR rc) const
{
    assert(fn_Gbm_err != NULL);
    return fn_Gbm_err(rc);
}


