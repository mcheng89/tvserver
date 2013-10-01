#ifndef TV_THREADS_THREAD_H
#define TV_THREADS_THREAD_H

#include <windows.h>

namespace TV {
namespace Threads {

    class Thread {
    public:
        Thread();
        virtual ~Thread();

        virtual bool start();
        virtual void run() = 0;
        //bool isRunning();
        virtual void stop();
    private:
        HANDLE	hThread;
        DWORD	threadID;
    };

}}

#endif // TV_THREADS_THREAD_H
