#ifndef TV_XMLTV_EPGUPDATER_H
#define TV_XMLTV_EPGUPDATER_H

#include "../utils/process.h"
#include "../threads/mutex.h"
#include "../threads/thread.h"

#include <time.h>

namespace TV {
    class Server;
namespace XMLTV {

    class EPGUpdater: public Threads::Thread {
    public:
        EPGUpdater(Server *parent);
        ~EPGUpdater() {}

        bool start();
        time_t nextRunTime(time_t current_time);
        bool stop(bool shutdown=false);

    private:
        void run();

        Server *parent;
        bool runOnce; //startup - check for stale epg
        bool runNext; //dont modify next_time until we actually run
        bool isRunning;
        time_t next_time;
        //Threads::RWLock epg_rwlock;
        Threads::Mutex startup_mutex;
        Threads::Mutex update_mutex;
        Utils::Process xmltv_updater;

        EPGUpdater(const EPGUpdater&);     // do not give these a body
        EPGUpdater& operator = (const EPGUpdater&);
    };

}}

#endif // TV_XMLTV_EPGUPDATER_H
