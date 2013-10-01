#ifndef TV_THREADS_RWLOCK_H
#define TV_THREADS_RWLOCK_H

#include "condvar.h"
#include "mutex.h"

namespace TV {
namespace Threads {

    //http://stackoverflow.com/questions/8587786/reader-writer-preference-using-semaphores
    class RWLock {
    public:
        RWLock();
        ~RWLock() {}

        void readLock();
        void writeLock();
        void readUnlock();
        void writeUnlock();
    private:
        Mutex read_mutex;
        Mutex write_mutex;
        CondVar read_cv;
        CondVar write_cv;

        Mutex writers_lock;

        bool writer_;
        int readers_;

        RWLock(const RWLock&);
        RWLock& operator = (const RWLock&);
    };

}}

#endif // TV_THREADS_RWLOCK_H
