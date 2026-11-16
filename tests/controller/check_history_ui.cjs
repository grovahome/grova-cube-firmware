// Browser workflow acceptance against production HTML, with no hardware access.
const fs=require('fs'),assert=require('assert');
const args={};for(let i=2;i<process.argv.length;i+=2)args[process.argv[i]]=process.argv[i+1];
const {chromium}=require(args['--playwright']||'playwright');
(async()=>{
 const d=JSON.parse(fs.readFileSync(args['--blank'],'utf8')),packets=[],errors=[];
 d.devices=[{type:'SHT4x',source:'SHT4x@ch0/0x44',channel:0,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:26},{id:'climate.humidity_pct',unit:'%',valid:true,value:70}]},{type:'SHT4x',source:'SHT4x@ch1/0x44',channel:1,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:27},{id:'climate.humidity_pct',unit:'%',valid:true,value:75}]}];
 d.epoch_s=Date.UTC(2026,10,13,8,0)/1000;d.time_synced=true;d.schedule_timezone='Europe/Berlin';
 let historyRequests=0,historyError=false;let history={volatile:true,capacity:120,interval_s:30,max_series:64,omitted:0,samples:[{uptime_ms:0,epoch_s:null},{uptime_ms:30000,epoch_s:null},{uptime_ms:60000,epoch_s:d.epoch_s}],series:[{source:'SHT4x@ch0/0x44',signal:'climate.temperature_c',kind:'sensor',unit:'C',values:[24,null,26]},{source:'SHT4x@ch1/0x44',signal:'climate.temperature_c',kind:'sensor',unit:'C',values:[30,31,32]},{source:'FAN1',signal:'actuation.percent',kind:'output',unit:'%',values:[0,40,70]}]};
 const html=fs.readFileSync('src/controller/page.h','utf8').match(/R"HTML\(([\s\S]*)\)HTML"/)[1];
 const browser=await chromium.launch({headless:true,...(args['--browser']?{executablePath:args['--browser']}:{})});
 try{
  const page=await browser.newPage({viewport:{width:1100,height:950}});page.on('pageerror',e=>{errors.push(e.message);console.error('Page:',e.message);});
  await page.route('http://grova.test/**',async route=>{
   const req=route.request();if(req.url().endsWith('/api/v1/history')){historyRequests++;return route.fulfill({status:historyError?503:200,json:historyError?{error:'unavailable'}:history});}if(req.url().endsWith('/api/v1/status'))return route.fulfill({json:d});
   if(req.url().endsWith('/api/v1/control')){
    const p=req.postDataJSON();packets.push(p);
    if(p.cmd==='set_rest_mode')d.rest_mode.enabled=p.enabled;
    if(p.cmd==='configure_equipment'){
     assert.equal(p.expected_revision,d.equipment_revision);
     d.equipment=p.equipment.map(e=>({...e,max_runtime_ms:e.max_runtime_ms||0,startup_remaining_ms:0,remaining_ms:0,requested_percent:0,applied_percent:0,reason:e.plan.enabled?'SCHEDULE':'REST'}));
     d.fan_ports=p.fan_ports;d.equipment_revision++;if(p.start_plans)d.rest_mode.enabled=false;
     for(const o of d.outputs)o.assigned=d.equipment.some(e=>e.output===o.id||e.power_output===o.id);
    }
    return route.fulfill({json:{ok:true,message:'Saved'}});
   }
   return route.fulfill({contentType:'text/html',body:html});
  });
  const idle=()=>page.waitForFunction(()=>!busy&&!statusBusy&&!pendingEquipment);
  await page.goto('http://grova.test/');await page.waitForSelector('#port_OUT12_1_active',{state:'attached'});await idle();
  assert.equal(historyRequests,0);await page.locator('#trendHistory summary').click();await page.waitForFunction(()=>!!historyData&&!historyBusy);
  assert.equal(await page.locator('#trendSeries option').count(),3);assert((await page.locator('#trendSummary').innerText()).includes('26 °C'));
  const lines=await page.evaluate(()=>{const c=el('trendCanvas').getContext('2d'),original=c.lineTo.bind(c);let calls=0;c.lineTo=(...p)=>{calls++;return original(...p);};drawHistory();c.lineTo=original;return calls;});assert.equal(lines,5,'Only grid lines: null gap must not be bridged');
  await page.locator('#trendSeries').selectOption('SHT4x@ch1/0x44|climate.temperature_c');assert((await page.locator('#trendSummary').innerText()).includes('32 °C'));
  history.samples.push({uptime_ms:90000,epoch_s:d.epoch_s+30});for(const series of history.series)series.values.push(null);await page.evaluate(()=>refreshHistory());assert.equal(await page.locator('#trendSeries').inputValue(),'SHT4x@ch1/0x44|climate.temperature_c');assert((await page.locator('#trendSummary').innerText()).includes('Kein gültiger Messwert'));
  historyError=true;await page.evaluate(()=>refreshHistory());assert((await page.locator('#trendStatus').innerText()).includes('zuletzt geladen'));assert.equal(await page.evaluate(()=>historyData.samples.length),4);
  historyError=false;history.samples=[{uptime_ms:0,epoch_s:null}];history.series=[{...history.series[0],values:[null]}];history.omitted=2;await page.evaluate(()=>refreshHistory());assert((await page.locator('#trendSummary').innerText()).includes('Noch keine gültigen'));assert((await page.locator('#trendStatus').innerText()).includes('2 Messreihen'));assert.equal(await page.evaluate(()=>historyData.samples.length),1);
  await page.evaluate(()=>showOffline());assert((await page.locator('#trendStatus').innerText()).includes('zuletzt geladen'));assert.equal(packets.length,0);
  for(const width of [390,768]){await page.setViewportSize({width,height:900});assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1));}
  assert.deepEqual(errors,[]);console.log('History UI passed: lazy read-only load, concrete sources, unit/range, null gaps, selected-source retention, stale failure, reboot reset, limits and mobile');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
