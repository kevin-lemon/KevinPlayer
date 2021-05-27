#include <jni.h>
#include <string>
#include <android/log.h>
#include "MediaPlayer.h"
#include <android/native_window_jni.h>
#define TAG "JNI_LOG"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型

JavaVM* vm = nullptr;
MediaPlayer * player = nullptr;
ANativeWindow  * window = nullptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
jint JNI_OnLoad(JavaVM * vm,void * args){
    ::vm = vm;
    return JNI_VERSION_1_6;
}
void renderFrame(uint8_t * src_data,int width,int height,int src_line_size){
    pthread_mutex_lock(&mutex);
    if (!window){
        pthread_mutex_unlock(&mutex);//为了防止出死锁，释放
    }
    //设置窗口的大小
    ANativeWindow_setBuffersGeometry(window,width,height,WINDOW_FORMAT_RGBA_8888);
    //缓冲区buffer
    ANativeWindow_Buffer windowBuffer;
    if (ANativeWindow_lock(window,&windowBuffer, nullptr)){
        ANativeWindow_release(window);
        window = nullptr;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //真正渲染RGBA数据进行字节对齐渲染
    uint8_t * dst_data = static_cast<uint8_t *>(windowBuffer.bits);
    int dst_line_size = windowBuffer.stride * 4;
    for (int i = 0; i <windowBuffer.height; ++i) {//显示实际是一行一行显示,遍历高度
        //分辨率宽高426*240
        //426*4(rgba8888) = 1704
        //memcpy(dst_data+i*1704,src_data+i * 1704,1704);会花屏
        //ANativeWindow_Buffer 16字节对齐 1704/64有余数，无法以64位字节对齐
        //要有空16字节，不然会有问题。
        //memcpy(dst_data+i*1792,src_data+i * 1704,1792);
        memcpy(dst_data+i*dst_line_size,src_data+i * src_line_size,dst_line_size);
        ANativeWindow_unlockAndPost(window);//解锁后并且POST数据到surface
    }
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_kevin_player_media_MediaPlayer_getFFmpegVersion(JNIEnv *env, jobject thiz) {
        std::string info = "FFmpeg的版本号是:";
        info.append(av_version_info());
        return env->NewStringUTF(info.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_player_media_MediaPlayer_nativePrepare(JNIEnv *env, jobject thiz,
                                                      jstring data_source) {
    const char *data_source_ = env->GetStringUTFChars(data_source,0);
    auto * jniCallbackHelper =new JNICallbackHelper(vm,env,thiz);
    auto *player = new MediaPlayer(data_source_,jniCallbackHelper);
    player->setRenderCallback(renderFrame);
    player->prepare();
    env->ReleaseStringUTFChars(data_source,data_source_);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_player_media_MediaPlayer_nativeStart(JNIEnv *env, jobject thiz) {
    if(player){
        player->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_player_media_MediaPlayer_nativeRelease(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeRelease()
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_player_media_MediaPlayer_nativeOnPrepared(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeOnPrepared()
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_player_media_MediaPlayer_nativeStop(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeStop()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_player_media_MediaPlayer_nativeSetSurface(JNIEnv *env, jobject thiz,
                                                         jobject surface) {
    pthread_mutex_lock(&mutex);
    //先释放之前现实的窗口
    if (window){
        ANativeWindow_release(window);
        window = nullptr;
    }
    window = ANativeWindow_fromSurface(env,surface);
    pthread_mutex_unlock(&mutex);
}