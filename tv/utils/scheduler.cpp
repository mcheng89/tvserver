#include "scheduler.h"
#include "../log.h"

namespace TV {
namespace Utils {

Scheduler::Scheduler(void *params) {
    this->params = params;
    isRunning = false;
    hasTasks = false;
    event = CreateEvent( NULL, TRUE, FALSE, NULL );
}
Scheduler::~Scheduler() {
    CloseHandle(event);
}

bool Scheduler::start() {
    startup_mutex.lock();
    _next_time = 0;
    _next_signal = 0;
    hasTasks = false;
    isRunning = true;
    ResetEvent(event);
    Threads::Thread::start();
    startup_mutex.unlock();
    return true;
}

void Scheduler::run() {
    int sleep_time = 0;
    time_t current_time;

    for(;;)
    {
        //Log::Debug() << "hasTasks="<<hasTasks<<",sleep_time="<<sleep_time<<std::endl;
        WaitForSingleObject(event, (hasTasks?1000*sleep_time:INFINITE));
        //Log::Debug() << "Scheduler: todo..." << isRunning << std::endl;

        current_time = time(0);
        scheduler_mutex.lock();
        if (!isRunning) break;
        //Log::Debug() << "next_time="<<_next_time-current_time<<",next_signal="<<_next_signal-current_time<<std::endl;
        if (_next_signal!=0 && (_next_time==0 || _next_signal<_next_time) && current_time<_next_signal)
            goto SkipJobs;

        _next_signal = 0;
        ResetEvent(event);
        scheduler_mutex.unlock();

        //Logger::Info() << "Scheduler: Checking for tasks to run" << std::endl;

        _next_time = RunJob(params, current_time);

        scheduler_mutex.lock();
        SkipJobs:
        if (!isRunning) break;
        if (_next_time==0 && _next_signal==0)
            hasTasks = false;
        //what if theres something important soon?
        //change next time...
        else {
            if (_next_time == 0)
                _next_time = _next_signal;
            else if (_next_time>0 && _next_signal>0 && _next_signal<_next_time)
                _next_time = _next_signal;
            ResetEvent(event);
        }
        _next_signal = 0;
        scheduler_mutex.unlock();

        sleep_time = _next_time - current_time;
        if (sleep_time < 0) sleep_time = 0;

        if (hasTasks)
            Log::Info() << "Scheduler: Sleeping " << sleep_time << " seconds until next task" << std::endl;
        else
            Log::Info() << "Scheduler: All tasks are completed. Sleeping..." << std::endl;
        //Log::Debug() << "Scheduler: sleep [isSleeping=" << isSleeping << ",sleepTime="<< sleepTime<< "]" << std::endl;
    }
    Log::Info() << "Scheduler: Received shutdown request..." << std::endl;
    shutdown_cv.signal();
    scheduler_mutex.unlock();
}

void Scheduler::wake() {
    wake(time(0));
}

void Scheduler::wake(time_t new_time) {
    scheduler_mutex.lock();
    if (_next_signal==0 || new_time < _next_signal)
    {
        _next_signal = new_time;
        if (!hasTasks || _next_signal<_next_time)
        {
            hasTasks = true;
            SetEvent(event);
        }
    }
    scheduler_mutex.unlock();
}

void Scheduler::stop() {
    startup_mutex.lock();
    scheduler_mutex.lock();
    if (isRunning)
    {
        isRunning = false;
        SetEvent(event);
        shutdown_cv.wait(&scheduler_mutex);
    }
    Threads::Thread::stop();
    scheduler_mutex.unlock();
    startup_mutex.unlock();
}

}}
