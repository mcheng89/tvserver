#include "tv/tv.h"
#include <xercesc/util/PlatformUtils.hpp>

TV::Server *server;

void open_status_page() {
    Log::Info() << "Server: Ready event fired. Starting status page." << std::endl;
    char url[33];
    snprintf(url, 33, "http://127.0.0.1:%i/m/status/", server->getSettings()->getServerPort());
    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}
void tvserver_onReady() {
    open_status_page();
}

int main()
{
    WORD wVersionRequested = MAKEWORD(2,2);
    WSADATA wsaData;
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
        return -1;

    xercesc::XMLPlatformUtils::Initialize();

    TV::Server tv_server(tvserver_onReady);
    server = &tv_server;

    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, "tv::server1.0");
    if (!hMutex)
        hMutex = CreateMutex(0, 0, "tv::server1.0");
    else {
        open_status_page();
        return 0;
    }

    tv_server.run();

    ReleaseMutex(hMutex);

    xercesc::XMLPlatformUtils::Terminate();
    return 0;
}
