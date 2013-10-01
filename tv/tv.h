#ifndef TV_TV_H
#define TV_TV_H

#include "http/httpserver.h"
#include "threads/rwlock.h"
#include "config.h"
#include "tuner.h"
#include "scheduler.h"
#include "devices/device.h"
#include "sqlite/sqlite3.h"

namespace TV {
    typedef void ( *fTVEvent)();

    class Tuner;
    namespace XMLTV {
        class EPGUpdater;
    }
    namespace API {
        class EPGService;
        class ChannelListService;
    }

    class Server {
    public:
        Server(fTVEvent on_ready);

        void run();
        void restart();
        void stop();
        bool isRunning() { return is_running; }

        Config *getSettings() { return &settings; }
        std::list<Devices::Device*> getDevices() {return device_list;}
        std::vector<Tuner*> getTuners() {return tuner_list;}
        Tuner* getAvailableTuner(int tuner_idx=-1, int tuner_key=-1);

    private:
        fTVEvent on_ready;
        Config settings;
        bool restart_signal;
        bool is_running;

        bool init_devices();
        void cleanup_devices();
        std::list<Devices::Device*> device_list;
        std::vector<Tuner*> tuner_list;

        void setup_services();
        HTTP::HTTPServer http_server;

        Threads::CondVar shutdown_cv;
        Threads::Mutex shutdown_mutex;

        Scheduler scheduler;
        sqlite3 *epg_db;
        Threads::RWLock epg_rwlock;

        friend class Tuner;
        friend class API::EPGService;
        friend class XMLTV::EPGUpdater;

        Server(const Server&);
        Server& operator = (const Server&);
    };

    class TVService: public HTTP::HTTPService {
    public:
        TVService(Server *server, bool secured=false) {
            this->server = server;
            this->secured = secured;
        }
        void handleHTTPRequest(HTTP::HTTPRequest *request) {
            if (!secured || !server->getSettings()->isServerSecured() ||
                request->getClientIp() == "127.0.0.1" || request->authenticate(
                    server->getSettings()->getServerUsername(),
                    server->getSettings()->getServerPassword()
            ))
                handleRequest(request);
        }
        virtual void handleRequest(HTTP::HTTPRequest *request) = 0;
    protected:
        Server *server;
        bool secured;
    };

}

#endif // TV_TV_H
