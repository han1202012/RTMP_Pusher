#include <jni.h>
#include <string>
#include "librtmp/rtmp.h"
#include <x264.h>

extern "C" JNIEXPORT jstring JNICALL
Java_kim_hsl_rtmp_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    RTMP_Alloc();
    x264_picture_t *p = new x264_picture_t;
    return env->NewStringUTF(hello.c_str());
}
