#include <jni.h>
#include <string>
#include "librtmp/rtmp.h"
#include <x264.h>

#include "SafeQueue.h"

/**
 * RTMPPacket 结构体是打包好的 RTMP 数据包
 * 将该数据包发送到 RTMP 服务器中
 */
SafeQueue<RTMPPacket *> packets;

extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_rtmp_LivePusher_native_1init(JNIEnv *env, jobject thiz) {
    // 0. 将 x264 编码的过程, RTMPDump 的编码过程, 封装到单独的工具类中
    //    使用该工具类, 对数据进行编码


    // 1. 数据队列, 用于存储打包好的数据
    //    在单独的线程中将该队列中的数据发送给服务器
}