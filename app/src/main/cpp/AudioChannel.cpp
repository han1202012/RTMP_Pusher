//
// Created by octopus on 2020/6/17.
//

#include "AudioChannel.h"


/**
 * 构造方法
 */
AudioChannel::AudioChannel() {

}

/**
 * 析构方法
 */
AudioChannel::~AudioChannel() {
    // 释放 FAAC 编码 AAC 缓冲区
    if (mFaacEncodeOutputBuffer) {
        delete mFaacEncodeOutputBuffer;
        mFaacEncodeOutputBuffer = 0;
    }

    // 释放 FAAC 编码器
    if (mFaacEncHandle) {
        delete mFaacEncHandle;
        mFaacEncHandle = 0;
    }
}


/**
 * 打包 RTMP 包后的回调函数
 * @param rtmpPacketPackUpCallBack
 */
void AudioChannel::setRTMPPacketPackUpCallBack(RTMPPacketPackUpCallBack rtmpPacketPackUpCallBack) {
    this->mRtmpPacketPackUpCallBack = rtmpPacketPackUpCallBack;
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
				  unsigned long *mInputSamples,
                  unsigned long *mMaxOutputBytes);
        unsigned long sampleRate 参数 : 音频采样率
        unsigned int numChannels 参数 : 音频通道参数
        unsigned long *mInputSamples 参数 : 输入样本个数, 需要进行编码的 PCM 音频样本个数
            FAAC 编码器最多一次可以接收的样本个数
        unsigned long *mMaxOutputBytes 参数 : 输出数据最大字节数

        faacEncHandle 返回值 : FAAC 音频编码器

        注意上述 样本个数, 字节个数 区别 : 16 位采样位数, 一个样本有 2 字节
     */
    mFaacEncHandle = faacEncOpen(sampleRateInHz, channelConfig, &mInputSamples, &mMaxOutputBytes);

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

        音频数据交换格式 ( Audio Data Interchange Format ) , 只有一份音频解码信息 , 存储在文件开头
        这种格式适合存储音频文件 , 节省空间 , 但是必须从开始播放才可以 , 从中间位置无法播放 ;

        音频数据传输流格式 ( Audio Data Transport Stream ) , 每隔一段音频数据
        就会有一份音频解码信息 , 这种格式适合音频流传输 , 可以在任何位置开始解码播放 ;

        RTMP 推流时, 不使用上述两种格式
        推流视频时, 先将 SPS, PPS 解码数据包的信息推流到服务器上
        推流音频时, 也是将解码相关的数据先推流到服务器中

        AAC 编码时, 会编码成 ADTS 数据
        但是推流音频时, 推流的是 AAC 裸数据, 需要将 ADTS 音频格式中的头信息去掉

        博客中截图 FLV 第一帧 AAC 音频数据标签 和 后续 AAC 音频数据标签

        这里设置输出格式 0, 就是 FAAC 将 PCM 采样进行编码, 编码出的格式是 AAC 原始数据
        即没有解码信息的 ADIF 和 ADTS 的 AAC 纯样本裸数据
     */
    configurationPtr->outputFormat = 0;

    // 为 mFaacEncHandle 音频编码器设置 configurationPtr 编码器参数
    faacEncSetConfiguration(mFaacEncHandle, configurationPtr);

    // 初始化输出缓冲区, 保存 FAAC 编码输出数据
    mFaacEncodeOutputBuffer = new unsigned char[mMaxOutputBytes];
}

/**
 * 获取 FAAC 编码器每次输入的样本个数
 * @return
 */
int AudioChannel::getInputSamples() {
    return mInputSamples;
}

/**
 * 音频数据编码
 * 接收 int8_t 类型的原因是, 这里处理的是 jbyte* 类型参数
 * jbyte 类型就是 int8_t 类型
 * @param data
 */
void AudioChannel::encodeAudioData(int8_t *data) {

    /*
        函数原型 :
        int FAACAPI faacEncEncode(
            faacEncHandle hEncoder,
            int32_t * inputBuffer,
            unsigned int samplesInput,
            unsigned char *outputBuffer,
            unsigned int bufferSize);

        faacEncHandle hEncoder 参数 : FAAC 编码器
        int32_t * inputBuffer 参数 : 需要编码的 PCM 音频输入数据
        unsigned int samplesInput : 传入的 PCM 样本个数
        unsigned char *outputBuffer : 编码后的 AAC 格式音频输出缓冲区
        unsigned int bufferSize : 输出缓冲区最大字节大小

        返回值 : 编码后的数据字节长度
     */
    int encodeAacDataByteCount = faacEncEncode(
            mFaacEncHandle, // FAAC 编码器
            reinterpret_cast<int32_t *>(data), // 需要编码的 PCM 音频输入数据
            mInputSamples, // 传入的 PCM 样本个数
            mFaacEncodeOutputBuffer, // 编码后的 AAC 格式音频输出缓冲区
            mMaxOutputBytes); // 输出缓冲区最大字节大小


    // 组装 RTMP 数据包
    if (encodeAacDataByteCount > 0) {
        /*
            数据的大小 :
            前面有 2 字节头信息
            音频解码配置信息 : 前两位是 AF 00 , 指导 AAC 数据如何解码
            音频采样信息 : 前两位是 AF 01 , 实际的 AAC 音频采样数据
         */
        int rtmpPackagesize = 2 + encodeAacDataByteCount;

        // 创建 RTMP 数据包对象
        RTMPPacket *rtmpPacket = new RTMPPacket;

        // 为 RTMP 数据包分配内存
        RTMPPacket_Alloc(rtmpPacket, rtmpPackagesize);

        /*
            根据声道数生成相应的 文件头 标识
            AF / AE 头中的最后一位为 1 表示立体声, 为 0 表示单声道
            AF 是立体声
            AE 是单声道
         */
        rtmpPacket->m_body[0] = 0xAF;   //默认立体声

        if (mChannelConfig == 1) {
            // 如果是单声道, 将该值修改成 AE
            rtmpPacket->m_body[0] = 0xAE;
        }

        // 编码出的声音 都是 0x01, 本方法是对音频数据进行编码的方法, 头信息肯定是 AF 01 数据
        // 数据肯定是 AAC 格式的采样数据
        rtmpPacket->m_body[1] = 0x01;

        // 拷贝 AAC 音频数据到 RTMPPacket 数据包中
        memcpy(&rtmpPacket->m_body[2], mFaacEncodeOutputBuffer, encodeAacDataByteCount);

        // 设置绝对时间, 一般设置 0 即可
        rtmpPacket->m_hasAbsTimestamp = 0;
        // 设置 RTMP 数据包大小
        rtmpPacket->m_nBodySize = rtmpPackagesize;
        // 设置 RTMP 包类型, 视频类型数据
        rtmpPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        // 分配 RTMP 通道, 该值随意设置, 建议在视频 H.264 通道基础上加 1
        rtmpPacket->m_nChannel = 0x11;
        // // 设置头类型, 随意设置一个
        rtmpPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;

        // 调用回调接口, 将该封装好的 RTMPPacket 数据包放入 native-lib 类中的 线程安全队列中
        // 这是个 RTMPPacketPackUpCallBack 类型的函数指针
        mRtmpPacketPackUpCallBack(rtmpPacket);
    }

}


/**
 * 获取音频解码信息
 * 推流音频数据时, 先发送解码信息包, 再推流 AAC 音频采样包
 * @return 音频解码数据包
 */
RTMPPacket *AudioChannel::getAudioDecodeInfo() {

    /*
        下面的数据信息用于指导 AAC 数据如何进行解码
        类似于 H.264 视频信息中的 SPS 与  PPS 数据

        int FAACAPI faacEncGetDecoderSpecificInfo(
                    faacEncHandle hEncoder,
                    unsigned char **ppBuffer,
                    unsigned long *pSizeOfDecoderSpecificInfo);


     */
    // 该指针用于接收获取的 FAAC 解码特殊信息
    unsigned char *pBuffer;
    // 该指针用于接收获取的 FAAC 解码特殊信息长度
    unsigned long sizeOfDecoderSpecificInfo;
    // 生成 FAAC 解码特殊信息数据
    faacEncGetDecoderSpecificInfo(mFaacEncHandle, &pBuffer, &sizeOfDecoderSpecificInfo);


    // 组装 RTMP 数据包

    /*
        数据的大小 :
        前面有 2 字节头信息
        音频解码配置信息 : 前两位是 AF 00 , 指导 AAC 数据如何解码 ( 是这个 )
        音频采样信息 : 前两位是 AF 01 , 实际的 AAC 音频采样数据
     */
    int rtmpPackagesize = 2 + sizeOfDecoderSpecificInfo;

    // 创建 RTMP 数据包对象
    RTMPPacket *rtmpPacket = new RTMPPacket;

    // 为 RTMP 数据包分配内存
    RTMPPacket_Alloc(rtmpPacket, rtmpPackagesize);

    /*
        根据声道数生成相应的 文件头 标识
        AF / AE 头中的最后一位为 1 表示立体声, 为 0 表示单声道
        AF 是立体声
        AE 是单声道
     */
    rtmpPacket->m_body[0] = 0xAF;   //默认立体声

    if (mChannelConfig == 1) {
        // 如果是单声道, 将该值修改成 AE
        rtmpPacket->m_body[0] = 0xAE;
    }

    // 编码出的声音 都是 0x01, 本方法是对音频数据进行编码的方法, 头信息肯定是 AF 01 数据
    // 数据肯定是 AAC 格式的采样数据
    rtmpPacket->m_body[1] = 0x00;

    // 拷贝 AAC 音频数据到 RTMPPacket 数据包中
    memcpy(&rtmpPacket->m_body[2], pBuffer, sizeOfDecoderSpecificInfo);

    // 设置绝对时间, 一般设置 0 即可
    rtmpPacket->m_hasAbsTimestamp = 0;
    // 设置 RTMP 数据包大小
    rtmpPacket->m_nBodySize = rtmpPackagesize;
    // 设置 RTMP 包类型, 视频类型数据
    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    // 分配 RTMP 通道, 该值随意设置, 建议在视频 H.264 通道基础上加 1
    rtmpPacket->m_nChannel = 0x11;
    // // 设置头类型, 随意设置一个
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;

    // 调用回调接口, 将该封装好的 RTMPPacket 数据包放入 native-lib 类中的 线程安全队列中
    // 这是个 RTMPPacketPackUpCallBack 类型的函数指针
    // 这里不回调, 直接返回 rtmpPacket
    //mRtmpPacketPackUpCallBack(rtmpPacket);

    return rtmpPacket;
}