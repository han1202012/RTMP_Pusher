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
    private AudioChannel audioChannel;

    /**
     * 处理视频的通道
     */
    private VideoChannel videoChannel;

    /**
     *
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
        native_init();
        videoChannel = new VideoChannel(this, activity, width, height, bitrate, fps, cameraId);
        audioChannel = new AudioChannel();
    }

    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        videoChannel.setPreviewDisplay(surfaceHolder);
    }

    public void switchCamera() {
        videoChannel.switchCamera();
    }

    public void startLive(String path) {
        native_start(path);
        videoChannel.startLive();
        audioChannel.startLive();
    }

    public void stopLive(){
        videoChannel.stopLive();
        audioChannel.stopLive();
        native_stop();
    }


    public native void native_init();

    public native void native_start(String path);

    public native void native_setVideoEncInfo(int width, int height, int fps, int bitrate);

    public native void native_pushVideo(byte[] data);

    public native void native_stop();

    public native void native_release();
}
