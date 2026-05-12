#include <vector>
#include <initializer_list>
#include <algorithm>
#include "MiniVoxJ.h"
#include "utility/utf8_decode.h"

/**********************************************************
    定数・テーブル
 **********************************************************/

// 暫定定数 TODO
constexpr float F0  = 160.0f; // 基本周波数
constexpr float Amp = 0.5f * 32767.0f; // 振幅

// Unicode
static constexpr uint16_t KANA_BEGIN = 0x30A1; // カタカナの最初 ァ
static constexpr uint16_t KANA_END   = 0x30F3; // カタカナの最後 ン
static constexpr uint16_t LONG_MARK  = 0x30FC; // 長音記号 ※使用しないがいちおうサポート
static constexpr uint16_t ACCENT_MARK = 0x0027; // アクセント記号 '

// カナ → モーラ表 
static constexpr Mora KANA_TO_MORA[83] = {
    // ァアィイゥウェエォオ (U+30A1-U+30AA)
    {L,A},{_0,A},{L,I},{_0,I},{L,U},{_0,U},{L,E},{_0,E},{L,O},{_0,O},
    // カガキギクグケゲコゴ (U+30AB-U+30B4)
    {K,A},{G,A},{K,I},{G,I},{K,U},{G,U},{K,E},{G,E},{K,O},{G,O},
    // サザシジスズセゼソゾ (U+30B5-U+30BE)
    {S,A},{DZ,A},{Sj,I},{DZj,I},{S,U},{DZ,U},{S,E},{DZ,E},{S,O},{DZ,O},
    // タダサチヂッツヅテデトド (U+30BF-U+30C9)
    {T,A},{D,A},{TSj,I},{DZj,I},{Q,X},{TS,U},{DZ,U},{T,E},{D,E},{T,O},{D,O},
    // ナニヌネノ (U+30CA-U+30CE)
    {N,A},{N,I},{N,U},{N,E},{N,O},
    // ハバパヒビピフブプヘベペホボポ (U+30CF-U+30DD)
    {H,A},{B,A},{P,A},{Ch,I},{B,I},{P,I},{Ph,U},{B,U},{P,U},{H,E},{B,E},{P,E},{H,O},{B,O},{P,O},
    // マミムメモ (U+30DE-U+30E2)
    {M,A},{M,I},{M,U},{M,E},{M,O},
    // ャヤュユョヨ (U+30E3-U+30E8)
    {L,jA},{J,A},{L,jU},{J,U},{L,jO},{J,O},
    // ラリルレロ (U+30E9-U+30ED)
    {R,A},{R,I},{R,U},{R,E},{R,O},
    // ヮワヰヱヲン (U+30EE-U+30F3) ※ヰヱヲは使用しない
    {W,A},{W,A},{_0,I},{_0,E},{_0,O},{Nn,X},
};

//********** フォルマントパラメータ **********

// 母音 a,i,ɯ,e,o
static constexpr FormantParams VowelA = { 800, 1200, 2600, 80, 100, 120, 1.0f, SourceType::Impulse };
static constexpr FormantParams VowelI = { 300, 2300, 3000, 80, 120, 150, 1.0f, SourceType::Impulse };
static constexpr FormantParams VowelU = { 350, 1300, 2500, 80, 100, 120, 1.0f, SourceType::Impulse };
static constexpr FormantParams VowelE = { 500, 1900, 2600, 80, 100, 120, 1.0f, SourceType::Impulse };
static constexpr FormantParams VowelO = { 500,  800, 2400, 80, 100, 120, 1.0f, SourceType::Impulse };

// 半母音 j, w (ヤ行・ワ行)
static constexpr FormantParams ConsJ  = { 280, 2300, 3000, 80, 100, 120, 1.0f, SourceType::Impulse };
static constexpr FormantParams ConsW  = { 320, 750,  2300, 80, 100, 120, 1.0f, SourceType::Impulse };

// 弾き音 ɾ (ラ行)
static constexpr FormantParams ConsR  = { 350, 1500, 2500, 80, 120, 150, 0.3f, SourceType::Impulse };

// 鼻音 n, m
static constexpr FormantParams ConsN  = { 250, 1700, 2600, 180, 200, 250, 0.3f, SourceType::Impulse };
static constexpr FormantParams ConsM  = { 250, 1000, 2200, 180, 200, 250, 0.3f, SourceType::Impulse };

// 無声摩擦音 ɕ, s, h, ç, ɸ (シ・ス・ハ・ヒ・フ)
static constexpr FormantParams ConsSj = { 200, 3800, 6000, 500, 2500, 2000, 0.4f,  SourceType::Noise };
static constexpr FormantParams ConsS  = { 200, 5500, 7500, 500, 3000, 2000, 0.4f,  SourceType::Noise };
static constexpr FormantParams ConsHa = { 800, 1200, 2600, 200,  300,  400, 0.15f, SourceType::Noise };
static constexpr FormantParams ConsHe = { 500, 1900, 2600, 200,  300,  400, 0.15f, SourceType::Noise };
static constexpr FormantParams ConsHo = { 500,  800, 2400, 200,  300,  400, 0.15f, SourceType::Noise };
static constexpr FormantParams ConsCh = { 200, 3000, 5000, 500, 2000, 2000, 0.3f,  SourceType::Noise };
static constexpr FormantParams ConsPh = { 200, 2500, 4000, 500, 3000, 2000, 0.15f, SourceType::Noise };
// ※ h は後続の母音によって変種あり

// 有声摩擦音 ʑ, z (ジ・ズ)
static constexpr FormantParams ConsZj = { 200, 3800, 6000, 500, 2500, 2000, 0.3f, SourceType::Mixed };
static constexpr FormantParams ConsZ  = { 200, 5500, 7500, 500, 3000, 2000, 0.3f, SourceType::Mixed };

// 促音や無声破裂音・破擦音の無音区間
static constexpr FormantParams Silence = { 100, 100, 100, 100, 100, 100, 0.0f, SourceType::Impulse };

// 有声破裂音・破擦音の閉鎖区間 (ボイスバー)
static constexpr FormantParams VoiceBar = { 200, 200, 200, 100, 200, 300, 0.08f, SourceType::Impulse };

// 破裂音・破擦音のバースト (無声・有声 共通)
// k, t, p, tɕ, ts / g, d, b, dʑ, dz
static constexpr FormantParams BurstK   = { 300, 1800, 2600, 500, 2000, 2000, 0.30f, SourceType::Noise };
static constexpr FormantParams BurstT   = { 300, 4000, 5000, 500, 2000, 2000, 0.35f, SourceType::Noise };
static constexpr FormantParams BurstP   = { 300, 1000, 2300, 500, 1500, 2000, 0.30f, SourceType::Noise };
static constexpr FormantParams BurstTSj = { 300, 3800, 6000, 500, 2000, 2000, 0.35f, SourceType::Noise };
static constexpr FormantParams BurstTS  = { 300, 5500, 7500, 500, 2500, 2000, 0.35f, SourceType::Noise };

// 破裂音のVOT (無声・有声 共通)
// k, t, p / g, d, b
static constexpr FormantParams VotK = { 800, 1200, 2600, 300, 400, 500, 0.10f, SourceType::Noise };
static constexpr FormantParams VotT = { 800, 1200, 2600, 300, 400, 500, 0.10f, SourceType::Noise };
static constexpr FormantParams VotP = { 800, 1200, 2600, 300, 400, 500, 0.10f, SourceType::Noise };

//********** 音価 **********
typedef std::initializer_list<FormantSeg> Phone;

// 母音
static constexpr Phone V_A  = { { &VowelA, 120 } }; // a ア
static constexpr Phone V_I  = { { &VowelI, 120 } }; // i イ
static constexpr Phone V_U  = { { &VowelU, 120 } }; // ɯ ウ
static constexpr Phone V_E  = { { &VowelE, 120 } }; // e エ
static constexpr Phone V_O  = { { &VowelO, 120 } }; // o オ
static constexpr Phone V_jA = { { &ConsJ, 40 }, { &VowelA, 120 } }; // ^ja ャ
static constexpr Phone V_jU = { { &ConsJ, 40 }, { &VowelU, 120 } }; // ^jɯ ュ
static constexpr Phone V_jO = { { &ConsJ, 40 }, { &VowelO, 120 } }; // ^jo ョ

// 子音
static constexpr Phone C_J  = { { &ConsJ,   60 } }; // j ヤ
static constexpr Phone C_W  = { { &ConsW,   60 } }; // w ワ
static constexpr Phone C_R  = { { &ConsR,   30 } }; // ɾ ラ
static constexpr Phone C_N  = { { &ConsN,   80 } }; // n ナ
static constexpr Phone C_M  = { { &ConsM,   80 } }; // m マ
static constexpr Phone C_Sj = { { &ConsSj, 120 } }; // ɕ シ
static constexpr Phone C_S  = { { &ConsS,  120 } }; // s ス
static constexpr Phone C_Ha = { { &ConsHa,  80 } }; // h ハ
static constexpr Phone C_He = { { &ConsHe,  80 } }; // h ヘ
static constexpr Phone C_Ho = { { &ConsHo,  80 } }; // h ホ
static constexpr Phone C_Ch = { { &ConsCh, 100 } }; // ç ヒ
static constexpr Phone C_Ph = { { &ConsPh, 100 } }; // ɸ フ

static constexpr Phone C_K = { { &Silence,  80 }, { &BurstK, 10 }, { &VotK, 30 } }; // k カ
static constexpr Phone C_G = { { &VoiceBar, 60 }, { &BurstK,  5 }, { &VotK,  0 } }; // g ガ
static constexpr Phone C_T = { { &Silence,  70 }, { &BurstT, 10 }, { &VotT, 20 } }; // t タ
static constexpr Phone C_D = { { &VoiceBar, 50 }, { &BurstT,  5 }, { &VotT,  0 } }; // d ダ
static constexpr Phone C_P = { { &Silence,  70 }, { &BurstP, 10 }, { &VotP, 15 } }; // p パ
static constexpr Phone C_B = { { &VoiceBar, 50 }, { &BurstP,  5 }, { &VotP,  0 } }; // b バ

static constexpr Phone C_TSj = { { &Silence,  70 }, { &BurstTSj, 5 }, { &ConsSj, 80 } }; // tɕ チ
static constexpr Phone C_DZj = { { &VoiceBar, 40 }, { &BurstTSj, 5 }, { &ConsZj, 60 } }; // dʑ ジ
static constexpr Phone C_TS  = { { &Silence,  70 }, { &BurstTS,  5 }, { &ConsS,  70 } }; // ts ツ
static constexpr Phone C_DZ  = { { &VoiceBar, 40 }, { &BurstTS,  5 }, { &ConsZ,  50 } }; // dz ズ

// 特殊なモーラ
static constexpr Phone C_Q  = { { &Silence, 120 } }; // 促音 ッ
static constexpr Phone C_Nn = { { &ConsN,   120 } }; // 撥音 ン

/**********************************************************
    内部関数
 **********************************************************/

// 拗音など2文字で1モーラの場合の処理
// m1: 前の文字のモーラ(仮)
// m2: 後の文字のモーラ(仮)
// return: 2文字からなるモーラ (規則に合わない場合は前の文字のモーラ)
static Mora twoCharMora(const Mora &m1, const Mora &m2)
{
    Mora m = m1;
    Consonant c = m1.c;
    Vowel v = m2.v;

    // 拗音の場合 (ャ・ュ・ョ)
    if (m1.v == I && (v == jA || v == jU || v == jO)) {
        // 口蓋化子音の場合 (シャ行, ジャ行, チャ行, ヂャ行, ヒャ行)
        if (c == Sj || c == Zj || c == TSj || c == DZj || c == Ch) {
            if      (v == jA) m.v = A;
            else if (v == jU) m.v = U;
            else              m.v = O;
        }
        // その他の拗音の場合 (ア行、促音、撥音は除外)
        else if(c != _0 && c != Q && c != Nn){
            m.v = v;
        }
    }
    // シェ・ジェ・チェ・ヂェ
    else if ((c == Sj || c == Zj || c == TSj || c == DZj) && m1.v == I && (v == E)) {
        m.v = v;
    }
    // ティ・ディ
    else if ((c == T || c == D) && m1.v == E && v == I) {
        m.v = v;
    }
    // トゥ・ドゥ
    else if ((c == T || c == D) && m1.v == O && v == U) {
        m.v = v;
    }
    // ツァ・ツィ・ツェ・ツォ
    else if (c == TSj && m1.v == U && (v == A || v == I || v == E || v == O)) {
        m.v = v;
    }
    // ファ・フィ・フェ・フォ
    else if (c == Ph && m1.v == U && (v == A || v == I || v == E || v == O)) {
        m.v = v;
    }
    // ウィ・ウェ・ウォ
    else if (c == W && m1.v == U && (v == I || v == E || v == O)) {
        m.v = v;
    }
    return m;
}

// カナ列をモーラ列に変換
// kana: カナ列
// return: モーラ列
static Phrase convertKana2Mora(const std::vector<uint16_t>& kanas)
{
    Phrase phrase;
    phrase.accent = -1;

    for (size_t i = 0; i < kanas.size(); i++) {
        const uint16_t c = kanas[i];
        Mora m;

        // アクセント記号 ' の場合、直前のモーラがアクセント核になる
        if (c == ACCENT_MARK) {
            if (phrase.moras.size() == 0) continue; // 前のモーラがなければ無効
            phrase.accent = (int)phrase.moras.size() - 1;
            continue;
        }
        // 長音記号 ー
        if (c == LONG_MARK) {
            if (phrase.moras.size() == 0) continue; // 前のモーラがなければ無効
            const Mora& last = phrase.moras.back();
            // 前のンを伸ばす
            if (last.c == Nn) {
                m.c = Nn;
                m.v = X;
            }
            // 前の母音を伸ばす
            else {
                m.c = _0;
                m.v = last.v;
            }
        }
        // カタカナ
        else if (KANA_BEGIN <= c && c <= KANA_END){
            // カナ → モーラ変換
            m = KANA_TO_MORA[c - KANA_BEGIN];

            // 拗音等の規則に当てはまらなかった小書き母音の場合
            if (m.c == L) {
                if      (m.v == jA) m = { J, A }; // ャ → ヤ
                else if (m.v == jU) m = { J, U }; // ュ → ユ
                else if (m.v == jO) m = { J, A }; // ョ → ヨ
                else m.c = _0; // ァィゥェォ → アイウエオ
            }
            // 次のカナが小書き母音の場合 → 2文字で1モーラ
            if (i + 1 < kanas.size()) {
                const uint16_t c2 = kanas[i + 1];
                if (KANA_BEGIN <= c2 && c2 <= KANA_END) {
                    Mora m2 = KANA_TO_MORA[c2 - KANA_BEGIN];
                    if (m2.c == L) {
                        Mora _m = twoCharMora(m, m2); // 2文字で1モーラの処理
                        if (m.v != _m.v) {
                            m.v = _m.v;
                            i++; // 1文字進める
                        }
                    }
                }
            }
        }
        // カナ以外の文字
        else {
            continue;
        }
        // モーラを追加
        phrase.moras.push_back(m);
    }
    return phrase;
}

// 区切り文字かどうか
// ch: 文字
// return: 区切り文字ならtrue、そうでなければfalse
static bool isDelimiter(uint16_t ch) {
    switch (ch) {
        case 0x0020: // 半角スペース
        case 0x3000: // 全角スペース
        case 0x3001: // 、
        case 0x3002: // 。
        case 0x002C: // ,
        case 0x002E: // .
        case 0x003F: // ?
        case 0xFF1F: // ？
        case 0x002F: // /
            return true;
        default:
            return false;
    }
}

// ポーズ時間
// ch: 区切り文字
// return: ポーズ時間[msec]
static int pauseTime(uint16_t ch) {
    switch (ch) {
        case 0x0020: // 半角スペース
        case 0x3000: // 全角スペース
            return 100;
        case 0x3001: // 、
        case 0x002C: // ,
            return 300;
        case 0x3002: // 。
        case 0x002E: // .
        case 0x003F: // ?
        case 0xFF1F: // ？
            return 1000;
        case 0x002F: // /
        default:
            return 0;
    }
}

// UTF-16の文字列をアクセント句の列に変換
// utf16str: UTF-16の文字列
// return: アクセント句の列
static std::vector<Phrase> convertStr2Phrases(const std::vector<uint16_t>& utf16str)
{
    std::vector<Phrase> phrases;
    std::vector<uint16_t> word;

    // 1文字ずつ処理
    for (uint16_t ch : utf16str) {
        // 区切り文字の場合
        if (isDelimiter(ch) && !word.empty()) {
            Phrase phr = convertKana2Mora(word); // カナ列 → モーラ列
            phr.delimiter = ch; // 区切り文字
            phrases.push_back(phr);
            word.clear();
        }
        // 区切り文字でない場合、単語に1文字追加
        else {
            word.push_back(ch);
        }
    }
    // 最後のアクセント句が区切り文字なしで終わった場合
    if (!word.empty()) {
        Phrase phr = convertKana2Mora(word); // カナ列 → モーラ列
        phr.delimiter = 0; // 区切り文字なし
        phrases.push_back(phr);
    }
    return phrases;
}

// アクセント句の各モーラのピッチを計算
// phrase: アクセント句
// return: ピッチ列
static std::vector<float> computePitch(const Phrase& phrase)
{
    const int N = (int)phrase.moras.size();
    const int accent = phrase.accent;

    const float pitch_L  = 0.8f;
    const float pitch_H  = 1.2f;
    const float pitch_HH = 1.3f;

    std::vector<float> pitches(N);

    for(int i = 0; i < N; i++){
        if(i == accent){
            pitches[i] = pitch_HH;
        }else{
            if(i == 0){
                pitches[i] = pitch_L;
            }else{
                if(i < accent){
                    pitches[i] = pitch_H;
                }else{
                    pitches[i] = pitch_L;
                }
            }
        }
    }
    return pitches;
}

// フォルマント区間を追加
// segs: フォルマント区間列
// phone: 音価
// pitch: ピッチ
static void formantSegAdd(std::vector<FormantSeg> &segs, const Phone &phone, float pitch)
{
    for (const FormantSeg &seg : phone) {
        FormantSeg s = seg;
        s.pitch = pitch;
        segs.push_back(s);
    }
}

// モーラ列をフォルマント区間列に変換
// moras: モーラ列
// pitches: ピッチ列
// returns: フォルマント区間列
static std::vector<FormantSeg> convertMora2FormantSeg(const std::vector<Mora>& moras, const std::vector<float>& pitches)
{
    std::vector<FormantSeg> segs;

    int mora_pos = 0;
    for (Mora m : moras) {

        // ピッチ
        float pitch = pitches[mora_pos];
        mora_pos++;

        // 促音 ッ
        if (m.c == Q) {
            formantSegAdd(segs, C_Q, pitch);
            continue;
        }
        // 撥音 ン
        if (m.c == Nn) {
            formantSegAdd(segs, C_Nn, pitch);
            continue;
        }
        // 子音
        switch (m.c) {
        case _0:  break;
        case K:   formantSegAdd(segs, C_K,   pitch); break; // k  カ
        case G:   formantSegAdd(segs, C_G,   pitch); break; // g  ガ
        case S:   formantSegAdd(segs, C_S,   pitch); break; // s  ス
        case Sj:  formantSegAdd(segs, C_Sj,  pitch); break; // ɕ  シ
        case DZj: formantSegAdd(segs, C_DZj, pitch); break; // dʑ ジ
        case DZ:  formantSegAdd(segs, C_DZ,  pitch); break; // dz ズ
        case T:   formantSegAdd(segs, C_T,   pitch); break; // t  タ
        case D:   formantSegAdd(segs, C_D,   pitch); break; // d  ダ
        case TSj: formantSegAdd(segs, C_TSj, pitch); break; // tɕ チ
        case TS:  formantSegAdd(segs, C_TS,  pitch); break; // ts ツ
        case N:   formantSegAdd(segs, C_N,   pitch); break; // n  ナ
        case H:
        {
            switch (m.v) {
            case A:  formantSegAdd(segs, C_Ha, pitch); break; // h ハ
            case E:  formantSegAdd(segs, C_He, pitch); break; // h ヘ
            case O:  formantSegAdd(segs, C_Ho, pitch); break; // h ホ
            default: break;
            }
        }
        break;
        case Ch:  formantSegAdd(segs, C_Ch, pitch);  break; // ç ヒ
        case Ph:  formantSegAdd(segs, C_Ph, pitch);  break; // ɸ フ
        case P:   formantSegAdd(segs, C_P,  pitch);  break; // p パ
        case B:   formantSegAdd(segs, C_B,  pitch);  break; // b バ
        case M:   formantSegAdd(segs, C_M,  pitch);  break; // m マ
        case J:   formantSegAdd(segs, C_J,  pitch);  break; // j ヤ
        case R:   formantSegAdd(segs, C_R,  pitch);  break; // ɾ ラ
        case W:   formantSegAdd(segs, C_W,  pitch);  break; // w ワ
        default: break;
        }
        // 母音
        switch (m.v) {
        case A:   formantSegAdd(segs, V_A,  pitch); break; // ア a
        case I:   formantSegAdd(segs, V_I,  pitch); break; // イ i
        case U:   formantSegAdd(segs, V_U,  pitch); break; // ウ ɯ
        case E:   formantSegAdd(segs, V_E,  pitch); break; // エ e
        case O:   formantSegAdd(segs, V_O,  pitch); break; // オ o
        case jA:  formantSegAdd(segs, V_jA, pitch); break; // ャ ^ja
        case jU:  formantSegAdd(segs, V_jU, pitch); break; // ュ ^jɯ
        case jO:  formantSegAdd(segs, V_jO, pitch); break; // ョ ^jo
        default: break;
        }
    }
    return segs;
}

// アクセント句列をフォルマント区間列に変換
// phrases: アクセント句列
// returns: フォルマント区間列
static std::vector<FormantSeg> convertPhrases2FormantSeg(const std::vector<Phrase>& phrases)
{
    std::vector<FormantSeg> segs;

    for (const Phrase& phr : phrases)
    {
        // アクセント位置をもとにピッチ列を計算
        std::vector<float> pitches = computePitch(phr);

        // モーラ列, ピッチ列 → フォルマント区間列
        std::vector<FormantSeg> phr_segs = convertMora2FormantSeg(phr.moras, pitches);

        // ポーズの追加
        if (phr.delimiter != 0) {
            FormantSeg pause = { &Silence, pauseTime(phr.delimiter), 1.0f };
            phr_segs.push_back(pause);
        }

        segs.insert(segs.end(), phr_segs.begin(), phr_segs.end());
    }
    return segs;
}

// フォルマントパラメータの線形補間
// a: 現在のフォルマントパラメータ
// b: 次のフォルマントパラメータ
// k: 相補係数 (0～1)
static FormantParams interpolate(const FormantParams& a, const FormantParams& b, float k) {
    return {
        a.f1 + (b.f1 - a.f1) * k,
        a.f2 + (b.f2 - a.f2) * k,
        a.f3 + (b.f3 - a.f3) * k,
        a.bw1 + (b.bw1 - a.bw1) * k,
        a.bw2 + (b.bw2 - a.bw2) * k,
        a.bw3 + (b.bw3 - a.bw3) * k,
        (a.source == b.source) ? a.gain + (b.gain - a.gain) * k : a.gain,
        a.source,  // 励振音源の種類は現在のものを使用）
    };
}

/**********************************************************
    公開関数
 **********************************************************/

// 音声合成する文字列をセットする
// string: アクセント記号付きカタカナ文字列
// max: 最大文字数 (既定値=255)
// 戻り値: 成否
bool MiniVoxJ::setText(const char* utf8_str, int max)
{
    // UTF-8をデコード (uint16_t型1個で1文字)
    std::vector<uint16_t> utf16str = utf8_decode(utf8_str, max);

    // 文字列をアクセント句列に
    std::vector<Phrase> phrases = convertStr2Phrases(utf16str);

    // アクセント句列をフォルマント区間列に
    _formantSegs = convertPhrases2FormantSeg(phrases);

    // 変数の初期化
    _seg_cnt = 0;
    _sub_cnt = 0;
    _total_cnt = 0;
    _status = PROCESSING;
    _gain = 1.0f;
    _pitch = 1.0f;
    _source = SourceType::Impulse;

    for (Resonator& filter : _resonators) filter.reset();

    return true;
}

// 1フレームぶん音声合成する
// buffer: 音声波形の出力バッファ (フレーム長以上のバッファを外部で確保すること)
// 戻り値: 音声合成したサンプル数
int MiniVoxJ::synthesize(int16_t* buffer)
{
    // 処理中のフォルマントと次のフォルマント(あれば)
    if (_seg_cnt >= _formantSegs.size()) {
        _status = COMPLETE;
        return 0;
    }
    const FormantParams* cur = _formantSegs[_seg_cnt].params;
    const FormantParams* nxt = (_seg_cnt + 1 < _formantSegs.size())
                                ? _formantSegs[_seg_cnt + 1].params : cur;
    // 処理中のフォルマントの長さ(サンプル数)
    int seg_len = ms2sa(_formantSegs[_seg_cnt].t_ms);
    // ピッチ
    float pitch_cur = _formantSegs[_seg_cnt].pitch;
    float pitch_nxt = (_seg_cnt + 1 < _formantSegs.size()) ? _formantSegs[_seg_cnt + 1].pitch : pitch_cur;

    // 1フレームぶん1サンプルずつ
    for (int cnt = 0; cnt < _FrameLen; cnt++)
    {
        // 係数更新スロット境界でフィルタ係数更新
        if (_sub_cnt % _SlotLen == 0) {
            // フォルマント区間の終盤では次のフォルマント区間と滑らかにつなぐ補間
            float t = static_cast<float>(_sub_cnt) / seg_len;
            float k = (t > 0.7f) ? (t - 0.7f) / 0.3f : 0.0f;
            FormantParams p = interpolate(*cur, *nxt, k);

            // 共振フィルタ係数とゲインの設定
            _resonators[0].set(p.f1, p.bw1);
            _resonators[1].set(p.f2, p.bw2);
            _resonators[2].set(p.f3, p.bw3);
            _gain = p.gain;
            _source = p.source;

            // ピッチも滑らかにつなぐ
            _pitch = pitch_cur + (pitch_nxt - pitch_cur) * k;
        }

        // 励振音源
        float s;
        float f0 = F0 * _pitch;
        switch (_source) {
        case SourceType::Impulse: s = _impulse.get(f0); break;
        case SourceType::Noise:   s = _noise.get();     break;
        case SourceType::Mixed:   s = _mixed.get(f0);   break;
        default: s = _impulse.get(f0);
        }

        // 共振フィルタを順次通す
        for (Resonator& filter : _resonators) s = filter.process(s);

        // バッファに出力
        float output = Amp * _gain * s;
        if (output >  32767.0f) output =  32767.0f;
        if (output < -32767.0f) output = -32767.0f;
        buffer[cnt] = static_cast<int16_t>(output);

        // カウンタ更新
        _total_cnt++;
        _sub_cnt++;
        if (_sub_cnt >= seg_len) { // フォルマント区間の終わりか
            _sub_cnt = 0;
            _seg_cnt++;
            if (_seg_cnt >= _formantSegs.size()) {
                // 全ての音声合成が完了
                _status = COMPLETE;
                return cnt + 1;
            } else {
                // 次のフォルマント区間へ
                cur = _formantSegs[_seg_cnt].params;
                nxt = (_seg_cnt + 1 < _formantSegs.size())
                    ? _formantSegs[_seg_cnt + 1].params : cur;
                seg_len = ms2sa(_formantSegs[_seg_cnt].t_ms);

                for (Resonator& filter : _resonators) filter.reset();
            }
        }
    }
    return _FrameLen;
}
