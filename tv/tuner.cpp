#include "tuner.h"
#include "log.h"
#include "utils/file.h"
#include "utils/stringutils.h"
#include <stdlib.h>

namespace TV {

Threads::Mutex Tuner::random_lock;

Tuner::Tuner(Server *parent, Devices::Tuner *tuner, int index) {
    this->parent = parent;
    this->tuner = tuner;
    shutdown_time = 0;
    has_stream_lock = false;
    this->index = index;
}

bool Tuner::isStreaming(bool tuner_in_use) {
    return vlc_process.isRunning() || (tuner_in_use && tuner->isPlaying());
}

time_t Tuner::getStreamShutdown() {
    return shutdown_time;
}
void Tuner::setStreamShutdown(time_t next_time) {
    shutdown_time = time(0)+next_time;
}

bool Tuner::startStream(StreamSettings stream_info) {
    if (stream_info.format!="ts" && (stream_info.video_bitrate==0 || stream_info.video_resolution==0)) {
        Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: Invalid stream parameters" << std::endl;
        return false;
    }

    mutex.lock();
    bool ret = false;

    Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: Starting video stream" << std::endl;
    this->stream_info = stream_info;
    //set has_stream_lock to false, so they can get the lock again!
    //if multicast, dont let them get lock, so we set it to true
    has_stream_lock = stream_info.multicast;

    random_lock.lock();
    srand(time(0));
    stream_key = (rand()%9000+1000)*10000+rand()%10000;
    random_lock.unlock();

    int cntrl_port = parent->getSettings()->getServerPort()+1+index;
    std::string tv_remote_config = "--no-crashdump --remote-log-dir logs --remote-port "+Utils::StringUtils::toString(cntrl_port)+" --intf tv_remote ";
    std::string stream_config = " :network-caching=25 :sout-deinterlace-mode=x :http-continuous :sout=#";
    std::string transcode_params = "";
    if (stream_info.video_resolution!=0) {
        transcode_params = "transcode{"
                "vcodec=h264,vb="+Utils::StringUtils::toString(stream_info.video_bitrate)+","+
                //"height="+Utils::StringUtils::toString(stream_info.video_resolution)+"fps=29.97,deinterlace,"+
                "height="+Utils::StringUtils::toString(stream_info.video_resolution)+",deinterlace,"+
                "acodec=mp3,ab="+Utils::StringUtils::toString(stream_info.audio_bitrate)+",channels=2,samplerate=44100,"+
                //"venc=x264{preset=faster,aud,profile=baseline,level=31,keyint=30,bframes=0,ref=1}"+
                "venc=x264{aud,profile=baseline,level=31,keyint=30,ref=1,"+
                    "vbv-maxrate="+Utils::StringUtils::toString(stream_info.video_bitrate*1.2)+","+
                    "vbv-bufsize="+Utils::StringUtils::toString(stream_info.video_bitrate*2.4)+
                "}"+
            "}:";
    }

    std::string stream_port = Utils::StringUtils::toString(getStreamPort());
    //std::string output_stream = stream_info.format=="ts"?"std{access=file,mux=ts,dst=-}":
    std::string output_stream = stream_info.format=="ts"?"std{access=http{mime=video/MP2T},mux=ts,dst=0.0.0.0:"+stream_port+"}":
            //stream_info.format=="flv"?"std{access=file,mux=ffmpeg{mux=flv},dst=-}":
            stream_info.format=="flv"?"std{access=http{mime=video/x-flv},mux=ffmpeg{mux=flv},dst=0.0.0.0:"+stream_port+"}":
				"std{access=livehttp{seglen=6,delsegs=true,numsegs=15,"
                "index=tmp/tuner"+Utils::StringUtils::toString(index)+".m3u8,"+
                "index-url=tuner"+Utils::StringUtils::toString(index)+"_##.ts},"+
                "mux=ts{use-key-frames},dst=tmp/tuner"+Utils::StringUtils::toString(index)+"_##.ts}";

    std::string stream_url = tuner->playChannel(stream_info.channel, index);
    if (stream_info.video_resolution!=0)
        Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: Setting VLC transcode parameters - " << transcode_params << std::endl;
    Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: Setting VLC stream format - " << stream_info.format << std::endl;
    //Log::Info() << stream_url << std::endl;
    if (stream_url != "") {
        for (int i=0; i<5 && !ret; i++) {
            vlc_process.close();
            if (vlc_process.execute(parent->getSettings()->getVLCProcessPath(), tv_remote_config+
                stream_url+stream_config+transcode_params+output_stream, true) )
            {
                if (tuner->setProgramId()) {
                    Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: VLC started. Setting up program id filter for stream. [program=" << tuner->getProgramNumber() << "]" << std::endl;
                    ret = vlcSendProgram(cntrl_port, tuner->getProgramNumber());
                } else {
                    Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: VLC started. Make sure it's really streaming..." << std::endl;
                    ret = vlcIsPlaying(cntrl_port);
                }
            }
        }
    }

    mutex.unlock();

    if (!ret) {
        stopStream();
        Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: Failed to start video stream" << std::endl;
    } else {
        setStreamShutdown(stream_info.multicast?stream_info.duration_minutes*60:TUNER_SHUTDOWN_TIME);
        parent->scheduler.wakeTuner(getStreamShutdown());
        Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: Stream started successfully!" << std::endl;
    }
    return ret;
}
void Tuner::stopStream(int stream_key) {
    mutex.lock();
    if (stream_key!=0 && stream_key!=this->stream_key)
        return;
    if (vlc_process.isRunning()) {
        Log::Info() << "Tuner[" << tuner->getDevice()->getIPAddress() << "][" << tuner->getTunerIndex()+1 << "]: Stopping video stream" << std::endl;
        vlc_process.close();
        if (stream_info.format == "hls") {
            std::string m3u8 = "tuner"+Utils::StringUtils::toString(index)+".m3u8";
            std::string sgmt = "tuner"+Utils::StringUtils::toString(index)+"_";
            std::vector<std::string> hlsFiles = Utils::File::ls("tmp");
            for (int i=0; i<hlsFiles.size(); i++) {
                std::string filename = hlsFiles.at(i);
                if (filename.find(sgmt)!=std::string::npos || filename.find(m3u8)!=std::string::npos)
                    remove(("tmp/"+filename).c_str());
            }
        }
    }
    if (tuner->isPlaying())
        tuner->stopChannel();
    mutex.unlock();
}

int Tuner::getStreamLock() {
    int stream_key = 0;
    mutex.lock();
    if (isStreaming() && !has_stream_lock && (stream_info.format=="ts" || stream_info.format=="flv")) {
        has_stream_lock = true;
        stream_key = this->stream_key;
    }
    mutex.unlock();
    return stream_key;
}

//==================================================
//BINARY FUNCTIONS
//==================================================
void intToBytes(int value, char * dest) {
    dest[3] =  value & 0x000000ff;
    dest[2] = (value & 0x0000ff00) >> 8;
    dest[1] = (value & 0x00ff0000) >> 16;
    dest[0] = (value & 0xff000000) >> 24;
}
int bytesToInt(char * src) {
    return (((unsigned char)src[0]<<24)|((unsigned char)src[1]<<16)|((unsigned char)src[2]<<8)|((unsigned char)src[3]));
}
bool Tuner::vlcSendCommand(int port, char *data, int len) {
    if (!vlc_socket.Connect("127.0.0.1",port)) {
        Log::Info() << "VLC: Failed to connect to vlc" << std::endl;
        return false;
    }

    bool success = false;
    char recv_data[3];
    for (int i=0; i<20 && !success; i++) { //75*20 = 1.5s
        if (vlc_socket.Send(data, len) <= 0) {
            Log::Info() << "VLC: Failed to send command to vlc" << std::endl;
            vlc_socket.Close();
            return false;
        }
        if (vlc_socket.Recv(recv_data, 3) <= 0) {
            Log::Info() << "VLC: Failed to receive response from vlc" << std::endl;
            vlc_socket.Close();
            return false;
        }
        if (recv_data[1] == 0x01)
            success = true;
        else Sleep(75);
    }
    vlc_socket.Close();
    return success;
}
bool Tuner::vlcSendProgram(int port, int program) {
    char setProgramCmd[7];
    setProgramCmd[0] = setProgramCmd[1] = setProgramCmd[2] = 0x01;
    intToBytes(program, setProgramCmd+3);
    return vlcSendCommand(port, setProgramCmd, 7);
}
bool Tuner::vlcIsPlaying(int port) {
    char isPlayingCmd[3];
    isPlayingCmd[0] = isPlayingCmd[1] = 0x01;
    isPlayingCmd[2] = 0x00;
    return vlcSendCommand(port, isPlayingCmd, 3);
}


}

