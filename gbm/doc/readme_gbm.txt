
GBM.DLL 1.76 and GBM command line tools
=======================================
(28-Oct-2012, Heiko Nitzsche)

Since the powerful GBM.DLL and the GBM command line tools were no longer
actively maintained for OS/2 by the initial author anymore, I took over this
task and also did several enhancements (e.g. PNG support, extended TIF support).
Meanwhile also extended versions for Windows and Linux are available.

The original packages (GBM.ZIP, GBMSRC.ZIP) can be found on the homepage of
the initial author Andy Key (http://www.nyangau.fsnet.co.uk) or on Hobbes.
If you have installed the packages just replace the installed files with the
ones from this archive.

Enhancements included
---------------------
Sources can be compiled with:
- OS/2    (32 bit)   : OpenWatcom 1.8, GCC 3.3.5, IBM VAC++ 3.08
- Windows (32/64 bit): OpenWatcom 1.8 (32bit), Microsoft Visual C++ (32/64bit)
- Linux   (32/64 bit): GCC 4.x

GBM.DLL:
- GBM.DLL has been updated to the latest level available for other platforms
- updated JPG support (via IJG library 8d)
  (reads also YCCK and CMYK encoded JPEGs, writes grayscale JPEGs (8bpp),
   some new write options are available like smoothing)
- extended PNG support (via Libpng, APNG patch and Zlib)
  (colour depths up to 64 bpp, alpha channel is supported)
- extended TIF support (via libtiff, IJG and Zlib)
  (colour depths up to 64 bpp, alpha channel is supported)
- extended BMP support (16 bpp, 32 bpp Windows RGB and BITFIELDS bitmaps)
- extended PBM support (1 bpp, binary type P4, ASCII type P1, multipage P4 PBMs)
- extended PGM support (8/16 bpp, binary type P5, ASCII type P2, multipage P5 PGMs)
- extended PPM support (24/48 bpp RGB bitmaps, binary type P6, ASCII type P3, multipage P6 PPMs)
- extended PNM support (same as PBM/PGM/PPM, multipage PNMs)
- extended TGA support (1,4,8,16,24,32bpp (24bpp + alpha channel), reads horizontal flipped TGAs)
- extended JPG support (reads 8(Grey)/24 (YUV)/32bpp (YCCK, CMYK, writes 8(Grey)/24(YUV) bpp)
- extended JPEG2000 support (reads 8(Grey)/24/48(YUV,RGB) bpp, writes 8(Grey)/24/48(RGB) bpp)
- extended XPM support (XPM2/XPM3 format only, not XPM1 or XPM2C)
- extended JBIG support (via JBIG-Kit 2.0)
  (grayscale 1bpp and 8bpp so far only)
- extended RAW support (via libraw, based on dcraw)
- API extension: query the number of pages of a multipage bitmap

GBM command line tools:
- simple bitmap converter gbmconv added
- gbmhdr can now show per page properties of multipage bitmap files
- gbmgamma, gbmhdr, gbmref, gbmsize and gbmsub now can handle bitmaps
  up to 64 bpp with alpha channel
- gbmsize now supports also resampled scaling with several filters
- all command line and PM tools (OS/2) now support filenames with commas


GBM.DLL is still fully backward compatible. So you can safely upgrade your
currently installed version.

By default bitmaps with extended color depths are downsampled to 24 bpp
to keep compatibility with existing applications. To enable the extended
support, set the option ext_bpp.

Example:  gbmsize -a -w 200 infile.png,ext_bpp outfile.tif

          Reads infile.png with the full color depth, resize it and
          save as outfile.tif with full color depth.

Please have a look at the GBM documentation for more options and description
of alpha channel handling (see below).

I strongly recommend to replace the old command line tools as well as the
GBM PM tools (OS/2) together with this new GBM.DLL because some of them will crash
when trying to read bitmaps with extended color depth (option ext_bpp specified).

GBM.DLL might be usable for other applications as well. An example is Mesa/2
which will use GBM.DLL, GBMDLG.DLL and GBMDLG.HLP (last 2 from GBMV2 package)
for importing bitmaps when they are found in it's installation directory.

Most applications (like Mesa/2) will not make use of the extended color
depths because this is a new feature of recent GBM.DLL versions.
But because GBM.DLL provides extended bitmaps by default in a backward
compatible format (may result in reduced bitmap quality), these applications
will still be able to use the new GBM.DLL as usual.
Except when using the provided command line tools, mainly application
developers can fully make use them.


Installation
------------
All applications included in this package should be used with the included
GBM.DLL version. So far all programs other than gbmhdr.exe can also be
used with earlier GBM.DLL versions higher than 1.11.

There is no special installation required. Simply copy the
files into an empty directory.

GBM.DLL can be installed at a central location of the system so that
it is found in the paths defined by LIBPATH in CONFIG.SYS. Thus sharing
between all programs of this package and the programs from the gbmos2pm
package is possible.

Note:
 If the GBM command line tools should be run from everywhere and not only
 from the installation directory, it is necessary to add the installation
 directory to some environment variables (OS/2: CONFIG.SYS,
 Windows: Environment Editor).

 The following changes are necessary:
 (It is assumed that the installation directory is D:\GBM)

 1. Add D:\GBM to the end of the line beginning with SET PATH separated
    by ; to the previous parameter.
    (e.g. SET PATH=C:\OS2;D:\GBM)

 2. Add D:\GBM to the end of the line beginning with LIBPATH separated
    by ; to the previous parameter. This is necessary so that the system
    can find GBM.DLL and GBMDLG.DLL.
    (e.g. LIBPATH=.;C:\OS2\DLL;D:\GBM)

 3. Reboot.

 4. For this step you need gbmver.exe. It is included in the GBM package.

    Open a command line and go to a directory OTHER THAN the installation.
    Then start gbmver.exe as if it would be in this directory.
    It should report the location and the version of GBM.DLL.
    Verify that the reported version is the installed version. If the
    reported version is found in a different path, do a file search
    for GBM.DLL on your system and check for duplicates or older versions.
    If an older version is found (version must be higher than 1.11 !!!),
    you can replace it with the GBM.DLL from this package.
    Alternatively modify the LIBPATH statement in CONFIG.SYS so that
    the correct path to GBM.DLL is placed before all other versions.
    After the changes are done, check again with gbmver.exe.


Notes for eCS 1.2/1.2R/2.0 users:

eComStation 1.2 and 2.0 ship with an old version of GBM.DLL (1.12) that is
loaded on startup of the WPS already. Thus other applications requiring a
newer version of GBM.DLL cannot use it as the operating system will provide
the functionality of the loaded DLL only.
The good news is that the GBM.DLL shipped with eCS can be safely replaced by
the latest version provided here (or on Hobbes). It doesn't hurt to make a
backup of the old gbm.dll before!
This also has some advantages. Some mechanisms of the eCS 1.2/2.0 WPS will
automatically make use of the enhanced features. Namely the bitmap preview
in the desktop folders will show many more TIF and PNG previews, especially
those with colour depths larger than 24bpp. The bad news is that the MMECS
subsystem does not make full use of the GBM features yet and thus for some
bitmaps previews will be shown but the system provided image viewer cannot
show them.

As an alternative the tool RUN! from Rich Walsh can be used for running
programs using different gbm.dll versions in parallel. Get it from Hobbes.


Detailed documentation
----------------------
A more comprehensive documentation of the tools is available on my GBM homepage
(see below).


Developers
----------
The updated source code of all GBM tools (GBM command line tools, GBM.DLL
together with IJG JPEG library 8d, Libpng 1.5.13, Zlib 1.2.7, Libtiff 3.9.5,
OpenJPEG 1.5.1, LibRaw 0.14.7, GBMV2, GBMV, ...) is available as a separate
package. Makefiles have been revised and tested with IBM Visual Age C++ V3.08,
OpenWatcom 1.8 and GCC 3.3.5 (all OS/2). On Windows OpenWatcom 1.8 and Microsoft
C++ (32/64bit) compilers are supported. On Linux GCC 4.x is supported (32/64bit).

An programming example is included in the user documentation.
You can also have a look at the header file gbm.h and the command line tools.

Note:
Beginning with GBM.DLL version 1.35 the OpenWatcom 1.4 compiler is supported
as well. This required some changes in the API use model to keep binary
backward compatibility with older applications using GBM.DLL.
Please see the change log for details.
OpenWatcom 1.5 is supported beginning with GBM version 1.38.
OpenWatcom 1.6 is supported beginning with GBM version 1.50.
OpenWatcom 1.7 is supported beginning with GBM version 1.53.
OpenWatcom 1.8 is supported beginning with GBM version 1.63.
GCC 3.3.5 is supported for OS/2 beginning with GBM version 1.61.
Microsoft C++ compiler for Windows (32bit) is supported beginning with GBM version 1.62.
GCC 4.x for Linux (32/64bit) is supported beginning with GBM version 1.62.
Microsoft C++ compiler for Windows (32/64bit) is supported beginning with GBM version 1.64.

Please be aware that not all image formats are supported by
IBM VAC builds of gbm.dll due to compiler limitations
(no RAW and JPEG2000 support).


Legal stuff
-----------
The package includes source code provided as Public Domain from Andy Key, the
original author of GBM and the GBM PM tools (http://www.nyangau.org).
For details please also have a look at gbm.htm, gbmsrc.htm and gbmos2pm.htm.

This software is based in part on the work of the Independent JPEG Group.
The source package ships with full source code of the Independent JPEG Group
JPEGLIB version 8d and may be used if they are credited, (which I do so here).

This software is based in part on the Libpng and Zlib libraries.
Both are Open Source and can be found on www.libpng.org and www.zlib.org.
The source package ships with full source code of Libpng 1.5.13 and Zlib 1.2.7.
Some minor adaptions for GBM have been done.

This software is based in part on the Libtiff library.
It is Open Source and can be found on www.libtiff.org.
The source package ships with full source code of Libtiff 3.9.5.
Some minor adaptions for GBM have been done.

This software is based in part on the OpenJPEG library.
It is Open Source and can be found on www.openjpeg.org.
The source package ships with full source code of OpenJPEG 1.5.1.
Some minor adaptions for GBM have been done.

This software is based in part on the LibRaw library.
It is Open Source and can be found on http://www.libraw.org.
The source package ships with full source code of Libraw 0.14.7.
Some minor adaptions for GBM have been done.

This software is based in part on the JBIG-Kit library.
It is Open Source and can be found on http://www.cl.cam.ac.uk/~mgk25.
The source package ships with full source code of the library.
Some minor adaptions for GBM have been done.
Note: Users of Non-GPL compatible software linking against GBM.DLL should study
      potential patenting issues. GBM.DLL can be built without JBIG support by
      disabling it in the main GBM makefile (e.g. gbm\makefile.os2\makefile.wat)
      if necessary.

All enhancements added to the original GBM version are provided as
Public Domain as well.

If you do modifications to the source code and want to publish them, please drop
me a note. This helps keeping track of the current status.

Have fun,
Heiko



Contact
-------
Heiko.Nitzsche@gmx.de (http://heikon.home.tlink.de)



History (since the point in time I took over the GBM tools)
-------

08-Jun-2003: GBM.DLL 1.12
             * Optimized for Pentium and compatibles

             GBM command line tools
             * Optimized for Pentium and compatibles


02-Aug-2005: GBM.DLL 1.13
             * Update to the latest available source of Andy Key

             GBM command line tools
             * Update to the latest available source of Andy Key
             * 4Rx4Gx4B color mapping capability added


24-Aug-2005: GBM.DLL 1.20
             * Added support for reading and writing of PNG bitmaps
               (uses Libpng 1.2.8 and Zlib 1.2.3, statically built)

             GBM command line tools
             * Updated gbm.htm documentation, now PNG support is included


27-Aug-2005: GBM.DLL 1.21 (not public)
             * Fix small memory leak in GBM PNG reader when the bitmap
               data are not loaded.


29-Sep-2005: GBM.DLL 1.30
             * PNG support:
               - Fix some issues in reading transparent bitmaps
               - Extended support for color depths up to 64 bpp with alpha channel
                 (alpha channel is available to the application on request)
             * TIFF support:
               - Original TIF support has been replaced by Libtiff 3.7.3 together
                 with IJG JPEG 6B and ZLib 1.2.3. There are now many compression
                 schemes supported (JPEG, Deflate (ZIP), CCITT, LZW, LZW with predictor, ...).
                 Reading and writing them is possible.
               - Extended support for color depths up to 64 bpp with alpha channel
                 (alpha channel is available to the application on request)

             GBM command line tools:
             * gbmgamma, gbmhdr, gbmref, gbmsize and gbmsub now can handle
               bitmaps up to 64 bpp with alpha channel
             * simple bitmap converter gbmconv added
             * detect unsupported color depths to prevent crashes

             GBM documentation:
             * updated documentation for PNG and TIF bitmaps including all
               available input and output options


02-Oct-2005: GBM.DLL 1.31
             * PPM support:
               - Original PPM support has been replaced to support reading and
                 writing of newer PPM files with 48 bpp
               - Multipage PPMs can now be read

             GBM documentation:
             * updated documentation for PPM bitmaps including all
               available input and output options

05-Jan-2006: GBM.DLL 1.32
             * TIF support:
               - Update to Libtiff 3.7.4
               - Fix decoding of separated RGB tiffs
               - Fix decoding of non-separated CMYK tiffs (8/16 bits per channel)


08-Mar-2006: All binaries are now compiled with OpenWatcom 1.4.

             GBM.DLL 1.35
             * added support for RLE24 compressed bitmaps (BMP)

             GBM command line tools
             * gbmhdr is now able to display all bitmaps of multipage images.
               The reported index can be used to reference a specific bitmap
               by using option index=n in all command line tools.
               Supported multipage image formats: BMP, GIF, TIF, PPM, SPR

             OS/2 compiler support extended:
             * GBM.DLL, command line tools, GBM file dialog, GBMV and GBMV2 can
               now also be compiled with Open Watcom 1.4. IBM Visual Age C++ for
               OS/2 is still supported. Binaries created with Open Watcom are
               fully compatible with older VAC builds (_Optlink is supported).
               The makefiles use nmake syntax but you can use "wmake /ms /f makefile.wat"
               to compile with Open Watcom's wmake.
               The DLL import lib can be used by all compilers (e.g. IBM VAC++).
               The utility archive libs are provided in 2 flavours (IBM VAC++, OpenWatcom).

             * Regression tests:
               A simple set of regression tests has been created. See gbmtest directory.

             * Code changes:
               All bitmap format descriptions have been moved to gbmdesc.h to
               support simplified localization.

               API extensions:
                 New APIs have been added:
                 1. gbm_restore_io_setup()
                 2. gbm_read_imgcount()
                 3. Compiler independent modes for gbm_io_open() and gbm_io_create()
                    are now provided. Use only the new GBM_O_* modes for new applications!
                    The defined values are backward compatible to VAC++ 3.0 to keep
                    compatibility with older applications.
                 4. gbm_io_lseek() should only be used with the new compiler independent
                    origin defines GBM_SEEK_*. The new definitions are backward compatible
                    to VAC++ 3.0 to keep compatibility with older applications.

                 The import library gbm.lib does reflect the extended API.
                 If your application has to be compatible to GBM.DLL releases
                 up to 1.32, you should link against gbmorg.lib. New APIs
                 will not be available then. You should still use the new defines.

                 Supported multipage image formats: BMP, GIF, TIF, PPM, SPR
                 (For all other image formats, the header is tested first and
                  on successful detection, 1 is reported.)


24-Mar-2006: GBM version 1.36
             * TIF support:
               Update to Libtiff 3.8.2
               Fix JPEG decoding via RGBA reader for YCbCr JPEGs


29-Mar-2006: GBM version 1.37
             * Some small performance optimizations for color mapping in TIF, PNG and PPM
               when importing/exporting


29-Apr-2006: All binaries can now also be compiled with OpenWatcom 1.5.

             GBM.DLL 1.38
             * Fix some issues to support OpenWatcom 1.5 compiler upgrade

             GBM command line tools
             * gbmver:
               - The tool now also checks for GBMDLG.DLL (GBM File Dialog).
               - Now additionally a directory can be specified where the tool
                 looks for GBM.DLL and GBMDLG.DLL.

             * gbmgamma:
               - Fix a rare case for bitmaps with >24bpp. If no map option is
                 specified the written file could be partly or completely black.

             * gbmhdr:
               Added general mechanism to resolve shell provided file names
               via regular expressions on OS/2 due to inclusion of OpenWatcom
               support (which doesn't have setargv.obj like IBM VAC++).
               Now calls like "gbmhdr *.jpg *.png" work also with the OpenWatcom
               binaries. The new mechanism additionally allows binding options
               to each file template.
               Example: gbmhdr *.bmp,index=3 *.tif,index=1

             * Fix issue with comma separation between file and options.
               The tools couldn't read bitmap files that have a comma in
               the filename (comma is used as option separator).
               Now a filename can be specified with quotes and thus allows
               clearly separating it from the options.
               Example: gbmhdr "\"file with ,*.bmp\",options"
               (\" is necessary because the shell otherwise removes the ",
                the other " is non-escaped quote is used by the shell to group
                parameters with spaces (like a filename) to one arguments)

               Recommendation is to always use this new syntax. See also
               the usage description available from the tools for specific details.


16-May-2006: GBM.DLL 1.39
             * PNG support:
               Upgrade to Libpng 1.2.10

             GBM command line tools
             * Add more restrict checking for completeness when using
               the new syntax for specifying filename with options.
             * gbmver
               Verify detected/specified GBM.DLL/GBMDLG.DLL/GBMRX.DLL/GBMDLGRX.DLL
               version by looking now at all exported functions and give some hints
               about backward compatibility to applications.
               As the calling convention of the functions and the parameters
               cannot be queried from the OS, there is still a small amount
               of uncertainty.


27-May-2006: GBM.DLL 1.40
             * Add support for reading Windows 16bpp and 32bpp
               BI_RGB and BI_BITFIELDS encoded bitmaps (BMP). Writing
               them will not be supported as Windows can also read
               standard 24bpp BMPs without this strange encoding.
               Also these formats mostly aren't supported by other platforms.
             * Fix the strange handling of 1bpp bitmaps. Now the colour
               palette is honored just as in all other formats.
             * Protect all readers and writer against illegal colour depth requests
               (gbm_read_palette, gbm_read_data, gbm_write)
             * Obsolete format codes GBM_FT_R16 and GBM_FT_W16 as these are not
               unique interpretable. Use 24bpp or 48bpp color depths instead.
               There has been no codec that used them so far anyway.

             GBM command line tools
             * gbmhdr:
               - Dynamically load gbm.dll for querying image page count
                 to allow it to load also with older gbm.dll versions
                 on OS/2.


24-Jun-2006: GBM.DLL 1.41, GBM command line tools, GBMV/GBMV2 1.28, GBMDLG, GBM REXX 1.01
             * Use default structure packing internally but keep externally used
               GBM/GBMDLG struct data 4 byte aligned to keep binary compatibility.
               OpenWatcom compiled binaries are now internally using compiler
               pentium optimized data packing (8 byte) to improve performance a bit.

             GBM.DLL 1.41
             * extended PPM support (24/48 bpp, types: ascii P3 and binary type P6, multipage type P6)
             * extended PGM support (8 bpp    , types: ascii P2 and binary type P5, multipage type P5)
             * new PBM support      (1 bpp    , types: ascii P1 and binary type P4, multipage type P4)
             * new PNM support      (same subformats as PBM/PGM/PPM, multipage PNMs)
             * shorten BMPs long description to simply "OS/2 / Windows bitmap"
             * Modified compiler options for OpenWatcom build after profiling.
               Performance benefits are between 5 and 20 percent depending on the
               image format and the specified options. Overall performance on my
               Athlon64 3000+ is now close to the IBM VAC build.

             GBM command line tools
             * Modified compiler options for OpenWatcom build after profiling.
             * gbmconv has now built-in conversion time measurement. It can be activated
               by enabling #define MEASURE_TIME at the beginning of gbmconv.c.


05-Jul-2006: GBM.DLL 1.42
             * Upgrade to Libpng 1.2.12 due to a possible security issue.
               For GBM.DLL now all the relevant code parts have been stripped
               (no impact on functionality, just debug reporting).
             * Modify OpenWatcom compiler options to fix random crashes due to
               bad stack. This was introduced with GBM.DLL 1.41 which had been
               speed optimized.
               GBM.DLL 1.41 should no longer be used and replaced by version 1.42.


29-Jul-2006: GBM.DLL 1.43
             * BMP support:
               Prevent possible buffer overflows in 4,8,24bpp RLE decoders due to
               badly encoded bitmap.
             * LBM support:
               Prevent possible buffer overflows in RLE decoder due to badly
               encoded bitmap.


16-Aug-2006: GBM.DLL 1.44
             * Add bldlevel info

             GBM command line tools (gbmver, gbmhdr)
             * Remove .DLL from the dynamically loaded libs so that
               their lookup also works with LIBPATHSTRICT enabled.

             Makefiles
             * All makefiles now support activating debug compile without
               editing them. Just use nmake or wmake with parameter debug=on


12-Dec-2006: GBM.DLL 1.50
             * New XPM codec
               - XPM2, XPM3 formats only, XPM1 or XPM2C are not supported
             * JPEG codec improvements:
               - Add support for reading YCCK and CMYK encoded JPEGs
                 (simple color space conversion only, no color management support!)
               - Add support for writing grayscale JPEGs (8bit) with r,g,b,k options
               - Add support for smoothing and optimized writing
               - Add support for writing a user comment
               - Undo hack in jmorecfg.h (changing oder of RGB out put from RGB to BGR),
                 it is now back to the original order as this is required by TIFF decoder.
                 The color component order is now swapped in the JPG codec.
             * PBM/PGM/PPM/PNM codec improvements:
               - Dramatically improved performance for ASCII format read/write
                 (by using read ahead and write cache)
             * TIFF codec improvements:
               - Report at least 1 page if libtiff reports 0. This is backward compatible
                 behaviour. The directory tag might be missing in the file and length
                 calculation fails as well due to compression scheme.
               - CMYK images (8/16bpp) can now be read (single and multiplane TIFFs)
                 (simple color space conversion only, no color management support!)
               - Report more specific error message for data decoding problem
             * TGA codec improvements:
               - Fix RLE8 decoder (wrong pointer used for ydown reading)
               - Reject unsupported interleaving schemes (except non interleaved)
               - Add support for reading horizontally flipped TGAs
               - Add support for reading 32bpp TGAs with alpha channel if option ext_bpp is set.
               - Add support for writing 32bpp TGAs keeping the alpha channel if source data is 32bpp.
               - New input option ext_bpp to allow activation of 32bpp input with alpha channel
             * SPR codec changes:
               - Attempt to protect the format detection better against wrong interpretations.
                 Number of pages is now restricted to 10000 due to this.
             * PNG codec:
               - upgrade to Libpng 1.2.14
             * Several performance improvements
             * Open Watcom binaries are now quite a bit faster (better compiler optimization enabled)

             GBM command line tools:
             * gbmhdr
               - Fix a division by zero crash that happened for bitmaps with size smaller
                 than 1 byte (e.g. 1x1x1bpp).
             * gbmver
               - Sync to changes of GBMDLG.DLL API

             Compiler support:
             * OpenWatcom 1.6 is now supported


22-Jan-2007: GBM.DLL 1.51
             * some smaller non-critical fixes

             GBM command line tools:
             * gbmsize
               - Allow -h and -w together with -a options. The minimum frame size is preserved.


06-Sep-2007: GBM.DLL 1.52
             * Seek to file end after reading/writing the image data as some
               readers/writers operate non-sequential.

             GBM command line tool gbmsize + gbmscale library
             * Add resampling scaler:
               -> Filter functions:
                  * Nearest Neighbor, Bilinear, Bell, BSpline, Mitchell, Lanczos
               -> Supported color depths:
                  *  8bpp (grayscale)
                  * 24bpp
                  * 32bpp (24bpp with alpha channel)
                  * 48bpp
                  * 64bpp (48bpp with alpha channel)

             Compiler support:
             * OpenWatcom 1.7 is now supported



04-Nov-2007  GBM.DLL 1.53, GBM command line tool gbmsize + gbmscale library
             * Resampling scaler with improved accuracy

             Compiler support:
             * Upgrade to OpenWatcom 1.7a due to issues found in the 1.7 release


27-Jan-2008  GBM.DLL 1.54
             * PNG codec:
               - upgrade to Libpng 1.2.24

             GBM command line tool gbmsize + gbmscale library
             * Add support for resampled scaling of 1bpp and 4bpp grayscale bitmaps.
               (automatic color depth expansion)
               -> Now supported color depths:
                  *  1bpp, 4bpp, 8bpp (grayscale only)
                  * 24bpp
                  * 32bpp (24bpp with alpha channel)
                  * 48bpp
                  * 64bpp (48bpp with alpha channel)

             GBM command line tool gbmver
             * Sync to GBMRX.DLL API changes

             GBM gbmmir library
             * fix potential buffer overflow in 4bpp transpose code


26-Mar-2008  GBM command line tools
             * All: Allocate memory from high memory for bitmap data to
                    stretch limit for out-of-memory errors
                    (requires kernel with high memory support)
             * gbmsize: Removed maximum size limit

             GBM.DLL 1.55
             * PNG codec:
               - upgrade to Libpng 1.2.25
               - Fix: Missing background color when downconverting RGBA
                      with 4 channels to 3 channels by stripping alpha channel.
                      This made some images look badly if the user did not
                      provide a custom background color. Now a white background
                      is used instead of simply stripping the alpha channel.

             New gbmmem.lib library used for platform independent memory management
             * on OS/2 high memory objects are preferred for allocation
             * C library fallback for other platforms
             * Must be linked to all code that uses gbm code libs (e.g. gbmscale)


28-Aug-2008  GBM.DLL 1.60
             * NEW: JPEG2000 Codec based on OpenJPEG 1.3+ (only for OpenWatcom builds)
               - reads 8/16/24/48 bpp gray level and colour files (sRGB, YUV)
               - writes  8 bpp gray level files (lossless)
               - writes 24/48 bpp gray level files (lossless)
               - uses high memory allocation via GBM memory handler
               - J2K_CODEC executables (OpenJPEG decoder and encoder) for OS/2
                 provided separately
             * NEW: Build support for Windows with OpenWatcom
               Supported targets:
               - GBM executables & DLL
               - GBM test and demo file creator
               - J2K_CODEC executables (OpenJPEG decoder and encoder)
             * FIX: Resolved problem with GBM_O_ and GBM_SEEK_ mode mapping for compilers
                    that start definitions at 0x0.
             * PNG codec:
               - upgrade to Libpng 1.2.31
               - use high memory allocation via GBM memory handler
             * JPG codec:
               - use high memory allocation via GBM memory handler
               - activate library defined code inlining
             * TIFF codec:
               - use high memory allocation via GBM memory handler
               - activate library defined code inlining
               - reject unsupport samplesPerPixel
                 (IBM TIFF seems to write buggy images for 32bpp which are marked
                  as graylevel image but has 4 samples per pixel)
             * BMP codec:
               - Add support for reading 16/32bpp BGR OS/2 1.2,1.2 bitmaps which
                 are written by some apps when graphics card runs at 16/32bpp.
             * Many Makefile updates to support new build platforms
             * API changes:
               - Due to sync to official GBM sources, the old types BOOLEAN, byte, word
                 and dword are obsolete and should no longer be used. Replacements are
                 gbm_boolean, gbm_u8, gbm_u16, gbm_u32. This does not hurt binary compatibility!
                 There is a fallback for clients, backward compatible types can be activated
                 by defining OLD_GBM_TYPES. Please prefer switching to the new types.


21-Sep-2008  GBM.DLL 1.61
             * JPEG2000 codec:
               - add JPWL read support
               - fix wrong extension to codec assignment
               - add support for rate based compression
               - add support for quality based compression
             * add GCC 3.3.5 compiler support for OS/2


28-Oct-2008  NEW: Build support for Microsoft Visual C++ (Windows 32bit)
             Supported targets (via makefile):
             * GBM executables & DLL
             * GBM test and demo file creator
             * J2K_CODEC executables (OpenJPEG decoder and encoder)

             NEW: Build support for Linux (32bit and 64bit)
             Supported targets:
             * GBM executables & shared lib
             * GBM test and demo file creator
             * J2K_CODEC executables (OpenJPEG decoder and encoder)

             GBM.DLL 1.62
             * JPEG codec:
               - allow a broader range of DC table symbols in huffman decoder
                 to support loading of PhotoPaint created JPEGs

             * TIFF codec:
               - Fix: Use a temporary scanline buffer for writing palette bitmap
                      because otherwise the original bitmap data are modified. This
                      by was found to be the case for instance with the LZW compressor
                      with horizontal prediction.

             * PNG codec:
               - Update to Libpng 1.2.32
               - add support for writing a comment

             * Fix buffer overrun when parsing very long option strings

             GBM command line tools 1.53,
             GBM PM tools  1.53 (GBMV, GBMV2, GBMVFSA, GBMLOGO)
             * The command line parser now additionally supports the following
               file specification templates:
               "filename"\",\"options
               "filename"\",\""options"
               This should be hopefully work for all variants now.
               "\"filename\"{,opt}" does not work together when &, |, < or >
               are part of the filename because the shell will interprete them
               as operators and cut the filename.
               The old template will continue to work for backward compatibility
               reasons but the tools will from now on recommend the new template


22-Feb-2009  GBM.DLL 1.63
             * PNG codec:
               - Update to Libpng 1.2.34

             * JPEG2000 codec:
               - less progressive checking for unknown color spaces, OpenJPEG does
                 it itself and reacts accordingly

             * OpenWatcom 1.8 compiler is now supported


12-Nov-2009  GBM.DLL 1.64
             * NEW JBIG codec:
               - based on JBIG Kit which is also used by the TIFF codec (see below)
               - so far only supports reading 1,8 bpp bitmaps and writing 1,8,24 bpp bitmaps
               Note: Users of Non-GPL compatible software linking against GBM.DLL should study
                     potential patenting issues. GBM.DLL can be built without JBIG support by
                     disabling it in the main GBM makefile (e.g. gbm\makefile.os2\makefile.wat)
                     if necessary.

             * PGM codec (P2: ASCII, P5: Binary):
               - Add support for reading 16bpp graylevel images (intensity up to 65535)
                 as either 8bpp palette image or 48bpp true color image (with ext_bpp)
               - Add support for writing graylevel equivalent images of 24/48bpp images

             * PNM codec:
               - Add support for reading 16bpp graylevel images (intensity up to 65535)
                 as either 8bpp palette image or 48bpp true color image (with ext_bpp)
               - Gray conversion of 24/48bpp RGB images now results correctly in P2/P5 image

             * PNG codec:
               - Update to Libpng 1.2.40

             * JPEG codec:
               - Update to IJG 7

             * TIFF codec:
               - Update to Libtiff 3.9.2
               - Fixes and some decoder/encoder enhancements (see also libtiff\doc\ChangeLog)
               - OJPEG format can now be read
               - JBIG  format is now supported (1bpp only, read/write).
                 Note: Users of Non-GPL compatible software linking against GBM.DLL should study
                       potential patenting issues. GBM.DLL can be built without JBIG support by
                       disabling it in the main GBM makefile (e.g. gbm\makefile.os2\makefile.wat)
                       if necessary.

             * JPEG2000 codec:
               - fix memory leak that may occur under a special conditions when encoding
                 (this case should normally not happen)

             * NEW: Build support for Microsoft Visual C++ (Windows 32/64bit)
               Supported targets (via makefile):
               - GBM executables & DLL
               - GBM test and demo file creator
               - J2K_CODEC executables (OpenJPEG decoder and encoder)


 2-Mar-2010  GBM.DLL 1.65
             * Fix buffer overrun in ILBM HAM6 encoder
             * Fix some compile errors on new GCC 4.4.2 (Linux x86/x64)

             * JPEG codec:
               - Update to IJG 8a
               - Reinstate IJG change that allows a broader range of DC table symbols in huffman
                 decoder to support loading of PhotoPaint created JPEGs (jdhuff.c). It was
                 implemented in GBM.DLL 1.62 but got lost during update to IJG 7 library.

             * PNG codec:
               - Update to Libpng 1.4.1

             * GIF codec:
               - Fix local vs. global color palette handling when reading multipage images
                 (global palette overwritten in multipage images where images before the requested
                  had a local palette but the requested did not)

             GBM command line tools 1.54
             * Make gbmhdr.exe on Windows compatible for use with old GBM.DLLs


24-Apr-2010  GBM.DLL 1.66
             * TIFF codec:
               - Update to ZLib 1.2.4


18-Jun-2010  GBM.DLL 1.67
             * Fix: gbm_guess_filetype() did only check for the first matching extension chars
                    but not the whole extension. This resulted in reporting of .p as valid
                    .png file for instance. Now the complete extension is checked.

             * TIFF codec:
               - Update to Libtiff 3.9.4
               - Update to ZLib 1.2.5
               - Finally fix longer lasting issue with libtiff 3.9.x when reading a vertically
                 flipped YCbCr image via RGBA reader with 64bit GBM. It led to a crash due to
                 an illegal write pointer which seems to be caused by wrongly used pointer arithmetic.
                 Workaround is that vertical flipping is now done by GBM and prevented in libtiff.

             * PNG codec:
               - Update to ZLib 1.2.5
               - Update to Libpng 1.4.2
               - Integrate APNG extension patch
               - Add APNG support (read individual animation images)

             * JPEG codec:
               - Update to IJG 8b


21-Sep-2010  GBM.DLL 1.70
             * NEW: RAW codec
               - Add support for reading all RAW formats supported by dcraw

             * PNG codec:
               - Update to Libpng 1.4.3
               - Integrate APNG extension patch

             * gbmscale.lib:
               - Fix resampling scaler buffer overflow
               - Protect scaler functions more robust against wrong user arguments
               - Quality fine tuning of resampling scalers
                 Flat colour areas are now preserved to get better quality.
               - Performance tuning of resampling scalers
                 - Multithreading support (fully dynamic CPU core usage)
                 - SSE support for Microsoft, GCC and Open Watcom Compilers

             * gbmmir.lib:
               - remove not multithread safe static lookup table
               - some performance improvements
                 (shorter loops, wider memory transfers, less memory accesses)

             * Static lib can now be built. It is not recommended for general use
               due to upgrading limitation when new versions will be released and
               also because of potential symbol conflicts with the linking executable.


13-Nov-2010  GBM.DLL 1.71
             * RAW codec
               - Update to libraw 0.11.2 (based on dcraw 9.05)
               - All supported RAW formats checked with some sample images
                 (added missing extensions, removed unsupported extensions like Sigma Foveon)
               - Better handling of additional rows/columns due to rounding accuracy
                 (offset between image info and post decoding size for Fuji cameras)
               - Fix wrong image size when combining halfsize and aber or denoise params
               - Default interpolation quality is now 2 (PPG) as a good compromise
                 between performance and quality
               - Allow manual gamma adjustment, default is sRGB
               - Add options for clipping, saturation, blacklevel and four color RGGB interpolation
               - Allow disabling camera withe balance correction

             * PNG codec:
               - Update to Libpng 1.4.4
               - Integrate APNG extension patch

             * gbmscale.lib, gbmsize:
               - Add resampling scaler filters:
                 * blackman, catmullrom, quadratic, gaussian, kaiser
               - Final fix for sometimes generated moire due to rounding issues
                 in sample contribution calculations.


20-Feb-2011  GBM.DLL 1.72
             * RAW codec
               - Update to libraw 0.13.1 (based on dcraw 9.06)
               - Several new cameras are supported now
               - Additional interpolation type available (option quality=4 which is DCB)

             * JPEG codec    : Update to IJG 8c
             * PNG codec     : Update to Libpng 1.5.1
             * JPEG2000 codec: Update to OpenJPEG 1.4

             * OS/2: Remove dependency to TCPIP32.DLL that came in with RAW support.
                     This should make GBM.DLL and thus all clients work on Warp 3
                     and Warp 4 without 32bit TCPIP stack installed working again.

             * Windows: Mark executables as NX compatible and large address aware
               (>2GB as 32bit on 64bit system)

             GBM command line tools 1.72
             * Windows: Mark executables as NX compatible and large address aware
               (>2GB as 32bit on 64bit system)


21-Jun-2011  GBM.DLL 1.73
             * RAW codec
               - Update to libraw 0.13.5 (based on dcraw 9.08)
               - Several new cameras are supported now, improvements for others
               - Increase stack size for worker thread to 350kb
               - Fix crash during reading image count due to stack overflow on OS/2

             * TIFF codec: Update to Libtiff 3.9.5
             * PNG codec : Update to Libpng 1.5.2


24-Oct-2011  GBM.DLL 1.74
             * RAW codec: Update to libraw 0.14.2 (based on dcraw 9.11)
             * PNG codec: Update to Libpng 1.5.5


26-Feb-2012  GBM.DLL 1.75
             * TGA codec
               - Add support for reading and writing 1bpp gray/bw/palette images
                 (uncompressed and RLE compressed)
               - Add support for reading and writing 4bpp gray/palette images
                 (uncompressed and RLE compressed)
               - Add support for writing 8bpp gray images
               - Add support for writing RLE compressed images (all colour depths)
               - Fix write bugs (e.g. uninitialized alpha channel for 24bpp -> 32bpp)

             * ZLIB updated to version 1.2.6
             * PNG codec     : Update to Libpng 1.5.9 (with APNG extension patch)
             * JPEG codec    : Update to IJG 8d
             * JPEG2000 codec: Update to OpenJPEG 1.5
             * RAW codec     : Update to libraw 0.14.5 (based on dcraw 9.12)
             * TIFF codec    : Will not be upgraded to libtiff 4.0.1 because then
                               the old IBM VAC compiler cannot be supported anymore
                               (64bit ints are required now). Also the new libtiff
                               mainly provides new support for BigTIFF which is not
                               widely spread. So there is no need for an upgrade.

28-Oct-2012  GBM.DLL 1.76
             * ZLIB updated to version 1.2.7
             * PNG codec     : Update to Libpng 1.5.13 (with APNG extension patch)
             * JPEG2000 codec: Update to OpenJPEG 1.5.1
             * RAW codec     : Update to libraw 0.14.7 (based on dcraw 9.15)
             * Fix building GBM without JBIG support
             * Use DosWaitEventSem on OS/2 instead of WinWaitEventSem
               to get rid of dependency to PMWIN.DLL. For GBM internal
               threads (gbmmthrd lib) there is no need to use Win function.
               As the gbmmthrd lib is also used by all command line tools
               and gbmrx.dll, they are also affected by this change.
    


