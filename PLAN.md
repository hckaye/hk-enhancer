# HK Enhancer — VST3 倍音エンハンサー 実装計画

## 概要

Waves Vitamin / Renaissance Bass にインスパイアされた、3バンド倍音エンハンサープラグイン。

## 技術スタック

- フレームワーク: JUCE 7 (CMake FetchContent)
- 言語: C++17
- ビルド: CMake
- フォーマット: VST3 / AU / Standalone

## DSP アーキテクチャ

```
Input (stereo)
    │
    ▼
MultibandSplitter (Linkwitz-Riley 4次, 200Hz / 4kHz 可変)
    │
    ├── Low  → SubharmonicGenerator (ゼロクロッシング検出でサブオクターブ生成)
    ├── Mid  → TubeSaturator (非対称tanh soft clipping)
    └── High → HarmonicExciter (HPF → 非線形処理 → エンベロープ追従)
    │
    ▼
3バンド合成 → Output Gain → 出力
```

## パラメータ

| パラメータ | 範囲 | デフォルト |
|---|---|---|
| Low Amount | 0–100% | 0% |
| Mid Amount | 0–100% | 0% |
| High Amount | 0–100% | 0% |
| Low/Mid Crossover | 80–500 Hz | 200 Hz |
| Mid/High Crossover | 1k–8k Hz | 4000 Hz |
| Output Gain | -12–+12 dB | 0 dB |
| Dry/Wet Mix | 0–100% | 100% |

## ディレクトリ構成

```
hk-enhancer/
├── CMakeLists.txt
├── .gitignore
└── Source/
    ├── PluginProcessor.h/cpp
    ├── PluginEditor.h/cpp
    ├── DSP/
    │   ├── MultibandSplitter.h/cpp
    │   ├── SubharmonicGenerator.h/cpp
    │   ├── TubeSaturator.h/cpp
    │   ├── HarmonicExciter.h/cpp
    │   └── EnvelopeFollower.h/cpp
    └── GUI/
        ├── LookAndFeel.h/cpp
        └── BandControl.h/cpp
```

## 実装フェーズ

### Phase 1: スケルトン
- CMakeLists.txt + JUCE FetchContent
- PluginProcessor (APVTS + パラメータ定義 + パススルー)
- PluginEditor (基本ノブUI)
- ビルド確認

### Phase 2: クロスオーバー
- MultibandSplitter (LR4 x2 + allpass位相補正)
- processBlockに統合、フラットレスポンス確認

### Phase 3: バンドプロセッサー
- EnvelopeFollower
- TubeSaturator (非対称tanh + オーバーサンプリング)
- HarmonicExciter (HPF + cubic soft clip + エンベロープ)
- SubharmonicGenerator (ゼロクロッシング + エンベロープ)

### Phase 4: 仕上げ
- パラメータスムージング (SmoothedValue)
- カスタムLookAndFeel (バンド色分け)
- レイテンシー報告
- テスト
