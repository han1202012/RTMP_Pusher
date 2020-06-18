//
// Created by octopus on 2020/6/17.
//

#ifndef RTMP_PUSHER_AUDIOCHANNEL_H
#define RTMP_PUSHER_AUDIOCHANNEL_H

#include <memory.h>
#include <inttypes.h>
#include <faac.h>
#include <pthread.h>
#include "librtmp/rtmp.h"


/**
 * 音频编码器
 */
class AudioChannel {

    /**
     * 函数指针, 当 RTMPPacket 数据包封装完毕后调用该回调函数
     * 将该封装好的 RTMPPacket 数据包放入线程安全队列中
     */
    typedef void (*RTMPPacketPackUpCallBack)(RTMPPacket* packet);

public:
    AudioChannel();
    ~AudioChannel();

    /**
     * 设置音频编码参数
     * @param sampleRateInHz 采样率
     * @param channelConfig  声道数
     */
    void setAudioEncoderParameters(int sampleRateInHz, int channelConfig);

    /**
     * 设置打包完毕回调函数
     * 当 RTMPPacket 数据包打包完毕后, 就会回调该函数
     * @param rtmpPacketPackUpCallBack
     *              函数指针类型
     */
    void setRTMPPacketPackUpCallBack(RTMPPacketPackUpCallBack rtmpPacketPackUpCallBack);

    /**
     * Java 层获取每次 FAAC 可以获取多少样本个数
     * 用于 AudioRecord 的 read 方法
     * 一次性读取多少样本
     */
    int getInputSamples();

    /**
     * 音频数据编码
     * 接收 int8_t 类型的原因是, 这里处理的是 jbyte* 类型参数
     * jbyte 类型就是 int8_t 类型
     * @param data
     */
    void encodeAudioData(int8_t *data);

private:

    /**
     * RTMPPacket 数据包打包完毕回调函数
     */
    RTMPPacketPackUpCallBack mRtmpPacketPackUpCallBack;

    /**
     * 音频通道数
     */
    int mChannelConfig;

    /**
     * 输入样本个数, 需要进行编码的 PCM 音频样本个数
     * FAAC 编码器最多一次可以接收的样本个数
     * 传递下面两个数值的地址到 faacEncOpen 函数中, 用于当做返回值使用
     *
     * 该数据需要返回给 Java 层
     * Java 层每次从 AudioRecord 中读取 mInputSamples 个数据
     */
    unsigned long mInputSamples;

    /**
     * FAAC 编码器最多一次可以接收的样本个数
     * 传递下面两个数值的地址到 faacEncOpen 函数中, 用于当做返回值使用
     */
    unsigned long mMaxOutputBytes;

    /**
     * PCM 音频 FAAC 编码器
     * 将 PCM 采样数据编码成 FAAC 编码器
     */
    faacEncHandle mFaacEncHandle;

    /**
     * FAAC 编码输出缓冲区
     * FAAC 编码后的 AAC 裸数据, 存储到该缓冲区中
     * 该缓冲区在初始化 FAAC 编码器时创建
     */
    unsigned char* mFaacEncodeOutputBuffer;

};


#endif //RTMP_PUSHER_AUDIOCHANNEL_H
