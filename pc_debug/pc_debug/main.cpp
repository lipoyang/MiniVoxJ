#include <stdio.h>
#include "WavFileCreator.h"
#include "MiniVoxJ.h"

int main(void)
{
    const int fs = 16000;       // サンプリング周波数[Hz]
    const int BUFF_SIZE = 256;  // バッファサイズ
    int16_t buffer[BUFF_SIZE];  // バッファ

    WavFileCreator wav(fs);
    MiniVoxJ vox(fs, BUFF_SIZE);

    if (wav.create("output.wav") == false) {
        printf("Failed to create WAV file.\n");
        return -1;
    }

    vox.setText("ア'ル/ヒノ'/クレガタノ'/コト'デアル。ヒト'リノ/ゲニンガ'、ラショ'オモンノ/シタデ' アマヤミオ'/マ'ッテ/イタ'。");

    while (true) {
        int size = vox.synthesize(buffer);
        wav.write(buffer, size);
        if (size < BUFF_SIZE) {
            int status = vox.getStatus();
            if (status == vox.COMPLETE) {
                printf("synthesizing complete!\n");
            } else {
                printf("error! (%d)\n", status);
            }
            break;
        }
    }

    wav.close();

    return 0;
}
