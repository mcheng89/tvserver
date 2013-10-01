#ifndef TV_HTTP_HTTPSERVER_H
#define TV_HTTP_HTTPSERVER_H

#include "../threads/mutex.h"
#include "../mongoose/mongoose.h"
#include "httprequest.h"
#include "httpservice.h"
#include <map>
#include <string>

namespace TV {
namespace HTTP {

    class HTTPServer {
    public:
        HTTPServer();
        virtual ~HTTPServer();

        bool start(int port);
        void stop();

        void addService(HTTPService *service, std::string path, HTTPService::HTTPPathType type=HTTPService::PATH_MATCH);
        HTTPService *getService(std::string path);

    private:
        static void *callback(enum mg_event event, struct mg_connection *conn);

        Threads::Mutex startup_mutex;
        struct mg_context *server_ctx;
        char **server_options;

        std::map<std::string,HTTPService*> services_match;
        struct starts_with_key {
            enum {INSERT, FIND} action;
            std::string path;
        };
        struct starts_with_cmp {
            bool operator() (const struct starts_with_key a, const struct starts_with_key b) {
                if (b.action == b.FIND) {
                    return b.path.find(a.path)==0?false:a.path.compare(b.path)<0;
                } else if (a.action == a.FIND) {
                    return a.path.find(b.path)==0?false:a.path.compare(b.path)<0;
                } else {
                    return a.path.compare(b.path)<0;
                }
            }
        };
        std::map<struct starts_with_key,HTTPService*,starts_with_cmp> services_start;

        HTTPServer(const HTTPServer&);
        HTTPServer& operator = (const HTTPServer&);
    };

}}

#endif // TV_HTTP_HTTPSERVER_H
