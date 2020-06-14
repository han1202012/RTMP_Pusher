package kim.hsl.rtmp;

import android.app.Activity;
import android.view.SurfaceHolder;

public class LivePusher {
    static {
        System.loadLibrary("native-lib");
    }

    /**
     * 处理音频的通道
     */
    private AudioChannel mAudioChannel;

    /**
     * 处理视频的通道
     */
    private VideoChannel mVideoChannel;

    /**
     * 是否已经开启过直播, 使用该变量, 控制开启直播的方法只调用一次
     */
    private boolean mIsStartLive = false;

    /**
     * 创建直播推流器
     * @param activity
     *          初始化推流功能的界面
     * @param width
     *          图像宽度
     * @param height
     *          图像高度
     * @param bitrate
     *          视频码率
     * @param fps
     *          视频帧率
     * @param cameraId
     *          指定摄像头
     */
    public LivePusher(Activity activity, int width, int height, int bitrate,
                      int fps, int cameraId) {
        // 初始化 native 层的环境
        native_init();
        // 初始化视频处理通道
        mVideoChannel = new VideoChannel(this, activity, width, height, bitrate, fps, cameraId);
        // 初始化音频处理通道
        mAudioChannel = new AudioChannel();
    }

    /**
     * 设置图像显示组件
     * @param surfaceHolder
     */
    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        mVideoChannel.setPreviewDisplay(surfaceHolder);
    }

    /**
     * 调用该方法可以切换摄像头
     */
    public void switchCamera() {
        mVideoChannel.switchCamera();
    }

    /**
     * 调用该方法 , 就会启动推流过程
     */
    public void startLive(String rtmpPushPath) {
        if(mIsStartLive){
            // 如果直播已经开启过, 那么该方法就不再调用
            return;
        }else{
            // 如果直播没有开启, 点击开启, 并设置标志, 表明直播已经开启
            mIsStartLive = true;
        }

        native_startRtmpPush(rtmpPushPath);
        mVideoChannel.startLive();
        mAudioChannel.startLive();
    }

    /**
     * 停止推流方法
     */
    public void stopLive(){
        mVideoChannel.stopLive();
        mAudioChannel.stopLive();
        native_stopPush();
    }


    /**
     * 初始化 NDK 环境
     */
    public native void native_init();

    /**
     * 开始向 RTMP 服务器推送数据
     * 连接远程 RTMP 服务器, 向该服务器推送数据
     * @param path
     */
    public native void native_startRtmpPush(String path);

    /**
     * 设置视频编码参数
     * @param width
     *          宽度
     * @param height
     *          高度
     * @param fps
     *          帧率
     * @param bitrate
     *          码率
     */
    public native void native_setVideoEncoderParameters(int width, int height, int fps, int bitrate);

    /**
     * 执行数据编码操作
     * @param data
     */
    public native void native_encodeCameraData(byte[] data);

    public native void native_stopPush();

    public native void native_release();
}
