---
layout: home
hero:
  name: 蚊取りシステム
  text: ESP32-S3 でつくる、測定できる試作機
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
    details: 接続確認から始め、捕獲方式の選定と効果測定に進みます。
---

## 現在の範囲

初期スケッチは毎秒のシリアルログのみを出力します。捕獲機構・センサー・ファン制御は未実装です。ボードの製品型番、捕獲方式、電源、部品は未確定であり、捕獲性能はまだ評価していません。
