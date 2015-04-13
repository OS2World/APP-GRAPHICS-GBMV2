
//***************************************************
// Generalised Bitmap Module C++ adapter for GBM plugins
//
// Copyright (C) 2006-2010 Heiko Nitzsche
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
//***************************************************

#ifndef _WINDEFS_H_
  #define _WINDEFS_H_

#ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0500 //define minimum windows version to "Windows 2000"
#endif
#ifndef WINVER
  #define WINVER 0x0500 //define minimum windows version to "Windows 2000"
#endif

#if (WINVER >= 0x0600)
  // For Vista and higher features (i.e. new file dialog)
  #ifndef NTDDI_VERSION
    #define NTDDI_VERSION 0x06000000
  #endif
  #ifndef _WIN32_IE
    #define _WIN32_IE 0x0700
  #endif
#endif

#include <windows.h>

#endif


