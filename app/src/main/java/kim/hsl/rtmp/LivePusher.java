package kim.hsl.rtmp;

import android.app.Activity;
import android.view.SurfaceHolder;

import kim.hsl.rtmp.channel.AudioChannel;
import kim.hsl.rtmp.channel.VideoChannel;

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
        //native_init();
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

    /*public void switchCamera() {
        mVideoChannel.switchCamera();
    }

    public void startLive(String path) {
        native_start(path);
        mVideoChannel.startLive();
        mAudioChannel.startLive();
    }

    public void stopLive(){
        mVideoChannel.stopLive();
        mAudioChannel.stopLive();
        native_stop();
    }


    public native void native_init();

    public native void native_start(String path);

    public native void native_setVideoEncInfo(int width, int height, int fps, int bitrate);

    public native void native_pushVideo(byte[] data);

    public native void native_stop();

    public native void native_release();*/
}
