#pragma once

#if defined(ARDUINO_ARCH_MBED)    // XIAO nRF52840

#include <Arduino.h>
#include <ArduinoBLE.h>

// 受信バッファサイズ
#define BLE_BUFF_SIZE 1024

// BLEコマンド受信クラス
class BleCom
{
public:
    void begin(const char* localName, void (*onReceived)(const char*));
    void loop(void);

private:
    BLEDevice central;
    bool isConnected;
    char payload[256];

    // 受信処理
    void receive();

    // 受信状態
    int state;
    // 受信バッファ
    int ptr;
    char buff[BLE_BUFF_SIZE];
    // 受信ハンドラ
    void (*onReceived)(const char* buff);
};
#endif