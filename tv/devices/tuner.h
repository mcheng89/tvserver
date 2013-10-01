#ifndef TV_DEVICES_TUNER_H
#define TV_DEVICES_TUNER_H

#include "device.h"

namespace TV {
    class Server;
namespace Devices {

    class Device;

    class Tuner {
    public:
        Tuner(Device *parent, int index) {
            this->parent = parent;
            this->index = index;
        }
        virtual ~Tuner() {}

        Device *getDevice() { return parent; }
        int getTunerIndex() { return index; }

        virtual std::string playChannel(int channel, int tuner_idx) = 0;
        virtual void stopChannel() = 0;
        virtual bool isPlaying() = 0;

        virtual int getChannelNumber() = 0;
        virtual bool setProgramId() = 0;
        virtual int  getProgramNumber() { return 0; }

    protected:
        Device *parent;

    private:
        int index;

        Tuner(const Tuner&);
        Tuner& operator = (const Tuner&);
    };

}}

#endif // TV_DEVICES_TUNER_H
