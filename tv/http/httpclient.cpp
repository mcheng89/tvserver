#include "httpclient.h"
#include "../utils/stringutils.h"

namespace TV {
namespace HTTP {

bool HTTPClient::connect(std::string hostname, int port, std::string path) {
    sock.Connect(hostname, port);
    std::string getReqeust = "GET "+path+" HTTP/1.1\r\nHost: "+hostname+"\r\nConnection: close\r\n\r\n";
    sock.Send(getReqeust.c_str(), getReqeust.length());

    int readLen;
    char buffer[1024];
    std::string getHeaders = "";
    while (getHeaders.find("\r\n\r\n")==std::string::npos) {
        readLen = sock.Recv(buffer, 1024);
        if (readLen <= 0)
            return false;
        getHeaders.append(buffer,readLen);
    }
    data = getHeaders.substr(getHeaders.find("\r\n\r\n")+4);
    getHeaders = getHeaders.substr(0, getHeaders.find("\r\n\r\n")+2);
    return true;
}
std::string HTTPClient::readData(int len) {
    std::string ret = "";
    if (data.length() > 0) {
        ret = data;
        data = "";
    } else {
        char buffer[len+1];
        int readLen = sock.Recv(buffer, len);
        if (readLen <= 0) {
            sock.Close();
            return "";
        }
        ret.append(buffer,readLen);
    }
    return ret;
}

std::string HTTPClient::getHTML(std::string hostname, int port, std::string path) {
    Utils::Socket sock;
    if (!sock.Connect(hostname, port)) return "";

    std::string getReqeust = "GET "+path+" HTTP/1.1\r\nHost: "+hostname+"\r\nConnection: close\r\n\r\n";
    sock.Send(getReqeust.c_str(), getReqeust.length());

    int readLen;
    char buffer[1024];
    std::string getHeaders = "";
    std::string getResponse = "";
    while (getHeaders.find("\r\n\r\n")==std::string::npos) {
        readLen = sock.Recv(buffer, 1024);
        if (readLen <= 0)
            goto httpGetRequestError;
        getHeaders.append(buffer,readLen);
    }
    getResponse = getHeaders.substr(getHeaders.find("\r\n\r\n")+4);
    getHeaders = getHeaders.substr(0, getHeaders.find("\r\n\r\n")+2);

    if (Utils::StringUtils::findIgnoreCase(getHeaders,"Transfer-Encoding: chunked")!=std::string::npos) {
        std::string chunkedData = getResponse;
        int chunkSize = 0;
        getResponse = "";
        for (;;) {
            if (chunkSize==0 && chunkedData.find("\r\n")==std::string::npos) {
                ///wait for next chunk
                readLen = sock.Recv(buffer, 1024);
                if (readLen <= 0)
                    goto httpGetRequestError;
                chunkedData.append(buffer,readLen);
            } else if (chunkSize>0) {
                ///still in middle of reading chunk data
                readLen = sock.Recv(buffer, (chunkSize>1024?1024:chunkSize));
                if (readLen <= 0)
                    goto httpGetRequestError;
                getResponse.append(buffer,readLen);
                chunkSize -= readLen;
                if (chunkSize == 0)
                    if (sock.Recv(buffer, 2)!=2)
                        goto httpGetRequestError;
            } else {
                ///found next chunk
                chunkSize = Utils::StringUtils::hexToInt(chunkedData.substr(0,chunkedData.find("\r\n")));
                if (chunkSize == 0) break;
                chunkedData = chunkedData.substr(chunkedData.find("\r\n")+2);
                int appendLen = (chunkedData.length()<chunkSize?chunkedData.length():chunkSize);
                getResponse.append(chunkedData,0,appendLen);
                if (chunkedData.length() == appendLen) {
                    chunkedData = "";
                } else chunkedData = chunkedData.substr(appendLen);
                chunkSize -= appendLen;
                if (chunkSize == 0) {
                    if (chunkedData.length() > 2)
                        chunkedData = chunkedData.substr(2);
                    else {
                        if (sock.Recv(buffer, 2-chunkedData.length())!=2-chunkedData.length())
                            goto httpGetRequestError;
                        chunkedData = "";
                    }
                }
            }
        }
    } else if (Utils::StringUtils::findIgnoreCase(getHeaders,"Content-Length: ")!=std::string::npos) {
        int start_pos = Utils::StringUtils::findIgnoreCase(getHeaders,"Content-Length: ");
        int end_pos = getHeaders.find("\r\n", start_pos+16);
        int contentLen = atoi(getHeaders.substr(start_pos+16, end_pos-(start_pos+16)).c_str());
        contentLen -= getResponse.length();
        while (contentLen > 0) {
            readLen = sock.Recv(buffer, (contentLen>1024?1024:contentLen));
            if (readLen <= 0) {
                sock.Close();
                return "";
            }
            getResponse.append(buffer,readLen);
            contentLen -= readLen;
        }
    } else if (Utils::StringUtils::findIgnoreCase(getHeaders,"Connection: close")!=std::string::npos) {
        for (;;) {
            readLen = sock.Recv(buffer, 1024);
            if (readLen <= 0) {
                sock.Close();
                break;
            }
            getResponse.append(buffer,readLen);
        }
    } else getResponse = "";

    sock.Close();
    return getResponse;

    httpGetRequestError:
    sock.Close();
    return "";
}

}}
