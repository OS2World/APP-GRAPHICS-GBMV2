
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

#include "Semaphore.hpp"
#include <assert.h>

#if defined(__OS2__)
  #define SEM_NULLHANDLE  NULLHANDLE
#elif defined(WIN32)
  #define SEM_NULLHANDLE  NULL
#endif

// --------------------------------------

MutexSemaphore::MutexSemaphore()
    : mMutex(SEM_NULLHANDLE)
{
   #if defined(__OS2__)
    DosCreateMutexSem(NULL, &mMutex, 0L, FALSE);
   #elif defined(WIN32)
    mMutex = CreateMutex(NULL, FALSE, NULL);
   #endif
    assert(mMutex != SEM_NULLHANDLE);
}

MutexSemaphore::~MutexSemaphore()
{
   #if defined(__OS2__)
    DosCloseMutexSem(mMutex);
   #elif defined(WIN32)
    CloseHandle(mMutex);
   #endif
    mMutex = SEM_NULLHANDLE;
}

BOOL MutexSemaphore::tryLock(unsigned long milliSeconds)
{
   #if defined(__OS2__)
    return (NO_ERROR == WinRequestMutexSem(mMutex, milliSeconds));
   #elif defined(WIN32)
    return (WAIT_OBJECT_0 == WaitForSingleObject(mMutex, milliSeconds));
   #endif
}

BOOL MutexSemaphore::lock()
{
   #if defined(__OS2__)
    return (NO_ERROR == WinRequestMutexSem(mMutex, SEM_INDEFINITE_WAIT));
   #elif defined(WIN32)
    return (WAIT_OBJECT_0 == WaitForSingleObject(mMutex, INFINITE));
   #endif
}

void MutexSemaphore::unlock()
{
   #if defined(__OS2__)
    DosReleaseMutexSem(mMutex);
   #elif defined(WIN32)
    ReleaseMutex(mMutex);
   #endif
}
    
// --------------------------------------

EventSemaphore::EventSemaphore()
    : mEvent(SEM_NULLHANDLE)
{
   #if defined(__OS2__)
    DosCreateEventSem(NULL, &mEvent, 0L, TRUE);
   #elif defined(WIN32)
    mEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
   #endif
   assert(mEvent != SEM_NULLHANDLE);
}

EventSemaphore::~EventSemaphore()
{
   #if defined(__OS2__)
    DosCloseEventSem(mEvent);
   #elif defined(WIN32)
    CloseHandle(mEvent);
   #endif
   mEvent = SEM_NULLHANDLE;
}

BOOL EventSemaphore::wait(unsigned long milliSeconds)
{
   #if defined(__OS2__)
    return (NO_ERROR == WinWaitEventSem(mEvent, milliSeconds));
   #elif defined(WIN32)
    return (WAIT_OBJECT_0 == WaitForSingleObject(mEvent, milliSeconds));
   #endif
}

BOOL EventSemaphore::wait()
{
   #if defined(__OS2__)
    return (NO_ERROR == WinWaitEventSem(mEvent, SEM_INDEFINITE_WAIT));
   #elif defined(WIN32)
    return (WAIT_OBJECT_0 == WaitForSingleObject(mEvent, INFINITE));
   #endif
}

void EventSemaphore::post()
{
   #if defined(__OS2__)
    DosPostEventSem(mEvent);
   #elif defined(WIN32)
    SetEvent(mEvent);
   #endif
}

void EventSemaphore::reset()
{
   #if defined(__OS2__)
    ULONG postCount;
    DosResetEventSem(mEvent, &postCount);
   #elif defined(WIN32)
    ResetEvent(mEvent);
   #endif
}
    
