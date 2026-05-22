# MiniVoxJ
マイコン向けの超軽量な日本語音声合成ライブラリです。

## 解説

* [イチから作る超軽量ローカル音声合成エンジン | ProtoPedia](https://protopedia.net/prototype/8364)

## ファイル
* [./lib/MiniVoxJ/](./lib/MiniVoxJ/) : エンジンのソースコード (アーキテクチャ非依存)
* このフォルダ : サンプルコードの PlatformIO プロジェクトフォルダ
* [./src/](./src/) : サンプルコード
* [./web_app/usb/](./web_app/usb/) : 上記サンプルコードと通信するWebアプリ (USB接続)
* [./web_app/ble/](./web_app/ble/) : 上記サンプルコードと通信するWebアプリ (BLE接続)
* [./pc_debug/](./pc_debug/) : Windows PC上での動作確認用の Visual Studio プロジェクト

## サンプルコードの対応マイコン
* Seeed Studio XIAO nRF52840
* Seeed Studio XIAO RP2040
* Seeed Studio XIAO ESP32C3

## 参照

* [ayutaz/tiny-formant-synth](https://github.com/ayutaz/tiny-formant-synth) : 母音・子音のパラメータはこちらから
* [ayutaz/vowel-playground](https://github.com/ayutaz/vowel-playground) : 声質のパラメータはこちらから
