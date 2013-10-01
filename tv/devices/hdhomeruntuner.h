#ifndef HDHOMERUNTUNER_H_INCLUDED
#define HDHOMERUNTUNER_H_INCLUDED

#include "hdhomerun.h"
#include "../utils/socket.h"

namespace TV {
namespace Devices {

    class HDHomeRunTuner: public Tuner {
    public:
        HDHomeRunTuner(HDHomeRun *parent, int index, UPnP::Device *tuner);

        std::string playChannel(int channel, int tuner_idx);
        void        stopChannel();
        bool        isPlaying() { return Mux_ProgramNumber()!=0; }
        int         getChannelNumber() { return CAS_GetChannel(); }
        bool        setProgramId() { return false; }
        int         getProgramNumber();

        std::string getUPnPDeviceType();

    private:
        UPnP::Device *tuner;
        UPnP::Service *AVTransport;
        UPnP::Service *CAS;
        UPnP::Service *ConnectionManager;
        UPnP::Service *Mux;

        std::string connectionID;
        std::string avtransportID;

        std::string SendRTSPCommand(std::string command);
        Utils::Socket rtsp_sock;
        std::string rtsp_session;
        std::string rtsp_uri;

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


#endif // HDHOMERUNTUNER_H_INCLUDED
