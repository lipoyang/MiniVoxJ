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
    void begin(I2sAudioPins& pins, int sampleRate, int bufferSize = 512, int bufferLen = 8);
    void loop();
    bool available() const { return _available; }
    int write(int16_t* data, int size);

private:
    QueueHandle_t _i2s_event_queue;
    int _bufferSize;
    int _bufferLen;
    bool _available;
};