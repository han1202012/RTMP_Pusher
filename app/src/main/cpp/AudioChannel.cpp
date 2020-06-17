//
// Created by octopus on 2020/6/17.
//

#include "AudioChannel.h"
#include "include/faac.h"


/**
 * 构造方法
 */
AudioChannel::AudioChannel(){

}

/**
 * 析构方法
 */
AudioChannel::~AudioChannel(){

}


/**
 * 打包 RTMP 包后的回调函数
 * @param rtmpPacketPackUpCallBack
 */
void AudioChannel::setRTMPPacketPackUpCallBack(RTMPPacketPackUpCallBack rtmpPacketPackUpCallBack){
    this->rtmpPacketPackUpCallBack = rtmpPacketPackUpCallBack;
}

/**
 * 设置音频编码参数
 * @param sampleRateInHz    音频采样率
 * @param channelConfig     音频采样通道, 单声道 / 立体声
 */
void AudioChannel::setAudioEncoderParameters(int sampleRateInHz, int channelConfig) {
    // 设置音频通道参数, 单声道 / 立体声
    mChannelConfig = channelConfig;

    /*
        打开编码器
        faacEncHandle FAACAPI faacEncOpen(unsigned long sampleRate,
				  unsigned int numChannels,
				  unsigned long *inputSamples,
                  unsigned long *maxOutputBytes);
        unsigned long sampleRate 参数 : 音频采样率
        unsigned int numChannels 参数 : 音频通道参数
        unsigned long *inputSamples 参数 : 输入样本个数, 需要进行编码的 PCM 音频样本个数
            FAAC 编码器最多一次可以接收的样本个数
        unsigned long *maxOutputBytes 参数 : 输出数据最大字节数

        faacEncHandle 返回值 : FAAC 音频编码器
     */
    mFaacEncHandle = faacEncOpen(sampleRateInHz, channelConfig, &inputSamples, &maxOutputBytes);

    // 获取编码器当前参数
    // 先获取参数, 然后设置参数, 最后再设置回去
    faacEncConfigurationPtr configurationPtr = faacEncGetCurrentConfiguration(mFaacEncHandle);

    // 设置编码格式标准, 使用 MPEG4 新标准
    configurationPtr->mpegVersion = MPEG4;

    /*
        设置 AAC 编码规格, 有 9 种
        MPEG-2 AAC LC低复杂度规格（Low Complexity）, MPEG-2 AAC Main主规格, MPEG-2 AAC SSR可变采样率规格（Scaleable Sample Rate）
        MPEG-4 AAC LC低复杂度规格（Low Complexity）, MPEG-4 AAC Main主规格, MPEG-4 AAC SSR可变采样率规格（Scaleable Sample Rate）
        MPEG-4 AAC LTP长时期预测规格（Long Term Predicition）, MPEG-4 AAC LD低延迟规格（Low Delay）, MPEG-4 AAC HE高效率规格（High Efficiency）
        这里选择低复杂度规格, 可选参数有 4 种
        MPEG-4 AAC LC低复杂度规格（Low Complexity） 是最常用的 AAC 编码规格, 即兼顾了编码效率, 有保证了音质;
        使用更高音质, 极大降低编码效率, 音质提升效果有限
        再提升编码效率, 会使音质降低很多
     */
    configurationPtr->aacObjectType = LOW;

    // 采样位数 16 位
    configurationPtr->inputFormat = FAAC_INPUT_16BIT;

    /*
        AAC 音频文件有两种格式 ADIF 和 ADTS
        AAC 文件解码时 : 音频解码信息定义在头部, 后续音频数据解码按照音频数据长度
        ADTS 文件 : 解码信息和音频数据交替存放

        RTMP 推流时, 不使用上述两种格式
        推流视频时, 先将 SPS, PPS 解码数据包的信息推流到服务器上
        推流音频时, 也是将解码相关的数据先推流到服务器中

        AAC 编码时, 会编码成 ADTS 数据
        但是推流音频时, 推流的是 AAC 裸数据, 需要将 ADTS 音频格式中的头信息去掉

        博客中截图 FLV 第一帧 AAC 音频数据标签 和 后续 AAC 音频数据标签
     */
    configurationPtr->outputFormat = 0;


    // 为 mFaacEncHandle 音频编码器设置 configurationPtr 编码器参数
    faacEncSetConfiguration(mFaacEncHandle, configurationPtr);
}