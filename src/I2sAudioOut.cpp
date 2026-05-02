#include "I2sAudioOut.h"

#if defined(ARDUINO_ARCH_ESP32)
/************************************************************
    XIAO ESP32C3
 ************************************************************/

#include <driver/i2s.h>

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
        .mck_io_num = I2S_PIN_NO_CHANGE,
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
#elif defined(ARDUINO_ARCH_RP2040)
/************************************************************
    XIAO RP2040
 ************************************************************/

#include <I2S.h>

I2S i2s(OUTPUT);

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

    // SCK(BCLK) と LRCLK(WS) は隣りあうピンでないといけない
    if(pins.WS != pins.SCK + 1){
        printf("Bad WS pin number! (SCK=%d, WS=%d)\n", pins.SCK, pins.SD);
        return;
    }

    i2s.setBCLK(pins.SCK);
    i2s.setDATA(pins.SD);
    i2s.setBitsPerSample(16); // 16ビットPCM
    i2s.setBuffers(bufferLen, bufferSize);
    i2s.begin(sampleRate);
}

// メインループ処理
void I2sAudioOut::loop()
{
    int availableSize = i2s.availableForWrite();
    if(availableSize > _bufferSize){
        _available = true;
    }else{
        _available = false;
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

    for(int i = 0; i < size; i++){
        int l = i2s.write(data[i], false);  // Left
        int r = i2s.write(data[i], false);  // Right
        if(l == 0 || r == 0){
            _available = false;
            return i; // 書き込んだサンプル数を返す
        }
    }
    return size;
}

#elif defined(ARDUINO_ARCH_MBED)    // XIAO nRF52840
// #elif defined(ARDUINO_NRF52_ADAFRUIT) // XIAO nRF52840
/************************************************************
    XIAO nRF52840
 ************************************************************/
#include <hal/nrf_i2s.h>

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

    // ゼロバッファ
    _zeros = new int16_t[bufferSize];
    memset(_zeros, 0, bufferSize * sizeof(int16_t));
    // データバッファ
    _buffer = new int16_t*[bufferLen];
    for(int i = 0; i < bufferLen; i++){
        _buffer[i] = new int16_t[bufferSize];
    }
    _wrIndex = 0;
    _rdIndex = 0;

    // TX 有効
    NRF_I2S->CONFIG.TXEN = I2S_CONFIG_TXEN_TXEN_ENABLE << I2S_CONFIG_TXEN_TXEN_Pos;

    // MCK 不使用
    NRF_I2S->CONFIG.MCKEN = I2S_CONFIG_MCKEN_MCKEN_DISABLE << I2S_CONFIG_MCKEN_MCKEN_Pos;
#if 0
    NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV63 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
    NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_64X << I2S_CONFIG_RATIO_RATIO_Pos;
#endif
    // Master mode, 16-bit, left-aligned
    NRF_I2S->CONFIG.MODE   = I2S_CONFIG_MODE_MODE_MASTER    << I2S_CONFIG_MODE_MODE_Pos;
    NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16BIT << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
    NRF_I2S->CONFIG.ALIGN  = I2S_CONFIG_ALIGN_ALIGN_LEFT    << I2S_CONFIG_ALIGN_ALIGN_Pos;
    NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S   << I2S_CONFIG_FORMAT_FORMAT_Pos;

    // モノラルで左チャンネルのみ有効
    NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_LEFT << I2S_CONFIG_CHANNELS_CHANNELS_Pos;

    // ピン割り当て
//  NRF_I2S->PSEL.MCK     Disable
    NRF_I2S->PSEL.SCK   = pins.SCK << I2S_PSEL_SCK_PIN_Pos;
    NRF_I2S->PSEL.LRCK  = pins.WS  << I2S_PSEL_LRCK_PIN_Pos;
    NRF_I2S->PSEL.SDOUT = pins.SD  << I2S_PSEL_SDOUT_PIN_Pos;

    // DMAバッファ
    NRF_I2S->RXTXD.MAXCNT = bufferSize;
    NRF_I2S->TXD.PTR = (uint32_t)_zeros;

    // Start I2S
    NRF_I2S->EVENTS_TXPTRUPD = 0;
    NRF_I2S->ENABLE = 1;
    NRF_I2S->TASKS_START = 1;
}

// メインループ処理
void I2sAudioOut::loop()
{
    if (NRF_I2S->EVENTS_TXPTRUPD)
    {
        if(_rdIndex == _wrIndex) {
            NRF_I2S->TXD.PTR = (uint32_t)_zeros;
        } else {
            NRF_I2S->TXD.PTR = (uint32_t)_buffer[_rdIndex];
        }
        NRF_I2S->EVENTS_TXPTRUPD = 0;
        
        _rdIndex = (_rdIndex + 1) % _bufferLen;
        _available = true;
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

    memcpy(_buffer[_wrIndex], data, size * sizeof(int16_t));

    _wrIndex = (_wrIndex + 1) % _bufferLen;
    if(_wrIndex == _rdIndex) {
        _available = false;
    }
    return size;
}

#endif
