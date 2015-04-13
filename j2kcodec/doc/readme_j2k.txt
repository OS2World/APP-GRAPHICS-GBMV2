
OpenJPEG JPEG2000 image decoder and encoder
===========================================
(28-Oct-2012, Heiko Nitzsche)

This package contains the JPEG2000 image decoder and encoder
provided by the OpenJPEG 1.5.1 library. JPWL support is included.


Installation
------------
There is no installation required. Just use the 2 command line tools.


Documentation
----------------------
Please have a look at http://www.openjpeg.org/index.php?menu=doc


Developers
----------
The source code can be found in the GBM (Generalised Bitmap Module)
source code package provided by Heiko Nitzsche.

You can find it on Hobbes or the GBM homepage:
http://heikon.home.tlink.de

The binaries can be build with OpenWatcom 1.8, GCC and MS C++.


Legal stuff
-----------
This software is based in part on the OpenJPEG library.
It is Open Source and can be found on www.openjpeg.org.

This software is based in part on the work of the Independent JPEG Group.
The source package ships with full source code of the Independent JPEG Group
JPEGLIB and may be used if they are credited, (which I do so here).

This software is based in part on the Libpng and Zlib libraries.
Both are Open Source and can be found on www.libpng.org and www.zlib.org.

This software is based in part on the Libtiff library.
It is Open Source and can be found on www.libtiff.org.
Some adaptions for OS/2 have been done.


Contact
-------
Heiko.Nitzsche@gmx.de (http://heikon.home.tlink.de)


History
-------
28-Aug-2008: * image_to_j2k.exe and j2k_to_image.exe shipped with OpenJPEG version 1.3+
             * Versions of used add-on libs:
               - libtiff 3.8.2
               - zlib 1.2.3
               - IJG 6b

11-Sep-2008  * add JPWL support

21-Sep-2008  * add GCC 3.3.5 compiler support for OS/2

25-Oct-2008  * add OpenWatcom compiler support for Windows NT/2000/XP/Vista
             * add Microsoft C++ compiler support for Windows NT/2000/XP/Vista
             * add GCC compiler support for Linux (32/64bit)

13-Nov-2009: * Now using libtiff 3.9.2 and IJG JPEG 7
             * Add Microsoft C++ compiler support for 64bit systems (tested on Windows 7)

 2-Mar-2010: * Now using IJG JPEG 8a

24-Apr-2010: * Now using zlib 1.2.4

18-Jun-2010: * Now using zlib 1.2.5
             * Now using IJG JPEG 8b
             * Now using libtiff 3.9.4

20-Feb-2011: * Now using IJG JPEG 8c
             * Now using OpenJPEG 1.4
             * Windows: Mark executables as NX compatible and large address aware
                        (>2GB as 32bit on 64bit system)

26-Feb-2012: * Now using OpenJPEG 1.5
             * Now using IJG JPEG 8d, Libpng 1.5.9, Zlib 1.2.6, Libtiff 3.9.5

28-Oct-2012: * Now using OpenJPEG 1.5.1
             * Now using IJG JPEG 8d, Libpng 1.5.13, Zlib 1.2.7

