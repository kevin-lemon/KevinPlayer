//
// Created by Derry on 2021/5/18.
//

#include "VideoChannel.h"

VideoChannel::VideoChannel(int stream_index,AVCodecContext *codecContext)
        :BaseChannel(stream_index,codecContext){

}

VideoChannel::~VideoChannel() {}

void *task_video_decode(void *args) {
    auto *video_channel = static_cast<VideoChannel *>(args);
    video_channel->video_decode();
    return 0;
}


void VideoChannel::video_decode() {
    AVPacket * pkt = 0;
    while (isPlaying){
        if (isPlaying && frames.size() > 100) {
            av_usleep(10 * 1000); // 单位 ：microseconds 微妙 10毫秒
            continue;
        }
        int r = packets.getQueueAndDel(pkt);
        if (!isPlaying){
            break;
        }
        if (!r){
            continue;
        }
        r = avcodec_send_packet(codecContext,pkt);
        releaseAVPacket(&pkt);
        if (r){
            break;
        }
        AVFrame *frame = av_frame_alloc();
        r = avcodec_receive_frame(codecContext,frame);
        if (r == AVERROR(EAGAIN)){
            continue;
        }else if(r != 0){
            if (frame) {
                releaseAVFrame(&frame);
            }
            break;
        }
        frames.insertToQueue(frame);
        av_packet_unref(pkt); // 减1 = 0 释放成员指向的堆区
        releaseAVPacket(&pkt); // 释放AVPacket * 本身的堆区空间
    }
    av_packet_unref(pkt); // 减1 = 0 释放成员指向的堆区
    releaseAVPacket(&pkt); // 释放AVPacket * 本身的堆区空间
}

void *task_video_play(void *args) {
    auto *video_channel = static_cast<VideoChannel *>(args);
    video_channel->video_play();
    return 0;
}

void VideoChannel::video_play() {
    AVFrame *frame = 0;
    uint8_t *dst_data[4]; // RGBA
    int dst_linesize[4]; // RGBA
    //给 dst_data 申请内存   width * height * 4 xxxx
    av_image_alloc(dst_data,dst_linesize,codecContext->width,codecContext->height,AV_PIX_FMT_RGBA,1);
    SwsContext *sws_ctx = sws_getContext(
            // 下面是输入环节
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt, // 自动获取 xxx.mp4 的像素格式  AV_PIX_FMT_YUV420P // 写死的

            // 下面是输出环节
            codecContext->width,
            codecContext->height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR, NULL, NULL, NULL);
    while (isPlaying) {
        int ret = frames.getQueueAndDel(frame);
        if (!isPlaying) {
            break; // 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }
        if (!ret) { // ret == 0
            continue; // 哪怕是没有成功，也要继续（假设：你生产太慢(原始包加入队列)，我消费就等一下你）
        }

        // 格式转换 yuv ---> rgba
        sws_scale(sws_ctx,
                // 下面是输入环节 YUV的数据
                  frame->data, frame->linesize,
                  0, codecContext->height,

                // 下面是输出环节  成果：RGBA数据
                  dst_data,
                  dst_linesize
        );
        // SurfaceView ----- ANatvieWindows
        //数组被传递退化成指针
        renderCallback(dst_data[0],
                       codecContext->width,
                       codecContext->height,
                       dst_linesize[0]);
        //渲染完成释放
        av_frame_unref(frame); // 减1 = 0 释放成员指向的堆区
        releaseAVFrame(&frame); // 释放AVFrame * 本身的堆区空间
    }
    av_frame_unref(frame); // 减1 = 0 释放成员指向的堆区
    releaseAVFrame(&frame); // 释放AVFrame * 本身的堆区空间
    isPlaying = false;
    av_free(&dst_data[0]);
    sws_freeContext(sws_ctx); // free(sws_ctx); FFmpeg必须使用人家的函数释放，直接崩溃

}

void VideoChannel::start() {
    isPlaying = true;
    packets.setWork(1);
    frames.setWork(1);
    // 第一个线程： 视频：取出队列的压缩包 进行编码 编码后的原始包 再push队列中去
    pthread_create(&pid_video_decode, 0, task_video_decode, this);

    // 第二线线程：视频：从队列取出原始包，播放
    pthread_create(&pid_video_play, 0, task_video_play, this);
}

void VideoChannel::stop(){

}

void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

