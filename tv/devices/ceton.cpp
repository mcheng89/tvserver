#include "ceton.h"
#include "cetontuner.h"
#include "../utils/file.h"
#include "../utils/stringutils.h"
#include "../http/httpclient.h"
#include "../log.h"

namespace TV {
namespace Devices {

Ceton::Ceton(std::string ip_address): Device(ip_address, 4) {
    ceton = 0;
}
Ceton::~Ceton() {
    cleanUp();
}

bool Ceton::loadDevice() {
    std::string cetonUPnPXML = "http://"+getIPAddress()+"/description.xml";
    std::vector<UPnP::Device *> tuners;

    ceton = new UPnP::Device();
    if (!ceton->Init(cetonUPnPXML)) {
        Log::Info() << "CetonInfiniTV4: Could net get device information" << std::endl;
        goto CleanUp;
    }

    if (getUPnPDeviceType().compare("urn:schemas-cetoncorp-com:device:SecureContainer:1")!=0) {
        Log::Info() << "CetonInfiniTV4: Invalid device type" << std::endl;
        goto CleanUp;
    }
    tuners = ceton->getDeviceList();
    if (tuners.size() != 6) {
        Log::Info() << "CetonInfiniTV4: Invalid tuner count" << std::endl;
        goto CleanUp;
    }
    this->tuners = new Tuner*[4];
    for (int i=0; i<4; i++)
        this->tuners[i] = 0;
    for (int i=0; i<4; i++) {
        this->tuners[i] = new CetonTuner(this, i, tuners.at(i));
        if (((CetonTuner*)this->tuners[i])->getUPnPDeviceType().compare("urn:schemas-upnp-org:device:MediaServer:1")!=0) {
            Log::Info() << "CetonInfiniTV4: Invalid tuner type" << std::endl;
            goto CleanUp;
        }
    }

    if (!Device::loadDevice())
        goto CleanUp;

    return true;
    CleanUp:
    cleanUp();
    return false;
}

std::string Ceton::getUPnPDeviceType() {
    if (!ceton) return Device::getDeviceType();
    return ceton->getDeviceType();
}

bool Ceton::loadChannelMap() {
    if (!Utils::File::exists("resources/ChannelMap-"+getDeviceType()+".csv")) {

        std::string html = HTTP::HTTPClient::getHTML(ip_address, 80, "/view_channel_map.cgi?page=0");
        if (html == "") return false;

        int pos = -1, endpos = 0;
        while ( (pos = html.find("<tr><td>", pos+1)) != std::string::npos) {
            CetonMap *channel = new CetonMap;
            endpos = html.find("<", pos+5);
            channel->number = atoi(html.substr(pos+8, endpos-(pos+8)).c_str());
            pos = endpos + 9;
            endpos = html.find("<", pos);
            channel->name = html.substr(pos, endpos-pos);
            pos = html.find("<td>", html.find("<td>", html.find("<td>", pos+1)+1)+1);
            endpos = html.find("<", pos+1);
            channel->program = atoi(html.substr(pos+4, endpos-(pos+4)).c_str());
            //std::cout << "channel:" << channel_num << "|name:" << channel_name << std::endl;
            channel_map.insert( std::pair<int,ChannelMap*>(channel->number, channel) );
        }
        if (channel_map.size()==0) return false;

        FILE *f = fopen(("resources/ChannelMap-"+getDeviceType()+".csv").c_str(), "wb");
        for ( std::map<int,ChannelMap*>::iterator it=channel_map.begin() ; it != channel_map.end(); it++ ) {
            CetonMap *channel = (CetonMap*)(*it).second;
            fprintf(f, "%i,%s,%i\n", channel->number, channel->name.c_str(), channel->program);
        }
        fclose(f);
        for( std::map<int,ChannelMap*>::iterator it = channel_map.begin(); it != channel_map.end(); ++it )
            channel_map_vector.push_back( it->second );
        return true;
    } else {
        FILE *f = fopen(("resources/ChannelMap-"+getDeviceType()+".csv").c_str(), "r");
        char buffer[256];
        while (!feof(f)) {
            std::string csv;
            while (csv.find("\n")==std::string::npos && !feof(f)) {
                fgets(buffer, 255, f);
                csv += buffer;
            }
            if (csv.find("\n")!=std::string::npos)
                csv = csv.substr(0, csv.size()-1);

            std::vector<std::string> channel_info = Utils::StringUtils::split(csv, ",");
            if (channel_info.size() == 3) {
                CetonMap *channel = new CetonMap;
                channel->number = atoi(channel_info.at(0).c_str());
                channel->name = channel_info.at(1);
                channel->program = atoi(channel_info.at(2).c_str());
                //std::cout << channel.number << ":" << channel.name << ":" << channel.program << std::endl;
                channel_map.insert( std::pair<int,ChannelMap*>(channel->number, channel) );
            }
        }
        fclose(f);
        if (channel_map.size()==0) return false;
        for( std::map<int,ChannelMap*>::iterator it = channel_map.begin(); it != channel_map.end(); ++it )
            channel_map_vector.push_back( it->second );
        return true;
    }
}

void Ceton::cleanUp() {
    Device::cleanUp();
    if (ceton) {
        ceton->Finish();
        delete ceton;
        ceton = 0;
    }
}

}}

