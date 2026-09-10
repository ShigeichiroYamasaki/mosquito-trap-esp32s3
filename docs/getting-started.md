# macOS / Arduino IDE

## 準備

1. Arduino IDE 2 を起動します。
2. Settings の Additional boards manager URLs に次の URL を追加します。既存の URL は残してください。

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

3. Boards Manager で Espressif Systems の **esp32** をインストールします。このプロジェクトの初期コンパイル対象は **3.3.10** です。
4. データ通信対応 USB ケーブルでボードを Mac に接続します。
5. `firmware/mosquito_trap/mosquito_trap.ino` を開きます。スケッチ名と親フォルダー名は一致させます。

## ボードと USB の設定

使用ボードは **Freenove ESP32-S3（16MBモデル）**です。詳細な製品品番・モジュール刻印と 16MB 表記の内訳は確認予定です。製品の公式説明に従ってボードを選択し、Flash / USB 設定を確定します。PSRAM は使用者指定の **OPI PSRAM** とします。CI は引き続き `ESP32S3 Dev Module` を汎用のコンパイル対象とし、実機設定の検証とは区別します。

| 接続方法 | シリアルの設定 |
| --- | --- |
| ESP32-S3 のネイティブ USB | USB CDC On Boot を Enabled に設定。USB Mode / Upload Mode は製品の案内に従う |
| USB-UART ブリッジ側の端子 | 原則 USB CDC On Boot を Disabled に設定し UART 側のポートを選ぶ |

Arduino IDE の **Tools → PSRAM → OPI PSRAM** を選択してください。PSRAM 容量は別途確認する項目です。

USB 端子が複数ある場合、使用する端子の役割を確認してください。Tools → Port で接続後に追加された `/dev/cu.*` を選びます。USB-UART 用ドライバーは搭載チップに応じてメーカー公式の案内を確認します。

## 書き込みと確認

1. Verify でコンパイルします。
2. Upload で書き込みます。
3. シリアルモニタを **115200 baud** で開きます。
4. 約 1 秒ごとに `uptime_ms=... state=BRINGUP outputs=DISABLED` が表示されることを確認します。

初回に認識されない場合は、ボードの説明に従い BOOT を押したまま RESET を押して離し、その後 BOOT を離してダウンロードモードへ入れます。ポートを選び直して書き込み、必要なら RESET してください。まずケーブル・端子・選択ポートを確認します。

## 公式資料

- [Arduino IDE のダウンロード](https://www.arduino.cc/en/software)
- [Espressif: Arduino コアの導入](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
- [Espressif: USB CDC / DFU](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/cdc_dfu_flash.html)
- [Espressif: Tools メニュー](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html)
