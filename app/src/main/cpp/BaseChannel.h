//
// Created by wxk on 2021/5/20.
//
#include "safe_queue.h"
#include "log4c.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}
#ifndef KEVINPLAYER_BASECHANNEL_H
#define KEVINPLAYER_BASECHANNEL_H
class BaseChannel{
public:
    int stream_index;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    bool isPlaying;
    AVCodecContext *codecContext = 0;
    AVRational time_base;//同步时间基

    BaseChannel(int stream_index,AVCodecContext *codecContext,AVRational time_rational)
                :stream_index(stream_index),
                 codecContext(codecContext),
                 time_base(time_rational){
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }

    virtual ~BaseChannel(){
        packets.clear();
        frames.clear();
    }

    static void releaseAVPacket(AVPacket ** p){
        if (p){
            av_packet_free(p);
            *p = 0;
        }
    }

    static void releaseAVFrame(AVFrame ** f){
        if (f){
            av_frame_free(f);
            *f = 0;
        }
    }
};
#endif //KEVINPLAYER_BASECHANNEL_H
