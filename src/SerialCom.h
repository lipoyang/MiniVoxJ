#pragma once
#include <Arduino.h>

// 受信バッファサイズ
#define RX_BUFF_SIZE 1024

// シリアルコマンド受信クラス
class SerialCom
{
public:
    void begin(Stream & serial, void (*onReceived)(const char*));
    void loop(void);

private:
    // シリアルポート
    Stream * serial;
    // 受信状態
    int state;
    // 受信バッファ
    int ptr;
    char buff[RX_BUFF_SIZE];
    // 受信ハンドラ
    void (*onReceived)(const char* buff);
};
