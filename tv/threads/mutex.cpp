#include "mutex.h"

namespace TV {
namespace Threads {

Mutex::Mutex() {
    mutexID = CreateMutex(NULL, FALSE, NULL);
}

Mutex::~Mutex()
{
	CloseHandle(mutexID);
	mutexID = 0;
}

bool Mutex::lock()
{
    if (!mutexID) return false;
    WaitForSingleObject(mutexID, INFINITE);
	return true;
}

bool Mutex::unlock()
{
    if (!mutexID) return false;
	ReleaseMutex(mutexID);
	return true;
}

}}
