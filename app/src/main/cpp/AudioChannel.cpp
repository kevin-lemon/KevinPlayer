//
// Created by Derry on 2021/5/18.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int stream_index,AVCodecContext *codecContext,AVRational time_rational)
:BaseChannel(stream_index,codecContext,time_rational){
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44111;
    out_buffers_size = out_channels * out_sample_size * out_sample_rate;
    out_buffers = static_cast<uint8_t *>(malloc(out_buffers_size));
    //FFmpeg 重采样
    swr_ctx = swr_alloc_set_opts(0,AV_CH_LAYOUT_STEREO,
    AV_SAMPLE_FMT_S16,out_sample_rate,codecContext->channel_layout,
    codecContext->sample_fmt,codecContext->sample_rate,0,0);
}
AudioChannel::~AudioChannel() {}


void *task_audio_decode(void *args) {
    auto *audio_channel = static_cast<AudioChannel *>(args);
    audio_channel->audio_decode();
}


void AudioChannel::audio_decode() {
    AVPacket * pkt = 0;
    while (isPlaying){
        if (isPlaying && frames.size() > 100) {
            av_usleep(10 * 1000); // 单位 ：microseconds 微妙 10毫秒
            continue;
        }
        int ret = packets.getQueueAndDel(pkt);
        if (!isPlaying){
            break;
        }
        if (!ret){
            continue;
        }
        ret = avcodec_send_packet(codecContext,pkt);
        releaseAVPacket(&pkt);
        if (ret){
            break;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext,frame);
        if (ret == AVERROR(EAGAIN)){
            continue;
        }else if(ret != 0){
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

void *task_audio_play(void *args) {
    auto *audio_channel = static_cast<AudioChannel *>(args);
    audio_channel->audio_play();
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq,void *args){
    auto *audio_channel = static_cast<AudioChannel * >(args);
    int pcm_size = audio_channel->getPCM();
    (*bq)->Enqueue(bq,
                   audio_channel->out_buffers,
                   pcm_size);
}

void AudioChannel::audio_play() {
    SLresult result;
    result = slCreateEngine(&engineObject,0,0,0,0,0);
    if (SL_RESULT_SUCCESS != result){
        return;
    }
    //FALSE 延时等待你创建成功
    result = (*engineObject)->Realize(engineObject,SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result){
        return;
    }
    result = (*engineObject)->GetInterface(engineObject,SL_IID_ENGINE,&engineInterface);
    if (SL_RESULT_SUCCESS != result){
        return;
    }
    if(!engineInterface) {
        return;
    }
    result = (*engineInterface)->CreateOutputMix(engineInterface,&outputMixObject,0,0,0);
    if (SL_RESULT_SUCCESS != result){
        return;
    }

    result = (*outputMixObject)->Realize(outputMixObject,SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result){
        return;
    }
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,10};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,2,SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_BACK_RIGHT,
                                   SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq,&format_pcm};
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX,outputMixObject};
    SLDataSink audioSnk = {&loc_outmix,NULL};
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] {SL_BOOLEAN_TRUE};
    result = (*engineInterface)->CreateAudioPlayer(
            engineInterface,&bqPlayerObject,
            &audioSrc,&audioSnk,1,ids,req
            );
    if (SL_RESULT_SUCCESS != result){
        return;
    }

    result = (*bqPlayerObject)->Realize(bqPlayerObject,SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result){
        return;
    }

    result = (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_PLAY,&bqPlayerPlay);
    if (SL_RESULT_SUCCESS != result){
        return;
    }
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject,SL_IID_BUFFERQUEUE,&bpPlayerBufferQueue);
    if (SL_RESULT_SUCCESS != result){
        return;
    }
    (*bpPlayerBufferQueue)->RegisterCallback(bpPlayerBufferQueue,  // 传入刚刚设置好的队列
                                             bqPlayerCallback,  // 回调函数
                                             this); // 给回调函数的参数

    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    bqPlayerCallback(bpPlayerBufferQueue, this);
}


void AudioChannel::start() {
    isPlaying = true;
    packets.setWork(1);
    frames.setWork(1);
    pthread_create(&pid_audio_decode,0,task_audio_decode, this);
    pthread_create(&pid_audio_play,0,task_audio_play, this);
}

void AudioChannel::stop(){

}

int AudioChannel::getPCM() {
    int pcm_data_size = 0;
    AVFrame  *frame = 0;
    while (isPlaying){
        int ret = frames.getQueueAndDel(frame);
        if (!isPlaying){
            break;
        }
        if (!ret){
            continue;
        }
        //来源10个48000----->目标44100 11个44100
        int dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx,frame->sample_rate),
                out_sample_rate,
                frame->sample_rate,
                AV_ROUND_UP
                );
        int samples_per_channel = swr_convert(swr_ctx,
                                              &out_buffers,
                                              dst_nb_samples,
                                              (const uint8_t **) frame->data,
                                              frame->nb_samples);
        pcm_data_size = samples_per_channel * out_sample_size * out_channels;

        //开始音视频同步，音频负责给时间

        audio_time = frame->best_effort_timestamp * av_q2d(time_base);
        break;
    }
    av_frame_unref(frame); // 减1 = 0 释放成员指向的堆区
    releaseAVFrame(&frame); // 释放AVFrame * 本身的堆区空间
    return pcm_data_size;
}
