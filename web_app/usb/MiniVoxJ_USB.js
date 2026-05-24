/********** UIの要素 ***********/
// ボタン
const btn_connect    = document.getElementById('btn_connect');     // 接続
const btn_disconnect = document.getElementById('btn_disconnect');  // 切断
const btn_play       = document.getElementById('btn_play');        // 文字列の再生
const btn_voice      = document.getElementById('btn_voice');       // 声質設定
const btn_speed      = document.getElementById('btn_speed');       // 発話速度設定
// テキスト
const text_kana    = document.getElementById('text_kana');  // アクセント付きカタカナ文字列
// セレクト
const select_voice = document.getElementById('select_voice'); // 声質選択
// 入力
const input_speed  = document.getElementById('input_speed'); // 発話速度入力
// 表示領域
const panel_connect = document.getElementById('panel_connect'); // 接続画面
const panel_main    = document.getElementById('panel_main');    // メイン画面

/********** コマンドの定数 ***********/
const COM_PLAY     = 'P'; // 音声再生コマンド
const COM_VOICE    = 'V'; // 声質設定コマンド
const COM_SPEED    = 'S'; // 発話速度設定コマンド
const CODE_STX     = '#'; // 電文の開始コード
const CODE_ETX     = '\n'; // 電文の終了コード

/********** シリアル通信の定数・変数 ***********/
let comPort = null; // COMポート
let reader = null;  // COMポートのリーダー
let closeRequested = false;

/********** シリアル通信処理 ***********/

// コマンドの送信
async function sendCommand(command, data)
{
  // 電文の生成
  const telegram =  CODE_STX + command + data + CODE_ETX;
  const encoder = new TextEncoder();
  const byteArray = encoder.encode(telegram);

  // 電文の送信
  const writer = comPort.writable.getWriter();
  await writer.write(byteArray);
  writer.releaseLock();
}

// COMポートからのデータ受信ループ
async function startReadLoop(port) {
  reader = port.readable.getReader();
  try {
    while (!closeRequested) {
      const { value, done } = await reader.read();
      if (done) break;  // reader.cancel() などで終了
      if (value) {
        const text = new TextDecoder("utf-8").decode(value);
        console.log(text);
      }
    }
  } finally {
    reader.releaseLock();
  }
}
/********** UIのイベントハンドラ ***********/
// 「接続」ボタン
btn_connect.addEventListener('click', async function () {
  try {
    // COMポートを開く
    comPort = await navigator.serial.requestPort();
    await comPort.open({ baudRate: 115200 });
    
    // 画面表示切替
    panel_connect.style.display = "none";
    panel_main.style.display = "block";

    // COMポートからのデータ受信ループ開始
    closeRequested = false;
    startReadLoop(comPort);

  } catch (error) {
    error_toast("ERROR: " + error)
    comPort = null;
  }
});

// 「切断」ボタン
btn_disconnect.addEventListener('click', async function (){
  if(comPort != null){
    closeRequested = true;
    if(reader != null){
      await reader.cancel()
    }
    await comPort.close();
  }
  // 画面表示切替
  panel_main.style.display = "none";
  panel_connect.style.display = "block";
});

// 「音声再生」ボタン
btn_play.addEventListener('click', async function (){
  if(comPort != null){
    const kana = text_kana.value;
    if(kana.length > 0){
      // コマンド送信
      await sendCommand(COM_PLAY, kana);
    }
  }
});

// 「声質設定」ボタン
btn_voice.addEventListener('click', async function (){
  if(comPort != null){
    const voiceId = select_voice.selectedIndex.toString();
    // コマンド送信
    await sendCommand(COM_VOICE, voiceId);
  }
});

// 「発話速度設定」ボタン
btn_speed.addEventListener('click', async function (){
  if(comPort != null){
    const speed = input_speed.value.toString();
    // コマンド送信
    await sendCommand(COM_SPEED, speed);
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
