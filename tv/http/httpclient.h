#ifndef TV_HTTP_HTTPCLIENT_H
#define TV_HTTP_HTTPCLIENT_H

#include "../utils/socket.h"
#include <string>

namespace TV {
namespace HTTP {

    class HTTPClient {
    public:
        bool connect(std::string hostname, int port, std::string path);
        std::string readData(int len);

        static std::string getHTML(std::string hostname, int port, std::string path);
    private:
        Utils::Socket sock;
        std::string data;
    };

}}

#endif // TV_HTTP_HTTPCLIENT_H
