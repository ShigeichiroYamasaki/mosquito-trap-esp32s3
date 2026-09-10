---
layout: home
hero:
  name: Deneuve 2
  text: ESP32-S3 でつくる蚊取りシステム
  actions:
    - theme: brand
      text: 開発環境を準備
      link: /getting-started
    - theme: alt
      text: GitHubの実行コード
      link: https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/tree/main/firmware/deneuve_runtime
---

## 誘引・捕獲の仕組み

重曹とクエン酸で作るCO₂と、半日以上履いた靴下を36°Cのヒーターで加温する「人工皮膚」で蚊を誘引します。噴出口と人工皮膚を10cm角のケースに収め、下面の5Vファンを555タイマーで4〜6秒間隔に動かしてCO₂と匂いを拡散する設計です。

18点の赤外線受光配列で影の動きを検出し、サーボで掃除機のボタンを操作して吸引します。

![CO₂発生・人工皮膚・複眼検出・掃除機を組み合わせた誘引捕獲システム](/detection/lure-capture.svg)

[システム図を拡大表示](/detection/lure-capture.svg)

### CO₂濃度にリズムを持たせる理由 {#co2-rhythm}

**背景濃度を一時的に上回るCO₂の変化**を宿主探索の手掛かりとして利用するため、ファンで間欠的に拡散します。

Geierら（1999）のネッタイシマカの風洞実験では、均一なCO₂の流れより、濃度が変動する乱流や筋状の流れで風上への飛行が増えました。[Geier, Bosch & Boeckh, 1999：Influence of odour plume structure on upwind flight of mosquitoes towards hosts. Journal of Experimental Biology, 202, 1639–1648](https://doi.org/10.1242/jeb.202.12.1639)

Majeedら（2014）は1秒ON・1秒OFFの刺激を使い、背景CO₂濃度が高いと飛翔開始や発生源への接触が妨げられることを示しました。背景に対して刺激が目立つことが重要だと示唆されます。[Majeed, Hill & Ignell, 2014：Impact of elevated CO₂ background levels on the host-seeking behaviour of Aedes aegypti. Journal of Experimental Biology, 217, 598–604](https://doi.org/10.1242/jeb.092718)

**4〜6秒間隔は呼吸を模した仮設定です。これらの論文は最適周期を示していないため、対象の蚊と設置環境で評価します。**

一方、Hawkesら（2012）のハマダラカの実験では、足の匂いとCO₂の連続提示で夜の一部時間帯の活動量が増え、30秒ごとに5秒の提示では有意な増加がありませんでした。活動量の結果であり、捕獲率の比較ではありません。[Hawkes, Young & Gibson, 2012：Modification of spontaneous activity patterns in the malaria vector Anopheles gambiae sensu stricto when presented with host-associated stimuli. Physiological Entomology, 37, 233–240](https://doi.org/10.1111/j.1365-3032.2012.00838.x)

Geierらの実験では皮膚臭は均一な流れが有効でした。CO₂と匂いの同時拡散に加え、CO₂だけを断続化する構成も比較します。

同じCO₂総供給量で連続・間欠拡散を比較し、周期・ON時間・風量・背景濃度を記録します。**蚊が接近する位置の濃度波形**を十分な応答速度の測定器で確認し、接近数・候補数・捕獲数を分けて評価します。

### 6時間運転するためのCO₂生成量 {#co2-six-hours}

安静時の成人1人分を目安に、平均200mL/分、6時間で約72Lを生成する計算です。体積は**0℃・1気圧の乾燥ガス換算**です。[安静時のCO₂排出量の参考資料（NIOSH）](https://www.cdc.gov/niosh/docket/archive/pdfs/niosh-148-a/0148-A-091709-Williams_pres.pdf)

|材料・条件|6時間分の理論量|
|---|---:|
|重曹|約270g|
|無水クエン酸|約206g|
|クエン酸一水和物を使う場合|約225g（無水クエン酸の代わり）|
|CO₂生成量|約72L（標準状態換算）|

無水クエン酸206gを水に溶かし、**水溶液全体の体積を約1.03L**にします。水1.03Lに加えるという意味ではありません。一水和物なら225gを使い、同じ最終体積にします。

- 無水クエン酸換算の濃度：0.20g/mL（20gを溶かして全量100mL）。
- 必要な酸の供給量：約0.572g/分。
- 滴下速度：0.572 ÷ 0.20 ≈ **2.86mL/分**（約172mL/時）。
- 20滴＝1mLの滴下器なら約57滴/分、約1秒に1滴。ただし滴の大きさは器具と溶液で変わるため、数分間の滴下体積を測って調整します。

```text
C₆H₈O₇ + 3 NaHCO₃ → Na₃C₆H₅O₇ + 3 H₂O + 3 CO₂
```

材料量は完全反応を仮定した理論値です。溶解や滴下速度の変化があるため、**噴出口のCO₂流量を実測して調整します**。

555タイマーは拡散ファンを間欠動作させるもので、CO₂の生成自体を止めません。反応液と泡のための容積を確保し、反応容器のガス出口を閉塞させない構成にします。

[機構の設計仕様を見る](./mechanism.md)

## 複眼方式による検出

「蚊の形」ではなく、小さく局所的な影が連続して移動する振る舞いを捉えます。左右のLEDを独立に追跡し、起動時の校正で受光点ごとの光量差を補正します。

**移動を確認し、影が見えている間に吸引を開始する方針です。** 消失は追跡終了として記録し、捕獲成功とは判定しません。

![18点の配列で影の移動を確認し、消失前に吸引を開始する設計方針](/detection/tracking-capture.svg)

[3相フレーム・初期校正・時間追跡を図で読む](./detection-design.md)

3相取得・校正・時間追跡は実装済みです。**現行コードは消失時に判定するため、吸引開始条件の修正が必要です。** 視差による奥行き判定は次期計画です。

## ソースコード

| コード | 内容 |
| --- | --- |
| [光学測定プログラム一式](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/tree/main/firmware/optical_measure) | 左右LED・18点のADC比較、Web表示、CSV保存。 [使い方](./optical-measure.md) |
| [実行版フォルダー一式](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/tree/main/firmware/deneuve_runtime) | 18点の影の判定・吸引制御・Webログ。ヘッダーとWi-Fi設定例を含む |
| [個別テスト：hardware_test.ino](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/blob/main/firmware/hardware_test/hardware_test.ino) | LED・受光・サーボ・掃除機の個別テスト |
| [接続確認：mosquito_trap.ino](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/blob/main/firmware/mosquito_trap/mosquito_trap.ino) | USBシリアル通信の初期確認 |

[リポジトリ全体](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3) · [実行版の使い方](./runtime.md) · [テストの使い方](./hardware-test.md)

## 全体接続図

3.3V系と5V系は共通GNDを使用します。5V電源の容量、555・ヒーターの詳細回路は検討中です。

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

## 検証状況

誘引効果、6時間の安定供給、検出・捕獲性能、実機の統合動作は未検証です。ヒーター制御は未実装です。

[ESP32・Arduino IDEの設定](./getting-started.md) · [実験計画と記録](./roadmap.md)
