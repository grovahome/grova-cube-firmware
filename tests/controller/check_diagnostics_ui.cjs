// Browser workflow acceptance against production HTML, with no hardware access.
const fs=require('fs'),assert=require('assert');
const args={};for(let i=2;i<process.argv.length;i+=2)args[process.argv[i]]=process.argv[i+1];
const {chromium}=require(args['--playwright']||'playwright');
(async()=>{
 const d=JSON.parse(fs.readFileSync(args['--blank'],'utf8')),packets=[],errors=[];
 d.devices=[{type:'SHT4x',source:'SHT4x@ch0/0x44',channel:0,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:26},{id:'climate.humidity_pct',unit:'%',valid:true,value:70}]},{type:'SHT4x',source:'SHT4x@ch1/0x44',channel:1,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:27},{id:'climate.humidity_pct',unit:'%',valid:true,value:75}]}];
 d.epoch_s=Date.UTC(2026,10,13,8,0)/1000;d.time_synced=true;d.schedule_timezone='Europe/Berlin';
 const html=fs.readFileSync('src/controller/page.h','utf8').match(/R"HTML\(([\s\S]*)\)HTML"/)[1];
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
  d.fan_ports[0]={enabled:true,name:'Klima',mode:'automatic',fan_role:'exhaust',plan:{enabled:false,slots:[]},automation:{fallback_percent:35,rules:[{source:'SHT4x@ch0/0x44',signal:'climate.temperature_c',direction:'lower',target:24,minimum:18,maximum:30,above_percent:100},{source:'SHT4x@ch1/0x44',signal:'climate.humidity_pct',direction:'lower',target:65,maximum:75,above_percent:70}]},automation_status:{reason:'ABOVE_MAXIMUM',demand_percent:100,winning_rule:0,warning_mask:10,execution_block:'NONE',evaluated_age_ms:5,rules:[{source:'SHT4x@ch0/0x44',signal:'climate.temperature_c',unit:'C',input_state:'OK',value:32,age_ms:1200,observed_limit:'ABOVE_MAXIMUM',active_limit:'ABOVE_MAXIMUM',normal_demand_percent:100,requested_percent:100,selected:true,limit_action_percent:100,pending_ms:0,decision:'SELECTED'},{source:'SHT4x@ch1/0x44',signal:'climate.humidity_pct',unit:'%',input_state:'OK',value:80,age_ms:1500,observed_limit:'ABOVE_MAXIMUM',active_limit:'ABOVE_MAXIMUM',normal_demand_percent:100,requested_percent:70,selected:false,limit_action_percent:70,pending_ms:0,decision:'LOWER_PRIORITY'}]}};
  d.fan1.current_pct=60;d.equipment_revision++;d.rest_mode.enabled=false;await page.evaluate(()=>refresh());
  assert.equal(await page.locator('#warningPanel').isVisible(),true);assert.equal(await page.locator('#warningList>div').count(),2);
  await page.locator('#warningList').getByRole('button',{name:'Zur Regeldiagnose',exact:true}).first().click();assert.equal(await page.locator('#diagnosis_FAN1').getAttribute('open'),'');
  const panel=page.locator('#diagnosisContent_FAN1');let text=await panel.innerText();assert(text.includes('32 °C')&&text.includes('1,2 s')&&text.includes('60 %')&&text.includes('100 %'));assert(text.includes('höherer Priorität'));
  await page.evaluate(()=>refresh());assert.equal(await page.locator('#diagnosis_FAN1').getAttribute('open'),'');
  // Measured limit violation during debounce is visible without claiming the action is active.
  const status=d.fan_ports[0].automation_status;status.rules[0].active_limit='NONE';status.rules[0].pending_ms=3000;status.rules[0].decision='DELAY';status.rules[0].selected=false;await page.evaluate(()=>refresh());assert((await page.locator('#warningList').innerText()).includes('noch nicht aktiv'));assert((await panel.innerText()).includes('noch 3 s'));
  // Missing and stale inputs are listed individually and null never turns into 0 °C.
  status.reason='SENSOR_MISSING';status.demand_percent=35;status.rules[0]={...status.rules[0],input_state:'MISSING',value:null,age_ms:null,observed_limit:'NONE',active_limit:'NONE',normal_demand_percent:null,requested_percent:null,decision:'MISSING'};status.rules[1]={...status.rules[1],input_state:'STALE',value:null,age_ms:14000,observed_limit:'NONE',active_limit:'NONE',normal_demand_percent:null,requested_percent:null,decision:'STALE'};await page.evaluate(()=>refresh());text=await panel.innerText();assert(text.includes('Kein gültiger Messwert')&&text.includes('14 s'));assert((await page.locator('#warningList').innerText()).includes('Ersatzverhalten: 35 %'));assert.equal(await page.locator('#warningList>div').count(),2);
  // Paused diagnostics use fresh readings while controller requests are explicitly unavailable.
  d.rest_mode.enabled=true;status.execution_block='REST';status.rules[0]={...status.rules[0],input_state:'OK',value:32,age_ms:100,observed_limit:'ABOVE_MAXIMUM',decision:'REST'};await page.evaluate(()=>refresh());assert((await panel.innerText()).includes('Betrieb pausiert'));assert((await panel.innerText()).includes('Nicht ausgewertet'));
  // Recovery clears central warnings; manual/disabled ports never show old sensor warnings.
  status.rules=[];await page.evaluate(()=>refresh());assert.equal(await page.locator('#warningPanel').isVisible(),false);
  d.warning='SETTINGS';await page.evaluate(()=>refresh());assert((await page.locator('#warningList').innerText()).includes('Einstellungen oder Speicher'));d.warning='OK';
  d.fan_ports[0].mode='manual';await page.evaluate(()=>refresh());assert.equal(await page.locator('#diagnosis_FAN1').count(),0);assert.equal(await page.locator('#warningPanel').isVisible(),false);
  // Offline data is labelled unknown; this page must not issue any control POST.
  await page.evaluate(()=>showOffline());assert((await page.locator('#warningStatus').innerText()).includes('unbekannt'));assert((await page.locator('#warningCount').innerText()).includes('unbekannt'));assert.equal(packets.length,0);
  for(const width of [390,768]){await page.setViewportSize({width,height:900});assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1));}
  assert.deepEqual(errors,[]);console.log('Diagnostics UI passed: values/units/ages, rule contributions, priority, pending limits, missing/stale/fallback, paused data, warning recovery/navigation, offline and mobile; no control requests');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
