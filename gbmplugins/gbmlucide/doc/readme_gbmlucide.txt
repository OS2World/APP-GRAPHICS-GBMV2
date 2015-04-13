GBM plugin for Lucide 1.43
--------------------------

GBM stands for Generalised Bitmap Module.

This is a plugin for the Lucide document viewer available for eComStation
that extends Lucide for the support of reading all bitmap formats
supported by GBM.DLL.

Supported features:
* document info
* loading all formats supported by GBM
* save loaded bitmap if it has only one page
* multipage bitmap viewing support
* load bitmap page on demand
* scaling and rotation
* file thumbnail creation
* high quality scaling algorithm for improved appearance
* configurable background rendering for fast page switching and continuous scrolling

For being able to use it, the following additional files are required:
- Lucide RC1 or higher
- GBM.DLL version 1.42 or higher (GBM Bitmap Format Handler)
  (version below will work but querying the number of pages
   within the bitmap will fail, GBM.DLL 1.41 is not supported)

More details to this package and about GBM in general can be found
at http://heikon.home.tlink.de or in the included documentation.


Installing GBM for Lucide
-------------------------
Simply copy lugbm.dll into the Lucide installation directory. Please check
the requirements listed for gbm.dll above. Copy gbm.dll either into the
Lucide installation directory beside lugbm.dll or put it somewhere in your
LIBPATH (defined in CONFIG.SYS). The last option is recommended if gbm.dll
is also required by other programs.


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

27-Jun-2006  <b>NEW:</b> GBM plugin for Lucide (LUGBM.DLL version 1.0 beta 1)
             * Initial public beta

30-Jun-2006  GBM plugin for Lucide (LUGBM.DLL version 1.0 beta 2)
             * Fixes for the most urgent issues
               - file extension list too long for file dialog (only keep the first extension)
                 which causes a cutted list of shown file extensions and a crash on pressing
                 Cancel in the file dialog (buffer overflow in Lucide)
             * Add error message reporting to load_file())

16-Jul-2006  GBM for Lucide 1.00 (LUGBM.DLL version 1.00 final)
             - It requires Lucide 1.2 due to plugin API changes
             - Added document info
             - Added rotated rendering
             - Added delayed loading of bitmap data (on user request)
             - Support transparent caching for fast scrolling of a scaled view

31-Jul-2006  GBM for Lucide 1.10 (LUGBM.DLL)
             - Save loaded bitmap in any GBM supported format if it has only one page
               (GBM does not support multipage save, requires Lucide beta 2.1)
             - Remove code for rendering to a buffer, rendering is now always via HPS
             - Sync to latest common code shared with the Mozilla plugin
               updated common code
             - Now supports dynamic lookup of GBM.DLL either via LIBPATH
               or look into the directory where lugbm.dll is located.

16-Aug-2006  GBM for Lucide 1.20 (LUGBM.DLL)
             - Adapt makefiles to use now the patched SOM header files
               part of the Lucide plugindev pack.
             - Fix for issue that GBM.DLL is not found when LIBPATHSTRICT (or RUN!)
               is used. Thanks to Rich Walsh for the background info about the root cause.
             - Add bldlevel info
             - Makefiles now support activating debug compile without editing them.
               Just use nmake or wmake with parameter debug=on

31-Aug-2006  Version 1.21
             - When reporting an extended error message after a loading error
               sprintf reads from an illegal address due to wrong parameter
               (int instead char pointer)

08-Sep-2006  Version 1.22
             - 1bpp bitmaps were shown inverted (colors swapped)

28-Nov-2006  Version 1.23
             - Improved detection of bitmap file formats
             - Adapt loadFile() and renderToPS() for Lucide 1.0 API
               (error reporting added to the API)
             - OpenWatcom 1.6 is now supported
               Note: In the Lucide plugin linker description file the exports have to be adapted
                     if OpenWatcom before 1.6 is used for compile (see the remarks in the file
                     lugbm.lnk)

20-Jan-2007  Version 1.24
             - Some fixes for upcoming Lucide RC1
             - Protect getPageSize() against writing to width or height
               for the unusual case they're NULL.
             - Don't write to error/errorCode pointer if it is NULL.
               This is a bit strange as one would expect that they
               would be always valid.

06-Sep-2007  Version 1.30
             - Add support for Lucide 1.02 API extension isFileSupported()
               to allow file format detection via file content
             - Add support for creating thumbnails in EAs
             - Report all available extensions again as the issue with
               Lucide file dialog has been resolved since Lucide 1.10.
               So this plugin version should not be used with older Lucide!
             - 8bpp grayscale and true color images are now scaled with a high quality
               algorithm to improve appearance.

04-Nov-2007  Version 1.31
             - Resampling scaler with improved accuracy

27-Jan-2008  Version 1.32
             - Resampling scaler now supports all grayscale bitmaps <=8bpp

08-Feb-2008  Version 1.33
             - Move HPS rendering code to common package GbmRenderer
               to save memory (now renders directly from cache buffer)
               without temporary subrectangle buffer
             - Allocate memory from high memory for bitmap data to
               stretch limit for out-of-memory errors
               (requires kernel with high memory support)
             - Custom settings definable in lugbm.cfg file
               (so far scaler={simple,nearestneighbor,bilinear,bell,bspline,mitchell,lanczos})
               The default scaler is simple to allow fast scaling on less powerful systems.

03-Jun-2008  Version 1.34
             - Wrong cache preparation for 1bpp bitmaps when simple scaler is active
               (no upconversion to 8bpp as for resampling scaler required)
             - New user configurable background rendering (option progressive_render_pages)

28-Aug-2008  Version 1.35
             - New user configurable background rendering (option progressive_render_pages)
               -> Improves continous scrolling/page switching performance, especially when
                  using high quality scaler. On dual-core systems the performance gain
                  will be much higher than on single-core systems.
               -> Always progressive_render_pages/2 pages backward and forward will
                  be calculated in background.
             - For lengthy operations now the hourglas mouse pointer is shown
             - The size and color depth of the current page is shown behind the format
               name in the document information dialog
             - Fix potential write to const memory object when building extension list

21-Sep-2008  Version 1.36
             - changes for GCC compile support

27-Sep-2008  Version 1.37
             Enforce struct padding to 4 byte for compiler independent compatibility

22-Feb-2009  Version 1.38
             Fix some incompatibilites with new OpenWatcom 1.8 compiler

21-May-2009  Version 1.39
             - Fix rendering of B&W bitmaps as PM needs special data ordering for it.

22-Apr-2010  Version 1.40
             - Fix huge memory consumption when doing background rendering
               of multiple pages that have a big size difference.

20-Aug-2010  Version 1.41
             - Quality fine tuning of resampling scalers
               (flat colour areas are now preserved to get better quality)
             - Some minor internal changes on the core classes shared with Mozilla
               plugin for the Windows port
             - Fix output size shift which lead to slightly distorted drawings
             - Fix wrong size calculation in background rendering of multiple pages
               which causes uneffective prefetching

02-Nov-2010  Version 1.42
             - Add resampling scaler filters:
               * blackman, catmullrom, quadratic, gaussian, kaiser
             - Final fix for generated moire due to rounding issues
               in contribution calculations.

28-Oct-2012  Version 1.43
             * Integrate updated gbmmthrd.lib (see gbm doc)


