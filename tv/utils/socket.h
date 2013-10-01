#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#include <string>

namespace TV {
namespace Utils {

    class Socket {
    public:
        Socket();
        bool Connect(std::string hostname, int port);
        void Close();
        int Send(const char *data, int len);
        int Recv(char *data, int len);
    private:
        int sock;

        Socket(const Socket&);     // do not give these a body
        Socket& operator = (const Socket&);
    };

}}

#endif // SOCKET_H_INCLUDED
