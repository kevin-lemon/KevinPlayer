//
// Created by Derry on 2021/5/18.
//

#ifndef DERRYPLAYER_AUDIOCHANNEL_H
#define DERRYPLAYER_AUDIOCHANNEL_H


#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
extern "C"{
    #include <libswresample/swresample.h>//对音频数据进行转换（重采样）
}

class AudioChannel: public BaseChannel {
private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;
    SLObjectItf engineObject = 0;
    SLEngineItf engineInterface = 0;
    SLPlayItf bqPlayerPlay = 0;
    SLObjectItf outputMixObject = 0;
    SLObjectItf bqPlayerObject = 0;
    SLAndroidSimpleBufferQueueItf bpPlayerBufferQueue;
public:
    int out_channels;
    int out_sample_size;
    int out_sample_rate;
    int out_buffers_size;
    uint8_t *out_buffers;
    SwrContext *swr_ctx;
    double audio_time = 0;
    AudioChannel(int stream_index,AVCodecContext *codecContext,AVRational time_rational);
    ~AudioChannel();
    void start();
    void stop();
    void audio_decode();
    void audio_play();
    int getPCM();
};


#endif //DERRYPLAYER_AUDIOCHANNEL_H
