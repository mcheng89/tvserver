#ifndef TV_THREADS_CONDVAR_H
#define TV_THREADS_CONDVAR_H

#include "mutex.h"
#include <windows.h>

namespace TV {
namespace Threads {

    //class Mutex;

    //http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
    class CondVar {
    public:
        CondVar();
        ~CondVar();
        void wait(Mutex *mutex);
        void signal();
        void broadcast();

    private:
        // Number of waiting threads.
        int waiters_count_;
        // Serialize access to <waiters_count_>.
        CRITICAL_SECTION waiters_count_lock_;
        // Semaphore used to queue up threads waiting for the condition to
        // become signaled.
        HANDLE sema_;
        // An auto-reset event used by the broadcast/signal thread to wait
        // for all the waiting thread(s) to wake up and be released from the
        // semaphore.
        HANDLE waiters_done_;
        // Keeps track of whether we were broadcasting or signaling.  This
        // allows us to optimize the code if we're just signaling.
        size_t was_broadcast_;

        CondVar(const CondVar&);
        CondVar& operator = (const CondVar&);
    };

}}

#endif // TV_THREADS_CONDVAR_H
