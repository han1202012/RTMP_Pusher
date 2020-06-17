//
// Created by octopus on 2020/6/17.
//

#ifndef RTMP_PUSHER_AUDIOCHANNEL_H
#define RTMP_PUSHER_AUDIOCHANNEL_H

#include <faac.h>

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


private:

    /**
     * RTMPPacket 数据包打包完毕回调函数
     */
    RTMPPacketPackUpCallBack rtmpPacketPackUpCallBack;

    /**
     * 音频通道数
     */
    int mChannelConfig;

    /**
     * 输入样本个数, 需要进行编码的 PCM 音频样本个数
     * FAAC 编码器最多一次可以接收的样本个数
     * 传递下面两个数值的地址到 faacEncOpen 函数中, 用于当做返回值使用
     */
    unsigned long inputSamples;

    /**
     * FAAC 编码器最多一次可以接收的样本个数
     * 传递下面两个数值的地址到 faacEncOpen 函数中, 用于当做返回值使用
     */
    unsigned long maxOutputBytes;

    /**
     * PCM 音频 FAAC 编码器
     */
    faacEncHandle mFaacEncHandle;

};


#endif //RTMP_PUSHER_AUDIOCHANNEL_H
