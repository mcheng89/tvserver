#include "devicelist.h"
#include "hdhomerun.h"
#include "hdhomerunhttp.h"
#include "ceton.h"

namespace TV {
namespace Devices {

#define addDevice(device_type, class_name) device_list.insert(std::pair<std::string,fGetDevice>(device_type, &class_name::getDevice))

DeviceList::DeviceList() {
    addDevice("HDHomeRunPrime", HDHomeRun);
    addDevice("HDHomeRunPrimeHTTP", HDHomeRunHTTP);
    addDevice("CetonInfiniTV4", Ceton);
}

bool DeviceList::isDevice(std::string device_type, std::string device_ip, Utils::JSON *device_config) {
    std::map<std::string,fGetDevice>::iterator it = getInstance()->device_list.find(device_type);
    if (it != getInstance()->device_list.end()) {
        fGetDevice f = it->second;
        Device *d = f(device_ip, device_config);
        if (d) {
            delete d;
            return true;
        }
    }
    return false;
}

Device* DeviceList::getDevice(std::string device_type, std::string device_ip, Utils::JSON *device_config) {
    std::map<std::string,fGetDevice>::iterator it = getInstance()->device_list.find(device_type);
    if (it != getInstance()->device_list.end()) {
        fGetDevice f = it->second;
        Device *d = f(device_ip, device_config);
        if (d && !d->loadDevice()) {
            delete d;
            return 0;
        }
        return d;
    }
    return 0;
}

}}
