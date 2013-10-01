#include "httpserver.h"
#include <stdio.h>

namespace TV {
namespace HTTP {

HTTPServer::HTTPServer() {
    server_ctx = 0;
    server_options = 0;
}
HTTPServer::~HTTPServer() {
    stop();
}

bool HTTPServer::start(int port) {
    startup_mutex.lock();
    char *buffer = new char[6];
    snprintf(buffer,6,"%i",port);

    server_options = new char*[3];
    server_options[0] = "listening_ports";
    server_options[1] = buffer;
    server_options[2] = NULL;
    server_ctx = mg_start(&callback, this, (const char**)server_options);

	if (!server_ctx) {
	    stop();
	    startup_mutex.unlock();
        return false;
	}
	startup_mutex.unlock();
    return true;
}

void HTTPServer::stop() {
    startup_mutex.lock();
    if (server_ctx)
        mg_stop(server_ctx);
    if (server_ctx) {
        delete []server_options[1];
        delete []server_options;
    }
    server_ctx = 0;
    server_options = 0;
	startup_mutex.unlock();
}

void HTTPServer::addService(HTTPService *service, std::string path, HTTPService::HTTPPathType type) {
    if (type == HTTPService::PATH_MATCH)
        services_match.insert(std::pair<std::string,HTTPService*>(path, service));
    else {
        struct starts_with_key key;
        key.action = key.INSERT;
        key.path = path;
        services_start.insert(std::pair<struct starts_with_key,HTTPService*>(key, service));
    }
}
HTTPService *HTTPServer::getService(std::string path) {
    std::map<std::string,HTTPService*>::iterator it = services_match.find(path);
    if (it != services_match.end())
        return it->second;
    struct starts_with_key key;
    key.action = key.FIND;
    key.path = path;
    std::map<struct starts_with_key,HTTPService*>::iterator it2 = services_start.find(key);
    if (it2 != services_start.end())
        return it2->second;
    return 0;
}

void *HTTPServer::callback(enum mg_event event, struct mg_connection *conn) {
    if (event == MG_NEW_REQUEST) {
        const struct mg_request_info *request_info = mg_get_request_info(conn);
        HTTPServer *server = ((HTTPServer*)request_info->user_data);

        HTTPRequest request(conn);
        HTTPService *service = server->getService(request.getUri());
        if (service) {
            service->handleHTTPRequest(&request);
        }

        return (void*)"";
    }
    return 0;
}

}}
