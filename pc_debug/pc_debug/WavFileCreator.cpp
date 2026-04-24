#include <stdio.h>
#include <stdint.h>
#include "WavFileCreator.h"

void WavFileCreator::writeHeader()
{
    const uint16_t bytesPerSample = bitsPerSample / 8u;
    const uint32_t dataChunkSize = sampleCount * channels * bytesPerSample;
    const uint32_t fmtChunkSize = 16u;
    const uint16_t audioFormat = 1u; // PCM形式
    const uint32_t byteRate = sampleRate * channels * bytesPerSample;
    const uint16_t blockAlign = channels * bytesPerSample;
    const uint32_t riffChunkSize = 4u + (8u + fmtChunkSize) + (8u + dataChunkSize);

    // RIFFヘッダ
    fwrite("RIFF", 1, 4, file);
    fwrite(&riffChunkSize, sizeof(riffChunkSize), 1, file);
    fwrite("WAVE", 1, 4, file);

    // fmtサブチャンク
    fwrite("fmt ", 1, 4, file);
    fwrite(&fmtChunkSize, sizeof(fmtChunkSize), 1, file);
    fwrite(&audioFormat, sizeof(audioFormat), 1, file);
    fwrite(&channels, sizeof(channels), 1, file);
    fwrite(&sampleRate, sizeof(sampleRate), 1, file);
    fwrite(&byteRate, sizeof(byteRate), 1, file);
    fwrite(&blockAlign, sizeof(blockAlign), 1, file);
    fwrite(&bitsPerSample, sizeof(bitsPerSample), 1, file);

    // dataサブチャンク
    fwrite("data", 1, 4, file);
    fwrite(&dataChunkSize, sizeof(dataChunkSize), 1, file);
}

bool WavFileCreator::create(const char* filename)
{
    // ファイルを開く
    file = fopen(filename, "wb");
    if (!file) {
        return false;
    }

    // サンプル数を初期化
    sampleCount = 0u;

    // ヘッダを書き込む(仮のサイズで)
    writeHeader();

    return true;
}

void WavFileCreator::write(int16_t sample)
{
    fwrite(&sample, sizeof(sample), 1, file);

    // 書き込んだサンプル数を更新
    sampleCount++;
}

void WavFileCreator::write(int16_t* buffer, int size)
{
    fwrite(buffer, sizeof(int16_t), size, file);

    // 書き込んだサンプル数を更新
    sampleCount += size;
}

void WavFileCreator::close()
{
    // ヘッダを更新
    fseek(file, 0, SEEK_SET);
    writeHeader();

    fclose(file);
}
