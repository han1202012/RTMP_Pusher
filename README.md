# RTMP_Pusher

@[TOC]


<br>
<br>

**Android 直播推流流程 :** <font color=blue>手机采集视频 / 音频数据 , <font color=red>视频数据使用 H.264 编码 , <font color=green>音频数据使用 AAC 编码 , <font color=purple>最后将音视频数据都打包到 RTMP 数据包中 , <font color=orange>使用 RTMP 协议上传到 RTMP 服务器中 ; 

<br>

**视频推流 :** 之前的一系列博客中完成<font color=blue>手机端采集视频数据操作 , <font color=green>并将视频数据传递给 JNI , <font color=red>在 NDK 中使用 x264 将图像转为 H.264 格式的视频 , <font color=purple>最后将 H.264 格式的视频打包到 RTMP 数据包中 , <font color=orange>上传到 RTMP 服务器中 ; 

<br>

**音频推流 :** 开始进行音频直播推流操作 , <font color=blue>先采集音频 , <font color=red>将音频编码为 AAC 格式 ,<font color=purple> 将编码后的音频打包成 RTMP 包 , <font color=orange>然后推流到服务器中 ; 

<br>
<br>
<br>
<br>

# 一、 安卓直播推流专栏博客总结

---

<br>


**[Android RTMP 直播推流技术专栏](https://blog.csdn.net/han1202012/category_10087360.html) :** 

<br>

**0 . 资源和源码地址 :** 

 - **资源下载地址 :** [资源下载地址](https://download.csdn.net/download/han1202012/12536735) , 服务器搭建 , x264 , faac , RTMPDump , 源码及交叉编译库 , 本专栏 Android 直播推流源码 ; 
 - **GitHub 源码地址 :** [han1202012 / RTMP_Pusher](https://github.com/han1202012/RTMP_Pusher)

<br>

**1. 搭建 RTMP 服务器 :** **下面的博客中讲解了如何在 VMWare 虚拟机中搭建 RTMP 直播推流服务器 ;** 


 - [【Android RTMP】RTMP 直播推流服务器搭建 ( Ubuntu 18.04.4 虚拟机 )](https://blog.csdn.net/shulianghan/article/details/106622762)


**2. 准备视频编码的 x264 编码器开源库 , 和 RTMP 数据包封装开源库 :** 

 - [【Android RTMP】RTMPDumb 源码导入 Android Studio ( 交叉编译 | 配置 CMakeList.txt 构建脚本 )](https://blog.csdn.net/shulianghan/article/details/106635412)


 - [【Android RTMP】Android Studio 集成 x264 开源库 ( Ubuntu 交叉编译 | Android Studio 导入函数库 )](https://blog.csdn.net/shulianghan/article/details/106648519)


**3. 讲解 RTMP 数据包封装格式 :** 

 - [【Android RTMP】RTMP 数据格式 ( FLV 视频格式分析 | 文件头 Header 分析 | 标签 Tag 分析 | 视频标签 Tag 数据分析 )](https://blog.csdn.net/shulianghan/article/details/106652147)

 - [【Android RTMP】RTMP 数据格式 ( FLV 视频格式分析 | AVC 序列头格式解析 )](https://blog.csdn.net/shulianghan/article/details/106662537)


**4. 图像数据采集 :** **从 Camera 摄像头中采集 NV21 格式的图像数据 , 并预览该数据 ;** 

 - [【Android RTMP】Android Camera 视频数据采集预览 ( 视频采集相关概念 | 摄像头预览参数设置 | 摄像头预览数据回调接口 )](https://blog.csdn.net/shulianghan/article/details/106675696)

 - [【Android RTMP】Android Camera 视频数据采集预览 ( NV21 图像格式 | I420 图像格式 | NV21 与 I420 格式对比 | NV21 转 I420 算法 )](https://blog.csdn.net/shulianghan/article/details/106688583)

 - [【Android RTMP】Android Camera 视频数据采集预览 ( 图像传感器方向设置 | Camera 使用流程 | 动态权限申请 )](https://blog.csdn.net/shulianghan/article/details/106698988)


**5. NV21 格式的图像数据编码成 H.264 格式的视频数据 :** 

 - [【Android RTMP】x264 编码器初始化及设置 ( 获取 x264 编码参数 | 编码规格 | 码率 | 帧率 | B帧个数 | 关键帧间隔 | 关键帧解码数据 SPS PPS )](https://blog.csdn.net/shulianghan/article/details/106713072)


 - [【Android RTMP】x264 图像数据编码 ( Camera 图像数据采集 | NV21 图像数据传到 Native 处理 | JNI 传输字节数组 | 局部引用变量处理 | 线程互斥 )](https://blog.csdn.net/shulianghan/article/details/106732417)

 - [【Android RTMP】x264 图像数据编码 ( NV21 格式中的 YUV 数据排列 | Y 灰度数据拷贝 | U 色彩值数据拷贝 | V 饱和度数据拷贝 | 图像编码操作 )](https://blog.csdn.net/shulianghan/article/details/106733480)


**6. 将 H.264 格式的视频数据封装到 RTMP 数据包中 :** 

 - [【Android RTMP】RTMPDump 封装 RTMPPacket 数据包 ( 封装 SPS / PPS 数据包 )](https://blog.csdn.net/shulianghan/article/details/106737015)


 - [【Android RTMP】RTMPDump 封装 RTMPPacket 数据包 ( 关键帧数据格式 | 非关键帧数据格式 | x264 编码后的数据处理 | 封装 H.264 视频数据帧 )](https://blog.csdn.net/shulianghan/article/details/106739259)

 - [【Android RTMP】RTMPDump 推流过程 ( 独立线程推流 | 创建推流器 | 初始化操作 | 设置推流地址 | 启用写出 | 连接 RTMP 服务器 | 发送 RTMP 数据包 )](https://blog.csdn.net/shulianghan/article/details/106725931)


**7. 阶段总结 : 阿里云服务器中搭建 RTMP 服务器 , 并使用电脑软件推流和观看直播内容 ;** 


 - [【Android RTMP】RTMP 直播推流 ( 阿里云服务器购买 | 远程服务器控制 | 搭建 RTMP 服务器 | 服务器配置 | 推流软件配置 | 直播软件配置 | 推流直播效果展示 )](https://blog.csdn.net/shulianghan/article/details/106634821)


 - [【Android RTMP】RTMP 直播推流阶段总结 ( 服务器端搭建 | Android 手机端编码推流 | 电脑端观看直播 | 服务器状态查看 )](https://blog.csdn.net/shulianghan/article/details/106752312)


**8. 处理 Camera 图像传感器导致的 NV21 格式图像旋转问题 :** 


 - [【Android RTMP】NV21 图像旋转处理 ( 问题描述 | 图像顺时针旋转 90 度方案 | YUV 图像旋转细节 | 手机屏幕旋转方向 )](https://blog.csdn.net/shulianghan/article/details/106756811)

 - [【Android RTMP】NV21 图像旋转处理 ( 图像旋转算法 | 后置摄像头顺时针旋转 90 度 | 前置摄像头顺时针旋转 90 度 )](https://blog.csdn.net/shulianghan/article/details/106759250)


**9. 下面这篇博客比较重要 , 里面有一个快速搭建 RTMP 服务器的脚本 , 强烈建议使用 ;** 

 - [【Android RTMP】NV21 图像旋转处理 ( 快速搭建 RTMP 服务器 Shell 脚本 | 创建 RTMP 服务器镜像 | 浏览器观看直播 | 前置 / 后置摄像头图像旋转效果展示 )](https://blog.csdn.net/shulianghan/article/details/106773500)


**10. 编码 AAC 音频数据的开源库 FAAC 交叉编译与 Android Studio 环境搭建 :**

 - [【Android RTMP】音频数据采集编码 ( 音频数据采集编码 | AAC 高级音频编码 | FAAC 编码器 | Ubuntu 交叉编译 FAAC 编码器 )](https://blog.csdn.net/shulianghan/article/details/106792955)

 - [【Android RTMP】音频数据采集编码 ( FAAC 头文件与静态库拷贝到 AS | CMakeList.txt 配置 FAAC | AudioRecord 音频采样 PCM 格式 )](https://blog.csdn.net/shulianghan/article/details/106795757)


**11. 解析 AAC 音频格式 :** 


 - [【Android RTMP】音频数据采集编码 ( AAC 音频格式解析 | FLV 音频数据标签解析 | AAC 音频数据标签头 | 音频解码配置信息 )](https://blog.csdn.net/shulianghan/article/details/106802700) 


**12 . 将麦克风采集的 PCM 音频采样编码成 AAC 格式音频 , 并封装到 RTMP 包中 , 推流到客户端 :** 

 - [【Android RTMP】音频数据采集编码 ( FAAC 音频编码参数设置 | FAAC 编码器创建 | 获取编码器参数 | 设置 AAC 编码规格 | 设置编码器输入输出参数 )](https://blog.csdn.net/shulianghan/article/details/106817750)


 - [【Android RTMP】音频数据采集编码 ( FAAC 编码器编码 AAC 音频解码信息 | 封装 RTMP 音频数据头 | 设置 AAC 音频数据类型 | 封装 RTMP 数据包 )](https://blog.csdn.net/shulianghan/article/details/106826659)


 - [【Android RTMP】音频数据采集编码 ( FAAC 编码器编码 AAC 音频采样数据 | 封装 RTMP 音频数据头 | 设置 AAC 音频数据类型 | 封装 RTMP 数据包 )](https://blog.csdn.net/shulianghan/article/details/106844248)


<br>
<br>
<br>
<br>

# 二、 相关资源介绍

---

<br>

**下载地址 :** [https://download.csdn.net/download/han1202012/12536735](https://download.csdn.net/download/han1202012/12536735)

<br>

整理了 RTMP 专栏中的博客中的资源 , 从服务器搭建的 Nginx 服务器 , 到使用的 x264 , RTMPDump , FAAC 开源库源码 , 及交叉编译结果 , 还有分析 RTMP 文件格式工具 , 以及可运行的 Android 应用源码 ( 修改下 服务器的 IP 地址 , 即可进行直播推流操作 ) ;  

<br>

**001_服务器搭建需要上传的文件** 

**002_远程Linux控制工具**

这是服务器搭建需要的文件 , 按照 [【Android RTMP】NV21 图像旋转处理 ( 快速搭建 RTMP 服务器 Shell 脚本 | 创建 RTMP 服务器镜像 | 浏览器观看直播 | 前置 / 后置摄像头图像旋转效果展示 )](https://blog.csdn.net/shulianghan/article/details/106773500) 博客中的操作说明搭建即可 ; 

<br>


**003_Android_应用程序_源码**

整个直播推流的 Android 端源码 , 包含已经交叉编译后的 x264 , faac 静态库 , RTMPDump 包源文件 , Camera 图像采集 , H.264 视频编码 , RTMP 推流 , 麦克风采集 PCM 音频样本, FAAC 音频编码为 AAC 格式 , RTMP 打包推流 , 整个过程 ; 

<br>

**005_RTMPDump_源码_直接拷贝到AS使用**

直接使用即可 , 已经集成到了 Android 应用中 , 参考该博客内容 [【Android RTMP】RTMPDumb 源码导入 Android Studio ( 交叉编译 | 配置 CMakeList.txt 构建脚本 )](https://blog.csdn.net/shulianghan/article/details/106635412) ; 

<br>

**006_x264_源码**

**007_x264_编译好的Android函数库**

这是 x264 源码和已经交叉编译好的 Android 平台可用的静态库 , 如果想要自己在 Ubuntu 中交叉编译一下 , 参考博客 [【Android RTMP】Android Studio 集成 x264 开源库 ( Ubuntu 交叉编译 | Android Studio 导入函数库 )](https://blog.csdn.net/shulianghan/article/details/106648519)


<br>

**008_FAAC_源码**

**009_FAAC_交叉编译后的静态库**

这是 FAAC 源码和已经交叉编译好的 Android 平台可用的静态库 , 如果想要自己在 Ubuntu 中交叉编译一下 , 参考博客 
[【Android RTMP】音频数据采集编码 ( 音频数据采集编码 | AAC 高级音频编码 | FAAC 编码器 | Ubuntu 交叉编译 FAAC 编码器 )](https://blog.csdn.net/shulianghan/article/details/106792955) 
 [【Android RTMP】音频数据采集编码 ( FAAC 头文件与静态库拷贝到 AS | CMakeList.txt 配置 FAAC | AudioRecord 音频采样 PCM 格式 )](https://blog.csdn.net/shulianghan/article/details/106795757)

<br>

**010_二进制查看工具**

**011_FLV_视频文件分析工具**

**012_FLV格式视频文件**


<br>

分析 RTMP 数据包时需要的工具 , 具体的数据信息我已经在博客中截图下来了 , 如果自己想要查看 , 自行下载分析 ; 

参考博客 
[【Android RTMP】RTMP 数据格式 ( FLV 视频格式分析 | 文件头 Header 分析 | 标签 Tag 分析 | 视频标签 Tag 数据分析 )](https://blog.csdn.net/shulianghan/article/details/106652147)
[【Android RTMP】RTMP 数据格式 ( FLV 视频格式分析 | AVC 序列头格式解析 )](https://blog.csdn.net/shulianghan/article/details/106662537)
[【Android RTMP】音频数据采集编码 ( AAC 音频格式解析 | FLV 音频数据标签解析 | AAC 音频数据标签头 | 音频解码配置信息 )](https://blog.csdn.net/shulianghan/article/details/106802700) 








<br>
<br>
<br>
<br>

# 三、 GitHub 源码地址

---

<br>




[han1202012 / RTMP_Pusher](https://github.com/han1202012/RTMP_Pusher)






<br>
<br>
<br>
<br>

# 四、 整体 Android 直播推流数据到服务器并观看直播演示过程

---

<br>

**1 . 服务器搭建 :** 按照

[【Android RTMP】RTMP 直播推流服务器搭建 ( Ubuntu 18.04.4 虚拟机 )](https://blog.csdn.net/shulianghan/article/details/106622762)
[【Android RTMP】RTMP 直播推流 ( 阿里云服务器购买 | 远程服务器控制 | 搭建 RTMP 服务器 | 服务器配置 | 推流软件配置 | 直播软件配置 | 推流直播效果展示 )](https://blog.csdn.net/shulianghan/article/details/106634821)
 [【Android RTMP】RTMP 直播推流阶段总结 ( 服务器端搭建 | Android 手机端编码推流 | 电脑端观看直播 | 服务器状态查看 )](https://blog.csdn.net/shulianghan/article/details/106752312)

 博客中的操作 , 购买阿里云服务器 , 或者在本地虚拟机中运行该博客中的一键安装脚本 ; 

<br>

**2 . Android 客户端修改 :** 从 GitHub 或者 CSDN 资源下载处获取源码 , 将 RTMP_Pusher 项目中的地址修改成你的服务器地址 , 即可点击开始推流 , 即可开始直播推流 ; 

修改源码 : 在主界面 MainActivity 中 , 将 IP 地址直接替换成你搭建的服务器 IP 地址 , 即可开始直播推流操作 ; 

![在这里插入图片描述](https://img-blog.csdnimg.cn/20200619220931567.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2hhbjEyMDIwMTI=,size_16,color_FFFFFF,t_70)


<br>


**3 . 服务器端网页修改 :** 

参考 [【Android RTMP】NV21 图像旋转处理 ( 快速搭建 RTMP 服务器 Shell 脚本 | 创建 RTMP 服务器镜像 | 浏览器观看直播 | 前置 / 后置摄像头图像旋转效果展示 ) __ 四、浏览器查看直播内容](https://blog.csdn.net/shulianghan/article/details/106773500#_205) 内容 , 修改服务器端 /root/rtmp/nginx-rtmp-module-1.2.1/test/www/index.html 页面中的直播源 IP 地址为搭建的服务器 IP 地址 ; 

此时使用 RTMP 服务器提供的网页端的 JWPlayer 播放 Android 手机端推流上去的视频 ; 

<br>

**4 . 手机端开启直播 :** 

![在这里插入图片描述](https://img-blog.csdnimg.cn/20200619223553964.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2hhbjEyMDIwMTI=,size_16,color_FFFFFF,t_70)


<br>

**5 . 查看服务器端直播状态 :** 可以看到 视频 和 音频都推流到了服务器中 ; 

![在这里插入图片描述](https://img-blog.csdnimg.cn/20200619223429208.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2hhbjEyMDIwMTI=,size_16,color_FFFFFF,t_70)

<br>

**6 . 服务器端 JWPlayer 查看直播内容 :** 

![在这里插入图片描述](https://img-blog.csdnimg.cn/20200619223505308.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2hhbjEyMDIwMTI=,size_16,color_FFFFFF,t_70)
