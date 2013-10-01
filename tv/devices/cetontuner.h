#ifndef TV_DEVICES_CETONTUNER_H
#define TV_DEVICES_CETONTUNER_H

#include "ceton.h"

namespace TV {
namespace Devices {

    class CetonTuner: public Tuner {
    public:
        CetonTuner(Ceton *parent, int index, UPnP::Device *tuner);

        std::string playChannel(int channel, int tuner_idx);
        void        stopChannel();
        bool        isPlaying() { return Mux_ProgramNumber()!=0; }
        int         getChannelNumber() { return CAS_GetChannel(); }
        bool        setProgramId() { return true; }
        int         getProgramNumber();

        std::string getUPnPDeviceType();

    private:
        UPnP::Device *tuner;
        UPnP::Service *AVTransport;
        UPnP::Service *CAS;
        UPnP::Service *ConnectionManager;
        UPnP::Service *Mux;

        std::string AVTransport_URI();
        void CAS_SetChannel(int channel);
        int CAS_GetChannel();
        void ConnectionManager_PrepareForConnection();
        void AVTransport_Play();
        void AVTransport_Stop();
        void ConnectionManager_ConnectionComplete();
        int Mux_ProgramNumber();
    };

}}

#endif // TV_DEVICES_CETONTUNER_H
