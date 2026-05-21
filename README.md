# MiniVoxJ
マイコン向けの超軽量な日本語音声合成ライブラリです。

## 解説

* [イチから作る超軽量ローカル音声合成エンジン | ProtoPedia](https://protopedia.net/prototype/8364)

## サンプルコードの開発環境
* PlatformIO / Arduinoフレームワーク
* ライブラリ ( ```MiniVoxJ``` クラス ) 本体はアーキテクチャ非依存
* Windows PC上での動作確認は [こちら](./pc_debug) ( Visual Studioのプロジェクト )

## サンプルコードの対応マイコン
* Seeed Studio XIAO nRF52840
* Seeed Studio XIAO RP2040
* Seeed Studio XIAO ESP32C3

## 参照

* [ayutaz/tiny-formant-synth](https://github.com/ayutaz/tiny-formant-synth) : 母音・子音のパラメータはこちらから
* [ayutaz/vowel-playground](https://github.com/ayutaz/vowel-playground) : 声質のパラメータはこちらから
