
GBMV/GBMV2 Version 1.60
=======================
(28-Oct-2012, Heiko Nitzsche)

Since the nice GBMV2 PM bitmap viewer doesn't seem to be actively maintained
by the initial author, I took over the task to sync it to the latest available
level as provided for other platforms.

This version of the PM viewer GBMV2 has been enhanced and also translated to German.

The original package (GBMOS2PM.ZIP) can be found on the homepage of the author
Andy Key (http://www.nyangau.fsnet.co.uk) or on Hobbes. If you have installed
the packages just replace the installed files with the ones from this archive.

Enhancements included
---------------------
- German and English version of GBMV2 (including online help)
- German and English version of the GBM file dialog (including online help)
- Fixes of memory leaks in GBMV2
- Support for 4Rx4Gx4B color mapping that has been introduced to the
  original GBM command line tools
- New resources for GBMV2 and GBM file dialog that better fit to
  the Warp 4 and eComStation look & feel. The package comes with resources
  for the fonts 8.Helv and 9.WarpSans in German and English.
  The size of the GBM file dialog has been increased.
  The file dialog has been extended to preview a selected bitmap.
- Several keyboard shortcuts have been added.
- Support for interpolated scaling with several filter types
- Red Eye correction filter
- High Memory support for Warp 4.5x/eCS kernels

GBMV and GBMV2 do not support the extended color depths provided by newer
GBM.DLL and they will probably never support them. But they will work fine
when ext_bpp option is not used for the bitmap import. Bitmaps with extended
color depths are downsampled to 24 bpp.

The included GBM file dialog might be usable for other applications as well.
An example is Mesa/2 which will use GBM.DLL, GBMDLG.DLL and GBMDLG.HLP for
importing bitmaps when they are found in it's installation directory.

All binaries are now optimized for Pentium or compatible processors but
they should run on earlier 32bit processors as well (untested).

The binaries with WarpSans fonts are thought to run on OS/2 Warp 4 or higher
or eComStation. OS/2 Warp 3 can still be used (reported by a user, I can't test it).

There are also resource files with 8.Helv font available in the source archive.
They can be compiled and attached to the binaries with the resource compiler
rc.exe usually installed on every OS/2 system.

Example:  Replace 9.WarpSans in GBMV2 with 8.Helv font

1. Get the source package.
2. Extract the resource file (gbmv2.rc) of your preferred language.
3. Copy it beside the executable.
4. Compile them with:
     rc -n -r -x2 gbmv2.rc
5. Attach the created binary gbmv2.res to gbmv2.exe:
     rc -n -x2 gbmv2.res gbmv2.exe

This also works for gbmdlg.dll (GBM File Dialog).


Attention when working with 1 bpp bitmaps in GBMV2
--------------------------------------------------
The whole handling of 1bpp bitmaps in GBMV2 has ever been a bit
messy and thus some issues are known which might cause wrong color
conversions (clipboard copy/paste, writing to metafile). These are
conceptual issues within the tool. The effort to fix all of them
would be huge. Thus they will not or only partly be fixed.

If you want reliable results, please convert 1bpp bitmaps to 24bpp.


Installation
------------
All applications included in this package require a version of GBM.DLL
higher than version 1.11. Recommended is GBM.DLL 1.35 or higher.

GBM.DLL can be installed at a central location of the system so that
it is found in the paths defined by LIBPATH in CONFIG.SYS. Thus sharing
between all programs of this package and the programs from the gbmos2
package is possible.

Depending on the applications you want to use, copy the following
files into an empty directory. For the PM applications GBMV and GBMV2
German and English versions are available.

GBMV:    This is a fast and simple bitmap file viewer.

         Required files: gbmv.exe gbm.dll


GBMV2:   This is a simple bitmap viewer and editor. It comes with it's
         own file dialog that is able to show a preview of the selected
         bitmap file.

         Required files: gbmv2.exe gbmv2.hlp gbmdlg.dll gbmdlg.hlp gbm.dll


GBMLOGO: Can create an OS/2 boot logo.

         Required files: gbmlogo.exe gbm.dll


GBMVFSA: A simple OS/2 full screen bitmap viewer. It has very limited
         resolution support (320x200) and might not work on all systems.

         Required files: gbmvfsa.exe gbm.dll


There is a REXX script included (gbmwpobj.cmd) that creates desktop
icons for GBMV and GBMV2. Simply copy it to their installation directory
and run it.

Note:
 If gbmv.exe and gbmv2.exe should be run from everywhere on the command line
 or a different work directory in the WPS program object than the program
 installation directory should be used, it is necessary to add the installation
 directory to some environment variables in CONFIG.SYS.

 The following changes are necessary:
 (It is assumed that the installation directory is D:\GBM)

 1. Add D:\GBM to the end of the line beginning with SET PATH separated
    by ; to the previous parameter.
    (e.g. SET PATH=C:\OS2;D:\GBM)

 2. Add D:\GBM to the end of the line beginning with SET HELP separated
    by ; to the previous parameter.  This is necessary so that the system
    can find GBMV2.HLP and GBMDLG.HLP.
    (e.g. SET HELP=C:\OS2\HELP;D:\GBM)

 3. Add D:\GBM to the end of the line beginning with LIBPATH separated
    by ; to the previous parameter. This is necessary so that the system
    can find GBM.DLL and GBMDLG.DLL.
    (e.g. LIBPATH=.;C:\OS2\DLL;D:\GBM)

 4. Reboot.

 5. For this step you need gbmver.exe. It is included in the GBM base
    package only, which you have already as you have GBM.DLL. Copy it
    into the installation directory if it's not already there.

    Open a command line and go to a directory OTHER THAN the installation.
    Then start gbmver.exe as if it would be in this directory.
    It should report the location and the version of GBM.DLL as well
    as the location of GBMDLG.DLL (GBM File Dialog).
    Verify that the reported files are the installed versions. If the
    reported files are found in a different path, do a file search
    for GBM.DLL or GBMDLG.DLL on your system and check for duplicates
    or older versions.
    If an older version of GBM.DLL is found (version must be higher than 1.11 !!!),
    you can replace it with the GBM.DLL from the GBM base package.
    Alternatively modify the LIBPATH statement in CONFIG.SYS so that
    the correct path to GBM.DLL and GBMDLG.DLL is placed before all other
    versions.
    After the changes are done, check again with gbmver.exe.



Detailed documentation
----------------------
A more comprehensive documentation of the tools is available on my GBM homepage
(see below).


Legal stuff
------------
The package includes source code provided as Public Domain from Andy Key,
the original author of GBM and the GBM PM tools (http://www.nyangau.fsnet.co.uk).

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

08-Jun-2003: GBMV2 1.10
             * New German version including translated online help
             * New Warp 4 & eComStation font resources for all dialogs
             * Optimized for Pentium and compatibles
             * Fixed some memory leaks

             GBMDLG
             * New German version including translated online help
             * New Warp 4 & eComStation font resources for the dialog
             * Resized (larger) file dialog
             * Optimized for Pentium and compatibles


02-Aug-2005: GBMV2 1.20
             * Support for 4Rx4Gx4B color mapping as available in the
               command line tools


24-Aug-2005: GBMDLG
             * added documentation for PNG bitmaps including all
               available input and output options


29-Sep-2005: GBMV2 1.21
             * detect unsupported color depths

             GBMDLG
             * updated documentation for PNG and TIF bitmaps including all
               available input and output options

             GBM documentation:
             * updated documentation for PNG and TIF bitmaps including all
               available input and output options


02-Oct-2005: GBMDLG
             * updated documentation for PPM bitmaps including all
               available input and output options

             GBM documentation:
             * updated documentation for PPM bitmaps including all
               available input and output options


05-Jan-2006: GBMDLG
             * Redesign to match standard file dialog (eComStation)
             * Add bitmap preview


08-Mar-2006: GBMDLG
             * updated documentation for BMP bitmaps
               (GBM.DLL 1.35 or higher now also supports RLE24 compressed bitmaps)

             All binaries are now compiled with OpenWatcom 1.4.


24-Mar-2006: GBMDLG/GBMV2 1.23
             * Fix bitmap preview, sometimes the bitmap was not shown due to a scaling issue


29-Mar-2006: GBMDLG/GBMV2 1.24
             * Bitmap preview enhanced
               If options are entered into options field and the field loses focus,
               they are now interpreted for the preview. ext_bpp is supported as well
               but because OS/2-PM only supports 24 bpp the bitmap is downsampled
               for the preview.
             * Bitmap info beside preview image extended to show also number of
               contained pages if a recent GBM.DLL is found.
             * Fix dialog resource issue with IBM RC compiler (get rid of CTLDATA at VALUESET)
               which caused a crash with IBM VAC compiled GBMDLG.DLL. OpenWatcom wrc
               didn't cause any issues.


29-Apr-2006: All binaries can now also be compiled with OpenWatcom 1.5.

             GBMDLG/GBMV2 1.25
             * Fix gbmwpobj.cmd which didn't run on some systems.
             * Fix GBM File Dialog issue which prevent that the options
               entered by the user are interpreted.
             * Fix some smaller issues in GBM File Dialog found during
               development of the REXX wrapper library.
             * Fix issue with comma separation between file and options.
               The tools couldn't read bitmap files that have a comma in
               the filename (comma is used as option separator).
               Now a filename can be specified with quotes and thus allows
               clearly separating it from the options.
               Example: gbmv "\"file with,xyz.bmp\",options"
               (\" is necessary because the shell otherwise removes the ",
                the other " is non-escaped quote is used by the shell to group
                parameters with spaces (like a filename) to one arguments)

               Recommendation is to always use this new syntax.
               The included REXX script gbmwpobj.cmd for creating the WPS objects
               has been updated and will take for this.

             GBM command line tool gbmver
             * The tool now also looks up the location of GBMDLG.DLL.
               For details please see the documentation of the GBM package.


16-May-2006: GBMV/GBMV2 1.26, GBMLOGO, GBMVFSA
             * Add more restrict checking for completeness when using
               the new syntax for specifying filename with options.


27-May-2006: GBMV/GBMV2 1.27
             * they got new program icons (created by David Graser)
             * Fix display of 1bpp color bitmap for which no RGB color
               table was created and thus only the closest pure color
               was used. This caused wrong colors in display but had
               no effect on bitmap operations.
             * Fix makefile issue with English IPFC country code. It's now 001.

             GBM File Dialog
             * Fix preview of 1bpp bitmaps
             * Updated online help to reflect newly supported BMP formats
             * Fix makefile issue with English IPFC country code. It's now 001.


24-Jun-2006: GBMV/GBMV2 1.28 and GBM File Dialog
             * Modified compiler options for OpenWatcom build after profiling.

             GBM File Dialog
             * Updated online help to reflect newly supported formats PBM, PGM, PPM, PNM


5-Jul-2006:  No new version number but recompiled with slightly modified
             OpenWatcom compiler options to improve stability.


16-Aug-2006: GBMV/GBMV2 1.29 and GBM File Dialog

             GBM File Dialog
             * Remove .DLL from the dynamically loaded GBM.DLL so that
               its lookup also works with LIBPATHSTRICT enabled.
             * Add bldlevel info

             Makefiles
             * Makefiles now support activating debug compile without editing them.
               Just use nmake or wmake with parameter debug=on


28-Nov-2006: GBM File Dialog 1.31
             * Dialog window procedure is now exported to allow dialog subclassing
             * Updated online help to reflect newly supported JPEG options
             * Updated online help to reflect newly supported TGA options
             * Added XPM format to online help
             * Some reformatting of the online help to make it more consistent

             All binaries can now also be compiled with OpenWatcom 1.6.


12-Dec-2006: GBM File Dialog 1.32
             * Fix wrong build of English gbmdlg.dll
             * Add language to bldlevel info (DE: German, EN: English)


07-Sep-2007: GBMV2 1.40
             * Add interpolated scaling support with several filter types
               (Nearest Neighbor, Bilinear, Bell, BSpline, Mitchell, Lanczos)

             GBMV 1.40
             * The viewer window is now restricted to the visible screen area,
               Larger images are downscaled to be completely visible. The aspect
               ratio is preserved.
             * For downscaling additionally a interpolation filter can be defined.
               -> new option -f filter
               -> filter can be: nearestneighbor, bilinear, bell, bspline, mitchell, lanczos

04-Nov-2007  GBMV/GBMV2 1.41, GBM File Dialog 1.41
             * Resampling scaler with improved accuracy
             * Correct online help for resize dialog

27-Jan-2008  GBMV/GBMV2 1.42
             * Add support for 1bpp and 4bpp grayscale resampled scaling.
             * Fix rotation direction for 90 and 270 degress

23-Mar-2008  GBMV/GBMDLG 1.50
             * Allocate memory from high memory for bitmap data to
               stretch limit for out-of-memory errors
               (requires kernel with high memory support)

             GBMV2 1.50
             * Allocate memory from high memory for bitmap data to
               stretch limit for out-of-memory errors
               (requires kernel with high memory support)
             * Add Red Eye fix capability
             * Fix some selection dependent menu activation
             * Reorder some menu items to better logical groups
             * Increase maximum allowed size for resize to 15000x15000
             * Render bitmap creation replaced by direct rendering
               (Reduced memory consumption, large bitmap can now be shown)
             * Fix error when trying to resize a bitmap if GBMV2 is compiled with VAC
             * Fix error during printing (printer driver data corrupted + wrong ENDDOC)
             * Remember main window size and restore on restart
             * Remember last path in file open/save dialog

28-Aug-2008  GBMV/GBMV2 1.51
             * Allow window positioning on non-byte aligned x coordinate
             * Adjust worker thread priority to idle time to make the system more responsible.
             * Remember open/save format and path separately across sessions

             GBM File Dialog 1.51
             * Added JPEG2000 format to online help
             * Some reformatting of the online help to make it more consistent
             * Selection of a different file type automatically changes the filename extension

21-Sep-2008  GBMV/GBMV2 1.52
             * Fix initialy wrong filename extension handling for file dialog
               (after profile settings were written once, the problem was no longer visible)
             * React on WM_xSCROLL in bitmap procedure messages so that mouse wheel scroll
               messages are processed
             * changes for GCC compile support

             GBM File Dialog 1.52
             * Fix some selection/preview area synchronization issues
             * Fix wrong documentation about JPEG2000 extension/codec assignment
             * Fix potential crash when programs use the dialog modeless
             * changes for GCC compile support

14-Oct-2008  GBMV/GBMV2 1.53
             * GBMV2:
               - Added Shift+A keyboard shortcut to activate selection mode
               - Reworked online help for more consistency, some corrections
               - Fix: The online help and the menu for rotating was inconsistent
                      (not mathematical correct).
             * Some correction for struct paddings for GCC builds

             GBM File Dialog 1.53
             * Add comment parameter for PNG export to the online help
             * improve description of comment parameter in online help

14-Nov-2009  Version 1.54
             - Fix rendering of B&W bitmaps as PM needs special data ordering for it.

17-Mai-2010  GBMV/GBMV2 1.55
             * GBMV2:
               - Fix handling of file dialog when the path previously stored in profile
                 does no longer exist. Now the current directory is used then.

             GBM File Dialog 1.55
             * Add note about Animated PNG support to the online help
             * Add index parameter for PNG read options to the online help


21-Sep-2010  GBMV/GBMV2 1.56, GBM File Dialog 1.56
             * Quality fine tuning of resampling scalers
               (flat colour areas are now preserved to get better quality)
             * Performance tuning of resampling scalers
               - Multithreading support (fully dynamic CPU core usage)
               - SSE support for Microsoft, GCC and Open Watcom Compilers
             * Fix issue with automatic file extension update in file dialog


 5-Nov-2010  GBMV2 1.57
             * Fix: INI file sometimes not correctly closed which caused a file handle leak
             * Add resampling scaler filters:
               - blackman, catmullrom, quadratic, gaussian, kaiser

             GBMV2 1.57, GBM File Dialog 1.57
             * Final fix for sometimes generated moire due to rounding issues
               in sample contribution calculations.


20-Feb-2011  GBMV2 1.58, GBM File Dialog 1.58
             * Update online help for additional quality of libraw interpolation

26-Feb-2012  GBMV2 1.59, GBM File Dialog 1.59
             * Update online help for TGA codec (new formats added)

28-Oct-2012  GBMV2 1.60, GBM File Dialog 1.60
             * Integrate updated gbmmthrd.lib (see gbm doc)



