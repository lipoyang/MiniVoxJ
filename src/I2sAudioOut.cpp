#include <driver/i2s.h>
#include "I2sAudioOut.h"

// I2Sのポート番号
#define I2S_PORT        (I2S_NUM_0)

// 初期化する
// pins: I2Sのピン番号
// sampleRate: サンプリング周波数[Hz]
// bufferSize: バッファサイズ[サンプル数]
// bufferCnt: バッファ段数
void I2sAudioOut::begin(I2sAudioPins& pins, int sampleRate, int bufferSize, int bufferLen)
{
    _bufferSize = bufferSize;
    _bufferLen = bufferLen;
    _available = true;

    // I2S構成の設定
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = (uint32_t)sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // モノラル
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, // I2S_COMM_FORMAT_I2S_MSB ※ TODO
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = bufferLen,
        .dma_buf_len = bufferSize,
        .use_apll = false,
        .tx_desc_auto_clear = true, 
        .fixed_mclk = 0
    };

    // ピン割り当ての設定
    i2s_pin_config_t pin_config = {
        .bck_io_num = pins.SCK,
        .ws_io_num = pins.WS,
        .data_out_num = pins.SD,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_PORT, &i2s_config, bufferLen, &_i2s_event_queue);
    i2s_set_pin(I2S_PORT, &pin_config);
    // i2s_zero_dma_buffer(I2S_PORT);
}

// メインループ処理
void I2sAudioOut::loop()
{
    // I2Sイベントキューからイベントを受け取る
    i2s_event_t evt;
    const TickType_t ticks_to_wait = 0; // 0でノンブロッキング、portMAX_DELAYでブロッキング
    if (xQueueReceive(_i2s_event_queue, &evt, ticks_to_wait) == pdTRUE) {
        if (evt.type == I2S_EVENT_TX_DONE) {
            _available = true;
        }
    }
}

// オーディオデータを書き込む
// data: 書き込むオーディオデータ
// size: 書き込むサンプル数
// 戻り値: 書き込んだサンプル数
int I2sAudioOut::write(int16_t* data, int size)
{
    // バッファに空きがない場合は0を返す
    if(_available == false) return 0;

    // I2Sにデータを書き込む
    size_t bytes_written;
    const TickType_t ticks_to_wait = 0; // 0でノンブロッキング、portMAX_DELAYでブロッキング
    i2s_write(I2S_PORT, data, size * sizeof(int16_t), &bytes_written, ticks_to_wait);
    int samples_written = bytes_written / sizeof(int16_t);

    if(samples_written < size) {
        _available = false;
    }

    return samples_written;
}
