
//***************************************************
// Generic Configuration handler class
//
// Copyright (C) 2008-2010 Heiko Nitzsche
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
// File format:
//   # comment
//   keyname1=value1 # comment
//   keyname2=value2
//
//***************************************************

#ifndef __CLASS_CONFIG_READER__
    #define __CLASS_CONFIG_READER__

#if defined(__OS2__)
  #include "os2defs.h"
#elif defined(WIN32)
  #include "windefs.h"
#endif

#include <string.h>
#include "Semaphore.hpp"

// ---------------------------------------------------------

class ConfigHandler
{
    public:
        class KeyIterator;

        class Key
        {
            private:
                char * mKeyname;
                char * mValue;

            public:
                ~Key()
                {
                    delete [] mKeyname;
                    delete [] mValue;
                }
                Key()
                  : mKeyname(NULL),
                    mValue(NULL)
                {
                }

                Key(const char * keyname, const char * value)
                  : mKeyname(NULL),
                    mValue(NULL)
                {
                    mKeyname = new char[strlen(keyname)+1];
                    if (mKeyname != NULL) strcpy(mKeyname, keyname);
                    mValue = new char[strlen(value)+1];
                    if (mValue != NULL) strcpy(mValue, value);
                }

                Key(const Key & key)
                  : mKeyname(NULL),
                    mValue(NULL)
                {
                    mKeyname = new char[strlen(key.mKeyname)+1];
                    if (mKeyname != NULL) strcpy(mKeyname, key.mKeyname);
                    mValue = new char[strlen(key.mValue)+1];
                    if (mValue != NULL) strcpy(mValue, key.mValue);
                }

                Key & operator = (const Key & key)
                {
                    if (&key == this) return *this;
                    delete [] mKeyname;
                    delete [] mValue;
                    mKeyname = new char[strlen(key.mKeyname)+1];
                    if (mKeyname != NULL) strcpy(mKeyname, key.mKeyname);
                    mValue = new char[strlen(key.mValue)+1];
                    if (mValue != NULL) strcpy(mValue, key.mValue);
                    return *this;
                }

                const char * getKeyname() const { return mKeyname; }
                const char * getValue()   const { return mValue; }
        };

    private:
        friend class KeyIterator;

        // --- data definitions ---

        char * mFilename;
        Key  * mKeys;
        int    mNumKeys;

        MutexSemaphore mInterfaceMutex;
        
        // --- functions ---

        ConfigHandler(const ConfigHandler & reader);
        ConfigHandler & operator = ( const ConfigHandler & reader );

        const Key * getKey(const char * keyname) const;
        
    public:

        class KeyIterator
        {
            private:
                int mIndex;
                const ConfigHandler & mReader;

                friend class ConfigHandler;
                KeyIterator(const ConfigHandler & reader);

            public:
                ~KeyIterator();

                BOOL hasMore() const;
                const Key & next();
        };

        ConfigHandler(const char * filename);
        virtual ~ConfigHandler();

        void clear();
        BOOL append(const char * keyname, const char * value);

        const char * getFilename() const;
        BOOL read();
        BOOL save() const;

        KeyIterator getIterator() const;

        const char * getValueOfKey(const char * keyname) const;
        int getNumberOfKeys() const;
};

#endif


