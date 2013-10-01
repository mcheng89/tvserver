#include "rwlock.h"

namespace TV {
namespace Threads {

RWLock::RWLock() {
    readers_ = 0;
    writer_ = false;
}

void RWLock::readLock() {
    read_mutex.lock();  // make sure Reader and Writer can't interfere during locking
    write_mutex.lock(); // lock mutex so waitfor can unlock
    while (writer_)
        write_cv.wait(&write_mutex); // no active writers
    ++readers_; // at least 1 reader present
    write_mutex.unlock();
    read_mutex.unlock();
}

void RWLock::readUnlock() {
    read_mutex.lock();
    bool noReaders = (--readers_ == 0);
    read_mutex.unlock();
    if (noReaders) read_cv.signal(); // signal when no more readers
}

void RWLock::writeLock() {
    writers_lock.lock(); // only 1 writer allowed
    read_mutex.lock();   // lock mutex so waitfor can unlock and no interference by lockReader
    while (readers_ != 0)
        read_cv.wait(&read_mutex); // wait until no more readers
    write_mutex.lock();
    writer_ = true;      // a writer is busy
    write_mutex.unlock();
    read_mutex.unlock();
    // WritersLock is still locked
}

void RWLock::writeUnlock() {
    write_mutex.lock();
    writer_ = false;
    write_mutex.unlock();
    write_cv.signal(); // no more writer (until WritersLock is unlocked)
    writers_lock.unlock();
}

}}
