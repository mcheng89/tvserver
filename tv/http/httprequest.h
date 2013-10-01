#ifndef TV_HTTP_HTTPREQUEST_H
#define TV_HTTP_HTTPREQUEST_H

#include "../mongoose/mongoose.h"
#include "../utils/json.h"
#include <string>
#include <map>

#define MAX_HTTP_PARAM_LEN 256

namespace TV {
namespace HTTP {

    class HTTPRequest {
    public:
        HTTPRequest(struct mg_connection *conn);

        enum HTTPStatus {HTTP_OK=200,HTTP_NOT_FOUND=404, HTTP_FORBIDDEN=403, HTTP_NOT_IMPLEMENTED=501};
        void sendResponse(HTTPStatus status, std::string response);
        void sendRedirect(std::string url);
        void sendFile(std::string file_path);
        void sendJSON(HTTPStatus status, Utils::JSON *json_obj);

        void startChunked(HTTPStatus status, std::string content_type="text/plain");
        int writeChunked(std::string buffer);
        void endChunked();

        void startStream(HTTPStatus status, std::string content_type="text/plain");
        int writeStream(std::string buffer);

        std::string getAuthParam(std::string authHeader, std::string authField);
        bool authenticate(std::string username, std::string password);

        std::string getParameter(std::string param);
        std::string getPostData();
        std::string getRequestHeader(std::string param);
        std::string getRequestMethod() { return request_info->request_method; }
        std::string getUri() { return request_info->uri; }
        std::string getClientIp() { return client_ip_address; }
        bool isHTTPS() { return request_info->is_ssl; }

        struct mg_connection *getConnection() { return conn; };

        void dump_headers() {
            printf("uri: %s\n", request_info->uri);
            for (int i=0; i<request_info->num_headers; i++) {
                printf("%s: %s\n", request_info->http_headers[i].name, request_info->http_headers[i].value);
            }
        }

    private:
        struct mg_connection *conn;
        std::string client_ip_address;
        const struct mg_request_info *request_info;
        char http_param_buf[MAX_HTTP_PARAM_LEN];

        HTTPRequest(const HTTPRequest&);
        HTTPRequest& operator = (const HTTPRequest&);
    };

}}

#endif // TV_HTTP_HTTPREQUEST_H
