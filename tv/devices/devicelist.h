#ifndef TV_DEVICES_DEVICELIST_H
#define TV_DEVICES_DEVICELIST_H

#include "device.h"
#include "../utils/json.h"

namespace TV {
namespace Devices {

    typedef Device* ( *fGetDevice)(std::string ip_address, Utils::JSON *device_config);

    class DeviceList {
    public:
        static DeviceList* getInstance() {
            static DeviceList singleton;
            return &singleton;
        }

        static bool isDevice(std::string device_type, std::string device_ip, Utils::JSON *device_config);
        static Device* getDevice(std::string device_type, std::string device_ip, Utils::JSON *device_config);

    private:
        DeviceList();
        std::map<std::string,fGetDevice> device_list;

        DeviceList(const DeviceList&);
        DeviceList& operator = (const DeviceList&);
    };

}}

#endif // TV_DEVICES_DEVICELIST_H
