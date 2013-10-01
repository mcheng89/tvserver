#include "tv.h"
#include "log.h"
#include "api/server.h"
#include "api/stream.h"
#include "api/tvdata.h"
#include "devices/devicelist.h"
#include "utils/file.h"

namespace TV {

Server::Server(fTVEvent on_ready): scheduler(this) {
    epg_db = 0;
    is_running = false;
    restart_signal = false;
    setup_services();
    settings.loadFile("settings.conf");
    this->on_ready = on_ready;
}

void Server::run() {
tv_restart:
    shutdown_mutex.lock();

    scheduler.start();
    epg_rwlock.writeLock();
    if (Utils::File::exists("resources/epgdata.db"))
        sqlite3_open("resources/epgdata.db", &epg_db);
    epg_rwlock.writeUnlock();
    init_devices();

    if (http_server.start(settings.getServerPort())) {
        is_running = true;
        if (settings.getEPGEnabled())
            scheduler.wakeEPGUpdate();
        on_ready();

        //wait for shutdown signal
        shutdown_cv.wait(&shutdown_mutex);
        is_running = false;
        http_server.stop();
        shutdown_mutex.unlock();

        settings.saveFile();
    }
    else shutdown_mutex.unlock();

    cleanup_devices();
    scheduler.stop();
    epg_rwlock.writeLock();
    if (epg_db) {
        sqlite3_close(epg_db);
        epg_db = 0;
    }
    epg_rwlock.writeUnlock();

    if (restart_signal) {
        restart_signal = false;
        goto tv_restart;
    }
}

void Server::stop() {
    shutdown_cv.signal();
}
void Server::restart() {
    restart_signal = true;
    stop();
}

void Server::setup_services() {
    http_server.addService(new API::FileService(this), "/m/", HTTP::HTTPService::PATH_START);
    http_server.addService(new API::SettingsService(this), "/settings");
    http_server.addService(new API::StatusService(this), "/status");
    http_server.addService(new API::ShutdownService(this), "/shutdown");
    http_server.addService(new API::PlayService(this), "/play");
    http_server.addService(new API::StopService(this), "/stop");
    http_server.addService(new API::StreamService(this), "/stream");
    http_server.addService(new API::StreamService(this), "/stream.flv");
    http_server.addService(new API::HLSService(this), "/hls/", HTTP::HTTPService::PATH_START);
    http_server.addService(new API::EPGService(this), "/getEPGData");
    http_server.addService(new API::ChannelListService(this), "/getChannels");
}

bool Server::init_devices() {
    //cleanup_tuners();
    Log::Info() << "Server: Initializing Devices" << std::endl;
    std::list<Config::Device*> config_devices = settings.getDevices();
    for (std::list<Config::Device*>::iterator i=config_devices.begin(); i!=config_devices.end(); i++) {
        Devices::Device *device = Devices::DeviceList::getDevice((*i)->getDeviceType(), (*i)->getIPAddress(), (*i)->getConfig());
        if (device) {
            Log::Info() << "Adding device: new Device[" << device->getDeviceType() << "][" << device->getIPAddress() << "]" << std::endl;
            device_list.push_back(device);

            std::list<bool> tuners_enabled = (*i)->getTunersEnabled();
            std::list<bool>::iterator x=tuners_enabled.begin();
            for (int y=0; y<device->getTunerCount(); y++) {
                if (x==tuners_enabled.end()) break;
                if (*x) {
                    Tuner *tuner = new Tuner(this, device->getTuner(y), tuner_list.size());
                    Log::Info() << "Adding tuner: new Tuner[" << device->getIPAddress() << "][" << device->getTuner(y)->getTunerIndex()+1 << "]" << std::endl;
                    tuner_list.push_back(tuner);
                }
                x++;
            }
        } else {
            Log::Info() << "Failed to add device: new Device[" << (*i)->getDeviceType() << "][" << (*i)->getIPAddress() << "]" << std::endl;
        }
    }
    if (tuner_list.size()==0)
        Log::Info() << "No devices were found" << std::endl;
}
void Server::cleanup_devices() {
    Log::Info() << "Server: Removing Devices" << std::endl;
    for (int i=0; i<tuner_list.size(); i++) {
        Devices::Tuner *tuner =  tuner_list.at(i)->getTuner();
        tuner_list.at(i)->stopStream();
        Log::Info() << "Removing tuner: delete Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]" << std::endl;
        delete tuner_list.at(i);
    }
    for (std::list<Devices::Device*>::iterator i=device_list.begin(); i!=device_list.end(); i++) {
        Log::Info() << "Removing device: delete Tuner[" << (*i)->getDeviceType() << "][" << (*i)->getIPAddress() << "]" << std::endl;
        delete *i;
    }
    tuner_list.clear();
    device_list.clear();
}

Tuner* Server::getAvailableTuner(int tuner_idx, int tuner_key) {
    if (tuner_idx != -1 && tuner_idx>=0 && tuner_idx<tuner_list.size()) {
        tuner_list.at(tuner_idx)->lock();
        if (tuner_list.at(tuner_idx)->isStreaming() && tuner_list.at(tuner_idx)->getStreamKey()==tuner_key) {
            Log::Info() << "Server: Unlocking tuner[" << (tuner_idx+1) << "] using key - " << tuner_key << std::endl;
            return tuner_list.at(tuner_idx);
        }
        tuner_list.at(tuner_idx)->unlock();
    }
    for (int x=0; x<5; x++) {
        for (int i=0; i<tuner_list.size(); i++) {
            tuner_list.at(i)->lock();
            if (!tuner_list.at(i)->isStreaming(true))
                return tuner_list.at(i);
            tuner_list.at(i)->unlock();
        }
        Sleep(250);
    }
    return 0;
}

}
