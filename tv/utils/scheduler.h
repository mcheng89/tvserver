#ifndef TV_UTILS_SCHEDULER_H
#define TV_UTILS_SCHEDULER_H

#include "../threads/condvar.h"
#include "../threads/mutex.h"
#include "../threads/thread.h"
#include <time.h>

namespace TV {
namespace Utils {

    class Scheduler: public Threads::Thread {//thread
    public:
        Scheduler(void *params);
        virtual ~Scheduler();

        virtual bool start();
        virtual void run();
        void wake();
        void wake(time_t new_time);
        virtual void stop();

        //return time in seconds to sleep
        virtual time_t RunJob(void *params, time_t current_time) = 0;
    private:
        Threads::Mutex startup_mutex;
        Threads::Mutex scheduler_mutex;
        Threads::CondVar shutdown_cv;
        HANDLE event;
        bool hasTasks;
        bool isRunning;

        time_t _next_signal;
        time_t _next_time;

        void *params;

        Scheduler(const Scheduler&);     // do not give these a body
        Scheduler& operator = (const Scheduler&);
    };

}} //namespace


#endif // TV_UTILS_SCHEDULER_H
