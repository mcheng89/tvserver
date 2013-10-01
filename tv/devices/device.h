#ifndef TV_DEVICES_DEVICE_H
#define TV_DEVICES_DEVICE_H

#include <map>
#include <vector>
#include <string>
#include "tuner.h"
#include "../log.h"

namespace TV {
    namespace API {
        class ChannelListService;
    }
namespace Devices {

    class ChannelMap {
    public:
        int number;
        std::string name;
    };

    class Tuner;

    class Device {
    public:
        Device(std::string ip_address, int tuner_count) {
            this->ip_address = ip_address;
            this->tuner_count = tuner_count;
            tuners = 0;
        }
        virtual ~Device() {
            cleanUp();
        }
        virtual bool loadDevice() {
            if (loadChannelMap())
                return true;
            Log::Info() << getDeviceType() << ": Could not get channel listing" << std::endl;
            return false;
        }

        virtual std::string getDeviceType() {return "";}
        Tuner *getTuner(int i) {
            if (!tuners || i>getTunerCount()-1 || i<0) return 0;
            return tuners[i];
        }
        int getTunerCount() { return tuner_count; }
        std::string getIPAddress() { return ip_address; }

        const ChannelMap* isChannel(int channel) {
            std::map<int,ChannelMap*>::iterator pos;
            if ( (pos=channel_map.find(channel)) != channel_map.end()) {
                return pos->second;
            }
            return 0;
        }

        std::vector<ChannelMap*> getChannelMap() { return channel_map_vector; }

    protected:
        virtual bool loadChannelMap() = 0;
        void cleanUp() {
            channel_map.clear();
            for (int i=0; i<channel_map_vector.size(); i++)
                delete channel_map_vector.at(i);
            channel_map_vector.clear();
            if (tuners) {
                for (int i=0; i<getTunerCount(); i++) {
                    if (tuners[i]) delete tuners[i];
                }
                delete[] tuners;
                tuners = 0;
            }
        }

        Tuner **tuners;
        std::string ip_address;
        int tuner_count;

        std::map<int,ChannelMap*> channel_map;
        std::vector<ChannelMap*> channel_map_vector;

    private:
        Device(const Device&);
        Device& operator = (const Device&);
    };

}}

#endif // TV_DEVICES_DEVICE_H
