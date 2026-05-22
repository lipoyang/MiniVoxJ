#pragma once
#include <vector>
#include <array>
#include <stdint.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**********************************************************
    型定義
 **********************************************************/

// 母音
enum Vowel
{
    A = 0,  // ア a
    I,      // イ i
    U,      // ウ ɯ
    E,      // エ e
    O,      // オ o
    jA,     // ャ ^ja
    jU,     // ュ ^jɯ
    jO,     // ョ ^jo
    X       // 母音無し (促音・撥音)
};

// 子音
enum Consonant
{
    _0 = 0,           // ゼロ子音 ∅ (ア行)
    K, T, P,          // 無声破裂音 k, t, p 
    G, D, B,          // 有声破裂音 g, d, b
    TSj, TS,          // 無声破擦音 tɕ, ts (チ・ツ)
    DZj, DZ,          // 有声破擦音 dʑ, dz (ジ・ズ)
    Sj, S, H, Ch, Ph, // 無声摩擦音 ɕ, s, h, ç, ɸ (シ・ス・ハ・ヒ・フ)
    Zj, Z,            // 有声摩擦音 ʑ, z, (ジ・ズ)
    N, M, R,          // 鼻音 n, m / 弾き音 ɾ (ラ行)
    J, W,             // 接近音(半母音) j, w (ヤ行・ワ行)
    Q, Nn,            // 特殊なモーラ (促音ッ・撥音ン)
    L                // 小書き母音用 (ァ・ィ・ゥ・ェ・ォ・ャ・ュ・ョ) ※ニ文字で1モーラになる
};

// モーラ
struct Mora
{
    Consonant c; // 子音
    Vowel v;     // 母音
};

// アクセント句
struct Phrase {
    std::vector<Mora> moras;    // モーラ列
    uint16_t delimiter;         // 区切り文字
    int accent;                 // アクセント核の位置 (0～moras.size()-1)
};

// 励振音源の種類
enum class SourceType {
    Impulse,    // インパルス
    Noise,      // ノイズ
    Mixed       // 混合
};

// フォルマントパラメータ
struct FormantParams
{
    float f1, f2, f3;       // フォルマント周波数(第1, 第2, 第3)
    float bw1, bw2, bw3;    // フォルマント帯域幅(第1, 第2, 第3)
    float gain;             // ゲイン
    SourceType source;      // 励振の種類
};

// フォルマント区間
struct FormantSeg
{
    const FormantParams* params; // フォルマントパラメータ
    int   t_ms;                  // 継続時間[msec]
    float pitch;                 // ピッチ (0.5～2.0程度)
};

// 声種別
enum class VoiceType {
	Male = 0, // 
	Female,
	Cute,
	Anime
};

/**********************************************************
    クラス定義
 **********************************************************/

// 二次振幅共振フィルタ
class Resonator {
public:
    // fs: サンプリング周波数
    Resonator(uint32_t fs) : fs(static_cast<float>(fs)), 
        b1(0), b2(0), a0(0), z1(0.0), z2(0.0) {}

    // フィルタの状態をリセット
    void reset() {
        z1 = z2 = 0.0;
    }

    // フィルタ係数を設定
    // f: フォルマント周波数 [Hz]
    // bw: 帯域幅 [Hz]
    void set(float f, float bw) {
        float R = expf(-F_PI * bw / fs);
        b1 = -2.0f * R * cosf(2.0f * F_PI * f / fs);
        b2 = R * R;
        a0 = 1.0f + b1 + b2; // ゲイン調整（簡易版）
    }

    // フィルタ実行
    // x: 入力サンプル
    // 戻り値: 出力サンプル
    float process(float x) {
        float y = a0 * x - b1 * z1 - b2 * z2;
        z2 = z1;
        z1 = y;
        return y;
    }

private:
    const float F_PI = static_cast<float>(M_PI);
    const float fs;    // サンプリング周波数[Hz]
    float b1, b2, a0;  // フィルタ係数
    float z1, z2;      // フィルタの状態（遅延要素）
};

// インパルス音源
class ImpulseGen {
public:
    // fs: サンプリング周波数 [Hz]
    ImpulseGen(float fs) : fs(fs) {}

    // 次のサンプルを取得
    // f0: インパルス列の周波数 [Hz]
    float get(float f0) {
        phase += f0 / fs;
        if (phase >= 1.0) {
            phase -= 1.0;
            return 1.0;
        }
        return 0.0;
    }
private:
    const float fs;    // サンプリング周波数[Hz]
    float phase = 0.0; // 周期内の位相(0～1)
};

// ノイズ音源 (ガウスノイズ)
class NoiseGen {
public:
    // seed: 乱数のシード
    NoiseGen(uint32_t seed = 22695477) : state(seed) {}
    
    // 次のサンプルを取得
    float get(){
        float sum = 0.0f;
        for (int i = 0; i < 12; i++) {
            sum += random();
        }
        float val = sum - 6.0f;
        val /= 6.0;
        if(val > 1.0f) val = 1.0f;
        if(val < -1.0f) val = -1.0f;
        return val;
    }
private:
    const uint32_t a = 1664525;
    const uint32_t c = 1013904223;
    const float MAX = static_cast<float>(INT32_MAX + 1.0f);
    uint32_t state;

    // 乱数生成
    float random() {
        // LCG（線形合同法) による疑似乱数
        state = a * state + c;
        return static_cast<float>(static_cast<int32_t>(state)) / MAX;
    }
};

// 混合音源
class MixedGen {
public:
    // gen1: インパルス源
    // gen2: ノイズ源
    // k: 相補係数(0～1)
    MixedGen(ImpulseGen* gen1, NoiseGen* gen2, float k = 0.6f) :
        a(k), b(1.0f - k), _gen1(gen1), _gen2(gen2) {}

    // 次のサンプルを取得
    // f0: インパルス列の周波数 [Hz]
    float get(float f0) {
        return a * _gen1->get(f0) + b * _gen2->get();
    }
private:
    const float a, b;
    ImpulseGen* const _gen1;
    NoiseGen*   const _gen2;
};

// 日本語音声合成エンジン
class MiniVoxJ {
public:
    // sampleRate: サンプリング周波数[Hz]
    // frameLen: フレーム長 (サンプル数)
    MiniVoxJ(int sampleRate = 16000, int frameLen = 512) :
        _SampleRate(sampleRate),
        _FrameLen(frameLen),
        _SlotLen(sampleRate * T_SLOT / 1000),
        _impulse((float)sampleRate),
        _noise(),
        _mixed(&_impulse, &_noise),
        _voice_type(VoiceType::Male){}

    bool setText(const char* utf8_str, int max = 255);
    int  synthesize(int16_t* buffer);

    int  getStatus() const { return _status; }
    int  getSampleCnt() const { return _total_cnt; }
    
    void setVoiceType(VoiceType type) { _voice_type = type; }

    const int PROCESSING = 0;   // 処理中
    const int COMPLETE = 1;     // 合成完了
    const int ERROR = -1;       // エラー

private:
    // 時間[msec]をサンプル数に換算
    inline int ms2sa(int ms)
    {
        return _SampleRate * ms / 1000;
    }

    // ※メンバ変数の宣言順序に注意 (依存関係あり)

    const int T_SLOT = 5;   // 係数更新スロット時間[msec]

    const int _SampleRate;  // サンプリング周波数
    const int _FrameLen;    // フレーム長 (サンプル数)
    const int _SlotLen;     // 係数更新スロット長 (サンプル数)

    std::vector<FormantSeg> _formantSegs;   // フォルマント区間列
    std::array<Resonator,3> _resonators = { // 共振フィルタ
        Resonator(_SampleRate),
        Resonator(_SampleRate),
        Resonator(_SampleRate)
    };

    size_t _seg_cnt = 0;// フォルマント区間列カウント
    int _sub_cnt = 0;   // フォルマント区間サンプルカウント
    int _total_cnt = 0; // 累計サンプルカウント
    int _status = COMPLETE;// ステータス
    float _gain = 1.0f; // ゲイン
    float _pitch = 1.0f;// ピッチ
    SourceType _source = SourceType::Impulse; // 励振音源の種類
    ImpulseGen _impulse;// インパルス音源
    NoiseGen _noise;    // ノイズ音源
    MixedGen _mixed;    // 混合音源
    VoiceType _voice_type; // 声質
};
