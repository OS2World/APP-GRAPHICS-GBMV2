
//***************************************************
// General Semaphore Implementations
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
//
//***************************************************

#ifndef __SEMAPHORES__
    #define __SEMAPHORES__

#include <assert.h>
    
#if defined(__OS2__)
  #include "os2defs.h"
  typedef  HMTX    MUTEX_SEM;
  typedef  HEV     EVENT_SEM;
#elif defined(WIN32)
  #include "windefs.h"
  typedef  HANDLE  MUTEX_SEM;
  typedef  HANDLE  EVENT_SEM;
#endif

// ---------------------------------------------------------

class MutexSemaphore
{
    public:
        MutexSemaphore();
        ~MutexSemaphore();
        
        BOOL tryLock(unsigned long milliSeconds);
        BOOL lock();
        void unlock();
    
    private:
        MUTEX_SEM mMutex;
        MutexSemaphore(const MutexSemaphore &);               // not copy
        MutexSemaphore & operator = (const MutexSemaphore &); // no assignment
};

class MutexGuard
{
    public:
        MutexGuard(MutexSemaphore & mutex)
            : mMutex(mutex)
        {
            if (! mMutex.lock())
            {
                assert(FALSE);
            }
        }
        ~MutexGuard()
        {
            mMutex.unlock();
        }
    
    private:
        MutexSemaphore & mMutex;
        MutexGuard(const MutexGuard &);               // not copy
        MutexGuard & operator = (const MutexGuard &); // no assignment
};

// ---------------------------------------------------------

class EventSemaphore
{
    public:
        EventSemaphore();
        ~EventSemaphore();
        
        BOOL wait();
        BOOL wait(unsigned long milliSeconds);
        void post();
        void reset();
    
    private:
        EVENT_SEM mEvent;
        EventSemaphore(const EventSemaphore &);               // not copy
        EventSemaphore & operator = (const EventSemaphore &); // no assignment
};

#endif


