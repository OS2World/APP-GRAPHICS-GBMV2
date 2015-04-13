
//***************************************************
// General Thread C++ Implementation
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

#include "Thread.hpp"
#include <assert.h>
#include <process.h>
#include <stdlib.h>

#if defined(__OS2__)
  #define THREAD_NULL ((int)-1)
#elif defined(WIN32)
  #define THREAD_NULL ((HANDLE)0L)
#else
 
  Unsupported architecture detected.

#endif

// --------------------------------------

#ifdef __IBMCPP__
 static void _Optlink Thread_threadFunc(void * arg)
#elif WIN32
 static unsigned __stdcall Thread_threadFunc(void * arg)
#else
 static void Thread_threadFunc(void * arg)
#endif
{
    Thread::THREAD_ARG * pThreadArgs = (Thread::THREAD_ARG *) arg;

    pThreadArgs->thread->run(pThreadArgs->task);
   #if defined(__OS2__)
    pThreadArgs->thread->postFinished();
   #endif

#if defined(WIN32)
    _endthreadex(0);
    return 0;
#else
    _endthread();
#endif
}

// --------------------------------------

Thread::Thread()
    : mStackSize(300*1024),
      mThread(THREAD_NULL)
{
    mThreadArgs.thread = this;
    mThreadArgs.task   = NULL;
}

Thread::Thread(unsigned int stackSize)
    : mStackSize(stackSize),
      mThread(THREAD_NULL)
{
    mThreadArgs.thread = this;
    mThreadArgs.task   = NULL;
}

Thread::~Thread()
{
    stop();
}

// --------------------------------------

BOOL Thread::scheduleTask(void * task)
{
    MutexGuard mutexGuard(mGuardMutex);
    stop();
    mThreadArgs.task = task;
    return start();
}

// --------------------------------------

#if defined(__OS2__)
void Thread::postFinished()
{
    mFinishedEvent.post();
}
#endif

// --------------------------------------

BOOL Thread::start()
{
    if (mThread == THREAD_NULL)
    {
       #if defined(__OS2__)
        /* DosWaitThread() is not reliable -> using event semaphore instead */
        mFinishedEvent.reset();
        mThread = _beginthread(Thread_threadFunc, NULL, mStackSize, &mThreadArgs);
       #elif defined(WIN32)
        unsigned threadId;
        mThread = (HANDLE) _beginthreadex(NULL, mStackSize, Thread_threadFunc,
                                          &mThreadArgs, 0, &threadId);
       #endif
       return (mThread != THREAD_NULL) ? TRUE : FALSE;
    }
    return FALSE;
}

// --------------------------------------

BOOL Thread::tryStop(unsigned long milliSeconds)
{
    if (mGuardMutex.tryLock(milliSeconds))
    {
        if (mThread != THREAD_NULL)
        {
           #if defined(__OS2__)
            /* DosWaitThread() is not reliable -> using event semaphore instead */
            if (mFinishedEvent.wait(milliSeconds))
           #elif defined(WIN32)
            if (WAIT_OBJECT_0 == WaitForSingleObject(mThread, milliSeconds))
           #endif
            {
                mThread = THREAD_NULL;
                mThreadArgs.task = NULL;
                return TRUE;
            }
            return FALSE;
        }
    }
    return TRUE;
}

// --------------------------------------

void Thread::stop()
{
    MutexGuard mutexGuard(mGuardMutex);
    if (mThread != THREAD_NULL)
    {
       #if defined(__OS2__)
        /* DosWaitThread() is not reliable -> using event semaphore instead */
        mFinishedEvent.wait();
       #elif defined(WIN32)
        WaitForSingleObject(mThread, INFINITE);
       #endif
        mThread = THREAD_NULL;
        mThreadArgs.task = NULL;
    }
}

// --------------------------------------

unsigned long Thread::getNumberOfCores()
{
    unsigned long numCores = 1;

   #if defined(__OS2__)

    ULONG processors = 1;
    if (NO_ERROR == DosQuerySysInfo(QSV_NUMPROCESSORS,
                                    QSV_NUMPROCESSORS,
                                    &processors,
                                    sizeof(ULONG)))
    {
        numCores = processors;
    }

   #elif defined(WIN32)

    SYSTEM_INFO sysinfo = { 0 };
    GetSystemInfo(&sysinfo);
    if (sysinfo.dwNumberOfProcessors >= 1)
    {
        DWORD i;
        numCores = 0;
        for (i = 1; i <= sysinfo.dwNumberOfProcessors; ++i)
        {
            if (i & sysinfo.dwActiveProcessorMask)
            {
                ++numCores;
            }
        }
    }

    #endif

    assert(numCores >= 1);
    return (numCores >= 1 ? numCores : 1);
}


