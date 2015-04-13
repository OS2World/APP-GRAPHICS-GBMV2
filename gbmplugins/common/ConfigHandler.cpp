
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
//***************************************************

#ifdef __IBMCPP__
  #pragma strings( readonly )
#endif

#include "ConfigHandler.hpp"
#include <string.h>
#include <stdio.h>
#include <assert.h>

// ---------------------------------------------------------
// ---------------------------------------------------------

ConfigHandler::ConfigHandler(const char * filename)
  : mFilename(NULL),
    mKeys(NULL),
    mNumKeys(0)
{
    const size_t len = strlen(filename);
    try
    {
        mFilename = new char[len + 1];
    }
    catch(...)
    {
    }
    if (mFilename != NULL)
    {
        strcpy(mFilename, filename);
    }
}

// ---------------------------------------------------------

ConfigHandler::~ConfigHandler()
{
    mInterfaceMutex.lock();

    delete [] mKeys;
    mKeys = NULL;
    mNumKeys = 0;

    delete [] mFilename;
    mFilename = NULL;
}

// ---------------------------------------------------------

void ConfigHandler::clear()
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    delete [] mKeys;
    mKeys = NULL;
    mNumKeys = 0;
}

// ---------------------------------------------------------

BOOL ConfigHandler::append(const char * keyname, const char * value)
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    int i;

    // check if the key already exists
    for (i = 0; i < mNumKeys; ++i)
    {
        const Key & key(mKeys[i]);
        if (strcmp(keyname, key.getKeyname()) == 0)
        {
            // update the entry
            mKeys[i] = Key(keyname, value);
            return TRUE;
        }
    }

    // increase the key array
    Key * newKeys = NULL;
    try
    {
        newKeys = new Key[mNumKeys + 1];
    }
    catch(...)
    {
        newKeys = NULL;
    }
    if (newKeys == NULL)
    {
        return FALSE;
    }
    // copy the old elements to the new array
    for (i = 0; i < mNumKeys; ++i)
    {
        newKeys[i] = mKeys[i];
        assert((newKeys[i].getKeyname() != NULL) &&
               (newKeys[i].getValue()   != NULL));
    }
    // add the new key
    newKeys[mNumKeys] = Key(keyname, value);
    assert((newKeys[mNumKeys].getKeyname() != NULL) &&
           (newKeys[mNumKeys].getValue()   != NULL));

    delete [] mKeys;
    mKeys = newKeys;

    ++mNumKeys;

    return TRUE;
}

// ---------------------------------------------------------

const char * ConfigHandler::getFilename() const
{
    return mFilename;
}

// ---------------------------------------------------------

BOOL ConfigHandler::read()
{
    MutexGuard interfaceGuard(mInterfaceMutex);
    clear();

    if (mFilename == NULL)
    {
        return FALSE;
    }

    // read an ASCII file line by line, ignore everything behind # character
    FILE * file = fopen(mFilename, "r");
    if (file != NULL)
    {
        char readBuffer[101]  = { 0 };
        char keyBuffer[101]   = { 0 };
        char valueBuffer[101] = { 0 };

        // get the next line
        while (fgets(readBuffer, sizeof(readBuffer), file) != NULL)
        {
            // skip line if it starts with a # (coment)
            char * pComment = readBuffer;
            while (*pComment == ' ')
            {
                ++pComment;
            }
            switch(*pComment)
            {
                case 0:
                case '\n':
                case '#':
                    continue; // read next line

                default:
                    // all other characters are handled below
                    break;
            }

            // split into key and value
            const int ret = sscanf(readBuffer,
                                   "%100[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_]%*[=]%100[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789]",
                                   keyBuffer, valueBuffer);
            // key/value found?
            if (ret == 2)
            {
                if (! this->append(keyBuffer, valueBuffer))
                {
                    fclose(file);
                    clear();
                    return FALSE;
                }
            }
        }
        fclose(file);
    }
    return TRUE;
}

// ---------------------------------------------------------

BOOL ConfigHandler::save() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if (mFilename == NULL)
    {
        return FALSE;
    }

    // read an ASCII file as a whole
    FILE * file = fopen(mFilename, "r");

    // ------------------------------------

    // the file does not exist -> create a new one
    if (file == NULL)
    {
        file = fopen(mFilename, "w");
        if (file == NULL)
        {
            return FALSE;
        }

        KeyIterator keyIt(getIterator());
        while (keyIt.hasMore())
        {
            const Key & key(keyIt.next());
            if (fprintf(file, "%s=%s\n", key.getKeyname(), key.getValue()) <= 0)
            {
                // I/O error
                fclose(file);
                remove(mFilename);
                return FALSE;
            }
        }
        // DONE
        fclose(file);
        return TRUE;
    }

    // ------------------------------------

    // the file exists -> replace existing key values
    if (fseek(file, 0, SEEK_END))
    {
        fclose(file);
        return FALSE;
    }
    long fileLength = ftell(file);
    if (fseek(file, 0, SEEK_SET))
    {
        fclose(file);
        return FALSE;
    }
    char * fileBuffer = NULL;
    try
    {
        fileBuffer = new char[fileLength + 1];
    }
    catch(...)
    {
        fileBuffer = NULL;
    }
    if (fileBuffer == NULL)
    {
        fclose(file);
        return FALSE;
    }
    // read the file contents into the buffer
    memset(fileBuffer, 0, fileLength + 1);
    fread(fileBuffer, sizeof(char), fileLength, file);
    if (ferror(file))
    {
        delete [] fileBuffer;
        fclose(file);
        return FALSE;
    }
    fclose(file);
    fileLength = (long)strlen(fileBuffer);

    // now reopen file for writing
    file = fopen(mFilename, "w");
    if (file == NULL)
    {
        delete [] fileBuffer;
        return FALSE;
    }

    // remember the keys that were merged
    int mergedKeysIndex = -1;
    const Key ** mergedKeys = NULL;
    try
    {
        mergedKeys = new const Key *[mNumKeys];
    }
    catch(...)
    {
        mergedKeys = NULL;
    }
    if (mergedKeys == NULL)
    {
        fclose(file);
        delete [] fileBuffer;
        return FALSE;
    }

    // ------------------------------------

    // search for the keys, replace the value if found
    const char * readPointer = fileBuffer;

    while ((readPointer - fileBuffer + 1) <= fileLength)
    {
        // skip white spaces and newlines
        while (((readPointer - fileBuffer + 1) <= fileLength) &&
               ((*readPointer == ' ') || (*readPointer == '\n')))
        {
            if (fwrite(readPointer, sizeof(char), 1, file) != sizeof(char))
            {
                // I/O error
                delete [] fileBuffer;
                delete [] mergedKeys;
                fclose(file);
                remove(mFilename);
                return FALSE;
            }
            ++readPointer;
        }

        // Comment found?
        if (((readPointer - fileBuffer + 1) <= fileLength) && (*readPointer == '#'))
        {
            // copy the whole line
            while (((readPointer - fileBuffer + 1) <= fileLength) && (*readPointer != '\n'))
            {
                if (fwrite(readPointer, sizeof(char), 1, file) != sizeof(char))
                {
                    // I/O error
                    delete [] fileBuffer;
                    delete [] mergedKeys;
                    fclose(file);
                    remove(mFilename);
                    return FALSE;
                }
                ++readPointer;
            }
            continue;
        }
        else
        {
            char keyBuffer[101] = { 0 };
            char valBuffer[101] = { 0 };
            int offsetVal = 0;

            // split into key and value
            const int ret = sscanf(readPointer,
                                   "%100[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_]%*[=]%n%100[ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789]",
                                   keyBuffer, &offsetVal, valBuffer);
            // key found?
            if (ret == 2)
            {
                const size_t valLen = strlen(valBuffer);

                // check if the key is stored
                const Key * storedKey = getKey(keyBuffer);
                if (storedKey)
                {
                    if (fprintf(file, "%s=%s\n", keyBuffer, storedKey->getValue()) <= 0)
                    {
                        // I/O error
                        delete [] fileBuffer;
                        delete [] mergedKeys;
                        fclose(file);
                        remove(mFilename);
                        return FALSE;
                    }

                    // put into the merged keys list
                    ++mergedKeysIndex;
                    assert(mergedKeysIndex < mNumKeys);
                    mergedKeys[mergedKeysIndex] = storedKey;
                }
                else
                {
                    // copy the key
                    if (fprintf(file, "%s=%s\n", keyBuffer, valBuffer) <= 0)
                    {
                        // I/O error
                        delete [] fileBuffer;
                        delete [] mergedKeys;
                        fclose(file);
                        remove(mFilename);
                        return FALSE;
                    }
                }
                readPointer += 1 + offsetVal + valLen;
            }
        }
    }

    // Any remaining keys? Append them to the file end.
    if (mergedKeysIndex > -1)
    {
        KeyIterator keyIt(getIterator());
        while (keyIt.hasMore())
        {
            const Key & key(keyIt.next());

            // Is the key merged already?
            BOOL found = FALSE;
            for (int i = 0; i <= mergedKeysIndex; ++i)
            {
                if (strcmp(key.getKeyname(), mergedKeys[i]->getKeyname()) == NULL)
                {
                    found = TRUE;
                    break;
                }
            }
            if (! found)
            {
                if (fprintf(file, "%s=%s\n", key.getKeyname(), key.getValue()) <= 0)
                {
                    // I/O error
                    delete [] fileBuffer;
                    delete [] mergedKeys;
                    fclose(file);
                    remove(mFilename);
                    return FALSE;
                }
            }
        }
    }

    delete [] fileBuffer;
    delete [] mergedKeys;
    fclose(file);

    return TRUE;
}

// ---------------------------------------------------------

ConfigHandler::KeyIterator ConfigHandler::getIterator() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return ConfigHandler::KeyIterator(*this);
}

// ---------------------------------------------------------

const ConfigHandler::Key * ConfigHandler::getKey(const char * keyname) const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if (keyname != NULL)
    {
        // lookup key
        for (int i = 0; i < mNumKeys; i++)
        {
            const Key & key(mKeys[i]);
            if (strcmp(keyname, key.getKeyname()) == 0)
            {
                return &(mKeys[i]);
            }
        }
    }
    return NULL;
}

// ---------------------------------------------------------

const char * ConfigHandler::getValueOfKey(const char * keyname) const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    if (keyname != NULL)
    {
        // lookup key
        for (int i = 0; i < mNumKeys; i++)
        {
            const Key & key(mKeys[i]);
            if (strcmp(keyname, key.getKeyname()) == 0)
            {
                return mKeys[i].getValue();
            }
        }
    }
    return NULL;
}

// ---------------------------------------------------------

int ConfigHandler::getNumberOfKeys() const
{
    MutexGuard interfaceGuard((MutexSemaphore &)mInterfaceMutex);
    return mNumKeys;
}

// ---------------------------------------------------------
// ---------------------------------------------------------

ConfigHandler::KeyIterator::KeyIterator(const ConfigHandler & reader)
  : mIndex(0),
    mReader(reader)
{
}

// ---------------------------------------------------------

ConfigHandler::KeyIterator::~KeyIterator()
{
}

// ---------------------------------------------------------

BOOL ConfigHandler::KeyIterator::hasMore() const
{
    return (mIndex < mReader.mNumKeys) ? TRUE : FALSE;
}

// ---------------------------------------------------------

const ConfigHandler::Key & ConfigHandler::KeyIterator::next()
{
    assert(hasMore());
    const Key & key(mReader.mKeys[mIndex]);
    mIndex++;
    return key;
}

