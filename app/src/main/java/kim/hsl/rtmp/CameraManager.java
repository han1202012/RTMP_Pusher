package kim.hsl.rtmp;

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

    /**
     * 画面显示画布 Holder
     */
    private SurfaceHolder mSurfaceHolder;

    /**
     * 回调接口
     */
    private Camera.PreviewCallback mPreviewCallback;

    /**
     * 屏幕旋转角度
     */
    private int mScreenRotation;

    /**
     * 屏幕大小改变回调接口
     * 通过该回调接口, 设置 JNI 层 x264 编码 H.264 的参数
     */
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

        /*
            获取屏幕相对于自然方向的角度
            自然方向就是正常的竖屏方向, 摄像头在上, Home 键在下, 对应 Surface.ROTATION_0

            ROTATION_0 是自然方向逆时针旋转 0 度, 竖屏
            头部 ( 摄像头的一边 ) 在上边
            尾部 ( Home / 返回 键的一边 ) 在下边
            一般竖屏操作方式, 也是最常用的方式

            ROTATION_90 是自然方向逆时针旋转 90 度, 横屏
            头部 ( 摄像头的一边 ) 在左边
            尾部 ( Home / 返回 键的一边 ) 在右边
            一般横屏操作方式

            ROTATION_180 是自然方向逆时针旋转 180 度, 竖屏
            头部 ( 摄像头的一边 ) 在下边
            尾部 ( Home / 返回 键的一边 ) 在上边
            一般很少这样操作

            ROTATION_270 是自然方向逆时针旋转 270 度, 横屏
            头部 ( 摄像头的一边 ) 在右边
            尾部 ( Home / 返回 键的一边 ) 在左边
            一般很少这样操作

            博客中配合截图说明这些方向
         */
        mScreenRotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();

        int degrees = 0;
        switch (mScreenRotation) {
            case Surface.ROTATION_0:
                degrees = 0;
                /*
                    Camera 图像传感器采集的数据是按照竖屏采集的
                    原来设置的图像的宽高是 800 x 400
                    如果屏幕竖过来, 其宽高就变成 400 x 800, 宽高需要交换一下
                    这里需要通知 Native 层的 x264 编码器, 修改编码参数 , 按照 400 x 800 的尺寸进行编码
                    需要重新设置 x264 的编码参数
                 */
                mOnChangedSizeListener.onChanged(mHeight, mWidth);
                break;
            case Surface.ROTATION_90:
                degrees = 90;
                mOnChangedSizeListener.onChanged(mWidth, mHeight);
                break;
            case Surface.ROTATION_180:
                degrees = 180;
                mOnChangedSizeListener.onChanged(mHeight, mWidth);
                break;
            case Surface.ROTATION_270:
                mOnChangedSizeListener.onChanged(mWidth, mHeight);
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
        // 该 SurfaceHolder.Callback 回调, 是驱动整个推流开始的接口
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


    /**
     * Camera 图像传感器采集图像数据回调函数
     * @param data
     *          Camera 采集的图像数据, NV21 格式
     * @param camera
     *          Camera 摄像头
     */
    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        // 处理 NV21 数据旋转问题
        if(mScreenRotation == Surface.ROTATION_0){
            nv21PictureDataClockwiseRotation90(data);
        }

        // 此时 NV21 数据是颠倒的
        mPreviewCallback.onPreviewFrame(data, camera);
        camera.addCallbackBuffer(mNv21DataBuffer);
    }

    /**
     * 将 NV21 格式的图片数据顺时针旋转 90 度
     * 后置摄像头顺时针旋转 90 度
     * 前置摄像头逆时针旋转 90 度
     * @param data
     */
    private void nv21PictureDataClockwiseRotation90(byte[] data){
        // Y 灰度数据的个数
        int YByteCount = mWidth * mHeight;

        // 色彩度 U, 饱和度 V 数据高度
        int UVByteHeight = mHeight / 2;

        // 色彩度 U, 饱和度 V 数据个数
        int UVByteCount = YByteCount / 4;

        // 数据处理索引值, 用于记录写入到 mNv21DataBuffer 中的元素个数
        // 及下一个将要写入的元素的索引
        int positionIndex = 0;

        /*
            后置摄像头处理
            后置摄像头需要将图像顺时针旋转 90 度
         */
        if(mCameraFacing == Camera.CameraInfo.CAMERA_FACING_BACK){

            /*
                读取 Y 灰度数据
                顺时针旋转 90 度
                外层循环 : 逐行遍历, 从第一行遍历到最后一行
                内存循环 : 遍历每一行时, 从底部遍历到顶部
             */
            for (int i = 0; i < mWidth; i++) {
                // 第 i 行, 从每一列的最后一个像素 ( 索引 mHeight - 1 ) 遍历到第一个像素 ( 索引 0 )
                for (int j = mHeight - 1; j >= 0; j--) {
                    // 将读取到的 Y 灰度值存储到 mNv21DataBuffer 缓冲区中
                    mNv21DataBuffer[positionIndex++] = data[mWidth * j + i];
                }
            }

            /*
                读取 UV 数据

                Y 数据的高度与图像高度相等
                UV 数据高度相当于 Y 数据高度的一半

                UV 数据排列 : V 色彩值在前, U 饱和度在后, UV 数据交替排列
                UV 数据交替排列, 一行 mWidth 中, 排布了 mWidth / 2 组 UV 数据

                UV 数据组有 mWidth / 2 行, mHeight / 2 列, 因此遍历时, 有如下规则 :
                按照行遍历 : 遍历 mWidth / 2 次
                按照列遍历 : 遍历 mHeight / 2 次

                外层遍历 : 遍历行从 0 到 mWidth / 2
                外层按照行遍历时, 每隔 2 行, 遍历一次, 遍历 mWidth / 2 次

                内层遍历时 : 遍历列, 从 mHeight / 2 - 1 遍历到 0
                UV 数据也需要倒着读 , 从 mHeight / 2 - 1 遍历到 0
             */
            for (int i = 0; i < mWidth / 2; i ++) {
                for (int j = UVByteHeight - 1; j >= 0; j--) {
                    // 读取数据时, 要从 YByteCount 之后的数据开始遍历
                    // 使用 mWidth 和 UVByteHeight 定位要遍历的位置
                    // 拷贝 V 色彩值数据
                    mNv21DataBuffer[positionIndex++] = data[YByteCount + mWidth / 2 * 2 * j + i];
                    // 拷贝 U 饱和度数据
                    mNv21DataBuffer[positionIndex++] = data[YByteCount + mWidth / 2 * 2 * j + i + 1];
                }
            }


        }else if(mCameraFacing == Camera.CameraInfo.CAMERA_FACING_FRONT){
            /*
                前置摄像头处理
                前置摄像头与后置摄像头相反, 后置摄像头顺时针旋转 90 度
                前置摄像头需要将图像逆时针旋转 90 度
             */

            /*
                读取 Y 灰度数据
                逆时针旋转 90 度
                外层循环 : 逐行遍历, 从最后一行遍历到第一行, 从 mWidth - 1 到 0
                内存循环 : 遍历第 i 行时, 从顶部遍历到底部, 从 0 到 mHeight - 1
             */
            for (int i = mWidth - 1; i >= 0; i--) {
                // 第 i 行, 从每一列的最后一个像素 ( 索引 mHeight - 1 ) 遍历到第一个像素 ( 索引 0 )
                for (int j = 0; j < mHeight; j++) {
                    // 将读取到的 Y 灰度值存储到 mNv21DataBuffer 缓冲区中
                    mNv21DataBuffer[positionIndex++] = data[mWidth * j + i];
                }
            }

            /*
                读取 UV 数据

                Y 数据的高度与图像高度相等
                UV 数据高度相当于 Y 数据高度的一半

                UV 数据排列 : V 色彩值在前, U 饱和度在后, UV 数据交替排列
                UV 数据交替排列, 一行 mWidth 中, 排布了 mWidth / 2 组 UV 数据

                UV 数据组有 mWidth / 2 行, mHeight / 2 列, 因此遍历时, 有如下规则 :
                按照行遍历 : 遍历 mWidth / 2 次
                按照列遍历 : 遍历 mHeight / 2 次

                外层遍历 : 遍历行从 mWidth / 2 - 1 到 0
                外层按照行遍历时, 每隔 2 行, 遍历一次, 遍历 mWidth / 2 次

                内层遍历时 : 遍历列, 从 0 遍历到 mHeight / 2 - 1
                UV 数据也需要倒着读 , 从 0 遍历到 mHeight / 2 - 1
             */
            for (int i = mWidth / 2 - 1; i >= 0 ; i --) {
                for (int j = 0; j < UVByteHeight; j++) {
                    // 读取数据时, 要从 YByteCount 之后的数据开始遍历
                    // 使用 mWidth 和 UVByteHeight 定位要遍历的位置
                    // 拷贝 V 色彩值数据
                    mNv21DataBuffer[positionIndex++] = data[YByteCount + mWidth / 2 * 2 * j + i];
                    // 拷贝 U 饱和度数据
                    mNv21DataBuffer[positionIndex++] = data[YByteCount + mWidth / 2 * 2 * j + i + 1];
                }
            }

        }
    }



    public void setOnChangedSizeListener(OnChangedSizeListener listener) {
        mOnChangedSizeListener = listener;
    }

    public interface OnChangedSizeListener {
        void onChanged(int width, int heighht);
    }
}
