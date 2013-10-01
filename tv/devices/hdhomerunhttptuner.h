#ifndef HDHOMERUNHTTPTUNER_H_INCLUDED
#define HDHOMERUNHTTPTUNER_H_INCLUDED

#include "hdhomerunhttp.h"

namespace TV {
namespace Devices {

    class HDHomeRunHTTPTuner: public Tuner {
    public:
        HDHomeRunHTTPTuner(HDHomeRunHTTP *parent, int index, UPnP::Device *tuner);

        std::string playChannel(int channel, int tuner_idx);
        void        stopChannel();
        bool        isPlaying() { return Mux_ProgramNumber()!=0; }
        int         getChannelNumber() { return CAS_GetChannel(); }
        bool        setProgramId() { return false; }
        int         getProgramNumber();

        std::string getUPnPDeviceType();

    private:
        UPnP::Device *tuner;
        UPnP::Service *Mux;
        UPnP::Service *CAS;

        int CAS_GetChannel();
        int Mux_ProgramNumber();
    };

}}


#endif // HDHOMERUNHTTPTUNER_H_INCLUDED
