package kim.hsl.rtmp;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

public class AudioChannel {

    /**
     * 直播推流器
     */
    private LivePusher mLivePusher;

    /**
     * 音频录制对象
     */
    private AudioRecord mAudioRecord;

    public AudioChannel(LivePusher mLivePusher) {
        this.mLivePusher = mLivePusher;

        /*
            获取 44100 立体声 / 单声道 16 位采样率的最小缓冲区大小
            使用最小缓冲区大小, 不能保证声音流畅平滑, 这里将缓冲区大小翻倍, 保证采集数据的流畅
            否则会有电流产生
         */
        int minBufferSize = AudioRecord.getMinBufferSize(44100,
                AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT) * 2;
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
                MediaRecorder.AudioSource.MIC,
                44100,
                AudioFormat.CHANNEL_IN_STEREO,
                AudioFormat.ENCODING_PCM_16BIT,
                minBufferSize);
    }

    public void stopLive() {
    }

    public void startLive() {
    }
}
