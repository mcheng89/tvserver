#ifndef TV_UTILS_PROCESS_H
#define TV_UTILS_PROCESS_H

#include <string>
#include <windows.h>

namespace TV {
namespace Utils {

    class Process {
    public:
        Process();
        ~Process();
        bool execute(std::string process, std::string args, bool handles=false);
        bool isRunning(bool shutdown=true);
        std::string readOutput(int read_len=-1);
        void close();
    private:
        PROCESS_INFORMATION process_pi;
        HANDLE g_hChildStd_IN_Rd;
        HANDLE g_hChildStd_IN_Wr;
        HANDLE g_hChildStd_OUT_Rd;
        HANDLE g_hChildStd_OUT_Wr;

        Process(const Process&);     // do not give these a body
        Process& operator = (const Process&);
    };

}}

#endif // TV_UTILS_PROCESS_H
