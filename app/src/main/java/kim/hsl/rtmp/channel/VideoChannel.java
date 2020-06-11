package kim.hsl.rtmp.channel;

import android.app.Activity;
import android.hardware.Camera;
import android.view.SurfaceHolder;

import kim.hsl.rtmp.LivePusher;


public class VideoChannel implements Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {


    private LivePusher mLivePusher;
    private CameraHelper mCameraHelper;
    private int mBitrate;
    private int mFps;
    private boolean isLiving;

    public VideoChannel(LivePusher livePusher, Activity activity, int width, int height, int bitrate, int fps, int cameraId) {
        mLivePusher = livePusher;
        mBitrate = bitrate;
        mFps = fps;

        // 初始化 Camera 相关参数
        mCameraHelper = new CameraHelper(activity, cameraId, width, height);
        mCameraHelper.setPreviewCallback(this);
        mCameraHelper.setOnChangedSizeListener(this);
    }

    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        mCameraHelper.setPreviewDisplay(surfaceHolder);
    }


    /**
     * 得到nv21数据 已经旋转好的
     *
     * @param data
     * @param camera
     */
    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (isLiving) {
            mLivePusher.native_pushVideo(data);
        }
    }

    public void switchCamera() {
        mCameraHelper.switchCamera();
    }

    /**
     * 真实摄像头数据的宽、高
     *
     * @param w
     * @param h
     */
    @Override
    public void onChanged(int w, int h) {
        //初始化编码器
        mLivePusher.native_setVideoEncInfo(w, h, mFps, mBitrate);
    }

    public void startLive() {
        isLiving = true;
    }

    public void stopLive() {
        isLiving = false;
    }
}
