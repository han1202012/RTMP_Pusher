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
public class CameraManager implements SurfaceHolder.Callback, Camera.PreviewCallback {

    private static final String TAG = "CameraHelper";

    /**
     * 摄像头显示数据组件所在的界面
     * 主要是为了方便的获取界面参数
     */
    private Activity mActivity;

    /**
     * 用户设置的高度
     */
    private int mHeight;

    /**
     * 用户设置的宽度
     */
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

    /**
     * Camera 预览数据 NV21 格式
     */
    private byte[] mNv21DataBuffer;
    private SurfaceHolder mSurfaceHolder;
    private Camera.PreviewCallback mPreviewCallback;
    private int mRotation;
    private OnChangedSizeListener mOnChangedSizeListener;

    public CameraManager(Activity activity, int cameraId, int width, int height) {
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
        stopCameraNV21DataPreview();
        startCameraNV21DataPreview();
    }

    /**
     * 释放 Camera 摄像头
     */
    private void stopCameraNV21DataPreview() {
        if (mCamera != null) {
            // 下面的 API 都是 Android 提供的

            // 1. 设置预览回调接口, 这里设置 null 即可
            mCamera.setPreviewCallback(null);
            // 2. 停止图像数据预览
            mCamera.stopPreview();
            // 3. 释放 Camera
            mCamera.release();
            mCamera = null;
        }
    }

    /**
     * 开启 Camera 摄像头
     */
    private void startCameraNV21DataPreview() {
        try {
            Log.i("octopus", "startCameraNV21DataPreview");
            // 1. 打开指定方向的 Camera 摄像头
            mCamera = Camera.open(mCameraFacing);
            // 2. 获取 Camera 摄像头参数, 之后需要修改配置该参数
            Camera.Parameters parameters = mCamera.getParameters();
            // 3. 设置 Camera 采集后预览图像的数据格式 ImageFormat.NV21
            parameters.setPreviewFormat(ImageFormat.NV21);
            // 4. 设置摄像头预览尺寸
            setPreviewSize(parameters);
            // 5. 设置图像传感器参数
            setCameraPreviewOrientation(parameters);
            mCamera.setParameters(parameters);
            // 6. 计算出 NV21 格式图像 mWidth * mHeight 像素数据大小
            mNv21DataBuffer = new byte[mWidth * mHeight * 3 / 2];
            // 7. 设置 Camera 预览数据缓存区
            mCamera.addCallbackBuffer(mNv21DataBuffer);
            // 8. 设置 Camera 数据采集回调函数, 采集完数据后
            //    就会回调此 PreviewCallback 接口的
            //    void onPreviewFrame(byte[] data, Camera camera) 方法
            mCamera.setPreviewCallbackWithBuffer(this);
            // 9. 设置预览图像画面的 SurfaceView 画布
            mCamera.setPreviewDisplay(mSurfaceHolder);

            // 11. 开始预览
            mCamera.startPreview();
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    /**
     * 设置 Camera 预览方向
     * 如果不设置, 视频是颠倒的
     * 该方法内容拷贝自 {@link Camera#setDisplayOrientation} 注释, 这是 Google Docs 提供的
     * @param parameters
     */
    private void setCameraPreviewOrientation(Camera.Parameters parameters) {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(mCameraFacing, info);
        mRotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (mRotation) {
            case Surface.ROTATION_0:
                degrees = 0;
                break;
            case Surface.ROTATION_90:
                degrees = 90;
                break;
            case Surface.ROTATION_180:
                degrees = 180;
                break;
            case Surface.ROTATION_270:
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


        // 2. 下面开始遍历获取与用户设置的宽高值最接近的, Camera 支持的宽高值


        // 获取系统 Camera 摄像头支持的最低的摄像头
        //      Camera.Size 中有宽度和高度参数
        Camera.Size currentSupportSize = supportedPreviewSizes.get(0);
        // 对比第 0 个系统支持的像素总数 与 用户设置的像素总数 差值
        //      之后会逐个对比, 每个系统支持的像素总数 与 用户设置像素总数 差值
        //      选择差值最小的那个像素值
        int minDeltaOfPixels = Math.abs(currentSupportSize.height * currentSupportSize.width - mWidth * mHeight);
        // 对比完成之后, 删除像素值
        //supportedPreviewSizes.remove(0);

        // 遍历系统支持的宽高像素集合, 如果
        Iterator<Camera.Size> iterator = supportedPreviewSizes.iterator();
        while (iterator.hasNext()) {
            // 当前遍历的的系统支持的像素宽高
            Camera.Size tmpSupportSize = iterator.next();
            // 计算当前的设备支持宽高与用户设置的宽高的像素点个数差值
            int tmpDeltaOfPixels = Math.abs(tmpSupportSize.height * tmpSupportSize.width - mWidth * mHeight);

            // 如果当前的差分值 小于 当前最小像素差分值, 那么使用当前的数据替代
            if (tmpDeltaOfPixels < minDeltaOfPixels) {
                minDeltaOfPixels = tmpDeltaOfPixels;
                currentSupportSize = tmpSupportSize;
            }
        }

        // 3. 选择出了最合适的 Camera 支持的宽高值
        mWidth = currentSupportSize.width;
        mHeight = currentSupportSize.height;

        // 4. 为 Camera 设置最合适的像素值
        parameters.setPreviewSize(mWidth, mHeight);

        // 5. 设置大小改变监听
        mOnChangedSizeListener.onChanged(mWidth, mHeight);
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
        stopCameraNV21DataPreview();
        startCameraNV21DataPreview();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopCameraNV21DataPreview();
    }


    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        // 此时 NV21 数据是颠倒的
        mPreviewCallback.onPreviewFrame(data, camera);
        camera.addCallbackBuffer(mNv21DataBuffer);
    }



    public void setOnChangedSizeListener(OnChangedSizeListener listener) {
        mOnChangedSizeListener = listener;
    }

    public interface OnChangedSizeListener {
        void onChanged(int w, int h);
    }
}
