
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

#ifndef __CLASS_GBM_EXCEPTION__
    #define __CLASS_GBM_EXCEPTION__

#include "gbm.h"

// ---------------------------------------------------------

class GbmException
{
    private:
        GbmException();

    protected:
        int       line;
        GBM_ERR   id;
        char    * message;
        char    * errorFilename;

    public:

        // constructor
        GbmException( const GBM_ERR   errorId,
                      const int       errorLine = __LINE__,
                      const char    * errorFile = __FILE__);

        // constructor
        GbmException( const GBM_ERR   errorId,
                      const char    * errorMessage,
                      const int       errorLine = __LINE__,
                      const char    * errorFile = __FILE__);

        // copy constructor
        GbmException( const GbmException & ex );

        // destructor
        virtual ~GbmException();

        // operator =
        GbmException & operator = ( const GbmException & ex );

        // queries
        GBM_ERR      getErrorId()      const { return id; }
        int          getErrorLine()    const { return line; }
        const char * getErrorFile()    const { return errorFilename; }

        // return a verbal error description
        const char * getErrorMessage() const { return message; }
};


#endif



