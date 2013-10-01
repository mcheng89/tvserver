#ifndef TV_DEVICES_HDHOMERUNHTTP_H
#define TV_DEVICES_HDHOMERUNHTTP_H

#include "device.h"
#include "../utils/json.h"
#include "../upnp/device.h"

namespace TV {
namespace Devices {

    class HDHomeRunHTTP: public Device {
    public:
        HDHomeRunHTTP(std::string ip_address);
        ~HDHomeRunHTTP();
        bool loadDevice();

        std::string getDeviceType() { return "HDHomeRunPrimeHTTP"; }
        std::string getUPnPDeviceType();
        static Device* getDevice(std::string ip_address, Utils::JSON *device_config) {
            return new HDHomeRunHTTP(ip_address);
        }

    private:
        bool loadChannelMap();
        void cleanUp();

        UPnP::Device *hdhomerun;
    };

}}

#endif // TV_DEVICES_HDHOMERUNHTTP_H
