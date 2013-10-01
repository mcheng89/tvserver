#ifndef TV_CONFIG_H
#define TV_CONFIG_H

#include <string>
#include <list>
#include "utils/json.h"
#include <stdio.h>

#define VERSION "0.5.9"
#define TUNER_SHUTDOWN_TIME 10
#define TUNER_SEGMENT_TIME 3

namespace TV {

    class Config {
    public:
        Config();
        ~Config() {
            for (std::list<Device*>::iterator i=device_list.begin(); i!=device_list.end(); i++)
                delete *i;
            device_list.clear();
        }

        void loadFile(std::string filename);
        void loadData(std::string data, bool save_config=false);
        void toJSON(Utils::JSON *json_obj);
        void saveFile();

        std::string getExecDirectory()  { return server_exec_dir; }
        int         getServerPort()     { return server_port; }
        bool        isServerSecured()   { return server_secured; }
        std::string getServerUsername() { return server_username; }
        std::string getServerPassword() { return server_password; }
        std::string getVLCProcessPath() { return vlc_process_path; }
        int         getVLCTranscodeMaxBitrate() { return vlc_transcode_max_bitrate; }
        bool        getEPGEnabled()     { return epg_enabled; }
        int         getEPGHourOfDay()   { return epg_hour_of_day; }

        class Device {
        public:
            Device(std::string device_type, std::string ip_address, std::list<bool> tuners_enabled, Utils::JSON *config);
            std::string     getDeviceType()    { return device_type; }
            std::string     getIPAddress()     { return ip_address; }
            std::list<bool> getTunersEnabled() { return tuners_enabled; }
            Utils::JSON*    getConfig()        { return &config; }

        private:
            std::string device_type;
            std::string ip_address;
            std::list<bool> tuners_enabled;
            Utils::JSON config;

            Device(const Device&);
            Device& operator = (const Device&);
        };
        std::list<Device*> getDevices() { return device_list; }

    private:
        std::string filename;

        std::string server_exec_dir;
        int         server_port;
        bool        server_secured;
        std::string server_username;
        std::string server_password;
        std::string vlc_process_path;
        int         vlc_transcode_max_bitrate;
        bool        epg_enabled;
        int         epg_hour_of_day;

        std::list<Device*> device_list;
    };

}

#endif // TV_CONFIG_H
