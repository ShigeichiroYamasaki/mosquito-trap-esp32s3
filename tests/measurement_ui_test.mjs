import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const src=readFileSync(new URL('../firmware/optical_measure/WebUi.h',import.meta.url),'utf8').split('<script>')[1].split('</script>')[0];
const elements=new Map();
const node=()=>({textContent:'',disabled:false,className:'',children:[],appendChild(x){this.children.push(x)},replaceChildren(){this.children=[]},reportValidity(){return true}});
const get=id=>{if(!elements.has(id))elements.set(id,node());return elements.get(id)};
let data={running:false,session:1,count:1,target:30,condition:'open',ledUs:2000,muxUs:50,samples:8,intervalMs:250,distanceMm:100,resistorKohm:100,pixels:[
{pt:1,raw:[2000,1000,2000,1100],noise:[1,2,3,4],left:1000,right:900,avgLeft:1000,avgRight:900},
{pt:2,raw:[2000,2100,2000,1990],noise:[1,1,1,1],left:-100,right:10,avgLeft:-100,avgRight:10},
{pt:3,raw:[4095,0,4095,0],noise:[0,0,0,0],left:4095,right:4095,avgLeft:4095,avgRight:4095}]};
let fail=false;
const ctx=vm.createContext({document:{getElementById:get,createElement:node,querySelectorAll:()=>[]},fetch:async()=>{if(fail)throw Error('offline');return {ok:true,json:async()=>data}},setTimeout:()=>{},URLSearchParams,console,confirm:()=>false});
vm.runInContext(src,ctx);
await vm.runInContext('refresh()',ctx);
assert.equal(get('rows').children.length,3);
assert.equal(get('rows').children[1].className,'warning');
assert.equal(get('rows').children[2].className,'bad');
assert.equal(get('rows').children[1].children[3].textContent,-100);
assert.equal(get('download').disabled,false);
data={...data,running:true};await vm.runInContext('refresh()',ctx);
assert.equal(get('start').disabled,true);assert.equal(get('stop').disabled,false);assert.equal(get('download').disabled,true);
data={...data,running:false,count:0,pixels:[]};await vm.runInContext('refresh()',ctx);assert.equal(get('download').disabled,true);assert.equal(get('rows').children.length,0);
fail=true;await vm.runInContext('refresh()',ctx);assert.equal(get('start').disabled,true);assert.match(get('message').textContent,/offline/);
console.log('Measurement UI state and rendering tests passed');
