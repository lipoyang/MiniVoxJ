#include "BleCom.h"

#if defined(ARDUINO_ARCH_MBED)    // XIAO nRF52840

// Nordic UART Service (NUS)
BLEService       svcNUS("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
BLECharacteristic chrRX("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLEWrite, BLE_PAYLOAD_SIZE);
BLECharacteristic chrTX("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLENotify, BLE_PAYLOAD_SIZE);

// 0x02/0x03だとデバッグしにくいので、#/\n を 電文開始/終了 に使う
// 電文開始
#define CODE_STX '#'
// 電文終了
#define CODE_ETX '\n'

// 電文開始待ち状態
#define STATE_READY     0
// 電文受信中状態
#define STATE_RECEIVING 1

// 初期化
// onReceived: 受信ハンドラ
void BleCom::begin(const char* localName, void (*onReceived)(const char*))
{
    this->onReceived = onReceived;
    this->state      = STATE_READY;
    this->ptr        = 0;

    this->isConnected = false;

    // BLEの開始
    if (!BLE.begin()) {
        Serial.println("ERROR: starting BLE module failed!");
        while (1);
    }
    // Connection Intervalの設定
    BLE.setConnectionInterval(6, 80); // 7.25ms - 100ms
    
    // アドバタイズするローカル名とサービスを設定
    BLE.setLocalName(localName);
    BLE.setAdvertisedService(svcNUS);

    // サービスにキャラクタリスティックを追加
    svcNUS.addCharacteristic(chrRX);
    svcNUS.addCharacteristic(chrTX);

    // サービスを追加
    BLE.addService(svcNUS);

    // 初期値
    chrTX.writeValue("");

    // アドバタイズ開始
    BLE.advertise();
}


// 受信ループ処理
void BleCom::loop()
{
    if(!isConnected){
        central = BLE.central();
        if (central)
        {
            isConnected = true;
            Serial.print("BLE Connected to central: ");
            Serial.println(central.address());
        }
    }else{
        if(central.connected())
        {
            if (chrRX.written())
            {
                int bytesRead = chrRX.readValue(payload, BLE_PAYLOAD_SIZE);
                payload[bytesRead] = '\0';
                Serial.print("Received: ");
                Serial.println(payload);
                this->receive();
            }
        }else{
            isConnected = false;
            Serial.print(F("BLE Disconnected from central: "));
            Serial.println(central.address());
             // BLEのアドバタイズ再開
             BLE.advertise();
        }
    }
}

void BleCom::receive()
{
    char c;
    
    for(int i = 0; i <= BLE_PAYLOAD_SIZE ; i++)
    {
        if(payload[i] == '\0') break;
        c = payload[i];

        switch(state)
        {
        /* 電文開始待ち状態 */
        case STATE_READY:
            /* 電文開始コードが来たら電文受信中状態へ */
            if(c == CODE_STX)
            {
                //serial->println("STX ");
                state = STATE_RECEIVING;
                ptr = 0;
            }
            break;
        /* 電文受信中状態 */
        case STATE_RECEIVING:
            /* もしも電文開始コードが来たら受信中のデータを破棄 */
            if(c == CODE_STX)
            {
                //serial->println("STX ");
                ptr = 0;
            }
            /* 電文終了コードが来たら、受信した電文のコマンドを実行 */
            else if(c == CODE_ETX)
            {
                //serial->println("ETX ");
                buff[ptr] = '\0';
                this->onReceived(buff);
                state = STATE_READY;
            }
            /* 1文字受信 */
            else
            {
                buff[ptr] = c;
                ptr++;
                if(ptr>=BLE_BUFF_SIZE)
                {
                    state = STATE_READY;
                }
            }
            break;
        default:
            state = STATE_READY;
        }
    }
}
#endif