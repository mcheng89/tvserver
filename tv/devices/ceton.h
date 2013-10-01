#ifndef TV_DEVICES_CETON_H
#define TV_DEVICES_CETON_H

#include "device.h"
#include "../utils/json.h"
#include "../upnp/device.h"

namespace TV {
namespace Devices {

    class CetonMap: public ChannelMap {
    public:
        //from ChannelMap
        //int number;
        //std::string name;
        int program;
    };

    class Ceton: public Device {
    public:
        Ceton(std::string ip_address);
        ~Ceton();
        bool loadDevice();

        std::string getDeviceType() { return "CetonInfiniTV4"; }
        std::string getUPnPDeviceType();
        static Device* getDevice(std::string ip_address, Utils::JSON *device_config) {
            return new Ceton(ip_address);
        }

    private:
        bool loadChannelMap();
        void cleanUp();

        UPnP::Device *ceton;
    };

}}

#endif // TV_DEVICES_CETON_H
