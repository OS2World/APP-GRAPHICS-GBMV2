//********************************************************************************
// Mozilla GBM plug-in unicode helpers: unicode.cpp
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
//********************************************************************************

// This module is fully Unicode compliant.
#define UNICODE

#include <stdlib.h>
#include <string.h>
#include <string>

// ---------------------------------------------------------

// Convert a char string to a wchar string
std::wstring string2wstring(const std::string & str)
{
    std::wstring wstr;

    const size_t numElements    = str.length();
    const size_t numElementsMax = numElements + 1;
    wchar_t * wcstring = NULL;
    try
    {
        wcstring = new wchar_t[numElementsMax];
    }
    catch(...)
    {
        wcstring = NULL;
    }
    if (wcstring)
    {
        memset(wcstring, 0, numElementsMax * sizeof(wchar_t));
        
        // Convert char* string to a wchar_t* string.
        const size_t convertedChars = mbstowcs(wcstring, str.c_str(), numElements);
        if (convertedChars == numElements)
        {
            wstr = wcstring;
        }
        delete [] wcstring;
    }
    return wstr;
}

// ---------------------------------------------------------

// Convert a wchar string to a char string
std::string wstring2string(const std::wstring & wstr)
{
    std::string str;

    const size_t numElements    = wstr.length();
    const size_t numElementsMax = numElements + 1;
    char * cstring = NULL;
    try
    {
        cstring = new char[numElementsMax];
    }
    catch(...)
    {
        cstring = NULL;
    }
    if (cstring)
    {
        memset(cstring, 0, numElementsMax);
        
        // Convert wchar_t* string to a char* string.
        const size_t convertedChars = wcstombs(cstring, wstr.c_str(), numElements);
        if (convertedChars == numElements)
        {
            str = cstring;
        }
        delete [] cstring;
    }
    return str;
}

