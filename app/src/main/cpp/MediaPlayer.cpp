#include "MediaPlayer.h"


MediaPlayer::MediaPlayer(const char *data_source, JNICallbackHelper *jniCallbackHelper) {
    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source, data_source); // 把源 Copy给成员
    this->jniCallbackHelper = jniCallbackHelper;
}

MediaPlayer::~MediaPlayer() {
    if (data_source) {
        delete data_source;
        data_source = nullptr;
    }

    if (jniCallbackHelper) {
        delete jniCallbackHelper;
        jniCallbackHelper = nullptr;
    }
}

void *task_prepare(void *args) {
    auto *player = static_cast<MediaPlayer *>(args);
    player->prepare_();
    return 0;// 必须返回，坑，错误很难找
}

void MediaPlayer::prepare_() {

    formatContext = avformat_alloc_context();

    AVDictionary *dictionary = 0;
    av_dict_set(&dictionary, "timeout", "5000000", 0);
    /**
     * 1，AVFormatContext *
     * 2，路径
     * 3，AVInputFormat *fmt  Mac、Windows 摄像头、麦克风， 我们目前安卓用不到
     * 4，各种设置：例如：Http 连接超时， 打开rtmp的超时  AVDictionary **options
     */
    int r = avformat_open_input(&formatContext, data_source, 0, &dictionary);

    av_dict_free(&dictionary);
    if (r) {
        if (jniCallbackHelper){
            jniCallbackHelper->onError(THREAD_CHILD,1);
        }
        return;
    }
    /**
    * TODO 第二步：查找媒体中的音视频流的信息
    */
    r = avformat_find_stream_info(formatContext, 0);
    if (r < 0) {
        // TODO 第一节课作业：JNI 反射回调到Java方法，并提示
        if (jniCallbackHelper){
            jniCallbackHelper->onError(THREAD_CHILD,2);
        }
        return;
    }
    /**
   * TODO 第三步：根据流信息，流的个数，用循环来找
   */

    for (int i = 0; i < formatContext->nb_streams; ++i) {
        /**
         * TODO 第四步：获取媒体流（视频，音频）
         */
        AVStream *stream = formatContext->streams[i];

        /**
         * TODO 第五步：从上面的流中 获取 编码解码的【参数】
         * 由于：后面的编码器 解码器 都需要参数（宽高 等等）
         */
        AVCodecParameters *parameters = stream->codecpar;

        /**
         * TODO 第六步：（根据上面的【参数】）获取编解码器
         */
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
        if (!codec){
            if (jniCallbackHelper){
                jniCallbackHelper->onError(THREAD_CHILD,3);
            }
            return;
        }
        /**
        * TODO 第七步：编解码器 上下文 （这个才是真正干活的）
        */
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            // TODO 第一节课作业：JNI 反射回调到Java方法，并提示
            if (jniCallbackHelper){
                jniCallbackHelper->onError(THREAD_CHILD,4);
            }
            return;
        }

        /**
         * TODO 第八步：他目前是一张白纸（parameters copy codecContext）
         */
        r = avcodec_parameters_to_context(codecContext, parameters);
        if (r < 0) {
            // TODO 第一节课作业：JNI 反射回调到Java方法，并提示
            if (jniCallbackHelper){
                jniCallbackHelper->onError(THREAD_CHILD,5);
            }
            return;
        }

        /**
         * TODO 第九步：打开解码器
         */
        r = avcodec_open2(codecContext, codec, 0);
        if (r) { // 非0就是true
            // TODO 第一节课作业：JNI 反射回调到Java方法，并提示
            if (jniCallbackHelper){
                jniCallbackHelper->onError(THREAD_CHILD,6);
            }
            return;
        }

        AVRational time = stream->time_base;
        /**
         * TODO 第十步：从编解码器参数中，获取流的类型 codec_type  ===  音频 视频
         */
        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) { // 音频
            audio_channel = new AudioChannel(i,codecContext,time);
        } else if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) { // 视频
            AVRational  fps_rational = stream->avg_frame_rate;
            int fps = av_q2d(fps_rational);
            video_channel = new VideoChannel(i,codecContext,time,fps);
            video_channel->setRenderCallback(renderCallback);
        }
        /**
        * TODO 第十一步: 如果流中 没有音频 也没有 视频 【健壮性校验】
        */
        if (!audio_channel && !video_channel) {
            // TODO 第一节课作业：JNI 反射回调到Java方法，并提示
            if (jniCallbackHelper){
                jniCallbackHelper->onError(THREAD_CHILD,7);
            }
            return;
        }

        /**
         * TODO 第十二步：恭喜你，准备成功，我们的媒体文件 OK了，通知给上层
         */
        if (jniCallbackHelper) {
            jniCallbackHelper->onPrepared(THREAD_CHILD);
        }
    }
}

void MediaPlayer::prepare() {
    // 问题：当前的prepare函数，是子线程 还是 主线程 ？
    // 答：此函数是被MainActivity的onResume调用下来的（主线程）

    // 解封装 FFmpeg来解析  data_source 可以直接解析吗？
    // 答：data_source == 文件io流，  直播网络rtmp， 所以按道理来说，会耗时，所以必须使用子线程

    // 创建子线程
    pthread_create(&pid_prepare, 0, task_prepare, this);
}

void *task_start(void *args) {
    auto *player = static_cast<MediaPlayer *>(args);
    player->start_();
    return 0;// 必须返回，坑，错误很难找
}

void MediaPlayer::start_() {
    while (isPlaying){
        if (video_channel && video_channel->packets.size() > 100) {
            av_usleep(10 * 1000); // 单位 ：microseconds 微妙 10毫秒
            continue;
        }
        if (audio_channel && audio_channel->packets.size() > 100) {
            av_usleep(10 * 1000); // 单位 ：microseconds 微妙 10毫秒
            continue;
        }
        //AVPacket可能是音频也可能是视频，压缩包
        AVPacket * packet = av_packet_alloc();
        int r = av_read_frame(formatContext,packet);
        if (!r){
            if (video_channel && video_channel->stream_index == packet->stream_index){
                video_channel->packets.insertToQueue(packet);
            } else if (audio_channel && audio_channel->stream_index == packet->stream_index){
                audio_channel->packets.insertToQueue(packet);
            }
        } else if (r == AVERROR_EOF){//END OF FILE
            if (video_channel->packets.empty() && audio_channel->packets.empty()) {
                break; // 队列的数据被音频 视频 全部播放完毕了，我在退出
            }
        }else{
            break;
        }
    }
    isPlaying = false;
    video_channel->stop();
    audio_channel->stop();
}

void MediaPlayer::start() {
    isPlaying = true;
    if (video_channel){
        video_channel->setAudioChannel(audio_channel);
        video_channel->start();
    }
    if (audio_channel){
        audio_channel->start();
    }
    //压缩包加入队列
    pthread_create(&pid_start, 0, task_start, this);
}

void MediaPlayer::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}
