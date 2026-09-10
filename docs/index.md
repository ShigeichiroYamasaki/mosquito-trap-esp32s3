---
layout: home
hero:
  name: Deneuve 2
  text: ESP32-S3 でつくる蚊取りシステム
  tagline: Arduino IDE × macOS × USB / ソースと開発記録を GitHub で管理
  actions:
    - theme: brand
      text: 開発環境を準備
      link: /getting-started
    - theme: alt
      text: GitHubの実行コード
      link: https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/tree/main/firmware/deneuve_runtime
features:
  - title: USB で開発
    details: Mac の Arduino IDE から書き込み、シリアルモニタで状態を確認します。
  - title: コードと記録を一緒に管理
    details: スケッチ、配線情報、実験結果を同じリポジトリに保存します。
  - title: 小さく試して測定
    details: 接続確認から始め、人工皮膚・影の検出・吸引を個別に検証します。
---

現時点のシステム名は **Deneuve 2** です。使用ボードは **Freenove ESP32-S3 Board Lite（カメラなし）**です。

## 誘引・捕獲の仕組み

重曹とクエン酸で発生させる CO₂ と、半日以上履いた靴下を 36°C のヒーターで加温する「人工皮膚」で蚊を誘引する構想です。CO₂ 噴出口と人工皮膚は 10cm 角のケースに格納し、下面の 5V ファンを 555 タイマー IC で動かして、4〜6秒間隔で CO₂ と匂いを拡散します。トンボの複眼を模した赤外線受光素子配列で蚊の影の動きを検出し、サーボモータで掃除機のトリガーを操作して吸引します。

[機構の設計仕様を見る](./mechanism.md)

## ソースコード

| コード | 内容 |
| --- | --- |
| [実行版：deneuve_runtime.ino](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/blob/main/firmware/deneuve_runtime/deneuve_runtime.ino) | 18点読み取り・影の候補判定・自動吸引・Webログ |
| [実行版フォルダー一式](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/tree/main/firmware/deneuve_runtime) | 型定義ヘッダーとWi-Fi設定例を含む。Arduino IDEで使う際はこちらを参照 |
| [個別テスト：hardware_test.ino](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/blob/main/firmware/hardware_test/hardware_test.ino) | LED・受光・サーボ・掃除機の個別テスト |
| [接続確認：mosquito_trap.ino](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/blob/main/firmware/mosquito_trap/mosquito_trap.ino) | USBシリアル通信の初期確認 |

[リポジトリ全体](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3) · [実行版の使い方](./runtime.md) · [テストの使い方](./hardware-test.md)

## 全体接続図

指定済みの配線を機能単位でまとめた図です。3.3V系と5V系は共通GNDを使用します。5Vの給電元・容量、555・ヒーターの詳細回路は未確定です。

![Deneuve 2 全体接続図。受光18点、MUX2個、ESP32、LED2個、サーボ、掃除機と誘引部。](/circuits/overall.svg)

[全体図を拡大表示](/circuits/overall.svg)

## 機能ごとの回路図

各項目を開くと回路図を表示します。端子名で接続を示しており、部品の実際のピン位置を表す図ではありません。

::: details 01 フォトトランジスタ受光回路

100kΩを各素子に1本ずつ。コレクタをMUX入力、エミッタを共通GNDへ接続します。

![01 フォトトランジスタ受光回路](/circuits/receiver.svg)

[図を拡大表示](/circuits/receiver.svg)

:::

::: details 02 CD74HC4067 × 2

VCCは3.3V、ENはGND。S0〜S3を共有し、SIGはGPIO1・GPIO2へ分けます。

![02 CD74HC4067 × 2](/circuits/mux.svg)

[図を拡大表示](/circuits/mux.svg)

:::

::: details 03 赤外線LED × 2 と2N2222

各LEDに330Ω、各Baseに1kΩ。左GPIO10・右GPIO11で交互点灯します。2N2222の実端子順は使用製品のデータシートで確認します。

![03 赤外線LED × 2 と2N2222](/circuits/led.svg)

[図を拡大表示](/circuits/led.svg)

:::

::: details 04 サーボと掃除機

赤は5V、黒／茶は共通GND、信号はGPIO16。停止状態から2回押して強吸引、吸引後に1回押して停止します。

![04 サーボと掃除機](/circuits/servo.svg)

[図を拡大表示](/circuits/servo.svg)

:::

::: details 05 CO₂・人工皮膚・555拡散部

ここは機能構成図です。555の抵抗・コンデンサー値、ファン駆動回路、ヒーターの電源・温度制御回路は未確定です。

![05 CO₂・人工皮膚・555拡散部](/circuits/lure.svg)

[図を拡大表示](/circuits/lure.svg)

:::

[配線表・部品構成の詳細](./hardware.md)

## 現在の範囲

初期スケッチは毎秒のシリアルログのみを出力します。誘引・検出・吸引の機構は設計方針を記録した段階で、受光配列・LED・サーボの個別操作は[ハードウェアテスト版](./hardware-test.md)に収録しました。検出から吸引への自動連動とWebログは[実行版](./runtime.md)に収録していますが、実機の統合動作は未検証です。ヒーター制御は未実装です。Arduino IDE の設定は Flash Size: 16MB / PSRAM: OPI PSRAM（8MB） です。ボードのUSB 設定は確認予定で、電源と周辺部品は未確定です。誘引効果・検出性能・捕獲性能はまだ評価していません。
