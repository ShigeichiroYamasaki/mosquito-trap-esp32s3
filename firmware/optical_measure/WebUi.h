#pragma once
#include <Arduino.h>
const char WEB_UI[] PROGMEM = R"HTML(<!doctype html>
<html lang="ja"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Deneuve 2 光学測定</title>
<style>
*{box-sizing:border-box}body{font-family:system-ui,sans-serif;margin:0;background:#f3f6fa;color:#19324e}main{max-width:1100px;margin:auto;padding:24px}h1{margin-bottom:4px}p{line-height:1.65}.panel{background:white;padding:20px;border-radius:12px;margin:20px 0}form{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:14px}label{font-size:14px}input,select,button{font:inherit;padding:9px;border:1px solid #a5b4c5;border-radius:6px;width:100%;margin-top:5px}button{cursor:pointer;background:#175ba2;color:white;border:0}button:disabled{opacity:.4;cursor:default}.actions{display:flex;gap:12px;flex-wrap:wrap}.actions>*{width:auto;min-width:130px}.scroll{overflow:auto}table{border-collapse:collapse;width:100%;white-space:nowrap;font-variant-numeric:tabular-nums}th,td{padding:9px;text-align:right;border-bottom:1px solid #dce3ec}th{background:#edf3fa}td:first-child,th:first-child{text-align:left}.warning{background:#fff1cf}.bad{background:#ffe1df}#status{font-weight:bold}small{color:#52677e}.legend{font-size:14px}#message{color:#a13912;white-space:pre-wrap}a{color:#175ba2}.averages{color:#52677e}progress{width:100%;height:16px}
</style><main>
<h1>Deneuve 2 光学測定</h1><p>18個の受光素子を、左右の赤外線LEDで測定。サーボは操作しません。</p>
<section class="panel"><h2>1. 測定条件</h2>
<form id="settings">
<label>条件ラベル<select name="condition"><option value="open">遮光なし (open)</option><option value="shadow">遮光あり (shadow)</option><option value="ambient">外光条件の比較 (ambient)</option></select></label>
<label>測定フレーム数<input name="frames" type="number" min="1" max="120" value="30" required></label>
<label>LED切替後の待ち (µs)<input name="ledUs" type="number" min="50" max="20000" value="2000" required></label>
<label>MUX切替後の待ち (µs)<input name="muxUs" type="number" min="5" max="1000" value="50" required></label>
<label>1点のADC平均回数<input name="samples" type="number" min="1" max="32" value="8" required></label>
<label>フレーム開始間隔 (ms)<input name="intervalMs" type="number" min="100" max="5000" value="250" required></label>
<label>距離 (mm・記録用)<input name="distanceMm" type="number" min="1" max="2000" value="100" required></label>
<label>抵抗 (kΩ・記録用)<input name="resistorKohm" type="number" min="1" max="1000" value="100" required></label>
</form><p><small>距離・抵抗・ラベルは記録用です。実際の配置や抵抗は手で変更してください。どのラベルでも同じ点灯順で測定します。開始すると前回のRAM内データを置き換えます。</small></p>
<div class="actions"><button id="start" type="button">測定を開始</button><button id="stop" type="button" disabled>途中で停止</button><button id="download" type="button" disabled>CSVを保存</button></div><p id="message" role="alert"></p></section>
<section class="panel"><h2>2. 結果</h2><p id="status" aria-live="polite">接続中…</p><progress id="progress" value="0" max="30"></progress><p id="recorded"></p>
<p class="legend">値は12bit ADC（0〜4095）。差分＝対応する消灯値−点灯値。表は最新フレーム、平均差分は今回の全フレームです。左右・各点は順次取得します。</p>
<div class="scroll"><table><thead><tr><th>入力</th><th>消灯L</th><th>左点灯</th><th>左差分</th><th>消灯R</th><th>右点灯</th><th>右差分</th><th>平均差分 L/R</th><th>最大振れ幅</th><th>確認の目安</th></tr></thead><tbody id="rows"></tbody></table></div>
<p class="legend">黄色：差分20未満または負値／赤色：いずれかのADC平均が20以下または4075以上。これは確認の目安で、飽和・故障を確定する判定ではありません。振れ幅は1点の平均に使ったサンプルの最大−最小です。</p>
</section><section class="panel"><h2>3. 比較の手順</h2><ol><li>光路を遮らず「遮光なし」で測定しCSVを保存。</li><li>細い不透明な物を光路に置き「遮光あり」で測定しCSVを保存。</li><li>左右の差分と端の受光素子を比較。LED待ち時間を変えるときは他の条件を固定。</li></ol><p>最大120フレームをRAMに保持します。再起動・新しい測定開始で消えるため、毎回CSVを保存してください。CSVは1フレームにつきPT1〜PT18の18行です。</p><p>これは光量・読み取り安定性の測定です。応答速度、距離、蚊の検出を直接測定するものではありません。</p></section>
</main><script>
const $=id=>document.getElementById(id);let latest=null,inFlight=false;
async function refresh(){
 try{const res=await fetch('/api',{cache:'no-store'});if(!res.ok)throw Error('HTTP '+res.status);const d=await res.json();latest=d;
 $('status').textContent=`測定 ${d.session} / ${d.condition}：${d.running?'測定中':d.count?'停止・保存可能':'未測定'} ${d.count}/${d.target}フレーム`;
 $('progress').max=d.target;$('progress').value=d.count;
 $('recorded').textContent=d.count||d.running?`記録条件：距離${d.distanceMm}mm / 抵抗${d.resistorKohm}kΩ / LED待ち${d.ledUs}µs / MUX待ち${d.muxUs}µs / 平均${d.samples}回 / 開始間隔${d.intervalMs}ms`:'';
 $('start').disabled=d.running||inFlight;$('stop').disabled=!d.running||inFlight;$('download').disabled=d.running||!d.count||inFlight;
 document.querySelectorAll('#settings input,#settings select').forEach(x=>x.disabled=d.running||inFlight);
 $('rows').replaceChildren();for(const p of d.pixels){let flags=[];if(p.raw.some(v=>v<=20||v>=4075))flags.push('ADC端付近');if(p.left<0||p.right<0)flags.push('負の差分');else if(p.left<20||p.right<20)flags.push('小さい差分');
 const tr=document.createElement('tr');if(flags.length)tr.className=flags.includes('ADC端付近')?'bad':'warning';
 const values=['PT'+p.pt,p.raw[0],p.raw[1],p.left,p.raw[2],p.raw[3],p.right,`${p.avgLeft} / ${p.avgRight}`,Math.max(...p.noise),flags.join('・')||'—'];
 for(const v of values){const td=document.createElement('td');td.textContent=v;tr.appendChild(td)}$('rows').appendChild(tr)}
 }catch(e){$('message').textContent='ESP32との通信を確認してください：'+e.message;$('start').disabled=true;$('stop').disabled=true;$('download').disabled=true}
}
async function command(path,body){inFlight=true;$('start').disabled=true;$('stop').disabled=true;$('message').textContent='';try{const r=await fetch(path,{method:'POST',body});if(!r.ok)throw Error(await r.text())}catch(e){$('message').textContent=e.message}finally{inFlight=false;await refresh()}}
$('start').onclick=()=>{if(!$('settings').reportValidity())return;if(latest?.count&&!confirm('前回の測定データを置き換えます。CSVは保存済みですか？'))return;command('/start',new URLSearchParams(new FormData($('settings'))))};
$('stop').onclick=()=>command('/stop');
$('download').onclick=async()=>{try{const r=await fetch('/data.csv');if(!r.ok)throw Error(await r.text());const blob=await r.blob();const url=URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download=`deneuve2-${Date.now()}-${latest?.condition||'measure'}.csv`;a.click();setTimeout(()=>URL.revokeObjectURL(url),10000)}catch(e){$('message').textContent=e.message}};
async function poll(){await refresh();setTimeout(poll,750)}poll();
</script></html>)HTML";
