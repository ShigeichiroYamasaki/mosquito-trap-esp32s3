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

現時点のシステム名は **Deneuve 2** です。使用ボードは **Freenove ESP32-S3 Board Lite（カメラなし）**です。

## 誘引・捕獲の仕組み

重曹とクエン酸で発生させる CO₂ と、半日以上履いた靴下を 36°C のヒーターで加温する「人工皮膚」で蚊を誘引する構想です。CO₂ 噴出口と人工皮膚は 10cm 角のケースに格納し、下面の 5V ファンを 555 タイマー IC で動かして、4〜6秒間隔で CO₂ と匂いを拡散します。トンボの複眼を模した赤外線受光素子配列で蚊の影の動きを検出し、サーボモータで掃除機のトリガーを操作して吸引します。

### 誘引・捕獲システムの構成

![CO₂発生・人工皮膚・複眼検出・掃除機を組み合わせた誘引捕獲システム](/detection/lure-capture.svg)

[システム図を拡大表示](/detection/lure-capture.svg)

### CO₂濃度にリズムを持たせる理由 {#co2-rhythm}

CO₂は、量や平均濃度だけでなく、**蚊がいる位置で背景濃度より一時的に高くなる、断続的な濃度変化**も宿主探索の手掛かりになります。Deneuve 2では、555タイマーによるファンの間欠動作でCO₂を含む空気を繰り返し送り出し、この変化を作ることを目指します。

Geierら（1999）は、ネッタイシマカの雌を用いた風洞実験で、均一なCO₂の流れよりも、乱流や細い筋状に途切れる流れで風上へ飛ぶ蚊の割合が増えることを示しました。均一なCO₂にも飛翔を開始させる効果はありましたが、風上への飛行は持続しにくい結果でした。これは濃度変動を設ける方針の根拠です。[Geier, Bosch & Boeckh, 1999：Influence of odour plume structure on upwind flight of mosquitoes towards hosts. Journal of Experimental Biology, 202, 1639–1648](https://doi.org/10.1242/jeb.202.12.1639)

Majeedら（2014）は、1秒ON・1秒OFFのCO₂刺激を用い、背景CO₂濃度が高いと刺激が目立ちにくくなり、飛翔開始や発生源への接触が妨げられることを示しました。この結果も、単に周囲全体の濃度を高めるだけでなく、背景からの濃度上昇を作ることの重要性を示しています。この論文は2秒周期の最適性を検証したものではありません。[Majeed, Hill & Ignell, 2014：Impact of elevated CO₂ background levels on the host-seeking behaviour of Aedes aegypti. Journal of Experimental Biology, 217, 598–604](https://doi.org/10.1242/jeb.092718)

**4〜6秒間隔は呼吸を模した設計上の仮設定であり、論文が最適周期と証明した値ではありません。** 濃度変動の有効性と、一定の呼吸リズムの有効性は区別します。また、上記の主な実験対象はネッタイシマカであり、対象となる蚊の種類と設置環境で確認が必要です。

断続化が常に有利とも限りません。Hawkesら（2012）のハマダラカの実験では、人の足の匂いとCO₂を連続提示した条件で夜の一部時間帯の活動量が増えましたが、30秒ごとに5秒提示した条件では有意な増加がありませんでした。これは活動量の評価であり、そのまま捕獲率を示すものではありません。[Hawkes, Young & Gibson, 2012：Modification of spontaneous activity patterns in the malaria vector Anopheles gambiae sensu stricto when presented with host-associated stimuli. Physiological Entomology, 37, 233–240](https://doi.org/10.1111/j.1365-3032.2012.00838.x)

さらにGeierらの実験では、人の皮膚臭は均一な流れの方が有効でした。したがって、CO₂と靴下の匂いを同じファンで断続的に送る構成は、組み合わせとして評価します。必要に応じて「匂いは継続して拡散し、CO₂だけを断続化する」構成も比較対象にします。

検証では、同じCO₂総供給量で連続拡散と間欠拡散を比較し、周期・ON時間・風量・背景濃度を記録します。ファンのON/OFFがそのまま濃度波形になるとは限らないため、**蚊が接近する位置で濃度の時間変化を測定**し、接近数・候補数・捕獲数を分けて評価します。濃度測定器には、対象周期を捉えられる応答速度が必要です。

### 6時間運転するためのCO₂生成量 {#co2-six-hours}

安静時の成人1人分の目安として、CO₂を平均200mL/分、6時間で約72L発生させる条件です。計算上の体積は**0℃・1気圧の乾燥ガス換算**で、噴出口の温度で測る体積とは異なります。[安静時のCO₂排出量の参考資料（NIOSH）](https://www.cdc.gov/niosh/docket/archive/pdfs/niosh-148-a/0148-A-091709-Williams_pres.pdf)

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
クエン酸 + 重曹3分子 → クエン酸三ナトリウム + 水3分子 + CO₂3分子
C₆H₈O₇ + 3 NaHCO₃ → Na₃C₆H₅O₇ + 3 H₂O + 3 CO₂
```

これらは完全反応を仮定した材料量です。混ざり方、CO₂の水への溶解、液面低下による滴下速度の変化などがあるため、**噴出口でCO₂流量を測って調整します**。6時間の安定供給や誘引・捕獲性能を実証した値ではありません。

555タイマーは拡散ファンを間欠動作させるもので、CO₂の生成自体を止めません。反応液と泡のための容積を確保し、反応容器のガス出口を閉塞させない構成にします。

[機構の設計仕様を見る](./mechanism.md)

## 複眼方式による検出

「蚊の形」ではなく、小さく局所的な影が連続して移動する振る舞いを捉えます。左右のLEDを独立に追跡し、起動時の校正で受光点ごとの光量差を補正します。

**影の消失を待たず、影が見えている間に連続した移動を確認して吸引を開始する方針です。** 影の消失は追跡終了の記録に使い、捕獲成功とは判定しません。短時間で通過したかどうかは、記録の分析に利用します。

![18点の配列で影の移動を確認し、消失前に吸引を開始する設計方針](/detection/tracking-capture.svg)

[3相フレーム・初期校正・時間追跡を図で読む](./detection-design.md)

現在は3相取得・校正・時間追跡を実装しています。**現行コードの検出イベントは消失時に発生するため、消失前の吸引開始にはコードの修正が必要です。**左右照明の視差による奥行き判定は次期計画です。検出・捕獲性能は実機で未評価です。

## ソースコード

| コード | 内容 |
| --- | --- |
| [光学測定プログラム一式](https://github.com/ShigeichiroYamasaki/mosquito-trap-esp32s3/tree/main/firmware/optical_measure) | 左右LED・18点のADC比較、Web表示、CSV保存。 [使い方](./optical-measure.md) |
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

初期スケッチは毎秒のシリアルログのみを出力します。誘引・検出・吸引の機構は設計方針を記録した段階で、受光配列・LED・サーボの個別操作は[ハードウェアテスト版](./hardware-test.md)に収録しました。検出から吸引への自動連動とWebログは[実行版](./runtime.md)に収録していますが、実機の統合動作は未検証です。ヒーター制御は未実装です。Arduino IDE の設定は Flash Size: 16MB / PSRAM: OPI PSRAM（8MB） です。USB設定は[ESP32とArduino IDEの設定](./getting-started.md)に記載しています。設定一式の実機検証、電源容量や周辺部品の詳細は確認が必要です。誘引効果・検出性能・捕獲性能はまだ評価していません。
