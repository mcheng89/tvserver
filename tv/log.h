#ifndef TV_LOG_H
#define TV_LOG_H

#include "threads/mutex.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>

typedef std::ostream& (*oendl)(std::ostream&);

class Log {
public:
    static Log *getInstance() {
        static Log instance("logs/logs-tv_server.log");
        return &instance;
    }
    static Log& Info() {
        getInstance()->mutex.lock();
        return *getInstance();
    }
    template <typename T> Log& operator<<(T a) {
        oss << a;
        return *this;
    }

    Log& operator<<( std::ostream&(*f)(std::ostream&) )
    {
        if( f == static_cast<oendl>(std::endl) )
        {
            //cout << current timestamp?
            time_t rawtime;
            time ( &rawtime );
            struct tm * timeinfo = localtime ( &rawtime );

            log_file << "[" <<
                (timeinfo->tm_mon+1) << "." << timeinfo->tm_mday << "." << (timeinfo->tm_year+1900) << "."
                << timeinfo->tm_hour << ":" << (timeinfo->tm_min<10?"0":"") << timeinfo->tm_min << ":" << (timeinfo->tm_sec<10?"0":"") << timeinfo->tm_sec
                << "]: " << oss.str() << std::endl;
            std::cout << "[" <<
                (timeinfo->tm_mon+1) << "." << timeinfo->tm_mday << "." << (timeinfo->tm_year+1900) << "."
                << timeinfo->tm_hour << ":" << (timeinfo->tm_min<10?"0":"") << timeinfo->tm_min << ":" << (timeinfo->tm_sec<10?"0":"") << timeinfo->tm_sec
                << "]: " << oss.str() << std::endl;
            oss.str("");
            log_file.flush();
            getInstance()->mutex.unlock();
        }
        return *this;
    }

private:
    Log(std::string file_name) {
        log_file.open(file_name.c_str());
    }
    Log(Log const&);
    void operator=(Log const&);
    std::ofstream log_file;
    std::ostringstream oss;
    TV::Threads::Mutex mutex;
};

#endif // TV_LOG_H
