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
      text: 開発計画を見る
      link: /roadmap
features:
  - title: USB で開発
    details: Mac の Arduino IDE から書き込み、シリアルモニタで状態を確認します。
  - title: コードと記録を一緒に管理
    details: スケッチ、配線情報、実験結果を同じリポジトリに保存します。
  - title: 小さく試して測定
    details: 接続確認から始め、人工皮膚・影の検出・吸引を個別に検証します。
---

現時点のシステム名は **Deneuve 2** です。使用ボードは **Freenove ESP32-S3**です。

## 誘引・捕獲の仕組み

重曹とクエン酸で発生させる CO₂ と、半日以上履いた靴下を 36°C のヒーターで加温する「人工皮膚」で蚊を誘引する構想です。CO₂ 噴出口と人工皮膚は 10cm 角のケースに格納し、下面の 5V ファンを 555 タイマー IC で動かして、4〜6秒間隔で CO₂ と匂いを拡散します。トンボの複眼を模した赤外線受光素子配列で蚊の影の動きを検出し、サーボモータで掃除機のトリガーを操作して吸引します。

[機構の設計仕様を見る](./mechanism.md)

## 現在の範囲

初期スケッチは毎秒のシリアルログのみを出力します。誘引・検出・吸引の機構は設計方針を記録した段階で、ヒーター・受光配列・サーボの制御は未実装です。Arduino IDE の設定は Flash Size: 16MB / PSRAM: OPI PSRAM（8MB） です。ボードの詳細品番・USB 設定は確認予定で、電源と周辺部品は未確定です。誘引効果・検出性能・捕獲性能はまだ評価していません。
