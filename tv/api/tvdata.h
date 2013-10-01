#ifndef TV_API_TVDATA_H
#define TV_API_TVDATA_H

#include "../tv.h"

namespace TV {
namespace API {

    class EPGService: public TVService {
    public:
        EPGService(Server *server):TVService(server, true) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

    class ChannelListService: public TVService {
    public:
        ChannelListService(Server *server):TVService(server, true) {};
        void handleRequest(HTTP::HTTPRequest *request);
    };

}}

#endif // EPGDATA_H_INCLUDED
