#include "hdhomerunhttptuner.h"
#include "../utils/stringutils.h"

namespace TV {
namespace Devices {

HDHomeRunHTTPTuner::HDHomeRunHTTPTuner(HDHomeRunHTTP *parent, int index, UPnP::Device *tuner):
    Tuner(parent, index)
{
    this->tuner = tuner;
    std::vector<UPnP::Service *> services = tuner->getServiceList();
    for (int i=0; i<services.size(); i++) {
        UPnP::Service *service = services.at(i);
        if (service->getServiceType() == "urn:schemas-opencable-com:service:CAS:1")
            CAS = service;
        else if (service->getServiceType() == "urn:schemas-opencable-com:service:Mux:1")
            Mux = service;
    }
}

std::string HDHomeRunHTTPTuner::getUPnPDeviceType() {
    return tuner->getDeviceType();
}

std::string HDHomeRunHTTPTuner::playChannel(int channel, int tuner_idx) {
    return "http://"+parent->getIPAddress()+":5004/auto/v"+Utils::StringUtils::toString(channel);
}
void HDHomeRunHTTPTuner::stopChannel() {}

int HDHomeRunHTTPTuner::getProgramNumber() {
    return Mux_ProgramNumber();
}

int HDHomeRunHTTPTuner::CAS_GetChannel() {
    return atoi(CAS->getStateVariable("VirtualChannelNumber").c_str());
}
int HDHomeRunHTTPTuner::Mux_ProgramNumber() {
    return atoi(Mux->getStateVariable("ProgramNumber").c_str());
}

}}
