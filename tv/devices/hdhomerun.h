#ifndef TV_DEVICES_HDHOMERUN_H
#define TV_DEVICES_HDHOMERUN_H

#include "device.h"
#include "../utils/json.h"
#include "../upnp/device.h"

namespace TV {
namespace Devices {

    class HDHomeRun: public Device {
    public:
        HDHomeRun(std::string ip_address);
        ~HDHomeRun();
        bool loadDevice();

        std::string getDeviceType() { return "HDHomeRunPrime"; }
        std::string getUPnPDeviceType();
        static Device* getDevice(std::string ip_address, Utils::JSON *device_config) {
            return new HDHomeRun(ip_address);
        }

    private:
        bool loadChannelMap();
        void cleanUp();

        UPnP::Device *hdhomerun;
    };

}}

#endif // TV_DEVICES_HDHOMERUN_H
