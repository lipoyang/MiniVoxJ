/********** UIの要素 ***********/
// ボタン
const btn_connect    = document.getElementById('btn_connect');     // 接続
const btn_disconnect = document.getElementById('btn_disconnect');  // 切断
const btn_play       = document.getElementById('btn_play');        // 文字列の再生
const btn_voice      = document.getElementById('btn_voice');       // 声質設定
// テキスト
const text_kana    = document.getElementById('text_kana');  // アクセント付きカタカナ文字列
// セレクト
const select_voice = document.getElementById('select_voice'); // 声質選択
// 表示領域
const panel_connect = document.getElementById('panel_connect'); // 接続画面
const panel_main    = document.getElementById('panel_main');    // メイン画面

/********** コマンドの定数 ***********/
const COM_PLAY     = 'P'; // 音声再生コマンド
const COM_VOICE    = 'V'; // 声質設定コマンド
const CODE_STX     = '#'; // 電文の開始コード
const CODE_ETX     = '\n'; // 電文の終了コード

/********** BLEの定数 ***********/
// BLEサービスのUUID
const UUID_NUS_SERVICE = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const UUID_NUS_RX_CHAR = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';
const UUID_NUS_TX_CHAR = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';

/********** BLEの変数 ***********/
// BLEデバイス
let bleDevice = null;
// BLEキャラクタリスティック
let chrRX;
let chrTX;
// 接続状態
const BLEStatus = {
  Disconnected : 0,
  Connecting   : 1,
  Connected    : 2
};
let btState = BLEStatus.Disconnected;

/********** BLE関連の関数 ***********/
// BLE接続
async function connectBLE() {
  try {
    btState = BLEStatus.Connecting;

    // デバイスを取得 (サービスのUUIDでフィルタ)
    console.log("Requesting Bluetooth Device...");
    bleDevice = await navigator.bluetooth.requestDevice({
        filters: [{ services: [UUID_NUS_SERVICE] }],
    });
    // 切断時イベントハンドラの登録
    bleDevice.addEventListener('gattserverdisconnected', onDisconnected);
    // デバイスに接続
    console.log("Connecting to GATT Server...");
    const server = await bleDevice.gatt.connect();
    // サービスを取得
    console.log("Getting Service...");
    const service = await server.getPrimaryService(UUID_NUS_SERVICE);
    // キャラクタリスティックを取得
    console.log("Getting Characteristics...");
    chrRX         = await service.getCharacteristic(UUID_NUS_RX_CHAR);
    chrTX         = await service.getCharacteristic(UUID_NUS_TX_CHAR);
    // 受信時の処理
    chrRX.addEventListener('characteristicvaluechanged', onReceived);
    chrRX.startNotifications();

    btState = BLEStatus.Connected;

  } catch (error) {
    console.log("ERROR! " + error);
    bleDevice = null;

    btState = BLEStatus.Disconnected;
  }
}

// BLE切断
function disconnectBLE(){
  if(bleDevice != null){
    bleDevice.gatt.disconnect();
  }
}

// コマンドの送信
async function sendCommand(command, data)
{
  // 電文の生成
  const telegram =  CODE_STX + command + data + CODE_ETX;
  const encoder = new TextEncoder();
  const byteArray = encoder.encode(telegram);

  await chrTX.writeValue(byteArray).then(() => {
    console.log('send:' + command);
  }).catch(()=>{
    console.log('send error:' + command);
  });
}

/********** BLEのイベントハンドラ ***********/
// 切断時
function onDisconnected(event) {
  const device = event.target;
  console.log(`Device ${device.name} is disconnected.`);
  bleDevice = null;

  btState = BLEStatus.Disconnected;
}

// 受信時
function onReceived(event) {
  const characteristic = event.target;
  const value = characteristic.value;
  const decoder = new TextDecoder();
  const text = decoder.decode(value);
  console.log(text);
}

/********** UIのイベントハンドラ ***********/
// 「接続」ボタン
btn_connect.addEventListener('click', async function () {
  // BLE接続
  await connectBLE();
  if(btState != BLEStatus.Connected){
    error_toast("ERROR: Failed to connect BLE device.");
    return;
  }
  // 画面表示切替
  panel_connect.style.display = "none";
  panel_main.style.display = "block";
});

// 「切断」ボタン
btn_disconnect.addEventListener('click', async function (){
  if(bleDevice != null){
    await disconnectBLE();
  }
  // 画面表示切替
  panel_main.style.display = "none";
  panel_connect.style.display = "block";
});

// 「音声再生」ボタン
btn_play.addEventListener('click', async function (){
  if(btState == BLEStatus.Connected){
    const kana = text_kana.value;
    if(kana.length > 0){
      // コマンド送信
      await sendCommand(COM_PLAY, kana);
    }
  }
});

// 「声質設定」ボタン
btn_voice.addEventListener('click', async function (){
  if(btState == BLEStatus.Connected){
    const voiceId = select_voice.selectedIndex.toString();
    // コマンド送信
    await sendCommand(COM_VOICE, voiceId);
  }
});

/********** トースト ***********/

// トースト表示
function show_toast(message)
{
  console.log(message);

  const jsFrame = new JSFrame();
  jsFrame.showToast({
    html: message, align: 'top', duration: 3000
  });
}

// トースト表示(エラー)
function error_toast(message)
{
  console.log(message);

  const jsFrame = new JSFrame();
  jsFrame.showToast({
    html: message, align: 'top', duration: 5000
  });
}
