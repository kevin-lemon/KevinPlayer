#include "JNICallbackHelper.h"

JNICallbackHelper::JNICallbackHelper(JavaVM *vm, JNIEnv *env, jobject job) {
    this->vm = vm;
    this->env = env;
    // this->job = job; // 坑： jobject不能跨越线程，不能跨越函数，必须全局引用
    this->job = env->NewGlobalRef(job); // 提示全局引用

    jclass clazz = env->GetObjectClass(job);
    jmd_prepared = env->GetMethodID(clazz, "onPrepared", "()V");
    jmd_error = env->GetMethodID(clazz, "onError", "(I)V");
}

JNICallbackHelper::~JNICallbackHelper() {
    vm = 0;
    env->DeleteGlobalRef(job);
    job = 0;
    env = 0;
}

void JNICallbackHelper::onPrepared(int thread_mode) {
    if (thread_mode == THREAD_MAIN) {
        env->CallVoidMethod(job, jmd_prepared);
    } else if (thread_mode == THREAD_CHILD){
        JNIEnv * env_child;
        vm->AttachCurrentThread(&env_child,0);
        env_child->CallVoidMethod(job,jmd_prepared);
        vm->DetachCurrentThread();
    }
}

void JNICallbackHelper::onError(int thread_mode,int error_code) {
    if (thread_mode == THREAD_MAIN) {
        env->CallVoidMethod(job, jmd_error);
    } else if (thread_mode == THREAD_CHILD){
        JNIEnv * env_child;
        vm->AttachCurrentThread(&env_child,0);
        env_child->CallVoidMethod(job,jmd_error,error_code);
        vm->DetachCurrentThread();
    }
}
