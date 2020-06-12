//
// Created by octopus on 2020/6/12.
//

#include "VedioChannel.h"
/**
 * 构造方法
 */
VedioChannel::VedioChannel() {
    // 初始化互斥锁, 设置视频编码参数 与 编码互斥
    pthread_mutex_init(&mMutex, 0);
}

/**
 * 析构函数
 */
VedioChannel::~VedioChannel() {
    // 销毁互斥锁, 设置视频编码参数 与 编码互斥
    pthread_mutex_destroy(&mMutex);
}

/**
 * 设置视频编码参数
 * @param width 宽度
 * @param height 高度
 * @param fps 帧率
 * @param bitrate 码率
 */
void VedioChannel::setVideoEncoderParameters(int width, int height, int fps, int bitrate) {
    // 加锁, 设置视频编码参数 与 编码互斥
    pthread_mutex_lock(&mMutex);

    // 设置视频编码参数
    mWidth = width;
    mHeight = height;
    mFps = fps;
    mBitrate = bitrate;

    // 设置 x264 编码器参数
    x264_param_t x264Param;

    /*
     * int x264_param_default_preset( x264_param_t *, const char *preset, const char *tune )
     * x264_param_t * : 编码参数指针
     *
     */
    //x264_param_default_preset(&x264Param, )

    // 解锁, 设置视频编码参数 与 编码互斥
    pthread_mutex_unlock(&mMutex);
}
