package kim.hsl.rtmp.channel;

import android.app.Activity;
import android.hardware.Camera;
import android.view.SurfaceHolder;

import kim.hsl.rtmp.LivePusher;


public class VideoChannel implements Camera.PreviewCallback, CameraManager.OnChangedSizeListener {


    private LivePusher mLivePusher;
    private CameraManager mCameraHelper;
    private int mBitrate;
    private int mFps;

    /**
     * 当前是否在直播
     */
    private boolean mIsLiving;

    public VideoChannel(LivePusher livePusher, Activity activity, int width, int height, int bitrate, int fps, int cameraId) {
        mLivePusher = livePusher;
        mBitrate = bitrate;
        mFps = fps;

        // 1. 初始化 Camera 相关参数
        mCameraHelper = new CameraManager(activity, cameraId, width, height);
        // 2. 设置 Camera 预览数据回调接口
        //    通过该接口可以在本类中的实现的 onPreviewFrame 方法中
        //    获取到 NV21 数据
        mCameraHelper.setPreviewCallback(this);
        // 3. 通过该回调接口, 可以获取到真实的 Camera 尺寸数据
        //    设置摄像头预览尺寸完成后, 会回调该接口
        mCameraHelper.setOnChangedSizeListener(this);
    }

    /**
     * 设置预览图像画布
     * @param surfaceHolder
     */
    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        mCameraHelper.setPreviewDisplay(surfaceHolder);
    }


    /**
     * Camera 摄像头采集数据完毕, 通过回调接口传回数据
     * 数据格式是 nv21 格式的
     * @param data
     * @param camera
     */
    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mIsLiving) {
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
        mIsLiving = true;
    }

    public void stopLive() {
        mIsLiving = false;
    }
}
