// Browser workflow acceptance against production HTML, with no hardware access.
const fs=require('fs'),assert=require('assert');
const args={};for(let i=2;i<process.argv.length;i+=2)args[process.argv[i]]=process.argv[i+1];
const {chromium}=require(args['--playwright']||'playwright');
(async()=>{
 const d=JSON.parse(fs.readFileSync(args['--blank'],'utf8')),packets=[],errors=[];
 d.devices=[{type:'SHT4x',source:'SHT4x@ch0/0x44',channel:0,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:26},{id:'climate.humidity_pct',unit:'%',valid:true,value:70}]},{type:'SHT4x',source:'SHT4x@ch1/0x44',channel:1,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:27},{id:'climate.humidity_pct',unit:'%',valid:true,value:75}]}];
 d.epoch_s=Date.UTC(2026,10,13,8,0)/1000;d.time_synced=true;d.schedule_timezone='Europe/Berlin';
 const html=fs.readFileSync('src/founder/page.h','utf8').match(/R"HTML\(([\s\S]*)\)HTML"/)[1];
 const browser=await chromium.launch({headless:true,...(args['--browser']?{executablePath:args['--browser']}:{})});
 try{
  const page=await browser.newPage({viewport:{width:1100,height:950}});page.on('pageerror',e=>{errors.push(e.message);console.error('Page:',e.message);});
  await page.route('http://grova.test/**',async route=>{
   const req=route.request();if(req.url().endsWith('/api/v1/status'))return route.fulfill({json:d});
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
  await page.locator('#eventHistory summary').click();
  d.event_log={volatile:true,capacity:64,count:3,total:70,overwritten:67,events:[{sequence:70,type:'ABOVE_MAXIMUM',limit:25,epoch_s:d.epoch_s,uptime_ms:9000,device:'FAN1',name:'Klima <script>bad</script>',priority:1,source:'SHT4x@ch0/0x44',signal:'climate.temperature_c',unit:'C',value:26.5},{sequence:69,type:'FALLBACK_REQUIRED',epoch_s:null,uptime_ms:8000,device:'FAN1',name:'Klima',percent:35,value:null},{sequence:68,type:'BOOT',epoch_s:null,uptime_ms:0,value:null}]};
  await page.evaluate(()=>refresh());assert.equal(await page.locator('#eventList>div').count(),3);const text=await page.locator('#eventList').innerText();assert(text.includes('Grenze: 25 °C'));assert(text.includes('26,5 °C')&&text.includes('Uhrzeit unbekannt')&&text.includes('Ersatzverhalten erforderlich'));assert(text.includes('<script>bad</script>'));assert.equal(await page.locator('#eventList script').count(),0);assert((await page.locator('#eventMeta').innerText()).includes('67 ältere'));
  await page.evaluate(()=>refresh());assert.equal(await page.locator('#eventHistory').getAttribute('open'),'');
  const downloadPromise=page.waitForEvent('download');await page.locator('#downloadEvents').click();const download=await downloadPromise;const saved=JSON.parse(fs.readFileSync(await download.path(),'utf8'));assert.equal(saved.capacity,64);assert.equal(saved.volatile,true);assert.equal(saved.events[0].sequence,70);assert.equal(saved.cube_id,d.cube_id);assert.equal(saved.connection_live,true);
  // New RAM session replaces old entries, no local resurrection after a reboot.
  d.event_log={volatile:true,capacity:64,count:1,total:1,overwritten:0,events:[{sequence:1,type:'BOOT',epoch_s:d.epoch_s+10,uptime_ms:0,value:null}]};await page.evaluate(()=>refresh());assert.equal(await page.locator('#eventList>div').count(),1);assert(!(await page.locator('#eventList').innerText()).includes('Klima'));
  await page.evaluate(()=>showOffline());assert((await page.locator('#eventMeta').innerText()).includes('Letzter empfangener'));assert.equal(packets.length,0);
  for(const width of [390,768]){await page.setViewportSize({width,height:900});assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1));}
  assert.deepEqual(errors,[]);console.log('Event UI passed: bounded history display, escaped device text, unknown time, JSON download, reboot replacement, offline and mobile; no control writes');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
