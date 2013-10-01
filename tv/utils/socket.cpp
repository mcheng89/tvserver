#include "socket.h"
#include <windows.h>

namespace TV {
namespace Utils {

Socket::Socket() {
    sock = 0;
}

bool Socket::Connect(std::string hostname, int port) {
    Close();

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        Close();
        return false;
    }
    struct sockaddr_in server_address;
    memset((char *) &server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    struct hostent *host = gethostbyname( hostname.c_str() );
    server_address.sin_addr.s_addr = ((struct in_addr *)(host->h_addr))->s_addr;

    int sendbuff = 96*1024;
    size_t optlen = sizeof(sendbuff);
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&sendbuff, optlen);

    if (SOCKET_ERROR == connect(sock, (struct sockaddr *)&server_address, sizeof(server_address))) {
        Close();
        return false;
    }
    return true;
}

void Socket::Close() {
    if (sock) closesocket(sock);
    sock = 0;
}

int Socket::Send(const char *data, int len) {
    return send(sock, data, len, 0);
}

int Socket::Recv(char *data, int len) {
    return recv(sock, data, len, 0);
}

}}

