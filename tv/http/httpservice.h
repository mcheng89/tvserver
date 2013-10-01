#ifndef TV_HTTP_HTTPSERVICE_H
#define TV_HTTP_HTTPSERVICE_H

#include "httprequest.h"

namespace TV {
namespace HTTP {

    class HTTPService {
    public:
        HTTPService() {}

        virtual void handleHTTPRequest(HTTPRequest *request) = 0;
        //PATH_MATCH = url has a full matches with this path
        //PATH_START = uri starts with this path
        enum HTTPPathType {PATH_MATCH, PATH_START};
    private:
        HTTPService(const HTTPService&);
        HTTPService& operator = (const HTTPService&);
    };

}}

#endif // TV_HTTP_HTTPSERVICE_H
