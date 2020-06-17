//
// Created by octopus on 2020/6/17.
//

#include "AudioChannel.h"



/**
 * 打包 RTMP 包后的回调函数
 * @param rtmpPacketPackUpCallBack
 */
void AudioChannel::setRTMPPacketPackUpCallBack(RTMPPacketPackUpCallBack rtmpPacketPackUpCallBack){
    this->rtmpPacketPackUpCallBack = rtmpPacketPackUpCallBack;
}