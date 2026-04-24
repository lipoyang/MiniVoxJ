#pragma once
#include <stdio.h>
#include <stdint.h>

// WAVファイル生成クラス
class WavFileCreator
{
public:
    WavFileCreator(uint32_t sampleRate = 16000u) : sampleRate(sampleRate) {}

    bool create(const char* filename);
    void write(int16_t sample);
    void write(int16_t* buffer, int size);
    void close();

private:
    const uint32_t sampleRate;          // サンプルレート [Hz]
    const uint16_t channels = 1u;       // チャンネル数 (モノラル)
    const uint16_t bitsPerSample = 16u; // ビット深度 (16ビット)

    FILE* file = nullptr;
    uint32_t sampleCount = 0; // 書き込んだサンプル数

    void writeHeader();
};
