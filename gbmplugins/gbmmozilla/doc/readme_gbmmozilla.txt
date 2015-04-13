GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey 1.47
------------------------------------------------------

GBM stands for Generalised Bitmap Module.

This is a plugin for the Webbrowsers Netscape, Mozilla, Firefox and
SeaMonkey available for OS/2 and eComStation as well as Windows that
extends them for the support of reading all bitmap formats supported
by GBM.DLL.

For being able to use it, the following additional files are required:

- On OS/2 (Warp 3/4) and eCS: Netscape, Mozilla, Firefox or SeaMonkey
  The plugin has been tested with Firefox 1.0.x to 10.x, SeaMonkey 1.0.x
  to 2.7.x, Netscape 4.61 and even Netscape 2.02 but it should work with
  other versions as well. Embedded printing only works on Mozilla/Firefox
  (1.x/2.x)/SeaMonkey (1.x).

- On Windows (2000/XP/Vista/7): Firefox or SeaMonkey
  The plugin has been tested with Firefox 3.6.x to 16.x and SeaMonkey
  1.0.x to 2.13.x but it should work with later versions as well. Embedded
  printing is supported but seems to have been broken in Firefox 6.0.x
  and SeaMonkey 2.3.x.

- GBM.DLL version 1.73 or higher (GBM Bitmap Format Handler)

More details to this package and about GBM in general can be found
at http://heikon.home.tlink.de or in the included documentation.


Supported features:
-------------------
There is the embedded mode with reduced functionality and the fullscreen mode
with full functionality. Both viewers automatically detect the file format,
so also wrongly named bitmaps often found in the world wide web can be shown.

FULL mode viewer with a load of features:
* English/German localized with autodetection via LANG variable (OS/2) or System Language (Windows)
* Menus, popup menu, scrollbars
* Variable scaled preview
* Transformations: mirror/transpose/rotate
* Multipage navigation with background rendering
* High quality scaling algorithm for improved appearance
* Bitmap format info dialog
* Save the shown page to any GBM supported bitmap file format
* On OS/2 the GBM File Dialog is included (including language dependent contextual help)
* Multicore CPU support


EMBED mode viewer with reduced functionality of FULL mode viewer:
* English/German localized with autodetection via LANG variable (OS/2) or System Language (Windows)
* Popup menu
* Transformations: mirror
* Multipage navigation with background rendering
* High quality scaling algorithm for improved appearance
* Bitmap format info dialog
* Save the shown page to any GBM supported bitmap file format
* On OS/2: uses GBM File Dialog if found, otherwise OS/2 File Dialog is used (dialog contextual help is available)
* Printing of embedded bitmaps is supported on Mozilla/Firefox/SeaMonkey
* Multicore CPU support


Installing GBM for Mozilla/Firefox/SeaMonkey
--------------------------------------------
Please see the included HTML documentation for details.

Note: This plugin requires that the browser cache is enabled.


License
-------
Copyright (C) 2006-2012 Heiko Nitzsche

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.



Contact
-------
Heiko.Nitzsche@gmx.de (http://heikon.home.tlink.de)



History
-------

16-Jul-2006  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.00)
             * Initial release


30-Jul-2006: GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.10)

             Added fullscreen mode viewer with a load of features
             * menus, popup menu, scrollers
             * variable scaled preview
             * transformations: mirror/transpose/rotate
             * multipage navigation
             * save the shown page to any GBM supported bitmap file format
             * uses GBM File Dialog if found, otherwise OS/2 File Dialog is used
             * English and German localized with autodetection via LANG variable

             Added embedded mode viewer with reduced functionality of fullscreen mode viewer
             * popup menu
             * transformations: mirror
             * multipage navigation
             * save the shown page to any GBM supported bitmap file format
             * uses GBM File Dialog if found, otherwise OS/2 File Dialog is used
             * English and German localized with autodetection via LANG variable
             * printing of embedded bitmaps is supported on Mozilla/Firefox/SeaMonkey

             Now supports dynamic lookup of GBM.DLL either via LIBPATH
             or look into the directory where npgbm.dll is located.


16-Aug-2006: GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.20)
             * Renamed menu "Reflect horizontally" to "Flip horizontal" and
               menu "Reflect vertically" to "Flip vertical" in English NLS.
             * Added bitmap information dialog box
             * Fix for issue that GBM.DLL and GBMDLG.DLL are not found when
               LIBPATHSTRICT (or RUN!) is used. Thanks to Rich Walsh for the background
               info about the root cause.
             * Add bldlevel info
             * Makefiles now support activating debug compile without editing them.
               Just use nmake or wmake with parameter debug=on


08-Sep-2006  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.21)
             * 1bpp bitmaps were shown inverted (colors swapped)


28-Nov-2006  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.30)
             * Add help support for the GBM file dialog (requires gbmdlg.dll 1.31 or higher)
             * Allow all dialogs to be moved outside the plugin window
             * Improved detection of bitmap file formats which allows showing bitmap files
               that carry wrong file extensions (e.g. .tif but in reality it's a jpg)
             * Some fixes in GUI handling (menus were sometimes active if file was not loaded)
             * Enable support for XPM (X Windows PixMap) format
               (requires GBM.DLL 1.50 or higher)

09-Sep-2007  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.31)
             * 8bpp grayscale and true color images are now scaled with a high quality
               algorithm to improve appearance.

04-Nov-2007  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.32)
             * Resampling scaler with improved accuracy

27-Jan-2008  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.33)
             * Resampling scaler now supports all grayscale bitmaps <=8bpp

08-Feb-2008  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.34)
             * Move HPS rendering code to common package GbmRenderer
               to save memory (now renders directly from cache buffer)
               without temporary subrectangle buffer
             * Allocate memory from high memory for bitmap data to
               stretch limit for out-of-memory errors
               (requires kernel with high memory support)
             * Custom settings definable in npgbm.cfg file
               (so far scaler={simple,nearestneighbor,bilinear,bell,bspline,mitchell,lanczos})
               The default scaler is simple to allow fast scaling on less powerful systems.

03-Jun-2008  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.35)
             - Wrong cache preparation for 1bpp bitmaps when simple scaler is active
               (no upconversion to 8bpp as for resampling scaler required)
             - Keep last 5 rendered pages in cache to speedup scrolling

28-Aug-2008  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.36)
             - New user configurable background rendering (option progressive_render_pages)
               -> Improves page switching performance, especially when using high quality
                  scaler. On dual-core systems the performance gain will be much higher
                  than on single-core systems.
               -> Always progressive_render_pages/2 pages backward and forward will
                  be calculated in background.
             - For lengthy operations now the hourglas mouse pointer is shown
             - JPEG2000 support (requires GBM.DLL 1.60 or higher)

21-Sep-2008  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.37)
             - Fix wrong extension to codec assignment for JPEG2000
             - Move the URL in the plugin info to the GBM homepage to allow correct
               appearance in Firefox's plugin manager
             - changes for GCC compile support

27-Sep-2008  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.38)
             - Enforce struct padding to 4 byte for compiler independent compatibility

21-Sep-2009  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.39)
             - Fix rendering of B&W bitmaps as PM needs special data ordering for it.
             - Fix grab focus issue in NPP_SetWindow and rework error handling (thanks to Rich Walsh)
             - Add JBIG support (1bpp, 8bpp)

22-Apr-2010  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.40)
             - Fix huge memory consumption when doing background rendering
               of multiple pages that have a big size difference.

21-Sep-2010  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.41)
             - Quality fine tuning of resampling scalers
               (flat colour areas are now preserved to get better quality)
             - Performance tuning of resampling scalers
               * Multithreading support (fully dynamic CPU core usage)
               * SSE support for Microsoft, GCC and Open Watcom Compilers
             - Some improvements for keyboard handling
               (add shortcuts for popup menus and embedded view)
             - Some restructuring
             - Fix output size shift which lead to slightly distorted drawings
             - Fix popup menu activation (now system conform)
             - Fix sometimes not reacting pages switch accelerators
             - Fix wrong size calculation in background rendering of multiple pages
               which causes uneffective prefetching
             - Add multithreaded rendering for improved SMP support
             - First Windows version of the plugin with almost the same features.
               Only the menu bar at the top in FULL mode is missing compared to the
               OS/2 version.

09-Nov-2010  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.42)
             - Add preferences dialog to setup scaler algorithm and prefetching pages
             - Add resampling scaler filters:
               * blackman, catmullrom, quadratic, gaussian, kaiser
             - Final fix for sometimes generated moire due to rounding issues
               in sample contribution calculations.

20-Feb-2011  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.43)
             * Windows: Mark plugin DLL as NX compatible and large address aware
               (>2GB as 32bit on 64bit system)
             * Add mime type image/x-bmp
             * Now requires GBM.DLL 1.72 (included in XPI package)

21-Jun-2011  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.44)
             * Now requires GBM.DLL 1.73 (included in XPI package)

24-Oct-2011  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.45)
             * Included gbm.dll updated to version 1.74
               -> Minimum required version is still 1.73 though update is encouraged

26-Feb-2012  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.46)
             * Included gbm.dll updated to version 1.75
               -> Minimum required version is still 1.73 though update is encouraged

28-Oct-2012  GBM plugin for Netscape/Mozilla/Firefox/SeaMonkey (NPGBM.DLL version 1.47)
             * Integrate updated gbmmthrd.lib (see gbm doc)

