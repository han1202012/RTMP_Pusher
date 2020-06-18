package kim.hsl.rtmp;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * 音频处理类
 * 音频采样, 编码, 推流控制
 */
public class AudioChannel {

    /**
     * 44100 Hz 采样率
     */
    public static final int SAMPLE_RATE_IN_HZ_44100 = 44100;
    /**
     * 单声道
     */
    public static final int AUDIO_CHANNEL_MONO = 1;
    /**
     * 立体声
     */
    public static final int AUDIO_CHANNEL_STEREO = 2;

    /**
     * 直播推流器
     */
    private LivePusher mLivePusher;

    /**
     * 音频录制对象
     */
    private AudioRecord mAudioRecord;

    /**
     * 是否已经开始推流
     */
    private boolean isStartPush;

    /**
     * 单线程线程池, 在该线程中进行音频采样
     */
    private ExecutorService mExecutorService;

    /**
     * FAAC 编码器一次可以读取的样本个数
     */
    private int mFaacInputSamplesCount;

    /**
     * FAAC 编码器一次可以读取的字节个数
     * mFaacInputSamplesCount * 2
     */
    private int mFaacInputBytesCount;

    /**
     * 每次从
     */
    private int mAudioRecordReadCount;

    public AudioChannel(LivePusher mLivePusher) {
        this.mLivePusher = mLivePusher;

        // 初始化线程池, 单线程线程池
        mExecutorService = Executors.newSingleThreadExecutor();

        // 调用该方法, 最终调用 JNI 层初始化 FAAC 编码器的参数
        // 44100 立体声, 是默认选项, 就设置这个参数
        // 后续初始化 AudioRecord 需要该设置过程中的 FAAC 编码器一次可以读取的样本个数
        mLivePusher.native_setAudioEncoderParameters(SAMPLE_RATE_IN_HZ_44100, AUDIO_CHANNEL_STEREO);

        // 获取 FAAC 编码器一次可以读取的样本个数
        mFaacInputSamplesCount = mLivePusher.native_getInputSamples();
        // 获取 FAAC 编码器一次可以读取的字节个数
        mFaacInputBytesCount = mFaacInputSamplesCount * 2;

        /*
            获取 44100 立体声 / 单声道 16 位采样率的最小缓冲区大小
            使用最小缓冲区大小, 不能保证声音流畅平滑, 这里将缓冲区大小翻倍, 保证采集数据的流畅
            否则会有电流产生
         */
        int minBufferSize = AudioRecord.getMinBufferSize(44100,
                AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT) * 2;


        /*
            这里需要计算 每次采集数据的最大缓冲区大小
            上述计算出来的 minBufferSize 只是参考值, 并不是一个严格的值
            比 minBufferSize 值大一些, 小一些都可以

            设置该最大缓冲区值的时候, 每次读取的数据不能小于 mFaacInputSamplesCount 值
            这是向 FAAC 编码器送入 PCM 样本的个数
            如果读取的数据小于 mFaacInputSamplesCount 值
            该 mFaacInputSamplesCount 字节大小的缓冲区就会读不满, 后面部分的数据都是空数据
            肯定会造成电流

            因此这里取值时, AudioRecord 创建时的最后一个参数 , 每次采集数据的最大缓冲区大小
            必须要大于 mFaacInputSamplesCount 值;

            下面的 maxBufferSizeInBytesForInitAudioRecord 值取
            minBufferSize 和 mFaacInputSamplesCount 中的最大值
         */
        int maxBufferSizeInBytesForInitAudioRecord =
                mFaacInputBytesCount > minBufferSize ? mFaacInputBytesCount : minBufferSize;


        /*
            public AudioRecord(int audioSource, int sampleRateInHz,
                               int channelConfig, int audioFormat,
                               int bufferSizeInBytes)

            int audioSource 参数 : 声音来源, 麦克风
            int sampleRateInHz 参数 : 音频采样率, 一般是 44100 Hz, 该采样率在所有设备支持比较好
            int channelConfig 参数 : 单声道 AudioFormat.CHANNEL_IN_MONO / 立体声 AudioFormat.CHANNEL_IN_STEREO,
            int audioFormat 参数 : 采样位数, 8 位 AudioFormat.ENCODING_PCM_8BIT / 16 位 AudioFormat.ENCODING_PCM_16BIT
            int bufferSizeInBytes 参数 : 每次采集数据的最大缓冲区大小

         */
        mAudioRecord = new AudioRecord(
                MediaRecorder.AudioSource.MIC,  // 声音来源 麦克风
                SAMPLE_RATE_IN_HZ_44100,            // PCM 音频采样率 44100 Hz
                AudioFormat.CHANNEL_IN_STEREO,  // 立体声
                AudioFormat.ENCODING_PCM_16BIT, // 采样位数 16 位
                maxBufferSizeInBytesForInitAudioRecord);                 // 最小采样缓冲区个数


    }

    /**
     * 开始推流
     */
    public void startLive() {
        isStartPush = true;
        // 执行音频采样线程
        // 如果在启动一个线程, 后续线程就会排队等待
        mExecutorService.submit(new AudioSampling());
    }

    /**
     * 停止推流
     */
    public void stopLive() {
        isStartPush = false;
    }

    public void release(){
        //释放音频录音对象
        mAudioRecord.release();
    }

    /**
     * 音频采样线程
     */
    class AudioSampling implements Runnable{
        @Override
        public void run() {
            // 开始录音采样
            mAudioRecord.startRecording();
            // FAAC 编码器每次读取 mFaacInputSamplesCount 个样本
            // 注意 : 一个样本 2 字节
            // 字节个数是 mFaacInputBytesCount 个字节
            byte[] readBuffer = new byte[mFaacInputBytesCount];
            while (isStartPush){
                // 循环读取录音
                int readLen = mAudioRecord.read(readBuffer, 0, readBuffer.length);

                // 如果读取到的 PCM 音频采样数据大于 0
                // 从到 JNI 层让 FAAC 编码器编码成 AAC 格式的音频数据
                if(readLen > 0){
                    // 将数据传入 JNI 层使用 FAAC 编码器进行编码
                    mLivePusher.native_encodeAudioData(readBuffer);
                }
            }

            // 停止录音采样
            mAudioRecord.stop();
        }
    }
}
