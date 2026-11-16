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
  const manage=async id=>{await page.locator('#nav-manage').click();await page.locator('#connectorSelect').selectOption(id);};
  const save=async()=>{await page.locator('#saveEquipment').click();await idle();};
  await manage('FAN1');await page.locator('#port_FAN1_active').check();await page.locator('#port_FAN1_role').selectOption('exhaust');await page.locator('#port_FAN1_mode').selectOption('automatic');
  await page.locator('#port_FAN1_sensor_add').click();let rules=page.locator('#port_FAN1_sensor_rules .sensor-rule');
  assert.equal(await rules.nth(0).locator('.rule-source option').count(),4);
  await rules.nth(0).locator('.rule-target').fill('24');await rules.nth(0).locator('.rule-min').fill('18');await rules.nth(0).locator('.rule-max').fill('30');await rules.nth(0).locator('.rule-above').fill('100');
  await page.locator('#port_FAN1_sensor_add').click();await rules.nth(1).locator('.rule-source').selectOption('SHT4x@ch1/0x44|climate.humidity_pct');await rules.nth(1).locator('.rule-target').fill('65');await rules.nth(1).locator('.rule-max').fill('75');await rules.nth(1).locator('.rule-above').fill('70');
  await rules.nth(1).getByRole('button',{name:'Höher priorisieren',exact:true}).click();assert.equal(await rules.nth(0).locator('.rule-target').inputValue(),'65');
  let before=packets.length;await rules.nth(0).locator('.rule-min').fill('80');await save();assert.equal(packets.length,before);assert((await page.locator('#message').innerText()).includes('Minimum'));await rules.nth(0).locator('.rule-min').fill('');
  await page.locator('#port_FAN1_fallback').fill('35');await save();assert.equal(d.fan_ports[0].fan_role,'exhaust');assert.equal(d.fan_ports[0].automation.rules[0].source,'SHT4x@ch1/0x44');assert.equal(d.fan_ports[0].automation.rules[0].above_percent,70);assert.equal(d.fan_ports[0].automation.fallback_percent,35);
  await page.reload();await idle();await manage('FAN1');assert.equal(await rules.nth(1).locator('.rule-target').inputValue(),'24');assert.equal(await page.locator('#port_FAN1_role').inputValue(),'exhaust');
  d.devices=[];await page.evaluate(()=>refresh());assert.equal(await rules.nth(1).locator('.rule-target').inputValue(),'24');assert((await rules.nth(0).locator('.rule-source option:checked').innerText()).includes('Nicht verbunden'));await save();assert.equal(d.fan_ports[0].automation.rules.length,2);
  d.devices=[{type:'SHT4x',source:'SHT4x@ch1/0x44',channel:1,address:68,signals:[{id:'climate.humidity_pct',unit:'%',valid:true,value:70}]}];await page.evaluate(()=>refresh());assert(!(await rules.nth(0).locator('.rule-source option:checked').innerText()).includes('Nicht verbunden'));assert.equal(await rules.nth(0).locator('.rule-target').inputValue(),'65');
  await manage('FAN2');await page.locator('#port_FAN2_active').check();await page.locator('#port_FAN2_role').selectOption('circulation');await page.locator('#port_FAN2_mode').selectOption('scheduled');await page.locator('#port_FAN2_pattern').selectOption('interval');await page.locator('#port_FAN2_every').fill('180');await page.locator('#port_FAN2_interval_duration').fill('10');await page.locator('#port_FAN2_interval_percent').fill('40');await save();assert.deepEqual(d.fan_ports[1].plan.interval,{every_minutes:180,anchor_minute:0,duration_minutes:10,percent:40});
  await page.locator('#nav-overview').click();assert((await page.locator('#fanCard2').innerText()).includes('Alle 180 min'));assert((await page.locator('#fanCard1').innerText()).includes('Ziel 65'));
  await page.reload();await idle();await manage('FAN2');assert.equal(await page.locator('#port_FAN2_pattern').inputValue(),'interval');assert.equal(await page.locator('#port_FAN2_interval_percent').inputValue(),'40');
  // Sensor-driven bounded pump and relay validation.
  d.devices=[{type:'SHT4x',source:'SHT4x@ch0/0x44',channel:0,address:68,signals:[{id:'climate.humidity_pct',unit:'%',valid:true,value:50}]}];
  await page.evaluate(()=>refresh());await manage('OUT5_1');await page.locator('#port_OUT5_1_active').check();await page.locator('#port_OUT5_1_profile').selectOption('pump');await page.locator('#port_OUT5_1_mode').selectOption('automatic');await page.locator('#port_OUT5_1_sensor_add').click();const pump=page.locator('#port_OUT5_1_sensor_rules .sensor-rule');await pump.locator('.rule-direction').selectOption('raise');await pump.locator('.rule-target').fill('60');await page.locator('#port_OUT5_1_sensor_pulse').fill('0.5');await page.locator('#port_OUT5_1_sensor_cooldown').fill('60');await save();const assigned=d.equipment.find(e=>e.profile==='pump');assert(assigned,await page.locator('#message').innerText());assert.equal(assigned.automation.pulse_ms,500);assert.equal(assigned.automation.cooldown_s,3600);assert.equal(assigned.automation.fallback_percent,0);
  await pump.locator('.rule-max').fill('80');await pump.locator('.rule-above').fill('50');before=packets.length;await save();assert.equal(packets.length,before);assert((await page.locator('#message').innerText()).includes('0 oder 100'));await pump.locator('.rule-above').fill('0');await save();
  await manage('FAN1');await page.locator('#port_FAN1_mode').selectOption('manual');await save();assert.equal(d.fan_ports[0].automation.rules.length,2);await page.locator('#port_FAN1_mode').selectOption('automatic');assert.equal(await rules.count(),2);
  // Deactivation never requires completing an unfinished sensor rule.
  await rules.nth(0).locator('.rule-target').fill('');await page.locator('#port_FAN1_active').uncheck();await save();assert.equal(d.fan_ports[0].enabled,false);
  for(const width of [390,768]){await page.setViewportSize({width,height:900});for(const tab of ['overview','manage','settings']){await page.evaluate(t=>showView(t),tab);assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1));}}
  assert.deepEqual(errors,[]);console.log('Automation UI passed: concrete sensor signals, goals/limits/actions, priority, validation, reload, missing source, fan roles, intervals, bounded pump, removal and mobile');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
