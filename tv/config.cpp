#include "config.h"
#include <fstream>
#include <windows.h>
#include "utils/file.h"
#include "utils/json.h"
#include "devices/devicelist.h"

namespace TV {

Config::Config() {
    char dir_buf [MAX_PATH];
    GetModuleFileName(0,dir_buf,MAX_PATH);
    server_exec_dir = dir_buf;
    server_exec_dir = server_exec_dir.substr(0,server_exec_dir.rfind(Utils::File::getPathSeparator())+1);

    server_port = 8888;
    server_secured = false;
    server_username = "admin";
    server_password = "admin";
    vlc_process_path = "C:\\Program Files\\VideoLAN\\VLC\\vlc.exe";
    if (!Utils::File::exists(vlc_process_path)) {
        HKEY vlc_registry;
        if (Utils::File::exists("C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe"))
            vlc_process_path = "C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe";
        else if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\VideoLAN\\VLC", 0, KEY_READ, &vlc_registry) == ERROR_SUCCESS) {
            DWORD dwType;
            char szValue[256];
            long lValueSize = 255;
            if (RegQueryValue(vlc_registry, NULL, szValue, &lValueSize) == ERROR_SUCCESS)
                vlc_process_path = szValue;
            RegCloseKey(vlc_registry);
        }
    }
    vlc_transcode_max_bitrate = 0;
    epg_enabled = false;
    epg_hour_of_day = 4;
    //Set the plugin path for vlc so that we dont have to copy it into the vlc folder
    _putenv( ("VLC_PLUGIN_PATH="+server_exec_dir+"resources"+Utils::File::getPathSeparator()+"vlc_plugin").c_str());
}
Config::Device::Device(std::string device_type, std::string ip_address, std::list<bool> tuners_enabled, Utils::JSON *config) {
    this->device_type = device_type;
    this->ip_address = ip_address;
    this->tuners_enabled = tuners_enabled;
    if (config) config->clone(&this->config);
}

void Config::loadFile(std::string filename) {
    this->filename = filename;

    if (!Utils::File::exists(filename))
        saveFile();
    else {
        std::string config_data, line;
        std::ifstream config_file(filename.c_str());
        if (config_file.is_open()) {
            while ( config_file.good() ) {
                getline (config_file,line);
                config_data += line;
            }
            config_file.close();
        }
        loadData(config_data);
    }
}
void Config::loadData(std::string data, bool save_config) {
    Utils::JSON json(Utils::JSON::OBJECT, false);
    if (json.load(data)) {
        json.getInteger("server_port", server_port);
        json.getBoolean("server_secured", server_secured);
        json.getString("server_username", server_username);
        json.getString("server_password", server_password);
        json.getString("vlc_process_path", vlc_process_path);
        json.getBoolean("epg_enabled", epg_enabled);
        json.getInteger("epg_hour_of_day", epg_hour_of_day);

        Utils::JSON json_devices(Utils::JSON::ARRAY, false);
        json.getArray("devices", &json_devices);
        int device_list_size = json_devices.getArraySize();
        if (device_list_size != -1) {
            device_list.clear();
            for (int i=0; i<device_list_size; i++) {
                Utils::JSON json_device(Utils::JSON::OBJECT, false), json_tuners_enabled(Utils::JSON::ARRAY, false), json_device_config(Utils::JSON::OBJECT, false);
                std::string device_type;
                std::string ip_address;
                std::list<bool> tuners_enabled;
                if (json_devices.getArrayObject(&json_device, i) &&
                    json_device.getString("device_type", device_type) &&
                    json_device.getString("ip_address", ip_address) &&
                    json_device.getObject("config", &json_device_config) &&
                    Devices::DeviceList::isDevice(device_type, ip_address, &json_device_config) &&
                    json_device.getArray("tuners_enabled", &json_tuners_enabled) &&
                    json_tuners_enabled.getArraySize()>0)
                {
                    for (int x=0; x<json_tuners_enabled.getArraySize(); x++) {
                        bool tuner_enabled = false;
                        json_tuners_enabled.getArrayBoolean(tuner_enabled, x);
                        tuners_enabled.push_back(tuner_enabled);
                    }
                    device_list.push_back(new Device(device_type, ip_address, tuners_enabled, &json_device_config));
                }
            }
        }

        if (save_config) saveFile();
    }
}
void Config::toJSON(Utils::JSON *json_obj) {
    json_obj->setInteger("server_port", server_port);
    json_obj->setBoolean("server_secured", server_secured);
    json_obj->setString("server_username", server_username);
    json_obj->setString("server_password", server_password);
    json_obj->setString("vlc_process_path", vlc_process_path);
    json_obj->setBoolean("epg_enabled", epg_enabled);
    json_obj->setInteger("epg_hour_of_day", epg_hour_of_day);

    Utils::JSON json_devices(Utils::JSON::ARRAY);
    for (std::list<Config::Device*>::iterator i=device_list.begin(); i!=device_list.end(); i++) {
        Utils::JSON json_device, json_tuners_enabled(Utils::JSON::ARRAY), json_device_config(Utils::JSON::OBJECT,false);
        json_device.setString("device_type", (*i)->getDeviceType());
        json_device.setString("ip_address", (*i)->getIPAddress());
        (*i)->getConfig()->clone(&json_device_config);
        json_device.setObject("config", &json_device_config);

        std::list<bool> tuners_enabled = (*i)->getTunersEnabled();
        for (std::list<bool>::iterator x=tuners_enabled.begin(); x!=tuners_enabled.end(); x++)
            json_tuners_enabled.addArrayBoolean(*x);

        json_device.setArray("tuners_enabled", &json_tuners_enabled);
        json_devices.addArrayObject(&json_device);
    }
    json_obj->setArray("devices", &json_devices);
}
void Config::saveFile() {
    Utils::JSON json_obj;
    toJSON(&json_obj);

    std::ofstream config_file;
    config_file.open (filename.c_str());
    config_file << json_obj.toString();
    config_file.close();
}

}
