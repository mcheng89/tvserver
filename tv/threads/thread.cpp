#include "thread.h"

namespace TV {
namespace Threads {

static DWORD WINAPI Win32ThreadProc(LPVOID lpParam)
{
	Thread *thread = (Thread *)lpParam;
	thread->run();
	return 0;
}

Thread::Thread() {
    hThread = 0;
    threadID = 0;
}
Thread::~Thread() {}

bool Thread::start() {
    hThread = CreateThread(NULL, 0, Win32ThreadProc, (LPVOID)this, 0, &threadID);
    return hThread != 0;
}

void Thread::stop() {
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    hThread = 0;
}

}}
