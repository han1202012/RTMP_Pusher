package kim.hsl.rtmp;

import android.Manifest;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.FrameMetrics;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.widget.TextView;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    /**
     * 显示图像的 SurfaceView 组件
     */
    private SurfaceView mSurfaceView;

    /**
     * 直播推流器
     */
    private LivePusher mLivePusher;

    /**
     * 需要获取的权限列表
     */
    private String[] permissions = new String[]{
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.INTERNET,
            Manifest.permission.MODIFY_AUDIO_SETTINGS,
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.CAMERA
    };

    /**
     * 动态申请权限的请求码
     */
    private static final int PERMISSION_REQUEST_CODE = 888;

    /**
     * 动态申请权限
     */
    @RequiresApi(api = Build.VERSION_CODES.M)
    private void initPermissions() {
        if (isLacksPermission()) {
            //动态申请权限 , 第二参数是请求吗
            requestPermissions(permissions, PERMISSION_REQUEST_CODE);
        }
    }

    /**
     * 判断是否有 permissions 中的权限
     * @return
     */
    @RequiresApi(api = Build.VERSION_CODES.M)
    public boolean isLacksPermission() {
        for (String permission : permissions) {
            if(checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED){
                return true;
            }
        }
        return false;
    }


    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        // 渲染性能测量
        renderingPerformanceMeasurement();

        /*
            此时应用首界面启动完成, 将主题恢复成其它主题
            此处也可以根据不同的设置, 为应用设置不同的主题
         */
        setTheme(R.style.AppTheme);
        super.onCreate(savedInstanceState);

        // 删除背景, 该调用必须在 super.onCreate 之后, setContentView 之前
        //getWindow().setBackgroundDrawable(null);

        // 初始化权限
        initPermissions();

        // ★ 1. 将追踪信息存放到该文件中
        File traceFile = new File(Environment.getExternalStorageDirectory(), "Method_Trace");
        // ★ 2. 开启方法追踪
        Debug.startMethodTracing(traceFile.getAbsolutePath());

        setContentView(R.layout.activity_main);

        mSurfaceView = findViewById(R.id.surfaceView);

        // 创建直播推流器, 用于将采集的视频数据推流到服务器端
        // 800_000 代表 800K 的码率
        mLivePusher = new LivePusher(this,
                640, 480, 800_000, 10,
                Camera.CameraInfo.CAMERA_FACING_BACK);

        // 设置 Camera 采集的图像本地预览的组件, 在 mSurfaceView 界面先绘制摄像头
        // 此处要为 SurfaceHolder 设置 SurfaceHolder.Callback 回调 , 通过里面的回调函数
        // 驱动整个推流开始
        mLivePusher.setPreviewDisplay(mSurfaceView.getHolder());

        findViewById(R.id.button_play).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // rtmp://123.56.88.254/myapp/0
                // 0 相当于 直播的 密码
                // 配置好服务器后, 记录 IP 地址, 替换 123.56.88.254 IP 地址
                // rtmp://123.56.88.254/myapp/mystream 地址推流后
                // 可以直接在 RTMP 服务器端的主页, 使用 JWPlayer 观看直播内容
                // 网页地址是 http//123.56.88.254:8080/
                String rtmpServerAddress = "rtmp://123.56.88.254/myapp/mystream";
                mLivePusher.startLive(rtmpServerAddress);
                ((TextView)findViewById(R.id.textViewUrl))
                        .setText("推流地址 : " + rtmpServerAddress);
            }
        });

        // ★ 3. 停止方法追踪
        Debug.stopMethodTracing();
    }

    /**
     * 渲染性能测量
     */
    public void renderingPerformanceMeasurement(){
        HandlerThread handlerThread = new HandlerThread("FrameMetrics");
        handlerThread.start();
        Handler handler = new Handler(handlerThread.getLooper());

        // 24 版本以后的 API 才能支持该选项
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            // 整个渲染过程测量都在 HandlerThread 线程中执行
            getWindow().addOnFrameMetricsAvailableListener(
                    new Window.OnFrameMetricsAvailableListener(){

                        @Override
                        public void onFrameMetricsAvailable(
                                Window window,
                                FrameMetrics frameMetrics,
                                int dropCountSinceLastInvocation) {

                            // 帧渲染测量完毕后, 会回调该方法
                            // 渲染性能参数都封装在 FrameMetrics frameMetrics 参数中

                            // 先拷贝一份, 之后从拷贝的数据中获取对应渲染时间参数
                            FrameMetrics fm = new FrameMetrics(frameMetrics);

                            // 1. 动画执行回调时间, 单位纳秒
                            Log.i("FrameMetrics", "ANIMATION_DURATION : " +
                                    fm.getMetric(FrameMetrics.ANIMATION_DURATION));

                            // 2. 向 GPU 发送绘制命令花费的时间, 单位纳秒
                            Log.i("FrameMetrics", "COMMAND_ISSUE_DURATION : " +
                                    fm.getMetric(FrameMetrics.COMMAND_ISSUE_DURATION));

                            // 3. 将组件树 ( View Hierarchy ) 转为显示列表 ( DisplayLists )
                            // 计算过程所花费的时间, 单位纳秒
                            Log.i("FrameMetrics", "DRAW_DURATION : " +
                                    fm.getMetric(FrameMetrics.DRAW_DURATION));

                            // 4. 绘制的该帧是否是第一帧, 0 是, 1 不是
                            // 第一帧渲染会慢一些
                            // 第一帧不会引发动画中的跳帧问题, 这些问题都会被窗口动画隐藏
                            // 不必进行显示过程中的 jank 计算
                            Log.i("FrameMetrics", "FIRST_DRAW_FRAME : " +
                                    fm.getMetric(FrameMetrics.FIRST_DRAW_FRAME));

                            // 5. 处理输入事件花费的时间, 单位纳秒
                            Log.i("FrameMetrics", "INPUT_HANDLING_DURATION : " +
                                    fm.getMetric(FrameMetrics.INPUT_HANDLING_DURATION));

                            // 6. 该值是个时间戳, 表示该帧的 vsync 信号发出时间
                            // 这个时间是当前帧的预期开始时间
                            // 如果该时间与 VSYNC_TIMESTAMP 时间戳不同
                            // 那么说明 UI 线程被阻塞了, 没有及时响应 vsync 信号
                            Log.i("FrameMetrics", "INTENDED_VSYNC_TIMESTAMP : " +
                                    fm.getMetric(FrameMetrics.INTENDED_VSYNC_TIMESTAMP));

                            // 7. 组件树 ( view hierarchy ) 测量 ( measure ) 和摆放 ( layout ) 花费的时间
                            // 单位 纳秒
                            Log.i("FrameMetrics", "LAYOUT_MEASURE_DURATION : " +
                                    fm.getMetric(FrameMetrics.LAYOUT_MEASURE_DURATION));

                            // 8. CPU 传递多维向量图形数据给 GPU 花费的时间, 单位纳秒
                            Log.i("FrameMetrics", "SWAP_BUFFERS_DURATION : " +
                                    fm.getMetric(FrameMetrics.SWAP_BUFFERS_DURATION));

                            // 9. 显示列表 ( DisplayLists ) 与显示线程同步花费的时间, 单位纳秒
                            Log.i("FrameMetrics", "SYNC_DURATION : " +
                                    fm.getMetric(FrameMetrics.SYNC_DURATION));

                            // 10. CPU 渲染到传递到 GPU 所用的总时间, 上述所花费的有意义的时间之和
                            // 单位纳秒
                            Log.i("FrameMetrics", "TOTAL_DURATION : " +
                                    fm.getMetric(FrameMetrics.TOTAL_DURATION));

                            // 11. UI 线程响应并开始处理渲染的等待时间, 一般是 0, 如果大于 0 说明出问题了
                            Log.i("FrameMetrics", "UNKNOWN_DELAY_DURATION : " +
                                    fm.getMetric(FrameMetrics.UNKNOWN_DELAY_DURATION));

                            // 12. vsync 信号发出的时间戳, 该时刻 GPU 应该进行绘制, 间隔 16ms
                            // 同时 CPU 开始渲染
                            Log.i("FrameMetrics", "VSYNC_TIMESTAMP : " +
                                    fm.getMetric(FrameMetrics.VSYNC_TIMESTAMP));
                        }
                    }, handler);
        }
    }

}
