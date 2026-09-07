import { chromium } from 'playwright';
import fs from 'fs';
const expected = JSON.parse(fs.readFileSync(new URL('./test_expected.json', import.meta.url),'utf8'));
const b = await chromium.launch({args:['--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
const p = await b.newPage({viewport:{width:1300,height:900}});
const errors=[]; p.on('console', m=>{ if(m.type()==='error'||m.type()==='warning') errors.push(m.type()+': '+m.text()); }); p.on('pageerror', e=>errors.push('pageerror: '+e.message));
await p.goto('http://127.0.0.1:8080/animal.html'); await p.waitForTimeout(1500);
const setRange=async(id,v)=>{ await p.evaluate(([id,v])=>{const e=document.getElementById(id); e.value=v; e.dispatchEvent(new Event('input',{bubbles:true}));},[id,v]); };
const setSel=async(id,v)=>{ await p.selectOption('#'+id, v); };
const setCheck=async(id,v)=>{ await p.evaluate(([id,v])=>{const e=document.getElementById(id); e.checked=v; e.dispatchEvent(new Event('change',{bubbles:true}));},[id,v]); };
// numerical check on swatches, split=0 (all simulated)
await p.setInputFiles('#fileInput',new URL('./test_swatch.png', import.meta.url).pathname); await p.waitForTimeout(800);
await setRange('split',0);
let maxErr=0;
for (const sp of Object.keys(expected)) {
  await setSel('species',sp); await p.waitForTimeout(400);
  const px = await p.evaluate(()=>{const c=document.getElementById('canvasOut'); const t=document.createElement('canvas'); t.width=c.width; t.height=c.height; const g=t.getContext('2d'); g.drawImage(c,0,0); return [0,1,2,3,4,5].map(i=>Array.from(g.getImageData(i*100+50,50,1,1).data).slice(0,3));});
  for (let i=0;i<6;i++){ const e=expected[sp][i]; const err=Math.max(...[0,1,2].map(k=>Math.abs(px[i][k]-e[k]))); maxErr=Math.max(maxErr,err); }
  console.log(sp, 'gpu', JSON.stringify(px), 'expected', JSON.stringify(expected[sp]));
}
console.log('MAX ABS ERROR (8-bit):', maxErr);
// screenshots on the sample image
await p.click('#sampleBtn'); await p.waitForTimeout(1000);
await setRange('split',50);
const shots=[['dog',{}],['cat',{}],['fruitfly',{}],['honeybee',{}],['blowfly',{}]];
for (const [sp] of shots){ await setSel('species',sp); await p.waitForTimeout(500); await p.locator('#canvasOut').screenshot({path:`shot_${sp}.png`}); console.log(sp, await p.textContent('#geomInfo')); }
await setSel('species','dog'); await setRange('light',0); await p.waitForTimeout(400); await p.locator('#canvasOut').screenshot({path:'shot_dog_dim.png'}); console.log('dog dim', await p.textContent('#geomInfo'));
await setRange('light',100); await setCheck('trueScale',true); await p.waitForTimeout(400); await p.locator('#canvasOut').screenshot({path:'shot_dog_truescale.png'}); console.log('dog true-scale', await p.textContent('#geomInfo'));
await setCheck('trueScale',false); await setSel('species','fruitfly'); await setCheck('hex',false); await p.waitForTimeout(400); await p.locator('#canvasOut').screenshot({path:'shot_fly_nohex.png'});
await p.screenshot({path:'shot_page.png', fullPage:true});
console.log('console errors:', errors.length? errors : 'none');
await b.close();
