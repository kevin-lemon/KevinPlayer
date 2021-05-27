//
// Created by Derry on 2021/5/18.
//

#ifndef DERRYPLAYER_VIDEOCHANNEL_H
#define DERRYPLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"
extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

typedef void(* RenderCallback) (uint8_t *,int,int,int);

class VideoChannel : public BaseChannel{
private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;
public:
    VideoChannel(int stream_index,AVCodecContext *codecContext);
    ~VideoChannel();

    void start();
    void stop();

    void video_decode();

    void video_play();

    void setRenderCallback(RenderCallback renderCallback);
};


#endif //DERRYPLAYER_VIDEOCHANNEL_H
