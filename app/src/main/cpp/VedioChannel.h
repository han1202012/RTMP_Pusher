//
// Created by octopus on 2020/6/12.
//

#ifndef RTMP_PUSHER_VEDIOCHANNEL_H
#define RTMP_PUSHER_VEDIOCHANNEL_H

#include <pthread.h>
#include <x264.h>


/**
 * 视频处理类
 *
 * 处理 NV21 转为 I420 格式
 * 处理 x264 转为 H.264 格式
 * 处理 H.264 封装为 RTMP 数据包
 */
class VedioChannel {

public:
    /**
     * 构造方法
     */
    VedioChannel();

    /**
     * 析构方法
     */
    ~VedioChannel();

    /**
     * 设置视频编码参数, 创建 x264 视频数据编码器
     * 用于将 I420 数据转为 H.264 数据
     * @param width 宽度
     * @param height 高度
     * @param fps 帧率
     * @param bitrate 码率
     */
    void setVideoEncoderParameters(int width, int height, int fps, int bitrate);

private:
    /**
     * 互斥锁
     * 数据编码时, 可能会重新设置视频编码参数 setVideoEncoderParameters, 如横竖屏切换, 改变了大小
     * setVideoEncoderParameters 操作线程, 需要与编码操作互斥
     *
     * 构造函数中初始化互斥锁
     * 析构函数中回收互斥锁
     * 设置参数时需要加锁
     * 数据编码时需要加锁
     */
    pthread_mutex_t mMutex;

    // 下面是视频编码参数, 宽度, 高度, 帧率, 码率

    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;
};


#endif //RTMP_PUSHER_VEDIOCHANNEL_H
