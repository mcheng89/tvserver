#include "cetontuner.h"
#include "../utils/stringutils.h"

namespace TV {
namespace Devices {

CetonTuner::CetonTuner(Ceton *parent, int index, UPnP::Device *tuner):
    Tuner(parent, index)
{
    this->tuner = tuner;
    std::vector<UPnP::Service *> services = tuner->getServiceList();
    for (int i=0; i<services.size(); i++) {
        UPnP::Service *service = services.at(i);
        if (service->getServiceType() == "urn:schemas-upnp-org:service:AVTransport:1")
            AVTransport = service;
        else if (service->getServiceType() == "urn:schemas-opencable-com:service:CAS:1")
            CAS = service;
        else if (service->getServiceType() == "urn:schemas-upnp-org:service:ConnectionManager:1")
            ConnectionManager = service;
        else if (service->getServiceType() == "urn:schemas-opencable-com:service:Mux:1")
            Mux = service;
    }
}

std::string CetonTuner::playChannel(int channel, int tuner_idx) {
    CAS_SetChannel(channel);
    ConnectionManager_PrepareForConnection();
    AVTransport_Play();
    return AVTransport_URI();
}
void CetonTuner::stopChannel() {
    AVTransport_Stop();
    ConnectionManager_ConnectionComplete();
    CAS_SetChannel(0);
}

int CetonTuner::getProgramNumber() {
    return Mux_ProgramNumber();
}

std::string CetonTuner::getUPnPDeviceType() {
    return tuner->getDeviceType();
}

std::string CetonTuner::AVTransport_URI() {
    return AVTransport->getStateVariable("AVTransportURI");
}

void CetonTuner::CAS_SetChannel(int channel) {
    UPnP::Action *setChannel = CAS->getAction("SetChannel");
    setChannel->setArgumentValue("NewCaptureMode","Live");
    setChannel->setArgumentValue("NewChannelNumber", Utils::StringUtils::toString(channel).c_str());
    setChannel->setArgumentValue("NewSourceId","0");

    UPnP::ActionResponse response;
    if (setChannel->postControlAction(response)) {
        //std::cout << "CAS::SetChannel()" << std::endl;
    }
}
int CetonTuner::CAS_GetChannel() {
    return atoi(CAS->getStateVariable("VirtualChannelNumber").c_str());
}

void CetonTuner::ConnectionManager_PrepareForConnection() {
    //PrepareForConnection();
    UPnP::Action *prepareConn = ConnectionManager->getAction("PrepareForConnection");
    prepareConn->setArgumentValue("RemoteProtocolInfo","");
    prepareConn->setArgumentValue("PeerConnectionManager","");
    prepareConn->setArgumentValue("PeerConnectionID","");
    prepareConn->setArgumentValue("Direction","Output");

    UPnP::ActionResponse response;
    if (prepareConn->postControlAction(response)) {
        //std::cout << "ConnectionManager::PrepareForConnection()" << std::endl;
    }
}
void CetonTuner::AVTransport_Play() {
    UPnP::Action *play = AVTransport->getAction("Play");
    play->setArgumentValue("InstanceID","0");
    play->setArgumentValue("Speed","1");

    UPnP::ActionResponse response;
    if (play->postControlAction(response)) {
        //std::cout << "AVTransport::Play()" << std::endl;
    }
}

void CetonTuner::AVTransport_Stop() {
    UPnP::Action *stop = AVTransport->getAction("Stop");
    stop->setArgumentValue("InstanceID","0");

    UPnP::ActionResponse response;
    if (stop->postControlAction(response)) {
        //std::cout << "AVTransport::Stop()" << std::endl;
    }
}
void CetonTuner::ConnectionManager_ConnectionComplete() {
    UPnP::Action *connCompl = ConnectionManager->getAction("ConnectionComplete");
    connCompl->setArgumentValue("ConnectionID","0");

    UPnP::ActionResponse response;
    if (connCompl->postControlAction(response)) {
        //std::cout << "ConnectionManager::ConnectionComplete()" << std::endl;
    }
}

int CetonTuner::Mux_ProgramNumber() {
    return atoi(Mux->getStateVariable("ProgramNumber").c_str());
}

}}

