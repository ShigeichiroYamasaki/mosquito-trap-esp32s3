# GitHub と公開手順

## ファイル構成

```text
firmware/mosquito_trap/mosquito_trap.ino  Arduino スケッチ
docs/                                    VitePress ドキュメント
.github/workflows/check.yml              PR / push 時の検証
.github/workflows/pages.yml              main から Pages 公開
```

Arduino IDE 本体やボードパッケージのバイナリはコミットせず、自作スケッチと設定・使用バージョンを管理します。秘密情報は `secrets.h` などの無視対象に置き、公開ドキュメントにも書き込まないでください。

## ローカルでドキュメントを見る

Node.js 24 を用意し、リポジトリのルートで実行します。

```sh
npm ci
npm run docs:dev
```

公開用ビルドの確認:

```sh
npm run docs:build
npm run docs:preview
```

## GitHub Pages

リポジトリの Settings → Pages → Build and deployment → Source を **GitHub Actions** にします。`main` に push すると Pages ワークフローが文書をビルド・公開します。初回は Actions 画面から手動実行もできます。

公開パスは `actions/configure-pages` が返す `base_path` を VitePress に渡すため、リポジトリ名を変更した場合も追従します。GitHub Pages は静的な開発ドキュメントであり、ESP32 の遠隔操作機能は含みません。

## 日々の変更

1. 作業ブランチでスケッチまたは Markdown を編集。
2. Arduino IDE の Verify と、必要に応じて実機書き込み・シリアル確認。
3. 文書は `npm run docs:build` で確認。
4. commit / push し、PR のチェック結果を確認して main に反映。

CI は Arduino CLI で同じ `.ino` をコンパイルします。仮のボード設定でのコンパイル成功は、実機の配線・USB 通信・捕獲性能の確認を意味しません。

[公式: VitePress の GitHub Pages 公開](https://vitepress.dev/guide/deploy#github-pages)

## 依存関係

VitePress は安定版 1.6.4 を固定しています。開発サーバーの既知の問題に対応するため、Vite を 6.4.3 に override しています。依存関係更新時はビルドと開発サーバーを再確認してください。
