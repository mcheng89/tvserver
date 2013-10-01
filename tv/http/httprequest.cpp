#include "httprequest.h"
#include "../utils/stringutils.h"
#include <stdio.h>
#include <time.h>
#include <windows.h>

namespace TV {
namespace HTTP {

HTTPRequest::HTTPRequest(struct mg_connection *conn) {
    this->conn = conn;
    request_info = mg_get_request_info(conn);

    // Get the ip address
    in_addr sa;
    memset(&sa, 0, sizeof(in_addr));
    sa.s_addr = htonl(request_info->remote_ip);
    client_ip_address = inet_ntoa(sa);

    int sendbuff = 96*1024;
    size_t optlen = sizeof(sendbuff);
    mg_setsockopt(conn, SOL_SOCKET, SO_SNDBUF, &sendbuff, optlen);
}

void HTTPRequest::sendResponse(HTTPStatus status, std::string response) {
    mg_printf(conn, "HTTP/1.1 %i Unknown\r\n"
        "Content-Length: %i\r\n"
        "Content-Type: text/plain\r\n\r\n", status, response.length());
    mg_write(conn, response.c_str(), response.length());
}
void HTTPRequest::sendRedirect(std::string url) {
    mg_printf(conn, "HTTP/1.1 302 Found\r\n"
        "Location: %s\r\n\r\n", url.c_str());
}

void HTTPRequest::sendFile(std::string file_path) {
    mg_send_file(conn, file_path.c_str());
}

void HTTPRequest::sendJSON(HTTPStatus status, Utils::JSON *json_obj) {
    sendResponse(status, json_obj->toString());
}

void HTTPRequest::startChunked(HTTPStatus status, std::string content_type) {
    mg_printf(conn, "HTTP/1.1 %i Unknown\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: %s\r\n\r\n", status, content_type.c_str());
}
int HTTPRequest::writeChunked(std::string buffer) {
    std::string chunkSize = Utils::StringUtils::intToHex(buffer.length());
    mg_write(conn, chunkSize.c_str(), chunkSize.length());
    mg_write(conn, "\r\n", 2);
    mg_write(conn, buffer.c_str(), buffer.length());
    return mg_write(conn, "\r\n", 2);
}
void HTTPRequest::endChunked() {
    mg_write(conn, "0\r\n\r\n", 7);
}

void HTTPRequest::startStream(HTTPStatus status, std::string content_type) {
    mg_printf(conn, "HTTP/1.1 %i Unknown\r\n"
        "Connection: close\r\n"
        "Content-Type: %s\r\n\r\n", status, content_type.c_str());
}
int HTTPRequest::writeStream(std::string buffer) {
    return mg_write(conn, buffer.c_str(), buffer.length());
}

std::string HTTPRequest::getParameter(std::string param) {
    if (!request_info->query_string)
        return "";
    int ret = mg_get_var(request_info->query_string, strlen(request_info->query_string), param.c_str(), http_param_buf, MAX_HTTP_PARAM_LEN-1);
    if (ret < 0) return "";
    return http_param_buf;
}

std::string HTTPRequest::getPostData() {
    if (getRequestMethod() != "POST")
        return "";
    const char *content_length = mg_get_header(conn,"Content-Length");
    int bytesRead = 0, contentLength = content_length?atoi(content_length):0;
    char buffer[1024];
    std::string post_data = "";
    while (contentLength > 0) {
        bytesRead = mg_read(conn, buffer, 1023);
        if (bytesRead == 0) break;
        post_data.append(buffer, bytesRead);
        contentLength -= bytesRead;
    }
    return post_data;
}

std::string HTTPRequest::getRequestHeader(std::string param) {
    const char *value =  mg_get_header(conn,param.c_str());
    return value?value:"";
}


std::string HTTPRequest::getAuthParam(std::string authHeader, std::string authField) {
    if (authHeader.find(authField+"=") == std::string::npos)
        return "";
    int start_pos = authHeader.find(authField+"=")+authField.length()+1;
    if (start_pos >= authHeader.length())
        return "";
    bool isQuoted = (authHeader.at(start_pos)=='\"');
    if (isQuoted) start_pos++;
    int end_pos = start_pos;
    while (end_pos < authHeader.length()) {
        if (isQuoted) {
            if (authHeader.at(end_pos) == '\\' && authHeader.at(end_pos+1) == '\\')
                end_pos += 2;
            else if (authHeader.at(end_pos) == '\\' && authHeader.at(end_pos+1) == '\"')
                end_pos += 2;
            else if (authHeader.at(end_pos) == '\"')
                break;
            else end_pos++;
        } else {
            if (authHeader.at(end_pos) == ',')
                break;
            end_pos++;
        }
    }
    return authHeader.substr(start_pos, end_pos-start_pos);
}

bool HTTPRequest::authenticate(std::string username, std::string password) {
    if (mg_get_header(conn, "Authorization") != 0) {
        std::string authHeaders = mg_get_header(conn, "Authorization");
        if (authHeaders.length() > 7) {
            authHeaders = authHeaders.substr(7);
            std::string auth_username = getAuthParam(authHeaders,"username"); // for future use? multi user support
            //if (time(0)-atoi(getAuthParam(authHeaders,"nonce").c_str()) < 20) { // give them 20 seconds to authenticate
                char response_1[33], response_2[33], response[33];
                mg_md5(response_1, username.c_str(), ":tv_server:", password.c_str(), 0);
                mg_md5(response_2, request_info->request_method, ":", getAuthParam(authHeaders,"uri").c_str(), 0);
                mg_md5(response, response_1, ":", getAuthParam(authHeaders,"nonce").c_str(), ":",
                    getAuthParam(authHeaders,"nc").c_str(), ":", getAuthParam(authHeaders,"cnonce").c_str(),
                    ":auth:", response_2, 0);
                if (getAuthParam(authHeaders,"response").compare(response)==0)
                    return true;
            //}
        }
    }

    mg_printf(conn, "HTTP/1.1 401 Unauthorized\r\n"
        "WWW-Authenticate: Digest realm=\"tv_server\", qop=\"auth\", nonce=\"%s\"\r\n"
        "Content-Length: 17\r\n\r\n"
        "Not Authenticated", Utils::StringUtils::toString(time(0)).c_str());
    return false;
}

}}
