#include "process.h"
//#include <iostream>
#include <stdio.h>

namespace TV {
namespace Utils {

Process::Process() {
    g_hChildStd_IN_Rd = 0;
    g_hChildStd_IN_Wr = 0;
    g_hChildStd_OUT_Rd = 0;
    g_hChildStd_OUT_Wr = 0;
    memset(&process_pi, 0, sizeof(PROCESS_INFORMATION));
}
Process::~Process() {
    close();
}

bool Process::execute(std::string process, std::string args, bool handles) {
    close();

    if (handles) {
        SECURITY_ATTRIBUTES sa;
        // Set the bInheritHandle flag so pipe handles are inherited.
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        // Create a pipe for the child process's STDOUT.
        if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0) )
            return false;
        // Ensure the read handle to the pipe for STDOUT is not inherited.
        if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) ) {
            close();
            return false;
        }
        // Create a pipe for the child process's STDIN.
        if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &sa, 0)) {
            close();
            return false;
        }
        // Ensure the write handle to the pipe for STDIN is not inherited.
        if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) ) {
            close();
            return false;
        }
    }

    STARTUPINFO si;
    memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    memset(&process_pi, 0, sizeof(PROCESS_INFORMATION));
    if (handles) {
        si.hStdError = 0;
        si.hStdOutput = g_hChildStd_OUT_Wr;
        si.hStdInput = g_hChildStd_IN_Rd;
        si.dwFlags |= STARTF_USESTDHANDLES;
    }
    if (!CreateProcess(NULL, (char*)(process+" "+args).c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &process_pi)) {
        close();
        return false;
    }

    // Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    CloseHandle(g_hChildStd_OUT_Wr);
    CloseHandle(g_hChildStd_IN_Rd);
    g_hChildStd_OUT_Wr = 0;
    g_hChildStd_IN_Rd = 0;
    return true;
}

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx
//http://support.microsoft.com/kb/190351
std::string Process::readOutput(int read_len) {
    if (g_hChildStd_IN_Wr) {
        CloseHandle(g_hChildStd_IN_Wr);
        g_hChildStd_IN_Wr = 0;
    }
    if (read_len==0 || read_len<-1) return "";

    char buffer[8192];
    DWORD dwRead;
    std::string output = "";
    for (;;) {
        bool success = ReadFile( g_hChildStd_OUT_Rd, buffer, read_len==-1?8192:read_len>8192?8192:read_len, &dwRead, NULL);
        if( ! success || dwRead == 0 ) break;
        output.append(buffer, dwRead);
        if (read_len != -1) {
            read_len -= dwRead;
            if (read_len==0) break;
        }
    }
    return output;
}

bool Process::isRunning(bool shutdown) {
    DWORD dwExitCode;
    if(GetExitCodeProcess(process_pi.hProcess,&dwExitCode))
        if(dwExitCode==STILL_ACTIVE)
            return true;//still running
        else {
            if (shutdown) close();
            return false;//not running anymore
        }
    //query failed, handle probably doesn't have the PROCESS_QUERY_INFORMATION access
    if (shutdown) close();
    return false;
}

void Process::close() {
    if (g_hChildStd_IN_Rd) CloseHandle(g_hChildStd_IN_Rd);
    if (g_hChildStd_IN_Wr) CloseHandle(g_hChildStd_IN_Wr);
    if (g_hChildStd_OUT_Rd) CloseHandle(g_hChildStd_OUT_Rd);
    if (g_hChildStd_OUT_Wr) CloseHandle(g_hChildStd_OUT_Wr);
    if (process_pi.hProcess != 0) {
        //if (isRunning(false)) {
            TerminateProcess(process_pi.hProcess, 0);
            //printf("GetLastError(): %i", GetLastError());
        //}
        CloseHandle(process_pi.hProcess);
        CloseHandle(process_pi.hThread);
    }

    g_hChildStd_IN_Rd = 0;
    g_hChildStd_IN_Wr = 0;
    g_hChildStd_OUT_Rd = 0;
    g_hChildStd_OUT_Wr = 0;
    //memset(&process_pi, 0, sizeof(PROCESS_INFORMATION));
    process_pi.hProcess = 0;
    process_pi.hThread = 0;
}

}}
