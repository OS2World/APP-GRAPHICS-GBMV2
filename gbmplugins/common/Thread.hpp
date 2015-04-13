
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

#ifndef __THREAD__
    #define __THREAD__

#include "Semaphore.hpp"

#if defined(__OS2__)
  #include "os2defs.h"
  typedef  int     THREAD;
#elif defined(WIN32)
  #include "windefs.h"
  typedef  HANDLE  THREAD;
#else
 
  Unsupported architecture detected.

#endif

// ---------------------------------------------------------

class Thread
{
    public:
        struct THREAD_ARG
        {
            void   * task;
            Thread * thread;
        };

        Thread();
        Thread(unsigned int stackSize);
        virtual ~Thread();

        BOOL scheduleTask(void * pTask);
        BOOL tryStop(unsigned long milliSeconds);
        void stop();
        
       #if defined(__OS2__)
        void postFinished();
       #endif

        // to be implemented by the client
        virtual void run(void * pTask) = 0;

        // Returns the number of CPU cores the system provides.
        static unsigned long getNumberOfCores();

    protected:
        MutexSemaphore    mGuardMutex;

    private:
        int               mStackSize;
        THREAD            mThread;
        struct THREAD_ARG mThreadArgs;
       #if defined(__OS2__)
        EventSemaphore    mFinishedEvent;
       #endif

        Thread(const Thread &);               // not copy
        Thread & operator = (const Thread &); // no assignment

        BOOL start();
};

#endif


