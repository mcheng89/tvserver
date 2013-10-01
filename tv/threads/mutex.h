#ifndef TV_THREADS_MUTEX_H
#define TV_THREADS_MUTEX_H

//#include "condvar.h"
#include <windows.h>

namespace TV {
namespace Threads {

    class Mutex {
    public:
        Mutex();
        ~Mutex();

        bool lock();
        bool unlock();
    private:
        HANDLE	mutexID;
        friend class CondVar;

        Mutex(const Mutex&);
        Mutex& operator = (const Mutex&);
    };

}}

#endif // TV_THREADS_MUTEX_H
