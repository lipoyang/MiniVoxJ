#include <Arduino.h>
#include "MiniVoxJ.h"
#include "I2sAudioOut.h"
#include "SerialCom.h"

const int fs = 16000;       // サンプリング周波数[Hz]
const int BUFF_SIZE = 512;  // バッファサイズ
const int BUFF_CNT = 8;     // バッファ段数
int16_t buffer[BUFF_SIZE];  // バッファ

// 音声合成エンジン
MiniVoxJ vox(fs, BUFF_SIZE);

// I2Sオーディオ出力
I2sAudioOut audioOut;
I2sAudioPins pins = {
    .SCK = D2, // I2S SCKピン
    .WS  = D3, // I2S WSピン
    .SD  = D4  // I2S SDピン
};

// シリアルコマンド受信クラス
SerialCom serialCom;

// コマンド受信ハンドラ
void onReceived(const char* buff)
{
  Serial.print("Received command: ");
  Serial.println(buff);

  char cmd = buff[0];
  switch(cmd) {
    case 'P':
      // 音声合成するテキストをセット
      vox.setText(&buff[1]);
      break;
    case 'V':
      // 声質を変更
      switch(buff[1]) {
        case '0': vox.setVoiceType(VoiceType::Male);   break;
        case '1': vox.setVoiceType(VoiceType::Female); break;
        case '2': vox.setVoiceType(VoiceType::Cute);   break;
        case '3': vox.setVoiceType(VoiceType::Anime);  break;
        default:
          Serial.println("Unknown voice type!");
      }
      break;
    default:
      Serial.println("Unknown command!");
  }
}

// 初期化処理
void setup()
{
  Serial.begin(115200);

  // I2Sオーディオ出力の初期化
  audioOut.begin(pins, fs, BUFF_SIZE, BUFF_CNT);

  // シリアルコマンド受信の初期化
  serialCom.begin(Serial, onReceived);

  // 音声合成するテキストをセット
  // vox.setText("コンニチワ'。");
}

// メインループ
void loop()
{
  // I2Sオーディオ出力の書き込み可能チェック
  audioOut.loop();
  if (audioOut.available())
  {
    // 音声合成処理中か？
    int status = vox.getStatus();
    if (status == vox.PROCESSING)
    {
      Serial.print("*");
      // 1フレームぶん音声合成してI2Sオーディオ出力に書き込む
      int size = vox.synthesize(buffer);
      if(size > 0) {
        if(size < BUFF_SIZE){
          for(int i = size; i < BUFF_SIZE; i++) buffer[i] = 0;
        }
        int written = audioOut.write(buffer, BUFF_SIZE);
        if(written != BUFF_SIZE) {
          Serial.print("Audio output Warning! : ");
          Serial.println(written);
        }
      }
      // 音声合成が完了しているか？
      if (size < BUFF_SIZE) {
          int status = vox.getStatus();
          if (status == vox.COMPLETE) {
              Serial.println("Synthesizing Complete!");
          } else {
              Serial.print("Synthesizing Error! : ");
              Serial.println(status);
          }
      }
    }
  }

  // シリアルコマンド受信処理
  serialCom.loop();
}
