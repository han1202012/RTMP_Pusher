//
// Created by octopus on 2020/6/12.
//

#ifndef RTMP_PUSHER_VEDIOCHANNEL_H
#define RTMP_PUSHER_VEDIOCHANNEL_H

#include <memory.h>
#include <inttypes.h>
#include <x264.h>
#include <pthread.h>
#include "librtmp/rtmp.h"

/*#include <memory.h>
#include "librtmp/rtmp.h"
#include <inttypes.h>
#include <x264.h>
#include <pthread.h>
#include "librtmp/rtmp.h"*/

/**
 * 视频处理类
 *
 * 处理 NV21 转为 I420 格式
 * 处理 x264 转为 H.264 格式
 * 处理 H.264 封装为 RTMP 数据包
 */
class VedioChannel {

    /**
     * 函数指针, 当 RTMPPacket 数据包封装完毕后调用该回调函数
     * 将该封装好的 RTMPPacket 数据包放入线程安全队列中
     */
    typedef void (*RTMPPacketPackUpCallBack)(RTMPPacket* packet);

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

    /**
     * 视频数据编码
     * 接收 int8_t 类型的原因是, 这里处理的是 jbyte* 类型参数
     * jbyte 类型就是 int8_t 类型
     * @param data
     */
    void encodeCameraData(int8_t *data);

    /**
     * 设置打包完毕回调函数
     * 当 RTMPPacket 数据包打包完毕后, 就会回调该函数
     * @param rtmpPacketPackUpCallBack
     *              函数指针类型
     */
    void setRTMPPacketPackUpCallBack(RTMPPacketPackUpCallBack rtmpPacketPackUpCallBack);

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

    // I240 / NV21 格式的图像数据信息
    // YUV 数据的个数
    // Y 代表灰度
    // U 代表色相, 色彩度, 指的是光的颜色
    // V 代表饱和度, 指的是光的纯度

    /**
     * Y 灰度数据的个数
     */
    int YByteCount;

    /**
     * 色彩度 U, 饱和度 V 数据个数
     */
    int UVByteCount;

    /**
     * x264 需要编码的图片
     */
    x264_picture_t *x264EncodePicture = 0;

    /**
     * RTMPPacket 数据包打包完毕回调函数
     */
    RTMPPacketPackUpCallBack rtmpPacketPackUpCallBack;

    /**
     * x264 视频编码器
     */
    x264_t *x264VedioCodec = 0;

    /**
     * 将 SPS / PPS 数据发送到 RTMP 服务器端
     * @param sps   SPS 数据
     * @param pps   PPS 数据
     * @param spsLen   SPS 长度
     * @param ppsLen PPS 长度
     */
    void sendSpsPpsToRtmpServer(uint8_t *sps, uint8_t *pps, int spsLen, int ppsLen);

    void sendFrameToRtmpServer(int type, int payload, uint8_t *p_payload);
};


#endif //RTMP_PUSHER_VEDIOCHANNEL_H
