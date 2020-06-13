#include <jni.h>
#include <android/log.h>
#include <string>
#include "librtmp/rtmp.h"
#include <x264.h>

#include "SafeQueue.h"
#include "VedioChannel.h"

/**
 * RTMPPacket 结构体是打包好的 RTMP 数据包
 * 将该数据包发送到 RTMP 服务器中
 */
SafeQueue<RTMPPacket *> packets;

/**
 * 视频处理对象
 */
VedioChannel* vedioChannel;

/**
 * 是否已经开始推流, 推流的重要标志位
 * 只要该标志位是 TRUE
 * 就一直不停的推流
 * 如果该标志位变成 FALSE
 * 停止推流
 */
int isStartRtmpPush = FALSE;

/**
 * 开始推流工作线程的线程 ID
 */
pthread_t startRtmpPushPid;

/**
 * 当前是否准备完毕, 进行推流
 */
int readyForPush = FALSE;

/**
 * 开始推流的时间
 */
uint32_t pushStartTime;

/**
 * 线程安全队列 SafeQueue<RTMPPacket *> packets 释放元素的方法
 * 函数的类型是 typedef void (*ReleaseHandle)(T &);
 * 返回值 void
 * 传入参数 T 元素类型的引用, 元素类型是 RTMPPacket * 的
 * @param rtmpPacket
 */
void releaseRTMPPackets(RTMPPacket * & rtmpPacket){
    if(rtmpPacket){
        delete rtmpPacket;
        rtmpPacket = 0;
    }
}

/**
 * 函数指针实现, 当 RTMPPacket 数据包封装完毕后调用该回调函数
 * 将该封装好的 RTMPPacket 数据包放入线程安全队列中
 * typedef void (*RTMPPacketPackUpCallBack)(RTMPPacket* packet);
 */
void RTMPPacketPackUpCallBack(RTMPPacket* rtmpPacket){
    if (rtmpPacket) {
        rtmpPacket->m_nTimeStamp = RTMP_GetTime() - pushStartTime;
        packets.push(rtmpPacket);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_rtmp_LivePusher_native_1init(JNIEnv *env, jobject thiz) {
    // 0. 将 x264 编码的过程, RTMPDump 的编码过程, 封装到单独的工具类中
    //    使用该工具类, 对数据进行编码
    vedioChannel = new VedioChannel;


    // 1. 数据队列, 用于存储打包好的数据
    //    在单独的线程中将该队列中的数据发送给服务器
    packets.setReleaseHandle(releaseRTMPPackets);
}


/**
 * 设置视频编码参数
 */
extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_rtmp_LivePusher_native_1setVideoEncoderParameters(JNIEnv *env, jobject thiz,
                                                               jint width, jint height, jint fps,
                                                               jint bitrate) {
    vedioChannel->setVideoEncoderParameters(width, height, fps, bitrate);
}


/**
 * 开始推流任务线程
 * 主要是调用 RTMPDump 进行推流
 * @param args
 * @return
 */
void* startRtmpPush (void* args){
    // 0. 获取 Rtmp 推流地址
    char* pushPath = static_cast<char *>(args);

    // rtmp 推流器
    RTMP* rtmp = 0;
    // rtmp 推流数据包
    RTMPPacket *packet = 0;

    /*
        将推流核心执行内容放在 do while 循环中
        在出错后, 随时 break 退出循环, 执行后面的释放资源的代码
        可以保证, 在最后将资源释放掉, 避免内存泄漏
        避免执行失败, 直接 return, 导致资源没有释放
     */
    do {
        // 1. 创建 RTMP 对象, 申请内存
        rtmp = RTMP_Alloc();
        if (!rtmp) {
            __android_log_print(ANDROID_LOG_INFO, "RTMP", "申请 RTMP 内存失败");
            break;
        }

        // 2. 初始化 RTMP
        RTMP_Init(rtmp);
        // 设置超时时间 5 秒
        rtmp->Link.timeout = 5;

        // 3. 设置 RTMP 推流服务器地址
        int ret = RTMP_SetupURL(rtmp, pushPath);
        if (!ret) {
            __android_log_print(ANDROID_LOG_INFO, "RTMP", "设置 RTMP 推流服务器地址 %s 失败", pushPath);
            break;
        }

        // 4. 启用 RTMP 写出功能
        RTMP_EnableWrite(rtmp);

        // 5. 连接 RTMP 服务器
        ret = RTMP_Connect(rtmp, 0);
        if (!ret) {
            __android_log_print(ANDROID_LOG_INFO, "RTMP", "连接 RTMP 服务器 %s 失败", pushPath);
            break;
        }

        // 6. 连接 RTMP 流
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) {
            __android_log_print(ANDROID_LOG_INFO, "RTMP", "连接 RTMP 流 %s 失败", pushPath);
            break;
        }

        // 准备推流相关的数据, 如线程安全队列
        readyForPush = TRUE;
        // 记录推流开始时间
        pushStartTime = RTMP_GetTime();
        // 线程安全队列开始工作
        packets.setWork(1);

        while (isStartRtmpPush) {
            // 从线程安全队列中
            // 取出一包已经打包好的 RTMP 数据包
            packets.pop(packet);

            // 确保当前处于推流状态
            if (!isStartRtmpPush) {
                break;
            }

            // 确保不会取出空的 RTMP 数据包
            if (!packet) {
                continue;
            }

            // 设置直播的流 ID
            packet->m_nInfoField2 = rtmp->m_stream_id;

            // 7. 将 RTMP 数据包发送到服务器中
            ret = RTMP_SendPacket(rtmp, packet, 1);

            // RTMP 数据包使用完毕后, 释放该数据包
            if (packet) {
                RTMPPacket_Free(packet);
                delete packet;
                packet = 0;
            }

            if (!ret) {
                __android_log_print(ANDROID_LOG_INFO, "RTMP", "RTMP 数据包推流失败");
                break;
            }
        }

    }while (0);


    // 面的部分是收尾部分, 释放资源


    // 8. 推流结束, 关闭与 RTMP 服务器连接, 释放资源
    if(rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }

    // 推流数据包 线程安全队列释放
    // 防止中途退出导致没有释放资源, 造成内存泄漏
    if (packet) {
        RTMPPacket_Free(packet);
        delete packet;
        packet = 0;
    }

    // 释放推流地址
    if(pushPath){
        delete pushPath;
        pushPath = 0;
    }
    return 0;
}

/**
 * 开始向远程 RTMP 服务器推送数据
 */
extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_rtmp_LivePusher_native_1startRtmpPush(JNIEnv *env, jobject thiz,
                                                                jstring path) {
    if(isStartRtmpPush){
        // 防止该方法多次调用, 如果之前调用过, 那么屏蔽本次调用
        return;
    }
    // 执行过一次后, 马上标记已执行状态, 下一次就不再执行该方法了
    isStartRtmpPush = TRUE;

    // 获取 Rtmp 推流地址
    // 该 pushPathFromJava 引用是局部引用, 超过作用域就无效了
    // 局部引用不能跨方法 , 跨线程调用
    const char* pushPathFromJava = env->GetStringUTFChars(path, 0);

    // 获取地址的长度, 加上 '\0' 长度
    char * pushPathNative = new char[strlen(pushPathFromJava) + 1];
    // 拷贝 pushPathFromJava 到堆内存 pushPathNative 中
    // 局部引用不能跨方法 , 跨线程调用, 这里需要在线程中使用该地址
    // 因此需要将该局部引用拷贝到堆内存中, 然后传递到对应线程中
    strcpy(pushPathNative, pushPathFromJava);

    // 创建线程
    pthread_create(&startRtmpPushPid, 0, startRtmpPush, pushPathNative);

    // 释放从 Java 层获取的字符串
    // 释放局部引用
    env->ReleaseStringUTFChars(path, pushPathFromJava);
}


extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_rtmp_LivePusher_native_1encodeCameraData(JNIEnv *env, jobject thiz, jbyteArray data) {
    if(!vedioChannel || !readyForPush){
        // 如果 vedioChannel 还没有进行初始化, 推流没有准备好了, 直接 return
        __android_log_print(ANDROID_LOG_INFO, "RTMP", "还没有准备完毕, 稍后再尝试调用该方法");
        return;
    }

    // 将 Java 层的 byte 数组类型 jbyteArray 转为 jbyte* 指针类型
    // 注意这是局部引用变量, 不能跨线程, 跨方法调用, 需要将其存放在堆内存中
    jbyte* dataFromJava = env->GetByteArrayElements(data, NULL);

    // jbyte 是 int8_t 类型的, 因此这里我们将 encodeCameraData 的参数设置成 int8_t* 类型
    // typedef int8_t   jbyte;    /* signed 8 bits */
    vedioChannel->encodeCameraData(dataFromJava);

    // 释放局部引用变量
    env->ReleaseByteArrayElements(data, dataFromJava, 0);
}


