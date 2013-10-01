#ifndef TV_TUNER_H
#define TV_TUNER_H

#include "tv.h"
#include "devices/tuner.h"
#include "threads/mutex.h"
#include "utils/socket.h"
#include "utils/process.h"

namespace TV {

    class Server;

    typedef struct _StreamSettings {
        int channel;
        std::string format;
        int video_resolution;
        int video_bitrate;
        int audio_bitrate;
        bool multicast;
        int duration_minutes;
        _StreamSettings():video_resolution(0),video_bitrate(0),audio_bitrate(0),format("ts"),multicast(false),duration_minutes(0){}
    } StreamSettings;

    class Tuner {
    public:
        Tuner(Server *parent, Devices::Tuner *tuner, int index);

        bool isStreaming(bool tuner_in_use=false);
        time_t getStreamShutdown();
        void setStreamShutdown(time_t next_time);

        bool startStream(StreamSettings stream_info);
        void stopStream(int stream_key=0);

        int getStreamLock();
        int getStreamKey() { return stream_key; }
        StreamSettings getStreamInfo() { return stream_info; }

        void lock() { mutex.lock(); }
        void unlock() { mutex.unlock(); }

        Devices::Tuner* getTuner() { return tuner; }
        //Utils::Process* getVLCProcess() { return &vlc_process; }
        int getStreamPort() { return 56000+index; }
        int getTunerIndex() { return index; }

    private:
        bool vlcSendCommand(int port, char *data, int len);
        bool vlcSendProgram(int port, int program);
        bool vlcIsPlaying(int port);

        Server *parent;
        Devices::Tuner *tuner;
        Threads::Mutex mutex;
        int index;

        time_t shutdown_time;
        Utils::Process vlc_process;
        Utils::Socket vlc_socket;

        int stream_key;
        StreamSettings stream_info;

        bool has_stream_lock;
        static Threads::Mutex random_lock;

        Tuner(const Tuner&);
        Tuner& operator = (const Tuner&);
    };

}

#endif // TV_TUNER_H
