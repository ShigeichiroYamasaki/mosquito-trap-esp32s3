# 蚊取りシステム / mosquito-trap-esp32s3

ESP32-S3 を macOS の Arduino IDE から USB 接続で開発するプロジェクトです。Arduino スケッチと VitePress の開発ドキュメントを同じリポジトリで管理します。

## 始め方

- Arduino IDE で `firmware/mosquito_trap/mosquito_trap.ino` を開く。
- ESP32 コア 3.3.10 と実際の製品に合うボード・USB 設定を選ぶ。
- USB で書き込み、115200 baud のシリアルモニタでログを確認。
- 文書: `npm ci` → `npm run docs:dev`（Node.js 24 推奨）。

詳細は [macOS セットアップ](docs/getting-started.md)、[公開手順](docs/workflow.md)、[開発計画](docs/roadmap.md) を参照してください。

現段階はシリアル通信を確認するひな形です。捕獲方式・ボード製品型番は未確定で、捕獲制御は未実装です。
