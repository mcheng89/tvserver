#include "hdhomerunhttp.h"
#include "hdhomerunhttptuner.h"
#include "../utils/file.h"
#include "../utils/json.h"
#include "../utils/stringutils.h"
#include "../http/httpclient.h"
#include "../log.h"

namespace TV {
namespace Devices {

HDHomeRunHTTP::HDHomeRunHTTP(std::string ip_address): Device(ip_address, 3) {
    hdhomerun = 0;
}
HDHomeRunHTTP::~HDHomeRunHTTP() {
    cleanUp();
}

bool HDHomeRunHTTP::loadDevice() {
    std::string hdhomerunUPnPXML = "http://"+getIPAddress()+"/dri/device.xml";
    std::vector<UPnP::Device *> tuners;

    hdhomerun = new UPnP::Device();
    if (!hdhomerun->Init(hdhomerunUPnPXML)) {
        Log::Info() << "HDHomeRunPrime: Could net get device information" << std::endl;
        goto CleanUp;
    }

    if (getUPnPDeviceType().compare("urn:schemas-dkeystone-com:device:SecureContainer:1")!=0) {
        Log::Info() << "HDHomeRunPrime: Invalid device type" << std::endl;
        goto CleanUp;
    }
    tuners = hdhomerun->getDeviceList();
    if (tuners.size() != 3) {
        Log::Info() << "HDHomeRunPrime: Invalid tuner count" << std::endl;
        goto CleanUp;
    }
    this->tuners = new Tuner*[3];
    for (int i=0; i<3; i++)
        this->tuners[i] = 0;
    for (int i=0; i<3; i++) {
        this->tuners[i] = new HDHomeRunHTTPTuner(this, i, tuners.at(i));
        if (((HDHomeRunHTTPTuner*)this->tuners[i])->getUPnPDeviceType().compare("urn:schemas-upnp-org:device:MediaServer:1")!=0) {
            Log::Info() << "HDHomeRunPrime: Invalid tuner type" << std::endl;
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

std::string HDHomeRunHTTP::getUPnPDeviceType() {
    if (!hdhomerun) return Device::getDeviceType();
    return hdhomerun->getDeviceType();
}

bool HDHomeRunHTTP::loadChannelMap() {
    if (!Utils::File::exists("resources/ChannelMap-"+getDeviceType()+".csv")) {
        std::string html = HTTP::HTTPClient::getHTML(ip_address, 80, "/lineup.json?show=unprotected");
        Utils::JSON json(Utils::JSON::ARRAY, false);
        if (!json.load(html)) return false;
        for (int i=0; i<json.getArraySize(); i++) {
            Utils::JSON json_channel(Utils::JSON::OBJECT, false);
            if (json.getArrayObject(&json_channel, i)) {
                ChannelMap *channel = new ChannelMap;
                std::string channel_number;
                json_channel.getString("GuideName", channel->name);
                json_channel.getString("GuideNumber", channel_number);
                channel->number = atoi(channel_number.c_str());
                channel_map.insert( std::pair<int,ChannelMap*>(channel->number, channel) );
            }
        }
        if (channel_map.size()==0) return false;

        FILE *f = fopen(("resources/ChannelMap-"+getDeviceType()+".csv").c_str(), "wb");
        for ( std::map<int,ChannelMap*>::iterator it=channel_map.begin() ; it != channel_map.end(); it++ ) {
            ChannelMap *channel = (*it).second;
            fprintf(f, "%i,%s\n", channel->number, channel->name.c_str());
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
            if (channel_info.size() == 2) {
                ChannelMap *channel = new ChannelMap;
                channel->number = atoi(channel_info.at(0).c_str());
                channel->name = channel_info.at(1);
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

void HDHomeRunHTTP::cleanUp() {
    Device::cleanUp();
    if (hdhomerun) {
        hdhomerun->Finish();
        delete hdhomerun;
        hdhomerun = 0;
    }
}

}}

