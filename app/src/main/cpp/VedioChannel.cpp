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

    // 释放 x264 编码图像资源, 只要是成员函数里有的, 都要在析构函数中释放
    if (x264EncodePicture) {
        x264_picture_clean(x264EncodePicture);
        delete x264EncodePicture;
        x264EncodePicture = 0;
    }
}

void VedioChannel::setRTMPPacketPackUpCallBack(RTMPPacketPackUpCallBack rtmpPacketPackUpCallBack){
    this->rtmpPacketPackUpCallBack = rtmpPacketPackUpCallBack;
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

    // 图像宽度
    mWidth = width;
    // 图像高度
    mHeight = height;
    // 帧率
    mFps = fps;
    // 码率
    mBitrate = bitrate;

    // 灰色值的个数, 单位字节
    YByteCount = width * height;
    // U 色彩值, V 饱和度 个数
    UVByteCount = YByteCount / 4;

    // 设置 x264 编码器参数
    x264_param_t x264Param;

    /*
     * 获取 x264 编码器参数
     * int x264_param_default_preset( x264_param_t *, const char *preset, const char *tune )
     * 参数一 : x264_param_t * : x264 编码参数指针
     *
     * 参数二 : const char *preset : 设置编码速度, 这里开发直播, 需要尽快编码推流,
     * 这里设置最快的速度 ultrafast, 字符串常量, 值从 下面的参数中选择 ;
     * static const char * const x264_preset_names[] = { "ultrafast", "superfast", "veryfast",
     * "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo", 0 };
     *
     * 参数三 : const char *tune : 视频编码场景设置, 这里选择 zerolatency 无延迟编码
     * static const char * const x264_tune_names[] = { "film", "animation", "grain",
     * "stillimage", "psnr", "ssim", "fastdecode", "zerolatency", 0 };
     *
     * 编码速度快, 意味着牺牲了画面的质量
     */
    x264_param_default_preset(&x264Param, "ultrafast", "zerolatency");

    // 编码规格设定, 32 对应的是 3.2 编码规格, 该规格下有指定的 码率, 帧率要求
    // 参考 https://www.wanweibaike.com/wiki-H.264 中的最大性能级别
    x264Param.i_level_idc = 32;

    // 设置输入到 x264 编码器的数据格式, 宽度, 高度等参数
    x264Param.i_csp = X264_CSP_I420;
    x264Param.i_width = mWidth;
    x264Param.i_height = mHeight;

    /*
       设置码率相关参数
       码率有三种模式 : X264_RC_CQP 恒定质量, X264_RC_CRF 恒定码率, X264_RC_ABR 平均码率
       这里设置一个平均码率输出
     */
    x264Param.rc.i_rc_method = X264_RC_ABR;
    // 设置码率, 单位是 kbps
    x264Param.rc.i_bitrate = bitrate / 1000;
    // 设置最大码率, 单位 kbps, 该配置与 i_vbv_buffer_size 配套使用
    x264Param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
    // 该配置与 i_vbv_max_bitrate 配置配套使用, 码率控制缓冲区大小
    x264Param.rc.i_vbv_buffer_size = bitrate / 1000;

    // 设置帧率相关参数, 帧率是个有理数, 使用分数形式表示
    x264Param.i_fps_num = fps;  // 分子
    x264Param.i_fps_den = 1;    // 分母
    x264Param.i_timebase_den = x264Param.i_fps_num; //分子
    x264Param.i_timebase_num = x264Param.i_fps_den; //分母


    // 下面是关于帧的先关设置

    /*
       关键帧数据 I 是否附带 SPS PPS 数据
       编码后, 会输出图像编码后的数据

       第一个图像数据输入到 x264 编码器后, 进行编码
       编码的第一个图像编码出来的数据 肯定是 SPS PPS 关键帧 三种数据
       SPS PPS 作用是告知后续如何解码视频中的图像数据

       第二个图像数据输入到 x264 编码器后, 进行编码
       编码的第二个图像编码出来的数据 是 P 帧

       后续 n 个图像编码出 n 个 P 帧

       第 n + 3 个图像又编码出一个关键帧 I

       任何一个画面都可以编码成关键帧

       直播时建议设置成 1
       因为中途会有新用户加入, 此时该用户的播放器必须拿到 SPS PPS 才能解码画面
       否则无法观看视频
       如果设置成 0, 那么就需要开发者自己维护 SPS PPS 数据
       保证后来的用户可以看到直播画面
     */
    x264Param.b_repeat_headers = 1;

    // 计算帧间距的依据, 该设置表示使用 fps 帧率计算帧间距
    // 两帧之间间隔多少 fps
    // 也可以使用时间戳计算帧间距
    x264Param.b_vfr_input = 0;

    /*
       关键帧的间距, 两个关键帧之间的距离
       fps 表示 1 秒钟画面帧的数量, fps * 2 表示 2 秒钟的帧数
       该设置表示每隔 2 秒, 采集一个关键帧数据

       关键帧间隔时间不能太长

       关键帧间隔不能设置太长, 如设置 10 秒
       当用户1观看直播时, 不影响观看
       当用户2进入房间, 此时刚过去一个关键帧, 10秒内没有关键帧
       该用户需要等待 10 秒后收到关键帧数据后, 才有画面显示出来
     */
    x264Param.i_keyint_max = fps * 2;

    // 设置 B 帧个数, 这里设置没有 B 帧, 只有 I 帧和 P 帧
    // B 帧解码时, 既要参考前面的帧, 又要参考后面的帧
    // B 帧能减少传输的数据量, 但同时降低了解码速度, 直播中解码速度必须要快
    x264Param.i_bframe = 0;

    // 是否开启多线程
    x264Param.i_threads = 1;


    // 只要调用该方法, x264_picture_t 必须重新进行初始化
    // 因为图片大小改变了, 那么对应的图片不能再使用原来的参数了
    // 释放原来的 x264_picture_t 图片, 重新进行初始化
    if (x264EncodePicture) {
        x264_picture_clean(x264EncodePicture);
        delete x264EncodePicture;
        x264EncodePicture = 0;
    }

    // 初始化 x264 编码图片
    x264EncodePicture = new x264_picture_t;
    // 为 x264 编码图片分配内存
    x264_picture_alloc(x264EncodePicture, X264_CSP_I420, x264Param.i_width, x264Param.i_height);


    // 关闭之前的视频编码器, 更新了参数, 需要创建新的编码器
    if (x264VedioCodec) {
        x264_encoder_close(x264VedioCodec);
        x264VedioCodec = 0;
    }
    // 打开 x264 视频编码器
    x264VedioCodec = x264_encoder_open(&x264Param);


    // 解锁, 设置视频编码参数 与 编码互斥
    pthread_mutex_unlock(&mMutex);
}



/**
 * 视频数据编码
 * 接收 int8_t 类型的原因是, 这里处理的是 jbyte* 类型参数
 * jbyte 类型就是 int8_t 类型
 * @param data 视频数据指针
 */
void VedioChannel::encodeCameraData(int8_t *data) {
    // 加锁, 设置视频编码参数 与 编码互斥
    pthread_mutex_lock(&mMutex);

    // 参数中的 data 是 NV21 格式的
    // 前面 YByteCount 字节个 Y 灰度数据
    // 之后是 UVByteCount 字节个 VU 数据交替存储
    // UVByteCount 字节 V 数据, UVByteCount 字节 U 数据

    // 从 Camera 采集的 NV21 格式的 data 数据中
    // 将 YUV 中的 Y 灰度值数据, U 色彩值数据, V 色彩饱和度数据提取出来
    memcpy(x264EncodePicture->img.plane[0], data, YByteCount);

    // 取出 NV21 数据中交替存储的 VU 数据
    // V 在前 ( 偶数位置 ), U 在后 ( 奇数位置 ), 交替存储
    for(int i = 0; i < UVByteCount; i ++){
        // U 色相 / 色彩值数据, 存储在 YByteCount 后的奇数索引位置
        *(x264EncodePicture->img.plane[1] + i) = *(data + YByteCount + i * 2 + 1);

        // V 色彩饱和度数据, 存储在 YByteCount 后的偶数索引位置
        *(x264EncodePicture->img.plane[2] + i) = *(data + YByteCount + i * 2);
    }

    // 下面两个是编码时需要传入的参数, 这两个参数地址, x264 编码器会想这两个地址写入值

    // 编码后的数据, 这是一个帧数据
    x264_nal_t *pp_nal;
    // 编码后的数据个数, 帧的个数
    int pi_nal;
    // 输出的图片数据
    x264_picture_t pic_out;

    /*
        int x264_encoder_encode( x264_t *, x264_nal_t **pp_nal, int *pi_nal,
                                 x264_picture_t *pic_in, x264_picture_t *pic_out );
        函数原型 :
            x264_t * 参数 : x264 视频编码器
            x264_nal_t **pp_nal 参数 : 编码后的帧数据, 可能是 1 帧, 也可能是 3 帧
            int *pi_nal 参数 : 编码后的帧数, 1 或 3
            x264_picture_t *pic_in 参数 : 输入的 NV21 格式的图片数据
            x264_picture_t *pic_out 参数 : 输出的图片数据

        普通帧 : 一般情况下, 一张图像编码出一帧数据, pp_nal 是一帧数据, pi_nal 表示帧数为 1
        关键帧 : 如果这个帧是关键帧, 那么 pp_nal 将会编码出 3 帧数据, pi_nal 表示帧数为 3
        关键帧数据 : SPS 帧, PPS 帧, 画面帧

     */
    x264_encoder_encode(x264VedioCodec, &pp_nal, &pi_nal, x264EncodePicture, &pic_out);

    // 下面要提取数据中 SPS 和 PPS 数据
    // 只有关键帧 ( I 帧 ) 数据, 并且配置了 x264Param.b_repeat_headers = 1 参数
    // 每个关键帧都会附带 SPS PPS 数据, 才能获取如下 SPS PPS 数据

    // SPS 数据长度
    int spsLen;
    // SPS 数据存储数组
    uint8_t sps[100];
    // PPS 数据长度
    int ppsLen;
    // PPS 数据存储数组
    uint8_t pps[100];

    /*
        pp_nal[i].p_payload 数据时编码后的数据, 前四位默认是 00 00 00 01
        pp_nal[i].i_payload 是编码后的数据大小, 这个大小包括前 4 位的 00 00 00 01 数据
            一般情况下, 是要将前四位数据扣除, pp_nal[i].i_payload - 4

        pp_nal[i].p_payload 是 x264 编码后的数据
        pp_nal[i].i_payload 是 x264 编码后的数据长度
        根据上述两个数据封装 RTMP 数据包
        封装好之后, 将 RTMP 数据包推流到服务器中

        帧数据的长度为 pp_nal[i].i_payload - 4 原理
        每个帧开始的位置是 4 字节的分隔符
        第 0, 1, 2, 3 四字节是 00 00 00 01 数据
        作用是分割不同的帧

        帧数据拷贝从 pp_nal[i].p_payload + 4 开始
        前 4 个字节 0, 1, 2, 3 都是分割符, 不是真实的数据
        这里要从 第 4 个字节开始拷贝数据

     */
    for(int i = 0; i < pi_nal; i ++){
        if(pp_nal[i].i_type == NAL_SPS){
            // 4 字节分隔符是 x264 编码后生成的 H.264 数据中的数据, 这里需要剔除该数据
            spsLen = pp_nal[i].i_payload - 4;
            // 拷贝 H.264 数据时, 需要越过 4 字节 间隔数据
            memcpy(sps, pp_nal[i].p_payload + 4, spsLen);

        }else if(pp_nal[i].i_type == NAL_PPS){
            // 4 字节分隔符是 x264 编码后生成的 H.264 数据中的数据, 这里需要剔除该数据
            ppsLen = pp_nal[i].i_payload - 4;
            // 拷贝 H.264 数据时, 需要越过 4 字节 间隔数据
            memcpy(pps, pp_nal[i].p_payload + 4, ppsLen);

            // 向 RTMP 服务器端发送 SPS 和 PPS 数据
            // 发送时机是关键帧编码完成之后
            sendSpsPpsToRtmpServer(sps, pps, spsLen, ppsLen);
        }else {
            // 封装关键帧 ( I 帧 ) , 非关键帧 ( P 帧 ) , 没有设置 B 帧, 因此这里没有 B 帧
            // pp_nal[i].i_payload 表示数据帧大小
            // pp_nal[i].p_payload 存放数据帧数据
            sendFrameToRtmpServer(pp_nal[i].i_type, pp_nal[i].i_payload, pp_nal[i].p_payload);
        }
    }

    // 解锁, 设置视频编码参数 与 编码互斥
    pthread_mutex_unlock(&mMutex);
}

/**
 * 将 SPS / PPS 数据发送到 RTMP 服务器端
 * @param sps       SPS 数据
 * @param pps       PPS 数据
 * @param spsLen    SPS 长度
 * @param ppsLen    PPS 长度
 */
void VedioChannel::sendSpsPpsToRtmpServer(uint8_t *sps, uint8_t *pps, int spsLen, int ppsLen) {
    // 创建 RTMP 数据包, 将数据都存入该 RTMP 数据包中
    RTMPPacket *rtmpPacket = new RTMPPacket;

    /*
        计算整个 SPS 和 PPS 数据的大小
        数据示例 :
                                 17 00 00 00 00
        0x00000192	:   01 64 00 32 FF E1 00 19
        0x0000019a	:   67 64 00 32 AC D9 80 78
        0x000001a2	:   02 27 E5 84 00 00 03 00
        0x000001aa	:   04 00 00 1F 40 3C 60 C6
        0x000001b2	:   68 01 00 05 68 E9 7B 2C
        0x000001ba	:   8B 00 00 00 39

        17 帧类型, 1 字节
        00 数据类型, 1 字节
        00 00 00 合成时间, 3 字节
        01 版本信息, 1 字节
        64 00 32 编码规则, 3 字节
        FF NALU 长度, 1 字节

        E1 SPS 个数, 1 字节
        00 19 SPS 长度, 2 字节

        截止到当前位置有 13 字节数据

        spsLen 字节数据, 这里是 25 字节

                        67 64 00 32 AC D9 80 78
        0x000001a2	:   02 27 E5 84 00 00 03 00
        0x000001aa	:   04 00 00 1F 40 3C 60 C6
        0x000001b2	:   68

        01 PPS 个数, 1 字节
        00 05 PPS 长度, 2 字节

        ppsLen 字节的 PPS 数据
                                    68 E9 7B 2C
        0x000001ba	:   8B

        后面的 00 00 00 39 是视频标签的总长度
        这里再 RTMP 标签中可以不用封装
     */
    int rtmpPackagesize = 10 + 3 + spsLen + 3 + ppsLen;

    // 为 RTMP 数据包分配内存
    RTMPPacket_Alloc(rtmpPacket, rtmpPackagesize);

    // 记录下一个要写入数据的索引位置
    int nextPosition = 0;

    // 帧类型数据 : 分为两部分;
    // 前 4 位表示帧类型, 1 表示关键帧, 2 表示普通帧
    // 后 4 位表示编码类型, 7 表示 AVC 视频编码
    rtmpPacket->m_body[nextPosition++] = 0x17;

    // 数据类型, 00 表示 AVC 序列头
    rtmpPacket->m_body[nextPosition++] = 0x00;

    // 合成时间, 一般设置 00 00 00
    rtmpPacket->m_body[nextPosition++] = 0x00;
    rtmpPacket->m_body[nextPosition++] = 0x00;
    rtmpPacket->m_body[nextPosition++] = 0x00;

    // 版本信息
    rtmpPacket->m_body[nextPosition++] = 0x01;

    // 编码规格
    rtmpPacket->m_body[nextPosition++] = sps[1];
    rtmpPacket->m_body[nextPosition++] = sps[2];
    rtmpPacket->m_body[nextPosition++] = sps[3];

    // NALU 长度
    rtmpPacket->m_body[nextPosition++] = 0xFF;

    // SPS 个数
    rtmpPacket->m_body[nextPosition++] = 0xE1;

    // SPS 长度, 占 2 字节
    // 设置长度的高位
    rtmpPacket->m_body[nextPosition++] = (spsLen >> 8) & 0xFF;
    // 设置长度的低位
    rtmpPacket->m_body[nextPosition++] = spsLen & 0xFF;

    // 拷贝 SPS 数据
    // 将 SPS 数据拷贝到 rtmpPacket->m_body[nextPosition] 地址中
    memcpy(&rtmpPacket->m_body[nextPosition], sps, spsLen);
    // 累加 SPS 长度信息
    nextPosition += spsLen;

    // PPS 个数
    rtmpPacket->m_body[nextPosition++] = 0x01;

    // PPS 数据的长度, 占 2 字节
    // 设置长度的高位
    rtmpPacket->m_body[nextPosition++] = (ppsLen >> 8) & 0xFF;
    // 设置长度的低位
    rtmpPacket->m_body[nextPosition++] = (ppsLen) & 0xFF;
    // 拷贝 SPS 数据
    memcpy(&rtmpPacket->m_body[nextPosition], pps, ppsLen);


    // 设置 RTMP 包类型, 视频类型数据
    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    // 设置 RTMP 包长度
    rtmpPacket->m_nBodySize = rtmpPackagesize;
    // 分配 RTMP 通道, 随意分配
    rtmpPacket->m_nChannel = 10;
    // 设置视频时间戳, 如果是 SPP PPS 数据, 没有时间戳
    rtmpPacket->m_nTimeStamp = 0;
    // 设置绝对时间, 对于 SPS PPS 赋值 0 即可
    rtmpPacket->m_hasAbsTimestamp = 0;
    // 设置头类型, 随意设置一个
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    // 调用回调接口, 将该封装好的 RTMPPacket 数据包放入 native-lib 类中的 线程安全队列中
    // 这是个 RTMPPacketPackUpCallBack 类型的函数指针
    rtmpPacketPackUpCallBack(rtmpPacket);
}

/**
 * 封装视频帧 , 关键帧 和 非关键帧
 * @param type  视频帧类型
 * @param payload   视频帧大小
 * @param p_payload 视频帧数据
 */
void VedioChannel::sendFrameToRtmpServer(int type, int payload, uint8_t *p_payload) {
    // 判定分隔符是 00 00 00 01 还是 00 00 01
    // 根据 第 2 位 的值判定
    // 如果 第 2 位 值为 01, 说明分隔符是 00 00 01
    // 如果 第 2 位 值为 00, 说明分隔符是 00 00 00 01
    if (p_payload[2] == 0x00){
        // 识别出分隔符是 00 00 00 01
        // 要将 x264 编码出的数据个数减去 4, 只统计实际的数据帧个数
        payload -= 4;
        // 从 x264 编码后的数据向外拿数据时, 越过开始的 00 00 00 01 数据
        p_payload += 4;
    } else if(p_payload[2] == 0x01){
        // 识别出分隔符是 00 00 01
        // 要将 x264 编码出的数据个数减去 3, 只统计实际的数据帧个数
        payload -= 3;
        // 从 x264 编码后的数据向外拿数据时, 越过开始的 00 00 01 数据
        p_payload += 3;
    }

    // 创建 RTMP 数据包
    RTMPPacket *rtmpPacket = new RTMPPacket;

    /*
        计算 RTMP 数据包大小

        帧类型 : 1 字节, 关键帧 17, 非关键帧 27
        包类型 : 1 字节, 1 表示数据帧 ( 关键帧 / 非关键帧 ), 0 表示 AVC 序列头
        合成时间 : 3 字节, 设置 00 00 00
        数据长度 : 4 字节, 赋值 payload 代表的数据长度

     */
    int rtmpPackagesize = 9 + payload;

    // 为 RTMP 数据包分配内存
    RTMPPacket_Alloc(rtmpPacket, rtmpPackagesize);
    // 重置 RTMP 数据包
    RTMPPacket_Reset(rtmpPacket);

    // 设置帧类型, 非关键帧类型 27, 关键帧类型 17
    rtmpPacket->m_body[0] = 0x27;
    if (type == NAL_SLICE_IDR) {
        rtmpPacket->m_body[0] = 0x17;
    }

    // 设置包类型, 01 是数据帧, 00 是 AVC 序列头封装 SPS PPS 数据
    rtmpPacket->m_body[1] = 0x01;
    // 合成时间戳, AVC 数据直接赋值 00 00 00
    rtmpPacket->m_body[2] = 0x00;
    rtmpPacket->m_body[3] = 0x00;
    rtmpPacket->m_body[4] = 0x00;

    // 数据长度, 需要使用 4 位表示
    rtmpPacket->m_body[5] = (payload >> 24) & 0xFF;
    rtmpPacket->m_body[6] = (payload >> 16) & 0xFF;
    rtmpPacket->m_body[7] = (payload >> 8) & 0xFF;
    rtmpPacket->m_body[8] = (payload) & 0xFF;

    // H.264 数据帧数据
    memcpy(&rtmpPacket->m_body[9], p_payload, payload);

    // 设置 RTMP 包类型, 视频类型数据
    rtmpPacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    // 设置 RTMP 包长度
    rtmpPacket->m_nBodySize = rtmpPackagesize;
    // 分配 RTMP 通道, 随意分配
    rtmpPacket->m_nChannel = 10;
    // 设置绝对时间, 对于 SPS PPS 赋值 0 即可
    rtmpPacket->m_hasAbsTimestamp = 0;
    // 设置头类型, 随意设置一个
    rtmpPacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    // 调用回调接口, 将该封装好的 RTMPPacket 数据包放入 native-lib 类中的 线程安全队列中
    // 这是个 RTMPPacketPackUpCallBack 类型的函数指针
    rtmpPacketPackUpCallBack(rtmpPacket);
}