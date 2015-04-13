
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

#ifndef __CLASS_GBM_ACCESSOR__
    #define __CLASS_GBM_ACCESSOR__

#include "gbm.h"

#if defined(__OS2__)
  #include <os2.h>
#elif defined(WIN32)
  #include <windows.h>
#endif

// ---------------------------------------------------------

class GbmAccessor
{
    private:
        // --- Function prototype for DLL functions ---

        typedef GBM_ERR      (GBMENTRYP_SYS tfn_Gbm_init)    (void);
        typedef GBM_ERR      (GBMENTRYP_SYS tfn_Gbm_deinit)  (void);
        typedef GBM_ERR      (GBMENTRYP_SYS tfn_Gbm_version) (void);
        typedef const char * (GBMENTRYP_SYS tfn_Gbm_err)     (GBM_ERR rc);

        typedef int     (GBMENTRYP_SYS tfn_Gbm_io_open)  (const char *fn, int mode);
        typedef int     (GBMENTRYP_SYS tfn_Gbm_io_create)(const char *fn, int mode);
        typedef void    (GBMENTRYP_SYS tfn_Gbm_io_close) (int fd);
        typedef long    (GBMENTRYP_SYS tfn_Gbm_io_lseek) (int fd, long  pos, int whence);
        typedef int     (GBMENTRYP_SYS tfn_Gbm_io_read)  (int fd, void *buf, int len);
        typedef int     (GBMENTRYP_SYS tfn_Gbm_io_write) (int fd, const void *buf, int len);
        typedef GBM_ERR (GBMENTRYP     tfn_Gbm_io_setup) (int  (GBMENTRYP)(const char *fn, int mode),
                                                          int  (GBMENTRYP)(const char *fn, int mode),
                                                          void (GBMENTRYP)(int fd),
                                                          long (GBMENTRYP)(int fd, long pos, int whence),
                                                          int  (GBMENTRYP)(int fd, void *buf, int len),
                                                          int  (GBMENTRYP)(int fd, const void *buf, int len));
        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_restore_io_setup)(void);

        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_query_n_filetypes)(int *n_ft);
        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_guess_filetype)   (const char *fn, int *ft);
        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_query_filetype)   (int ft, GBMFT *gbmft);

        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_read_imgcount)(const char *fn, int fd, int ft, int *pimgcnt);
        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_read_header)  (const char *fn, int fd, int ft, GBM *gbm, const char *opt);
        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_read_palette) (int fd, int ft, GBM *gbm, GBMRGB *gbmrgb);
        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_read_data)    (int fd, int ft, GBM *gbm, gbm_u8 *data);
        typedef GBM_ERR (GBMENTRYP_SYS tfn_Gbm_write)        (const char *fn, int fd, int ft, const GBM *gbm, const GBMRGB *gbmrgb, const gbm_u8 *data, const char *opt);

        // ---------------------------------------------------------
    
       #if defined(__OS2__)
        char    * moduleName;
       #elif defined(WIN32)
        wchar_t * moduleName;
       #endif
        HMODULE moduleHandle;
        BOOL    mIsInternalModule;

        GbmAccessor(const GbmAccessor & accessor);
        GbmAccessor & operator =(const GbmAccessor & accessor);

        gbm_boolean initFromModule(HMODULE hmod);

        tfn_Gbm_init    fn_Gbm_init;
        tfn_Gbm_deinit  fn_Gbm_deinit;
        tfn_Gbm_version fn_Gbm_version;
        tfn_Gbm_err     fn_Gbm_err;

        tfn_Gbm_io_open   fn_Gbm_io_open;
        tfn_Gbm_io_create fn_Gbm_io_create;
        tfn_Gbm_io_close  fn_Gbm_io_close;
        tfn_Gbm_io_lseek  fn_Gbm_io_lseek;
        tfn_Gbm_io_read   fn_Gbm_io_read;
        tfn_Gbm_io_write  fn_Gbm_io_write;
        tfn_Gbm_io_setup  fn_Gbm_io_setup;
        tfn_Gbm_restore_io_setup fn_Gbm_restore_io_setup;

        tfn_Gbm_query_n_filetypes fn_Gbm_query_n_filetypes;
        tfn_Gbm_guess_filetype    fn_Gbm_guess_filetype;
        tfn_Gbm_query_filetype    fn_Gbm_query_filetype;

        tfn_Gbm_read_imgcount fn_Gbm_read_imgcount;
        tfn_Gbm_read_header   fn_Gbm_read_header;
        tfn_Gbm_read_palette  fn_Gbm_read_palette;
        tfn_Gbm_read_data     fn_Gbm_read_data;
        tfn_Gbm_write         fn_Gbm_write;

    // ---------------------------------------------------------
    public:

        GbmAccessor();
        virtual ~GbmAccessor();

        // Loads the dll and initializes the accessor
       #if defined(__OS2__)
        virtual gbm_boolean init(const char * dllPath);
       #elif defined(WIN32)
        virtual gbm_boolean init(const wchar_t * dllPath);
       #endif

        // Initializes the accessor from already loaded module
        virtual gbm_boolean init(HMODULE hmod);

        // Unloads the dll and uninitializes the accessor
        virtual void dispose();

       #if defined(__OS2__)
        virtual const char * getModuleName() const;
       #elif defined(WIN32)
        virtual const wchar_t * getModuleName() const;
       #endif

        // GBM wrapper functions

        virtual int          Gbm_version()       const;
        virtual const char * Gbm_err(GBM_ERR rc) const;

        virtual int  Gbm_io_open  (const char *fn, int mode) const;
        virtual int  Gbm_io_create(const char *fn, int mode) const;
        virtual void Gbm_io_close (int fd) const;
        virtual long Gbm_io_lseek (int fd, long  pos, int whence) const;
        virtual int  Gbm_io_read  (int fd, void *buf, int len) const;
        virtual int  Gbm_io_write (int fd, const void *buf, int len) const;

        virtual GBM_ERR Gbm_io_setup(int  (GBMENTRYP open  )(const char *fn, int mode),
                                     int  (GBMENTRYP create)(const char *fn, int mode),
                                     void (GBMENTRYP close )(int fd),
                                     long (GBMENTRYP lseek )(int fd, long pos, int whence),
                                     int  (GBMENTRYP read  )(int fd, void *buf, int len),
                                     int  (GBMENTRYP write )(int fd, const void *buf, int len));
        virtual GBM_ERR Gbm_restore_io_setup();

        virtual GBM_ERR Gbm_query_n_filetypes(int *n_ft) const;
        virtual GBM_ERR Gbm_guess_filetype   (const char *fn, int *ft) const;
        virtual GBM_ERR Gbm_query_filetype   (int ft, GBMFT *gbmft) const;

        virtual GBM_ERR Gbm_read_imgcount(const char *fn, int fd, int ft, int *pimgcnt) const;
        virtual GBM_ERR Gbm_read_header  (const char *fn, int fd, int ft, GBM *gbm, const char *opt) const;
        virtual GBM_ERR Gbm_read_palette (int fd, int ft, GBM *gbm, GBMRGB *gbmrgb) const;
        virtual GBM_ERR Gbm_read_data    (int fd, int ft, GBM *gbm, gbm_u8 *data) const;
        virtual GBM_ERR Gbm_write        (const char *fn, int fd, int ft, const GBM *gbm, const GBMRGB *gbmrgb, const gbm_u8 *data, const char *opt) const;
};

#endif


