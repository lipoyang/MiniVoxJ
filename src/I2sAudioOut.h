#pragma once
#include <Arduino.h>
#include <stdint.h>

// I2Sのピン番号
struct I2sAudioPins{
    int SCK;
    int WS;
    int SD;
};

// I2Sオーディオ出力クラス
class I2sAudioOut{
public:
    void begin(I2sAudioPins& pins, int sampleRate, int bufferSize = 256, int bufferLen = 8);
    void loop();
    bool available() const { return _available; }
    int write(int16_t* data, int size);

private:
    int _bufferSize;
    int _bufferLen;
    bool _available;

#if defined(ARDUINO_ARCH_ESP32)     // XIAO ESP32C3
    QueueHandle_t _i2s_event_queue;
#elif defined(ARDUINO_ARCH_RP2040)  // XIAO RP2040

#elif defined(ARDUINO_ARCH_MBED)    // XIAO nRF52840
    
#endif
};