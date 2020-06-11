package kim.hsl.rtmp.channel;

import android.app.Activity;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.util.Iterator;
import java.util.List;

/**
 * 该类中封装了对 Camera 对象的一些操作
 */
public class CameraHelper implements SurfaceHolder.Callback, Camera.PreviewCallback {

    private static final String TAG = "CameraHelper";

    /**
     * 摄像头显示数据组件所在的界面
     * 主要是为了方便的获取界面参数
     */
    private Activity mActivity;
    private int mHeight;
    private int mWidth;

    /**
     * 摄像头选择, 前置摄像头还是后置像头
     * 前置摄像头 : Camera.CameraInfo.CAMERA_FACING_FRONT
     * 后置摄像头 : Camera.CameraInfo.CAMERA_FACING_BACK
     */
    private int mCameraFacing;

    /**
     * 摄像头
     */
    private Camera mCamera;

    private byte[] buffer;
    private SurfaceHolder mSurfaceHolder;
    private Camera.PreviewCallback mPreviewCallback;
    private int mRotation;
    private OnChangedSizeListener mOnChangedSizeListener;

    public CameraHelper(Activity activity, int cameraId, int width, int height) {
        mActivity = activity;
        mCameraFacing = cameraId;
        mWidth = width;
        mHeight = height;
    }

    public void switchCamera() {
        if (mCameraFacing == Camera.CameraInfo.CAMERA_FACING_BACK) {
            mCameraFacing = Camera.CameraInfo.CAMERA_FACING_FRONT;
        } else {
            mCameraFacing = Camera.CameraInfo.CAMERA_FACING_BACK;
        }
        stopPreview();
        startPreview();
    }

    /**
     * 释放 Camera 摄像头
     */
    private void stopPreview() {
        if (mCamera != null) {
            // 下面的 API 都是 Android 提供的
            // 1. 设置预览回调接口, 这里设置 null 即可
            mCamera.setPreviewCallback(null);
            // 2. 停止图像数据预览
            mCamera.stopPreview();
            // 3. 释放置空
            mCamera.release();
            mCamera = null;
        }
    }

    /**
     * 开启 Camera 摄像头
     */
    private void startPreview() {
        try {
            // 1. 打开指定方向的 Camera 摄像头
            mCamera = Camera.open(mCameraFacing);
            // 2. 获取 Camera 摄像头参数, 之后需要修改配置该参数
            Camera.Parameters parameters = mCamera.getParameters();
            // 3. 设置预览数据格式为nv21
            parameters.setPreviewFormat(ImageFormat.NV21);
            //这是摄像头宽、高
            setPreviewSize(parameters);
            // 设置摄像头 图像传感器的角度、方向
            setPreviewOrientation(parameters);
            mCamera.setParameters(parameters);
            buffer = new byte[mWidth * mHeight * 3 / 2];
            //数据缓存区
            mCamera.addCallbackBuffer(buffer);
            mCamera.setPreviewCallbackWithBuffer(this);
            //设置预览画面
            mCamera.setPreviewDisplay(mSurfaceHolder);
            mOnChangedSizeListener.onChanged(mWidth, mHeight);
            mCamera.startPreview();
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private void setPreviewOrientation(Camera.Parameters parameters) {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(mCameraFacing, info);
        mRotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (mRotation) {
            case Surface.ROTATION_0:
                degrees = 0;
                break;
            case Surface.ROTATION_90: // 横屏 左边是头部(home键在右边)
                degrees = 90;
                break;
            case Surface.ROTATION_180:
                degrees = 180;
                break;
            case Surface.ROTATION_270:// 横屏 头部在右边
                degrees = 270;
                break;
        }
        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360; // compensate the mirror
        } else { // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        //设置角度
        mCamera.setDisplayOrientation(result);
    }

    /**
     * 设置 Camera 摄像头的参数
     * 宽度, 高度
     *
     * 摄像头支持的宽高值是固定的, 不能人为的随意设置
     * 手机给出一组支持的宽高值, 可以选择其中的某一个进行设置
     *
     * 用户虽然设置了一个宽高值, 这个宽高值肯定不能直接设置给 Camera 摄像头
     * 需要对比 Camera 支持的一组宽高值, 哪一个与用户设置的最接近
     * 这个最相似的宽高值就是我们要设置的值
     *
     * 对比方法 : 对比像素总数
     * 用户设置像素总数 : 用户设置的 宽 高 像素值相乘, 就是用户设置像素总数
     * 系统支持像素总数 : 屏幕支持的 宽 高 像素值相乘, 就是系统支持的某个宽高的像素总数
     *
     * 找出上述 用户设置像素总数 和 系统支持像素总数 最接近的的那个 系统支持像素总数
     *      对应的 屏幕支持的 宽 高 值
     *
     * @param parameters
     */
    private void setPreviewSize(Camera.Parameters parameters) {
        // 1. 获取摄像头参数中的预览图像大小参数
        List<Camera.Size> supportedPreviewSizes = parameters.getSupportedPreviewSizes();


        // 下面开始遍历获取与用户设置的宽高值最接近的, Camera 支持的宽高值


        // 2. 获取系统 Camera 摄像头支持的最低的摄像头
        //      Camera.Size 中有宽度和高度参数
        Camera.Size size = supportedPreviewSizes.get(0);
        int m = Math.abs(size.height * size.width - mWidth * mHeight);
        supportedPreviewSizes.remove(0);

        Iterator<Camera.Size> iterator = supportedPreviewSizes.iterator();
        //遍历
        while (iterator.hasNext()) {
            Camera.Size next = iterator.next();
            Log.d(TAG, "支持 " + next.width + "x" + next.height);
            int n = Math.abs(next.height * next.width - mWidth * mHeight);
            if (n < m) {
                m = n;
                size = next;
            }
        }
        mWidth = size.width;
        mHeight = size.height;
        parameters.setPreviewSize(mWidth, mHeight);
        Log.d(TAG, "设置预览分辨率 width:" + size.width + " height:" + size.height);
    }


    /**
     * 设置预览组件
     * @param surfaceHolder
     */
    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        mSurfaceHolder = surfaceHolder;

        // 设置相关的回调接口
        // 在 mSurfaceHolder 对应的组件创建, 画布大小改变, 销毁时, 回调相应的接口方法
        mSurfaceHolder.addCallback(this);
    }

    public void setPreviewCallback(Camera.PreviewCallback previewCallback) {
        mPreviewCallback = previewCallback;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) { }

    /**
     * 处理画布改变
     *
     * 按下 Home 键, 退出界面, 横竖屏切换, 等操作
     *
     * @param holder
     * @param format
     * @param width
     * @param height
     */
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // 先释放 Camera, 然后重新启动
        stopPreview();
        startPreview();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopPreview();
    }


    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        // data数据依然是倒的
        mPreviewCallback.onPreviewFrame(data, camera);
        camera.addCallbackBuffer(buffer);
    }



    public void setOnChangedSizeListener(OnChangedSizeListener listener) {
        mOnChangedSizeListener = listener;
    }

    public interface OnChangedSizeListener {
        void onChanged(int w, int h);
    }
}
