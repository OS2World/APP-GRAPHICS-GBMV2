//***************************************************
// Generalised Bitmap Module C++ adapter for GBM plugins
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

#include "GbmException.hpp"

#include <string.h>
#include <stdio.h>


// constructor
// -----------
GbmException::GbmException( const GBM_ERR   errorId,
                            const int       errorLine,
                            const char    * errorFile)
    : line(errorLine),
      id  (errorId)
{
    // alloc memory for the filename and copy the filename to the buffer
    errorFilename = new char[strlen(errorFile) + 1];
    strcpy(errorFilename, errorFile);

    // set the user error message
    const char * userMessage = "";

    // allocate buffer for: gbmMessage contained in message
    message = new char[strlen(userMessage) + 1];
    strcpy(message, userMessage);
}

// constructor
// -----------
GbmException::GbmException( const GBM_ERR   errorId,
                            const char    * errorMessage,
                            const int       errorLine,
                            const char    * errorFile)
    : line(errorLine),
      id  (errorId)
{
    // alloc memory for the filename and copy the filename to the buffer
    errorFilename = new char[strlen(errorFile) + 1];
    strcpy(errorFilename, errorFile);

    // get the GBM error message
    const size_t errorMessageLen = strlen(errorMessage);

    // allocate buffer for: "errorMessage: gbmMessage"
    message = new char[errorMessageLen + 1];
    strcpy(message, errorMessage);
}


// copy constructor
// ----------------
GbmException::GbmException( const GbmException & ex )
    : line(ex.line),
      id  (ex.id)
{
    // copy error filename
    errorFilename = new char[strlen(ex.errorFilename) + 1];
    strcpy(errorFilename, ex.errorFilename);

    // copy error message
    message = new char[strlen(ex.message) + 1];
    strcpy(message, ex.message);
}


// destructor
// ----------
GbmException::~GbmException()
{
    delete [] errorFilename;
    delete [] message;
    errorFilename = NULL;
    message       = NULL;
}


// operator =
GbmException & GbmException::operator = ( const GbmException & ex )
{
    if (&ex == this)
    {
        return *this;
    }
    delete [] errorFilename;
    delete [] message;
    errorFilename = NULL;
    message       = NULL;

    // copy error filename
    errorFilename = new char[strlen(ex.errorFilename) + 1];
    strcpy(errorFilename, ex.errorFilename);

    // copy error message
    message = new char[strlen(ex.message) + 1];
    strcpy(message, ex.message);

    line = ex.line;
    id   = ex.id;

    return *this;
}


