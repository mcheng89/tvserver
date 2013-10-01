#include "server.h"
#include "../utils/file.h"

namespace TV {
namespace API {

void StatusService::handleRequest(HTTP::HTTPRequest *request) {
    Utils::JSON json_obj, json_tuners(Utils::JSON::ARRAY);
    json_obj.setString("status", "success");
    json_obj.setString("version", VERSION);

    std::vector<Tuner*> tuners = server->getTuners();
    for (int i=0; i<tuners.size(); i++) {
        Tuner *tuner = tuners.at(i);
        Utils::JSON json_tuner;
        json_tuner.setString("device_type", tuner->getTuner()->getDevice()->getDeviceType());
        json_tuner.setString("ip_address", tuner->getTuner()->getDevice()->getIPAddress());
        json_tuner.setBoolean("is_streaming", tuner->isStreaming());
        json_tuner.setInteger("channel", tuner->getTuner()->getChannelNumber());
        if (tuner->isStreaming()) {
            Utils::JSON json_stream;
            json_stream.setString("format", tuner->getStreamInfo().format);
            json_stream.setInteger("bitrate", tuner->getStreamInfo().video_bitrate);
            json_stream.setInteger("resolution", tuner->getStreamInfo().video_resolution);
            json_stream.setBoolean("multicast", tuner->getStreamInfo().multicast);
            json_stream.setInteger("shutdown_time", tuner->getStreamShutdown());
            json_stream.setInteger("key", tuner->getStreamKey());
            json_tuner.setObject("stream_info", &json_stream);
        }
        json_tuners.addArrayObject(&json_tuner);
    }
    json_obj.setObject("tuners", &json_tuners);

    request->sendJSON(HTTP::HTTPRequest::HTTP_OK, &json_obj);
}

void SettingsService::handleRequest(HTTP::HTTPRequest *request) {
    Utils::JSON json_obj, json_settings;
    boolean is_local_access = request->getClientIp() == "127.0.0.1";
    if (request->getRequestMethod() == "POST") {
        if (is_local_access)
            server->getSettings()->loadData(request->getPostData(), true);
        else {
            json_obj.setString("status", "error");
            json_obj.setString("message", "Not authorized!");
            request->sendJSON(HTTP::HTTPRequest::HTTP_OK, &json_obj);
            return;
        }
    }

    Config *config = server->getSettings();
    json_obj.setString("status", "success");
    json_obj.setString("access", is_local_access?"admin":"user");
    json_obj.setString("version", VERSION);
    json_obj.setString("server_exec_dir", config->getExecDirectory());

    config->toJSON(&json_settings);
    json_obj.setObject("settings", &json_settings);

    request->sendJSON(HTTP::HTTPRequest::HTTP_OK, &json_obj);
}

void ShutdownService::handleRequest(HTTP::HTTPRequest *request) {
    Utils::JSON json_obj;
    if (request->getClientIp() == "127.0.0.1") {
        json_obj.setString("status", "success");
        if (request->getParameter("restart")!="") {
            json_obj.setString("message", "Restarting server...");
            request->sendJSON(HTTP::HTTPRequest::HTTP_OK, &json_obj);
            server->restart();
        } else {
            json_obj.setString("message", "Shutting down...");
            request->sendJSON(HTTP::HTTPRequest::HTTP_OK, &json_obj);
            server->stop();
        }
    } else {
        json_obj.setString("status", "error");
        json_obj.setString("message", "Not authorized!");
        request->sendJSON(HTTP::HTTPRequest::HTTP_FORBIDDEN, &json_obj);
    }
}

void FileService::handleRequest(HTTP::HTTPRequest *request) {
    std::string filename = request->getUri().substr(3);
    if (filename == "" || filename.at(filename.length()-1)=='/') filename += "index.html";
    std::string cur_directory = server->getSettings()->getExecDirectory();
    std::string real_filename = Utils::File::getAbsolutePath(cur_directory+"webfiles/"+filename);
    if (Utils::File::exists( real_filename )) {
        if (real_filename.find(cur_directory+"webfiles"+Utils::File::getPathSeparator()) == 0) {
            request->sendFile(real_filename);
            return;
        }
    }
    request->sendResponse(HTTP::HTTPRequest::HTTP_NOT_FOUND, "File not found");
}

}}
