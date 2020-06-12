//
// Created by octopus on 2020/6/12.
//

#include <x264.h>
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

    // 图像宽度
    mWidth = width;
    // 图像高度
    mHeight = height;
    // 帧率
    mFps = fps;
    // 码率
    mBitrate = bitrate;

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

    // 解锁, 设置视频编码参数 与 编码互斥
    pthread_mutex_unlock(&mMutex);
}
