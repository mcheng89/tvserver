#ifndef TV_API_SERVER_H
#define TV_API_SERVER_H

#include "../tv.h"

namespace TV {
namespace API {

    class StatusService: public TVService {
    public:
        StatusService(Server *server):TVService(server, true) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

    class SettingsService: public TVService {
    public:
        SettingsService(Server *server):TVService(server, true) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

    class ShutdownService: public TVService {
    public:
        ShutdownService(Server *server):TVService(server, true) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

    class FileService: public TVService {
    public:
        FileService(Server *server):TVService(server) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

}}

#endif // TV_API_SERVER_H
