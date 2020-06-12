#ifndef DNRECORDER_SAFE_QUEUE_H
#define DNRECORDER_SAFE_QUEUE_H

#include <queue>
#include <pthread.h>

using namespace std;

/*
    SafeQueue 是一个模板类
        在该模板类中 , 将队列的操作进行了一系列线程安全封装
        所有的对类的修改 , 都在线程安全的条件下执行的
 */
template<typename T>
class SafeQueue {

    /**
     * 定义 线程安全队列 元素释放时回调的函数类型
     */
    typedef void (*ReleaseHandle)(T &);

    /**
     * 定义 线程安全队列 同步方法 函数类型 , 该方法用于丢弃一部分数据包
     */
    typedef void (*SyncHandle)(queue<T> &);

public:
    /**
     * 构造方法
     *
     * 初始线程互斥锁
     * 初始化条件变量
     */
    SafeQueue() {

        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);

    }

    /**
     * 析构方法
     *
     * 销毁线程互斥锁
     * 销毁线程条件变量
     */
    ~SafeQueue() {
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
    }

    /**
     * 向线程安全队列中添加元素
     * @param new_value
     */
    void push(T new_value) {

        pthread_mutex_lock(&mutex);
        if (work) {
            q.push(new_value);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
        }else{
            //如果没有加入到队列中 , 那么释放该值
            releaseHandle(new_value);
        }

        pthread_mutex_unlock(&mutex);

    }


    /**
     * 从线程安全队列中取出元素
     *
     *      该方法中需要修改 元素指针指向 , 这里的参数是引用参数
     *
     * @param value
     *          取出成功返回值为 1 , 失败返回值为 0
     * @return
     */
    int pop(T &value) {

        //用于记录是否成功取出数据 , 有特殊情况 , 线程没有阻塞住 , 取出了 NULL 数据
        //  如果成功取出数据 返回值设置为 , 反之为 0
        int ret = 0;

        //加互斥锁
        pthread_mutex_lock(&mutex);

        //工作的标志为 1 , 并且当前队列为空 , 开始等待
        //  如果没有数据 , 那么线程阻塞等待
        while (work && q.empty()) {
            //线程阻塞等待新的数据被放入后 , 解除阻塞
            pthread_cond_wait(&cond, &mutex);
        }

        //正常情况下 , 如果线程不为空 , 取出数据
        if (!q.empty()) {
            value = q.front();
            q.pop();
            ret = 1;
        }

        //解开互斥锁
        pthread_mutex_unlock(&mutex);

        return ret;
    }

    /**
     * 设置当前是否工作
     * @param work
     */
    void setWork(int work) {

        pthread_mutex_lock(&mutex);

        this->work = work;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);

    }

    int empty() {
        return q.empty();
    }

    int size() {
        return q.size();
    }

    void clear() {

        pthread_mutex_lock(&mutex);
        int size = q.size();
        for (int i = 0; i < size; ++i) {
            T value = q.front();
            releaseHandle(value);
            q.pop();
        }
        pthread_mutex_unlock(&mutex);

    }

    /**
     * 线程安全的前提下 , 让队列使用者使用队列使用该队列
     *      调用该方法 , 能够保证操作 queue 队列时是在同步块中执行的
     */
    void sync() {

        pthread_mutex_lock(&mutex);
        syncHandle(q);
        pthread_mutex_unlock(&mutex);

    }

    void setReleaseHandle(ReleaseHandle r) {
        releaseHandle = r;
    }

    void setSyncHandle(SyncHandle s) {
        syncHandle = s;
    }

private:


    pthread_cond_t cond;
    pthread_mutex_t mutex;

    queue<T> q;

    /**
     * 工作标记
     *      该标记设置为 1 , 才开始工作
     *      如果设置为 0 , 不工作
     */
    int work;

    /**
     * 函数指针类型
     *      typedef void (*ReleaseHandle)(T &)
     * 在元素释放时回调该函数
     */
    ReleaseHandle releaseHandle;

    /**
     * 函数指针类型
     *      typedef void (*SyncHandle)(queue<T> &)
     * 在元素同步时回调该函数
     */
    SyncHandle syncHandle;

};


#endif //DNRECORDER_SAFE_QUEUE_H
