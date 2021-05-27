#ifndef DERRYPLAYER_JNICALLBAKCHELPER_H
#define DERRYPLAYER_JNICALLBAKCHELPER_H

#include <jni.h>
#include "util.h"

class JNICallbackHelper {

private:

    JavaVM *vm = 0;
    JNIEnv *env = 0;
    jobject job;
    jmethodID jmd_prepared;
    jmethodID jmd_error;
public:
    JNICallbackHelper(JavaVM *vm, JNIEnv *env, jobject job);

    virtual ~JNICallbackHelper();

    void onPrepared(int thread_mode);

    void onError(int thread_mode, int error_code);
};


#endif //DERRYPLAYER_JNICALLBAKCHELPER_H
