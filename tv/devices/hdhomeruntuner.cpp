#include "hdhomeruntuner.h"
#include "../utils/stringutils.h"
#include "../log.h"

namespace TV {
namespace Devices {

HDHomeRunTuner::HDHomeRunTuner(HDHomeRun *parent, int index, UPnP::Device *tuner):
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



std::string HDHomeRunTuner::playChannel(int channel, int tuner_idx) {
    CAS_SetChannel(channel);
    ConnectionManager_PrepareForConnection();
    if (avtransportID == "") return "";
    AVTransport_Play();

    UPnP::Action *media_info = AVTransport->getAction("GetMediaInfo");
    media_info->setArgumentValue("InstanceID",avtransportID);
    UPnP::ActionResponse response;
    if (media_info->postControlAction(response)) {
        rtsp_uri = response.getValue("CurrentURI");

        if (!rtsp_sock.Connect(parent->getIPAddress(), 554))
            return "";
        int client_port = 57000+tuner_idx*2;
        std::string setup = "SETUP "+rtsp_uri+" RTSP/1.0\r\n"
            "CSeq: 1\r\n"
            "Transport: RTP/AVP;unicast;client_port="+Utils::StringUtils::toString(client_port)+"-"+Utils::StringUtils::toString(client_port+1)+"\r\n\r\n";
        std::string response = SendRTSPCommand(setup);
        if (response == "") return "";

        int start_pos = response.find("Session: ");
        int end_pos = response.find("\r\n", start_pos+9);
        rtsp_session = response.substr(start_pos+9, end_pos-(start_pos+9));
        Log::Info() << "HDHomeRunTuner: RTSP Session - " << rtsp_session << std::endl;

        std::string play = "PLAY "+rtsp_uri+" RTSP/1.0\r\n"
            "CSeq: 2\r\n"
            "Session: "+rtsp_session+"\r\n\r\n";
        response = SendRTSPCommand(play);
        if (response == "") return "";

        return "rtp://@:"+Utils::StringUtils::toString(client_port);
    }
    return "";
}
void HDHomeRunTuner::stopChannel() {
    std::string teardown = "TEARDOWN "+rtsp_uri+" RTSP/1.0\r\n"
      "CSeq: 3\r\n"
      "Session: "+rtsp_session+"\r\n\r\n";
    SendRTSPCommand(teardown);
    rtsp_sock.Close();

    AVTransport_Stop();
    ConnectionManager_ConnectionComplete();
    CAS_SetChannel(0);
    connectionID = "0";
    avtransportID = "0";
}

int HDHomeRunTuner::getProgramNumber() {
    return Mux_ProgramNumber();
}

std::string HDHomeRunTuner::getUPnPDeviceType() {
    return tuner->getDeviceType();
}

std::string HDHomeRunTuner::SendRTSPCommand(std::string command) {
    //std::cout << command << std::endl;
    if (rtsp_sock.Send(command.c_str(), command.length()) != command.length()) {
        Log::Info() << "HDHomeRunTuner: Could not send rtsp command" << std::endl;
        rtsp_sock.Close();
        return "";
    }

    char buffer[1024];
    std::string response = "";
    while (response.find("\r\n\r\n")==std::string::npos) {
        int len = rtsp_sock.Recv(buffer, 1023);
        if (len <= 0) {
            Log::Info() << "HDHomeRunTuner: Could not receive rtsp response" << std::endl;
            rtsp_sock.Close();
            return "";
        }
        response.append(buffer, len);
    }
    //std::cout << response << std::endl;
    return response;
}

std::string HDHomeRunTuner::AVTransport_URI() {
    return AVTransport->getStateVariable("AVTransportURI");
}

void HDHomeRunTuner::CAS_SetChannel(int channel) {
    UPnP::Action *setChannel = CAS->getAction("SetChannel");
    setChannel->setArgumentValue("NewCaptureMode","Live");
    setChannel->setArgumentValue("NewChannelNumber", Utils::StringUtils::toString(channel).c_str());
    setChannel->setArgumentValue("NewSourceId","0");

    UPnP::ActionResponse response;
    if (setChannel->postControlAction(response)) {
        //std::cout << "CAS::SetChannel()" << std::endl;
    }
}
int HDHomeRunTuner::CAS_GetChannel() {
    return atoi(CAS->getStateVariable("VirtualChannelNumber").c_str());
}

void HDHomeRunTuner::ConnectionManager_PrepareForConnection() {
    //PrepareForConnection();
    UPnP::Action *prepareConn = ConnectionManager->getAction("PrepareForConnection");
    prepareConn->setArgumentValue("RemoteProtocolInfo","rtsp-rtp-udp:*:dri-mp2t:*");
    prepareConn->setArgumentValue("PeerConnectionManager","");
    prepareConn->setArgumentValue("PeerConnectionID","-1");
    prepareConn->setArgumentValue("Direction","Output");

    UPnP::ActionResponse response;
    if (prepareConn->postControlAction(response)) {
        connectionID = response.getValue("ConnectionID");
        avtransportID = response.getValue("AVTransportID");
        //std::cout << "ConnectionManager::PrepareForConnection()" << std::endl;
        //std::cout << connectionID << "," << avtransportID << std::endl;
    }
}
void HDHomeRunTuner::AVTransport_Play() {
    UPnP::Action *play = AVTransport->getAction("Play");
    play->setArgumentValue("InstanceID",avtransportID);
    play->setArgumentValue("Speed","1");

    UPnP::ActionResponse response;
    if (play->postControlAction(response)) {
        //std::cout << "AVTransport::Play()" << std::endl;
    }
}

void HDHomeRunTuner::AVTransport_Stop() {
    UPnP::Action *stop = AVTransport->getAction("Stop");
    stop->setArgumentValue("InstanceID",avtransportID);

    UPnP::ActionResponse response;
    if (stop->postControlAction(response)) {
        //std::cout << "AVTransport::Stop()" << std::endl;
    }
}
void HDHomeRunTuner::ConnectionManager_ConnectionComplete() {
    UPnP::Action *connCompl = ConnectionManager->getAction("ConnectionComplete");
    connCompl->setArgumentValue("ConnectionID",connectionID);

    UPnP::ActionResponse response;
    if (connCompl->postControlAction(response)) {
        //std::cout << "ConnectionManager::ConnectionComplete()" << std::endl;
    }
}

int HDHomeRunTuner::Mux_ProgramNumber() {
    return atoi(Mux->getStateVariable("ProgramNumber").c_str());
}

}}
