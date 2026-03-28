# WT32-OSC: 32-Byte Programmable Wavetable Oscillator

KORG NTS-1 digital kit mkII 用カスタムオシレーター。
32サンプル周期波形テーブルエンジン + MIDI CC 経由の波形書き込み機能を持つ。

## 概要

80年代の波形メモリ音源と同じ「32バイト1周期」の波形テーブルを使用するオシレーター。
波形データはプリセット48種 + ユーザー書き込み可能6スロットの計54音色。
Stage A / Stage B に任意の音色をロードし、B ノブでモーフィングする。
外部から MIDI CC 経由で波形テーブルの内容を直接書き込むことができる。

## 機能

- 32サンプル波形テーブル × 54バンク（プリセット48 + ユーザー6）
- Stage A / B の独立したバンク選択
- A↔B クロスフェードモーフィング（B ノブ + shape LFO 対応）
- 補間モード切替（なし＝ステップ出力 / リニア補間＝滑らか）
- ビット深度リダクション（8bit → 1bit）
- デュアルオシレーターデチューン
- MIDI CC による波形テーブルのリアルタイム書き込み
- ダブルバッファリング（書き込み中の音切れ防止）

## パラメータ

NTS-1 mkII の A ノブは SDK 固定パラメータ shape に、B ノブは altshape にマッピングされる。
本実装はこれらを BnkA および Mrph として使用する。
エンコーダー操作対象の BnkB〜WrDt は SDK ユーザーパラメータ（OSC EDIT PARAM1-5）として定義する。

| # | 名前 | 操作範囲 | 操作子 | 機能 |
|---|------|---------|--------|------|
| shape | BnkA | 0-1023 | **A ノブ** | Stage A 波形バンク選択（内部で 0-63 にスケール） |
| altshape | Mrph | 0-1023 | **B ノブ** | A↔B モーフィング位置 |
| PARAM1 | BnkB | 0-63 | エンコーダー | Stage B 波形バンク選択 |
| PARAM2 | Ctrl | 0-15 | エンコーダー | bit[2:0]=ビット深度, bit[3]=補間 |
| PARAM3 | DTun | 0-100 | エンコーダー | デチューン量（0-50 cents） |
| PARAM4 | WrAd | 0-255 | CC | 書き込みアドレス設定 |
| PARAM5 | WrDt | 0-255 | CC | 書き込みデータ（自動インクリメント） |

shape LFO（`unit_runtime_osc_context_t.shape_lfo`、Q31）はレンダリング時に
Mrph のベース値へ加算され、モーフィング位置をリアルタイムに変調する。

## 波形バンク構成

| インデックス | 内容 |
|-------------|------|
| 0 | Sine |
| 1 | Square (50% duty) |
| 2 | Sawtooth |
| 3 | Triangle |
| 4 | Pulse 25% |
| 5 | Pulse 12.5% |
| 6 | Brass-like |
| 7 | Strings-like |
| 8 | Organ |
| 9 | Bell |
| 10 | Metallic |
| 11 | Soft pad |
| 12 | Bass |
| 13 | Harsh lead |
| 14 | Whistle |
| 15 | Vowel-A |
| 16-47 | 倍音合成プリセット（init 時に生成） |
| 48-53 | **ユーザー書き込みスロット** |

## 書き込みプロトコル

### アドレスマップ（PARAM4: WrAd）

| 値 | 書き込み先 |
|----|-----------|
| 0-31 | Stage A の sample[0-31] に直接書き込み |
| 32-63 | Stage B の sample[0-31] に直接書き込み |
| 64-95 | User Bank 0 の sample[0-31] |
| 96-127 | User Bank 1 の sample[0-31] |
| 128-159 | User Bank 2 の sample[0-31] |
| 160-191 | User Bank 3 の sample[0-31] |
| 192-223 | User Bank 4 の sample[0-31] |
| 224-255 | User Bank 5 の sample[0-31] |

WrAd の最大値は 255 であるため、MIDI CC 経由で書き込み可能なユーザーバンクは
User Bank 0-5 の6バンク。

### 書き込み手順

1. CC で WrAd を送信 → 書き込み先とインデックスを設定
2. CC で WrDt を連続送信 → サンプル値を書き込み（自動インクリメント）
3. 32サンプル書き込み完了時に自動コミット

### 値の変換

WrDt の CC 値（0-255）は内部で `signed_value = cc_value - 128` に変換される。
CC 値 128 が符号付き 0（無音相当）に対応する。

## ファイル構成

```
wt32-osc/
  Makefile         SDK テンプレート(dummy-osc)と同じもの
  config.mk        プロジェクト固有設定（PROJECT名、ソースファイル指定）
  header.c         unit_header_t 定義（パラメータ記述子）
  unit.cc          DSP 実装本体
  manifest.json    KONTROL Editor 用メタデータ
  wt32_sender.py   PC 側波形送信ツール
```

## ビルド手順

### 前提条件

- logue-sdk リポジトリ (https://github.com/korginc/logue-sdk)
- Docker（推奨）

### 手順

```bash
# 1. SDK をクローン
git clone https://github.com/korginc/logue-sdk.git
cd logue-sdk
git submodule update --init

# 2. プロジェクトを配置
cp -r platform/nts-1_mkii/dummy-osc platform/nts-1_mkii/wt32-osc
cp /path/to/wt32-osc/unit.cc        platform/nts-1_mkii/wt32-osc/
cp /path/to/wt32-osc/header.c       platform/nts-1_mkii/wt32-osc/
cp /path/to/wt32-osc/config.mk      platform/nts-1_mkii/wt32-osc/
cp /path/to/wt32-osc/manifest.json  platform/nts-1_mkii/wt32-osc/
cp /path/to/wt32-osc/Makefile  platform/nts-1_mkii/wt32-osc/

# 3. ビルド（Docker コンテナ内）
docker/run_interactive.sh
build nts-1_mkii/wt32-osc

# 4. 生成物
#    wt32_osc.nts1mkiiunit がプロジェクトディレクトリに出力される

# 5. 転送
#    KORG KONTROL Editor で .nts1mkiiunit をドラッグ＆ドロップ
```

Makefile は SDK テンプレート（dummy-osc）のものをそのまま使用できる。
`make` を直接実行するとツールチェインパスが解決されないため、
必ず Docker コンテナ内の `build` コマンドを使用すること。

### SDK v2 API

本実装は SDK v2 の common ヘッダー群に直接準拠している。

- `unit_init(const unit_runtime_desc_t *)` — プラットフォーム/API バージョン検証
- `unit_render(const float *in, float *out, uint32_t frames)` — float 出力（-1.0〜1.0）
- `unit_set_param_value(uint8_t id, int32_t value)` / `unit_get_param_value`
- ピッチ: `unit_runtime_osc_context_t.pitch` → `osc_w0f_for_note()`
- shape LFO: `unit_runtime_osc_context_t.shape_lfo`（Q31）→ `q31_to_f32()`
- 倍音生成: SDK 提供の `osc_sinf()`（LUT ベース）
- 補間: SDK 提供の `linintf()`
- クランプ: SDK 提供の `clipminmaxf(min, x, max)`

## コンパニオンツール

### wt32_sender.py

Python スクリプトで PC から波形データを MIDI 経由で送信する。

NTS-1 mkII MIDI 実装チャートに基づく送信プロトコル：
- BnkA / Mrph（A/B ノブ）: 14-bit CC ペア（CC#54+102、CC#55+103）
- BnkB〜WrDt（エンコーダー/CC パラメータ）: NRPN（MSB=0、LSB=0-4）、データ 14-bit

```bash
pip install python-rtmidi

# MIDI ポート一覧
python wt32_sender.py --list

# デモ波形を送信
python wt32_sender.py --port 0 --demo

# 数式から波形生成して送信
python wt32_sender.py --port 0 --formula "sin(x * 2 * pi) + 0.5 * sin(x * 4 * pi)" --target user:0

# 矩形波（デューティ比指定）
python wt32_sender.py --port 0 --formula "1 if x < 0.3 else -1" --target stage_a

# バイナリファイルから送信（32バイト符号付き整数）
python wt32_sender.py --port 0 --file wave.bin --target user:1

# インタラクティブモード
python wt32_sender.py --port 0 --interactive
```

### インタラクティブモードのコマンド

```
wt32> formula sin(x * 2 * pi)        # 数式から生成（変数 x = 0.0〜1.0）
wt32> harmonics 1:1.0 3:0.5 5:0.25   # 倍音指定
wt32> show                            # ASCII 波形表示
wt32> send stage_a                    # Stage A に送信
wt32> send user:3                     # ユーザーバンク3に送信（0-5）
wt32> bank a 51                       # Stage A をバンク51に切替
wt32> morph 50                        # モーフィング位置を50%に設定
wt32> param BnkB 6                    # パラメータ名で直接設定
wt32> quit
```

## メモリ使用量（実測値）

| セクション | サイズ |
|-----------|--------|
| text（コード） | 4,952 bytes |
| data（初期化済みデータ） | 168 bytes |
| bss（未初期化データ） | 2,224 bytes |
| **合計** | **7,344 bytes** |

## ライセンス

MITライセンス
