//
// Created by wxk on 2021/5/20.
//

#ifndef KEVINPLAYER_SAFE_QUEUE_H
#define KEVINPLAYER_SAFE_QUEUE_H
#include <queue>
#include <pthread.h>

using namespace std;
template<typename T>
class SafeQueue{
private:
    typedef void (*ReleaseCallback)(T *);
private:
    queue<T> queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int work;
    ReleaseCallback releaseCallback;
public:
    SafeQueue(){
        pthread_mutex_init(&mutex,0);
        pthread_cond_init(&cond,0);
    }
    ~SafeQueue(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    void insertToQueue(T value){
        pthread_mutex_lock(&mutex);
        if (work){
            queue.push(value);
            pthread_cond_signal(&cond);
        } else{
            if (releaseCallback){
                releaseCallback(&value);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    int getQueueAndDel(T &value){
        int ret = 0;
        pthread_mutex_lock(&mutex);
        while (work&&queue.empty()){
            pthread_cond_wait(&cond,&mutex);
        }
        if (!queue.empty()){
            value = queue.front();
            queue.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&mutex);
        return ret;
    }

    void setWork(int work){
        pthread_mutex_lock(&mutex);
        this->work = work;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    void clear(){
        unsigned int size = queue.size();
        for (int i = 0;i<size;i++){
            T value = queue.front();
            if (releaseCallback){
                releaseCallback(&value);
            }
            queue.pop();
        }
    }

    int size(){
        return queue.size();
    }

    bool empty(){
        return queue.empty();
    }

    void setReleaseCallback(ReleaseCallback callback){
        this->releaseCallback = callback;
    }

};
#endif //KEVINPLAYER_SAFE_QUEUE_H
