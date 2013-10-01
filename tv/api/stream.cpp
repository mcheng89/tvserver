#include "stream.h"
#include "../utils/json.h"
#include "../utils/file.h"
#include "../http/httpclient.h"
#include "../utils/stringutils.h"

namespace TV {
namespace API {

void PlayService::handleRequest(HTTP::HTTPRequest *request) {
    Utils::JSON json_obj;
    json_obj.setString("status", "error");

    int tuner_idx = -1, tuner_lock_key = -1;
    if (request->getParameter("tuner") != "" && request->getParameter("key") != "") {
        tuner_idx = atoi(request->getParameter("tuner").c_str());
        tuner_lock_key = atoi(request->getParameter("key").c_str());
    }

    Tuner *tuner = server->getAvailableTuner(tuner_idx, tuner_lock_key);
    if (tuner) {
        if (request->getParameter("channel") == "") {
            json_obj.setString("message","Did not receive a channel parameter");
            goto HandlePlayRequestRet;
        }
        bool redirect = request->getParameter("redirect")=="true";
        StreamSettings stream_info;
        stream_info.channel = atoi(request->getParameter("channel").c_str());
        std::string format = request->getParameter("format");
        if (format == "ts" || format == "flv" || format == "hls")
            stream_info.format = format;
        if (request->getParameter("bitrate")!="" && request->getParameter("resolution")!="") {
            stream_info.video_bitrate = atoi(request->getParameter("bitrate").c_str());
            stream_info.video_resolution = atoi(request->getParameter("resolution").c_str());
            if (stream_info.video_bitrate <= 200)
                stream_info.audio_bitrate = 64;
            else if (stream_info.video_bitrate <= 1200)
                stream_info.audio_bitrate = 92;
            else if (stream_info.video_bitrate <= 2000)
                stream_info.audio_bitrate = 128;
            else stream_info.audio_bitrate = 160;
        }
        if (request->getParameter("multicast")=="true" && request->getParameter("duration")!="") {
            stream_info.multicast = true;
            stream_info.duration_minutes = atoi(request->getParameter("duration").c_str());
        }

        Log::Info() << "Stream: Received play request for channel " << stream_info.channel << std::endl;
        if (tuner->startStream(stream_info)) {
            std::string uri = request->getUri();
            uri = uri.substr(0,uri.rfind("/")+1);
            std::string url = request->isHTTPS()?"https://":"http://"+request->getRequestHeader("Host")+uri;
            if (format!="hls")
                url += std::string(format=="flv"?"stream.flv":"stream")+"?tuner="+Utils::StringUtils::toString(tuner->getTunerIndex())+"&q="+Utils::StringUtils::toString(time(0));
            else
                url += "hls/tuner"+Utils::StringUtils::toString(tuner->getTunerIndex())+".m3u8";
            if (format!="hls" && redirect) {
                request->sendRedirect(url);
                goto HandlePlayRequestRedirect;
            }

            if (format == "hls") {
                json_obj.setInteger("tuner", tuner->getTunerIndex());
                json_obj.setInteger("key", tuner->getStreamKey());
            }
            json_obj.setString("url", url);
            json_obj.setString("status", "success");
            json_obj.setString("message", "Playing channel: "+Utils::StringUtils::toString(stream_info.channel));
        } else {
            json_obj.setString("message", "Could not play channel: "+Utils::StringUtils::toString(stream_info.channel));
        }
    } else {
        json_obj.setString("message", "Could not get an available tuner");
    }

HandlePlayRequestRet:
    request->sendJSON(HTTP::HTTPRequest::HTTP_OK, &json_obj);
HandlePlayRequestRedirect:
    if (tuner) tuner->unlock();
}

void StopService::handleRequest(HTTP::HTTPRequest *request) {
    Utils::JSON json_obj;
    json_obj.setString("status", "error");

    if (request->getParameter("tuner") != "") {
        int tuner_idx = atoi(request->getParameter("tuner").c_str());
        std::vector<Tuner*> tuner_list = server->getTuners();
        if (tuner_idx<0 || tuner_idx>=tuner_list.size()) {
            json_obj.setString("message", "Did not receive valid tuner");
            goto HandleStopRequestRet;
        }

        Tuner *tuner = tuner_list.at(tuner_idx);
        tuner->lock();
        if (tuner->isStreaming(true)) {
            Log::Info() << "Stream: Received force stop request for tuner " << tuner_idx << std::endl;
            tuner->stopStream();
            json_obj.setString("status", "success");
            json_obj.setString("message", "Stopped tuner from streaming.");
        } else {
            json_obj.setString("message", "Could not stop tuner. Tuner is not running.");
        }
        tuner->unlock();
    } else {
        json_obj.setString("message", "Did not receive a tuner parameter");
    }

HandleStopRequestRet:
    request->sendJSON(HTTP::HTTPRequest::HTTP_OK, &json_obj);
}

void StreamService::handleRequest(HTTP::HTTPRequest *request) {
    if (request->getParameter("tuner") != "") {
        int tuner_idx = atoi(request->getParameter("tuner").c_str());
        std::vector<Tuner*> tuner_list = server->getTuners();
        if (tuner_idx<0 || tuner_idx>=tuner_list.size())
            goto HandleStreamRequestError;

        Tuner *tuner = tuner_list.at(tuner_idx);
        tuner->lock();
        if (tuner->isStreaming()) {
            int stream_key = tuner->getStreamLock();
            tuner->unlock();

            HTTP::HTTPClient vlc;
            std::string video_data = "";
            int write_len = 0;

            request->startStream(HTTP::HTTPRequest::HTTP_OK, tuner->getStreamInfo().format=="ts"?"video/MP2T":"video/x-flv");
            if (vlc.connect("127.0.0.1", tuner->getStreamPort(), "/")) {
                while (server->isRunning()) {
                    video_data = vlc.readData(8192);
                    if (video_data.size()==0)
                        break;
                    else {
                        while (video_data.size() > 0) {
                            if ( (write_len = request->writeStream(video_data)) <= 0)
                                break;
                            video_data = video_data.substr(write_len);
                        }
                        if (video_data.size() > 0)
                            break;
                        else if (stream_key)
                            tuner->setStreamShutdown(TUNER_SHUTDOWN_TIME);
                    }
                }
            }
            if (stream_key) tuner->stopStream(stream_key); //the function handles the lock! :)
            return;
        }
        tuner->unlock();
    }

HandleStreamRequestError:
    request->sendResponse(HTTP::HTTPRequest::HTTP_NOT_FOUND, "File not found");
}

void HLSService::handleRequest(HTTP::HTTPRequest *request) {
    //reminder: if adaptive streaming, lock and unlock between start and stop when changing stream settings
    std::string filename = request->getUri().substr(5);
    //if (filename == "" || filename.at(filename.length()-1)=='/') filename += "index.html";
    std::string cur_directory = server->getSettings()->getExecDirectory();
    std::string real_filename = Utils::File::getAbsolutePath(cur_directory+"tmp/"+filename);
    if (real_filename.find(cur_directory+"tmp"+Utils::File::getPathSeparator()) == 0) {
        int start_pos = (cur_directory+"tmp/tuner").length();
        int end_pos = start_pos+1;
        while (real_filename.at(end_pos)!='.' && real_filename.at(end_pos)!='_' && end_pos<real_filename.length())
            end_pos++;

        int tuner_idx = atoi(real_filename.substr(start_pos, end_pos-start_pos).c_str());
        //printf("tuner selected is %i\n", tuner_idx);

        std::vector<Tuner*> tuner_list = server->getTuners();
        if (tuner_idx>=0 && tuner_idx<tuner_list.size()) {
            Tuner *tuner = server->getTuners().at(tuner_idx);
            tuner->lock();
            if (tuner->isStreaming() && tuner->getStreamInfo().format=="hls") {
                tuner->setStreamShutdown(TUNER_SHUTDOWN_TIME);
                if (Utils::File::exists( real_filename )) {
                    tuner->unlock();
                    request->sendFile(real_filename);
                    return;
                }
            }
            tuner->unlock();
        }
    }
    request->sendResponse(HTTP::HTTPRequest::HTTP_NOT_FOUND, "File not found");
}

}}
