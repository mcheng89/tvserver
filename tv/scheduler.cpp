#include "scheduler.h"
#include "tv.h"

namespace TV {

Scheduler::Scheduler(Server *parent)
    :Utils::Scheduler(parent),epgUpdater(parent) {
    this->parent = parent;
}

void Scheduler::wakeTuner(time_t shutdown_time) {
    wake(shutdown_time);
}
void Scheduler::wakeEPGUpdate() {
    if (parent->getSettings()->getEPGEnabled())
        wake(epgUpdater.nextRunTime(time(0)));
}

time_t Scheduler::RunJob(void *params, time_t current_time) {
    Server *parent = (Server*)params;

    time_t next_time = 0;

    std::vector<Tuner*> tuner_list = parent->getTuners();
    for (int i=0; i<tuner_list.size(); i++) {
        Tuner *tuner = tuner_list.at(i);
        tuner->lock();
        if ( tuner->isStreaming() ) {
            //std::cout << "current_time=" << current_time << ", shutdown_time=" << tuner->getStreamShutdown() << std::endl;
            if (current_time >= tuner->getStreamShutdown()) {
                tuner->stopStream();
            } else if (next_time==0 || next_time > tuner->getStreamShutdown()) {
                next_time = tuner->getStreamShutdown();
            }
        }
        tuner->unlock();
    }
    if (next_time!=0 && next_time-current_time < 1)
        next_time = current_time+1;

    if (parent->getSettings()->getEPGEnabled()) {
        if (epgUpdater.nextRunTime(current_time) <= current_time) {
            epgUpdater.stop();
            epgUpdater.start();
        }
        if (next_time==0 || epgUpdater.nextRunTime(current_time) < next_time)
            next_time = epgUpdater.nextRunTime(current_time);
    }

    if (next_time == 0) return 0;
    return next_time;
}

void Scheduler::stop() {
    epgUpdater.stop();
    Utils::Scheduler::stop();
}

}
