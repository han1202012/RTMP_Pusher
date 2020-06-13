//
// Created by octopus on 2020/6/12.
//

#include <x264.h>
#include <cstring>
#include "VedioChannel.h"
#include "librtmp/rtmp.h"

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
            spsLen = pp_nal[i].i_payload - 4;
            memcpy(sps, pp_nal[i].p_payload + 4, spsLen);

        }else if(pp_nal[i].i_type == NAL_PPS){
            ppsLen = pp_nal[i].i_payload - 4;
            memcpy(pps, pp_nal[i].p_payload + 4, ppsLen);

            // 向 RTMP 服务器端发送 SPS 和 PPS 数据
            // 发送时机是关键帧编码完成之后
            sendSpsPpsToRtmpServer(sps, pps, spsLen, ppsLen);
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
    RTMPPacket *packet = new RTMPPacket;

    // 计算整个 SPS 和 PPS 数据的大小
    int bodysize = 13 + spsLen + 3 + ppsLen;
    // 设置 RTMP 数据包大小
    RTMPPacket_Alloc(packet, bodysize);

    // 记录下一个要写入数据的索引位置
    int nextPosition = 0;
    //固定头
    packet->m_body[nextPosition++] = 0x17;
    //类型
    packet->m_body[nextPosition++] = 0x00;
    //composition time 0x000000
    packet->m_body[nextPosition++] = 0x00;
    packet->m_body[nextPosition++] = 0x00;
    packet->m_body[nextPosition++] = 0x00;

    //版本
    packet->m_body[nextPosition++] = 0x01;
    //编码规格
    packet->m_body[nextPosition++] = sps[1];
    packet->m_body[nextPosition++] = sps[2];
    packet->m_body[nextPosition++] = sps[3];
    packet->m_body[nextPosition++] = 0xFF;

    //整个sps
    packet->m_body[nextPosition++] = 0xE1;
    //sps长度
    packet->m_body[nextPosition++] = (spsLen >> 8) & 0xff;
    packet->m_body[nextPosition++] = spsLen & 0xff;
    memcpy(&packet->m_body[nextPosition], sps, spsLen);
    nextPosition += spsLen;

    //pps
    packet->m_body[nextPosition++] = 0x01;
    packet->m_body[nextPosition++] = (ppsLen >> 8) & 0xff;
    packet->m_body[nextPosition++] = (ppsLen) & 0xff;
    memcpy(&packet->m_body[nextPosition], pps, ppsLen);


    //视频
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodysize;
    //随意分配一个管道（尽量避开rtmp.c中使用的）
    packet->m_nChannel = 10;
    //sps pps没有时间戳
    packet->m_nTimeStamp = 0;
    //不使用绝对时间
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    //callback(packet);
}