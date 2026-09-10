# Deneuve 2 — 蚊取りシステム

現時点のシステム名は **Deneuve 2** です。使用ボードは **Freenove ESP32-S3（16MBモデル）**です。PSRAM 設定は **OPI PSRAM** です。詳細品番・メモリ容量・USB 設定は実物を確認して確定します。

ESP32-S3 を macOS の Arduino IDE から USB 接続で開発するプロジェクトです。Arduino スケッチと VitePress の開発ドキュメントを同じリポジトリで管理します。

[開発ドキュメント](https://shigeichiroyamasaki.github.io/mosquito-trap-esp32s3/) · [GitHub](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3)

## 始め方

- Arduino IDE で `firmware/mosquito_trap/mosquito_trap.ino` を開く。
- ESP32 コア 3.3.10 と実際の製品に合うボード・USB 設定を選ぶ。
- USB で書き込み、115200 baud のシリアルモニタでログを確認。
- 文書: `npm ci` → `npm run docs:dev`（Node.js 24 推奨）。

詳細は [macOS セットアップ](docs/getting-started.md)、[公開手順](docs/workflow.md)、[開発計画](docs/roadmap.md) を参照してください。

## 機構

重曹とクエン酸由来の CO₂ と、半日以上履いた靴下を 36°C のヒーターで加温する「人工皮膚」で蚊を誘引します。CO₂ 噴出口と人工皮膚は 10cm 角のケースに格納し、下面の 5V ファンを 555 タイマー IC で動かして、4〜6秒間隔で CO₂ と匂いを拡散します。トンボの複眼を模した赤外線受光素子配列で蚊の影の動きを認識し、サーボモータで掃除機のトリガーを操作して吸引する構想です。

詳しくは [機構仕様](docs/mechanism.md) を参照してください。現段階のソフトはシリアル通信を確認するひな形で、機構の制御は未実装、実機の誘引・検出・捕獲性能は未検証です。
