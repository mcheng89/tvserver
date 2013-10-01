#ifndef TV_API_STREAM_H
#define TV_API_STREAM_H

#include "../tv.h"

namespace TV {
namespace API {

    class PlayService: public TVService {
    public:
        PlayService(Server *server):TVService(server, true) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

    class StopService: public TVService {
    public:
        StopService(Server *server):TVService(server, true) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

    class StreamService: public TVService {
    public:
        StreamService(Server *server):TVService(server) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

    class HLSService: public TVService {
    public:
        HLSService(Server *server):TVService(server) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

}}

#endif // TV_API_STREAM_H
