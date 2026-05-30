#include <stdio.h>
#include <stdlib.h>
#include "WavFileCreator.h"
#include "MiniVoxJ.h"

// 音声合成
// voice : 声質
// text : 音声記号カタカナ文字列
// filename : 出力WAVファイル名
int synthesis(VoiceType voice, float speed, const char* text, const char* filename)
{
    const int fs = 16000;       // サンプリング周波数[Hz]
    const int BUFF_SIZE = 256;  // バッファサイズ
    int16_t buffer[BUFF_SIZE];  // バッファ

    WavFileCreator wav(fs);
    MiniVoxJ vox(fs, BUFF_SIZE);

    if (wav.create(filename) == false) {
        printf("Failed to create WAV file.\n");
        return -1;
    }

    vox.setVoiceType(voice);
    vox.setSpeed(speed);
    vox.setText(text);

    while (true) {
        int size = vox.synthesize(buffer);
        wav.write(buffer, size);
        if (size < BUFF_SIZE) {
            int status = vox.getStatus();
            if (status == vox.COMPLETE) {
                printf("synthesizing complete!\n");
            }
            else {
                printf("error! (%d)\n", status);
            }
            break;
        }
    }

    wav.close();

    return 0;
}

// 対話/スクリプトコマンドの実行
// file: 標準入力またはスクリプトファイル
void executeCommand(FILE* file)
{
    VoiceType type = VoiceType::Male;
    float speed = 1.0f;
    char buf[1024];
    char filename[32];
    char command[32];
    int fcnt = 0;
    printf("Interactive mode\n");
    while (1) {
        printf("> ");
        if (fgets(buf, sizeof(buf), file) != NULL) {
            // vコマンド : 声質設定
            if (buf[0] == 'v') {
                int val = buf[1] - '0';
                if (val >= 0 && val < (int)VoiceType::TypeCount) {
                    type = (VoiceType)val;
                    printf("Voice type %d\n", (int)type);
                }
            }
            // sコマンド : 発話速度設定
            else if (buf[0] == 's') {
                float val;
                int ret = sscanf(&buf[1], "%f", &val);
                if (ret == 1) {
                    if (val > 0.0f) {
                        speed = val;
                        printf("Speed %f\n", speed);
                    }
                }
                else {
                    printf("sscanf error\n");
                }
            }
            // qコマンド : 終了
            else if (buf[0] == 'q') {
                printf("Quit!\n");
                break;
            }
            // それ以外 : 音声合成
            else {
                sprintf(filename, "output%d.wav", fcnt);
                synthesis(type, speed, buf, filename);
                sprintf(command, "start %s", filename);
                if(file == stdin) {
                    system(command);
                }
                fcnt++;
            }
        }
        else {
            break;
        }
    }
}

// メイン関数
int main(int argc, char const* argv[])
{
    system("chcp 65001"); // Windowsでコンソールの文字コードをUTF-8にする。(デフォルトはシフトJIS)

    // オプション無しのとき：固定の文字列から音声合成
    if (argc < 2) {
        const char* text = 
            (const char*)"ア'ル/ヒノ'/クレガタノ'/コト'デアル。ヒト'リノ/ゲニンガ'、ラショ'オモンノ/シタデ' アマヤミオ'/マ'ッテイタ。";
        
        synthesis(VoiceType::Male, 1.0f, text, "output.wav");
        system("start output.wav");
    }
    // オプション "i" のとき：対話実行
    else if (argv[1][0] == 'i')
    {
        executeCommand(stdin);
    }
    // オプション "s" のとき：スクリプト実行
    else if (argv[1][0] == 's')
    {
        FILE* file = fopen("script.txt", "r");
        if (file == NULL) {
            printf("Failed to open script.txt\n");
        }
        executeCommand(file);
    }
    return 0;
}
