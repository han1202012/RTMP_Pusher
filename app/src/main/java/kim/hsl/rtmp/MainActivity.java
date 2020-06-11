package kim.hsl.rtmp;

import android.hardware.Camera;
import android.os.Bundle;
import android.view.SurfaceView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    /**
     * 显示图像的 SurfaceView 组件
     */
    private SurfaceView mSurfaceView;

    /**
     * 直播推流器
     */
    private LivePusher mLivePusher;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSurfaceView = findViewById(R.id.surfaceView);

        // 创建直播推流器, 用于将采集的视频数据推流到服务器端
        mLivePusher = new LivePusher(this, 800, 480, 800_000, 10, Camera.CameraInfo.CAMERA_FACING_BACK);

        // 设置 Camera 采集的图像本地预览的组件, 在 mSurfaceView 界面先绘制摄像头
        mLivePusher.setPreviewDisplay(mSurfaceView.getHolder());
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
