#include "condvar.h"
//#include <iostream>

namespace TV {
namespace Threads {

CondVar::CondVar() {
    waiters_count_ = 0;
    was_broadcast_ = 0;
    //CreatedSemaphore(no security, initially 0, max count, unnamed);
    sema_ = CreateSemaphore (NULL,0,0x7fffffff,NULL);
    InitializeCriticalSection (&waiters_count_lock_);
    //CreateEvent(no security,auto-reset,non-signaled initially,unnamed);
    waiters_done_ = CreateEvent (NULL, FALSE, FALSE, NULL);
}
CondVar::~CondVar() {
    CloseHandle(sema_);
    DeleteCriticalSection(&waiters_count_lock_);
    CloseHandle(waiters_done_);
}

void CondVar::wait(Mutex *mutex) {
    // Avoid race conditions.
    EnterCriticalSection (&waiters_count_lock_);
    waiters_count_++;
    LeaveCriticalSection (&waiters_count_lock_);

    // This call atomically releases the mutex and waits on the
    // semaphore until <pthread_cond_signal> or <pthread_cond_broadcast>
    // are called by another thread.
    SignalObjectAndWait (mutex->mutexID, sema_, INFINITE, FALSE);

    // Reacquire lock to avoid race conditions.
    EnterCriticalSection (&waiters_count_lock_);

    // We're no longer waiting...
    waiters_count_--;

    // Check to see if we're the last waiter after <pthread_cond_broadcast>.
    int last_waiter = was_broadcast_ && waiters_count_ == 0;

    LeaveCriticalSection (&waiters_count_lock_);

    // If we're the last waiter thread during this particular broadcast
    // then let all the other threads proceed.
    if (last_waiter)
        // This call atomically signals the <waiters_done_> event and waits until
        // it can acquire the <external_mutex>.  This is required to ensure fairness.
        SignalObjectAndWait (waiters_done_, mutex->mutexID, INFINITE, FALSE);
    else
        // Always regain the external mutex since that's the guarantee we
        // give to our callers.
        WaitForSingleObject (mutex->mutexID, INFINITE);
}

void CondVar::signal() {
    EnterCriticalSection (&waiters_count_lock_);
    int have_waiters = waiters_count_ > 0;
    LeaveCriticalSection (&waiters_count_lock_);

    // If there aren't any waiters, then this is a no-op.
    if (have_waiters) ReleaseSemaphore (sema_, 1, 0);
}

void CondVar::broadcast() {
    // This is needed to ensure that <waiters_count_> and <was_broadcast_> are
    // consistent relative to each other.
    EnterCriticalSection (&waiters_count_lock_);
    int have_waiters = 0;

    if (waiters_count_ > 0) {
        // We are broadcasting, even if there is just one waiter...
        // Record that we are broadcasting, which helps optimize
        // <pthread_cond_wait> for the non-broadcast case.
        was_broadcast_ = 1;
        have_waiters = 1;
    }

    if (have_waiters) {
        // Wake up all the waiters atomically.
        ReleaseSemaphore (sema_, waiters_count_, 0);

        LeaveCriticalSection (&waiters_count_lock_);
        // Wait for all the awakened threads to acquire the counting
        // semaphore.
        WaitForSingleObject (waiters_done_, INFINITE);
        // This assignment is okay, even without the <waiters_count_lock_> held
        // because no other waiter threads can wake up to access it.
        was_broadcast_ = 0;
    }
    else LeaveCriticalSection (&waiters_count_lock_);
}

}}
