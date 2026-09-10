# 使用するESP32と設定方法

## Freenove ESP32-S3 Board Lite

Deneuve 2では、**Freenove ESP32-S3 Board Lite（カメラなしESP32-S3開発ボード）**を採用します。使用する個体はFlash 16MB、PSRAM 8MB（OPI）です。

![Freenove ESP32-S3 Board Liteの外観](https://raw.githubusercontent.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite/main/Board.png)

[メーカー公式リポジトリ](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite)

このシステムで採用する理由は、受光信号の処理、ログ保存への拡張、Wi-FiによるWeb表示を1台で扱えることです。

|用途|構成と現在の実装|
|---|---|
|フォトトランジスタ18点の観測|CD74HC4067を2個使い、ADCで順番に読み取る。必要な時間分解能が得られるかは実フレーム周期で評価する|
|将来の36点化|MUX2個の32入力を超えるため、MUX追加など取得回路の拡張と走査時間の評価が必要|
|捕獲候補ログの保存|Flashを不揮発ストレージとして利用できる。現行コードはRAM内の候補ログのみで、再起動で消える。Flashへの永続保存は未実装|
|Webから記録を参照|実行版は同じLANからWebログを参照できる。インターネットからの遠隔参照は未実装|

PSRAMは作業用メモリであり、電源を切っても残るストレージではありません。また、検出候補の件数は捕獲成功の件数とは区別します。

## ピン配置を確認する

![Freenove ESP32-S3 Board Liteの公式ピン配置](https://raw.githubusercontent.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite/main/ESP32S3_Lite_Pinout.png)

[公式ピン配置図を拡大表示](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite/blob/main/ESP32S3_Lite_Pinout.png)

**基板の印刷・ピンの位置番号と、プログラムのGPIO番号を混同しないでください。必ず上図のGPIO表記と照合して接続します。** 例えばコードの `1` はGPIO1であり、コネクタの1番目の端子という意味ではありません。配線は[ハードウェアのGPIO割当](./hardware.md)に従います。

画像出典：Freenove。メーカー公式リポジトリの画像を変更せず参照しています。[ライセンス：CC BY-NC-SA 3.0](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite/blob/main/LICENSE.txt)。

## Arduino IDEにESP32を追加する

1. macOSでArduino IDE 2を起動し、Arduino IDE → Settingsを開きます。
2. Additional boards manager URLs（追加のボードマネージャーのURL）に次を追加します。既存のURLは残してください。

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Espressifが案内するURL `https://espressif.github.io/arduino-esp32/package_esp32_index.json` を登録済みなら、そのまま利用できます。

3. Boards Managerで **esp32 by Espressif Systems** をインストールします。実行版・測定版で使用するコアは **3.3.11** です。初期接続確認スケッチのCIは3.3.10でも検証しています。
4. Tools → Board → esp32 → **ESP32S3 Dev Module** を選びます。
5. 実行版・個別ハードウェアテスト版には、Library Managerから **ESP32Servo 3.2.1** もインストールします。光学測定版には不要です。

## ツール設定

以下は本システムの設定値です。ネイティブUSB側で書き込みとシリアル通信を行う構成を示します。

|項目|設定値|
|---|---|
|Board|ESP32S3 Dev Module|
|USB CDC On Boot|Enabled|
|USB Mode|Hardware CDC and JTAG|
|CPU Frequency|240MHz (WiFi)|
|USB DFU On Boot|Disabled（項目が表示される場合）|
|Upload Mode|UART0 / Hardware CDC|
|Flash Mode|QIO 80MHz|
|Flash Size|16MB|
|PSRAM|OPI PSRAM（搭載容量8MB）|
|Partition Scheme|16M Flash (3MB APP/9.9MB FATFS)|
|Arduino Runs On|Core1|
|Events Run On|Core1|
|Upload Speed|921600|

USB Modeは提示されたUpload Modeに合わせた設定です。コアのバージョンやUSB Modeにより、メニューの表示項目は変わります。設定方法は[EspressifのCDC説明](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/cdc_dfu_flash.html)も参照してください。

Partition Schemeはアプリとデータ領域の割当です。上記の3MB APP / 9.9MB FATFSはコア3.3.11のメニューに存在します。「Default 16MB」という名前は環境によって異なるため、上記の具体的な項目を選びます。FATFS領域を選ぶだけではログは自動保存されません。現在のCIは別の既定パーティションでコンパイルしており、この表の設定一式による実機動作確認は未実施です。

## MacからUSBで書き込む

1. データ通信対応USBケーブルで、ボードのネイティブUSB側をMacへ接続します。
2. Tools → Portで接続後に現れた `/dev/cu.*` ポートを選択します。
3. 接続確認には `firmware/mosquito_trap/mosquito_trap.ino`、測定には `firmware/optical_measure/optical_measure.ino` を開きます。スケッチ名と親フォルダー名は一致させます。
4. Verifyでコンパイルし、Uploadで書き込みます。
5. シリアルモニタを **115200 baud** で開きます。接続確認スケッチでは約1秒ごとに `uptime_ms=... state=BRINGUP outputs=DISABLED` が表示されます。

**USB-UART側の端子を使う場合**は、シリアル出力先もUART側にするためUSB CDC On BootをDisabledにし、その端子のポートを選びます。メーカーの[USB-UART設定例](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite/blob/main/Arduino_Configuration_USB_UART.png)と[USB側の設定例](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite/blob/main/Arduino_Configuration_USB_OTG.png)で、端子と設定を照合してください。

初回に認識されない場合は、BOOTを押したままRESETを押して離し、BOOTを離してダウンロードモードに入ります。ポートを選び直して書き込み、必要ならRESETします。書き込みが不安定なら、まずケーブル・端子・ポートを確認し、Upload Speedを460800または115200に下げて試します。Upload Speedとシリアルモニタの115200 baudは別の設定です。

## 公式資料

- [Freenove ESP32-S3 Board Lite](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite)
- [Freenove：macOSのトラブルシューティング](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite/blob/main/ESP32S3-Troubleshoot-macOS.pdf)
- [Arduino IDEのダウンロード](https://www.arduino.cc/en/software)
- [Espressif：Arduinoコアの導入](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
- [Espressif：Toolsメニュー](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html)
