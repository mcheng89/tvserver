#ifndef TV_SCHEDULER_H
#define TV_SCHEDULER_H

#include "xmltv/epgupdater.h"
#include "utils/scheduler.h"

namespace TV {

    class Server;

    class Scheduler: public Utils::Scheduler {
    public:
        Scheduler(Server *params);
        ~Scheduler() {}

        void wakeTuner(time_t shutdown_time);
        void wakeEPGUpdate();

        void stop();

        //return time in seconds to sleep
        time_t RunJob(void *params, time_t current_time);
    private:
        XMLTV::EPGUpdater epgUpdater;
        Server *parent;

        Scheduler(const Scheduler&);
        Scheduler& operator = (const Scheduler&);
    };

}

#endif // TV_SCHEDULER_H
