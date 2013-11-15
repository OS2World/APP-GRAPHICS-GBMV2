GBM REXX adapter DLL version 1.16
---------------------------------
(Heiko Nitzsche, Public Domain)

GBM stands for Generalised Bitmap Module.

GBMRX.DLL is a REXX adapter DLL for OS/2 and eComStation which
REXX programs may use to load or save all bitmap file formats
supported by the GBM.DLL (to/from a file).

Almost full feature set of GBM.DLL is provided by the REXX API.
I/O remapping is not supported due to language restrictions.
Scaling functionality and palette bitmap to 24bpp expansion is
also available.

For being able to use it, the following additional files are required:
- GBM.DLL version 1.35 or higher (GBM Bitmap Format Handler)
  (version below will work but querying the number of pages
   within the bitmap will fail)

More details to this package (description, API reference) and about
GBM in general can be found at http://heikon.home.tlink.de or in
the included documentation.



GBMDLG REXX adapter DLL version 1.04
------------------------------------
(Heiko Nitzsche, Public Domain)

GBM stands for Generalised Bitmap Module.

GBMDLGRX.DLL is a REXX adapter DLL for OS/2 and eComStation which
REXX programs may use to select bitmap filenames to load or save.

It uses GBMDLG (GBM File Dialog). Full online help is available
via GBMDLG.HLP.

For being able to use it, the following additional files are required:
- GBM.DLL     (GBM Bitmap Format Handler)
- GBMDLG.DLL  (GBM File Dialog, version 1.31)
- GBMDLG.HLP  (GBM File Dialog Help)

More details to this package (description, API reference) and about
GBM in general can be found at http://heikon.home.tlink.de or in
the included documentation.



Contact
-------
Heiko.Nitzsche@gmx.de (http://heikon.home.tlink.de)



History
-------

16-May-2006: GBMRX.DLL 1.00, a REXX adapter DLL for GBM.DLL
             * Supports full feature set of GBM.DLL except I/O remapping
             * Requires GBM.DLL 1.35 for using all features but also
               works with older version (> 1.09) with reduced feature set.
               (no possibility to query the number of pages contained in the file)

             GBMDLGRX.DLL 1.01
             * Add function to query module version


5-Jun-2006:  Modified compiler options. No functional changes.

16-Aug-2006: GBMRX.DLL 1.01
             * Load GBM.DLL via "GBM" rather than "GBM.DLL" so that
               it is found also with LIBPATHSTRICT enabled.
             * Add bldlevel info

             GBMDLGRX.DLL 1.02
             * Add bldlevel info

             Makefiles
             * Makefiles now support activating debug compile without editing them.
               Just use nmake or wmake with parameter debug=on

28-Nov-2006: GBMDLGRX.DLL 1.02
             * Fix file dialog subclassing, requires now GBM File Dialog 1.31
             * Make library usable from multiple threads of a process (is thread-safe now)

27-Jan-2008: GBMRX.DLL 1.10
             * Add scaling capability (simple + resampling)
             * Add query for available scaling algorithms
             * Add test routine for checking of supported scaling algorithm
             * Add routine to expand palette bitmaps to 24bpp true color
             * Add support for reflect and transpose
             * Add support for rotating in 90 degree increments
             * Add description of new APIs to documentation

12-Dec-2008: GBMDLGRX.DLL 1.03
             * Fix file dialog subclassing, requires now GBM File Dialog 1.31
             * Make library usable from multiple threads of a process (is thread-safe now)

06-Feb-2008: GBMRX.DLL 1.11
             * Allocate memory from high memory for bitmap data to
               stretch limit for out-of-resource exhausted errors
               (requires kernel with high memory support)

21-Sep-2008  GBMRX.DLL 1.11, GBMDLGRX.DLL 1.04
             * changes for GCC compile support

28-Oct-2008  GBMRX.DLL 1.12, GBMDLGRX.DLL 1.04
             * incorporate changes done during porting GBM to Windows & Linux

25-Sep-2010  GBMRX.DLL 1.14
             * Quality fine tuning of resampling scalers
               (flat colour areas are now preserved to get better quality)
             * Multithreading & SSE support for resampling scalers (fully dynamic CPU core usage)

02-Nov-2010: GBMRX.DLL 1.15
             * Add resampling scaler filters:
               - blackman, catmullrom, quadratic, gaussian, kaiser
             * Final fix for sometimes generated moire due to rounding issues
               in sample contribution calculations.

28-Oct-2012: GBMRX.DLL 1.16
             * Integrate updated gbmmthrd.lib (see gbm doc)



