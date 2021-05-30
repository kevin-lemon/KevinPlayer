//
// Created by wxk on 2021/5/18.
//

#ifndef CSTUDY_MEDIAPLAYER_H
#define CSTUDY_MEDIAPLAYER_H
#include <cstring>
#include <pthread.h>
#include "JNICallbackHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "util.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

class MediaPlayer {
private:
    char *data_source = 0; // 指针 请赋初始值
    pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *formatContext = 0;
    AudioChannel *audio_channel = 0;
    VideoChannel *video_channel = 0;
    JNICallbackHelper * jniCallbackHelper;
    RenderCallback renderCallback;
    bool isPlaying;
public:
    MediaPlayer(const char *data_source,JNICallbackHelper * jniCallbackHelper);
    ~MediaPlayer();
    void prepare();
    void prepare_();
    void start_();
    void start();

    void setRenderCallback(RenderCallback renderCallback);
};


#endif //CSTUDY_MEDIAPLAYER_H
