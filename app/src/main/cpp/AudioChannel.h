//
// Created by octopus on 2020/6/17.
//

#ifndef RTMP_PUSHER_AUDIOCHANNEL_H
#define RTMP_PUSHER_AUDIOCHANNEL_H

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

};


#endif //RTMP_PUSHER_AUDIOCHANNEL_H
