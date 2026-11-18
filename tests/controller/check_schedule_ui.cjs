// Browser workflow acceptance against production HTML, with no hardware access.
const fs=require('fs'),assert=require('assert');
const args={};for(let i=2;i<process.argv.length;i+=2)args[process.argv[i]]=process.argv[i+1];
const {chromium}=require(args['--playwright']||'playwright');
(async()=>{
 const d=JSON.parse(fs.readFileSync(args['--blank'],'utf8')),packets=[],errors=[];
 d.epoch_s=Date.UTC(2026,10,13,8,0)/1000;d.time_synced=true;d.schedule_timezone='Europe/Berlin';
 const html=fs.readFileSync('src/controller/page.h','utf8').match(/R"HTML\(([\s\S]*)\)HTML"/)[1];
 const browser=await chromium.launch({headless:true,...(args['--browser']?{executablePath:args['--browser']}:{})});
 try{
  const page=await browser.newPage({viewport:{width:1100,height:950}});page.on('pageerror',e=>errors.push(e.message));
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
  const bulk=async id=>{await page.locator('#nav-manage').click();if(!await page.locator('#connectorSelect').isVisible())await page.locator('#bulkEquipment').click();await page.locator('#connectorSelect').selectOption(id);};
  await bulk('OUT12_1');
  await page.locator('#port_OUT12_1_active').check();await page.locator('#port_OUT12_1_profile').selectOption('light_switch');
  assert.equal(await page.locator('#port_OUT12_1_name').inputValue(),'Licht');assert.equal(await page.locator('#port_OUT12_1_plan_enabled').isChecked(),true);
  const slots=page.locator('#port_OUT12_1_plan_slots .plan-slot');
  await slots.nth(0).locator('.plan-start').fill('08:00');await slots.nth(0).locator('.plan-end').fill('10:00');
  await page.locator('#port_OUT12_1_plan_add').click();await slots.nth(1).locator('.plan-start').fill('12:00');await slots.nth(1).locator('.plan-end').fill('14:00');
  await page.evaluate(()=>refresh());assert.equal(await slots.count(),2);assert.equal(await slots.nth(1).locator('.plan-start').inputValue(),'12:00');
  // Reject overlaps before any request is sent.
  await slots.nth(1).locator('.plan-start').fill('09:00');await page.locator('#saveEquipment').click();assert.equal(packets.length,0);
  assert((await page.locator('#message').innerText()).includes('überschneiden'));
  await slots.nth(1).locator('.plan-start').fill('12:00');await page.locator('#saveEquipment').click();await idle();
  assert.equal(d.equipment[0].voltage,12);assert.equal(d.equipment[0].plan.slots.length,2);assert.equal(d.equipment[0].plan.slots[1].start_minute,720);assert.equal(packets.at(-1).start_plans,false);assert.equal(d.rest_mode.enabled,true);
  await page.reload();await page.waitForSelector('#port_OUT12_1_active',{state:'attached'});await idle();
  assert.equal(await page.locator('#port_OUT12_1_plan_slots .plan-slot').count(),2);
  await page.locator('#nav-overview').click();await page.locator('#rest').click();assert.match(await page.locator('#resumeDevices').innerText(),/Licht/);await page.locator('#confirmResume').click();await idle();assert((await page.locator('#equipment').innerText()).includes('08:00–10:00'));assert((await page.locator('#dev1_next').innerText()).includes('heute 10:00 · Aus'));assert.equal(await page.locator('#dev1_history').count(),0);
  await page.locator('#equipment').getByRole('button',{name:'Zeitplan bearbeiten'}).click();await page.locator('#wizardNext').click();await page.locator('#wizardNext').click();
  assert.equal(await page.locator('#port_OUT12_1_plan_slots .plan-slot').count(),2);
  if(args['--screenshot'])await page.locator('#assignmentCard').screenshot({path:args['--screenshot']});
  // Editing an active plan pauses first. Starting operation remains a separate action.
  await page.locator('#wizardBack').click();await page.locator('#wizardBack').click();
  await page.locator('#port_OUT12_1_name').fill('Regallicht');const before=packets.length;
  for(let i=0;i<4;i++)await page.locator('#wizardNext').click();await idle();assert.equal(packets[before].cmd,'set_rest_mode');assert.equal(packets[before].enabled,true);assert.equal(packets[before+1].cmd,'configure_equipment');assert.equal(d.equipment[0].name,'Regallicht');assert.equal(d.rest_mode.enabled,true);await bulk('OUT12_1');
  await page.locator('#port_OUT12_1_plan_slots .plan-slot').nth(1).getByRole('button',{name:'Entfernen'}).click();await page.locator('#saveEquipment').click();await idle();assert.equal(d.equipment[0].plan.slots.length,1);
  // Pump UI has pulse duration, no continuous ON window.
  await page.locator('#connectorSelect').selectOption('OUT5_1');await page.locator('#port_OUT5_1_active').check();await page.locator('#port_OUT5_1_profile').selectOption('pump');await page.locator('#port_OUT5_1_mode').selectOption('scheduled');
  assert.equal(await page.locator('#port_OUT5_1_plan_slots .plan-end').count(),0);assert.equal(await page.locator('#port_OUT5_1_plan_slots .plan-duration').count(),1);
  await page.locator('#port_OUT5_1_catchup').selectOption('30');await page.locator('#saveEquipment').click();await idle();
  assert.equal(d.equipment.find(e=>e.profile==='pump').plan.catch_up_minutes,30);
  await page.reload();await page.waitForSelector('#port_OUT5_1_active',{state:'attached'});await idle();
  assert.equal(await page.locator('#port_OUT5_1_catchup').inputValue(),'30');
  const pump=d.equipment.find(e=>e.profile==='pump');pump.last_run={state:'INTERRUPTED_UNKNOWN',scheduled_date_key:2026001,scheduled_minute:480,durable:true};
  await page.evaluate(()=>refresh());await page.locator('#nav-overview').click();
  assert((await page.locator('#'+pump.id+'_history').innerText()).includes('Abschluss unklar'));
  assert(!(await page.locator('#'+pump.id+'_history').innerText()).includes('regulär beendet'));
  // Manual devices have neither plan summaries nor planned watering history.
  await bulk('OUT5_1');await page.selectOption('#port_OUT5_1_mode','manual');await page.click('#saveEquipment');await idle();
  await page.click('#nav-overview');assert.equal(await page.locator('#'+pump.id+'_history').count(),0);assert.equal(await page.locator('#'+pump.id+'_next').count(),0);assert.equal(await page.locator('#'+pump.id+'_duration').count(),1);
  d.time_synced=false;await page.evaluate(()=>refresh());assert((await page.locator('#dev1_next').innerText()).includes('gültige Uhrzeit'));d.time_synced=true;
  const edge=await page.evaluate(()=>{const d={time_synced:true,epoch_s:Date.UTC(2026,10,13,22,0)/1000,schedule_timezone:'Europe/Berlin'},e={plan:{slots:[{start_minute:1320,end_minute:360,percent:100}]}},p={timed:false,connection:'dc_switch'};return nextPlanAction(d,e,p);});assert(edge.includes('morgen 06:00 · Aus'));
  await bulk('OUT5_1');await page.uncheck('#port_OUT5_1_active');await page.click('#saveEquipment');await idle();
  await page.selectOption('#connectorSelect','OUT12_1');await page.uncheck('#port_OUT12_1_active');await page.click('#saveEquipment');await idle();await page.click('#nav-overview');assert.equal(await page.locator('#equipment article').count(),0);assert.equal(await page.locator('#equipment [id$="_next"]').count(),0);assert.equal(await page.locator('#equipment [id$="_history"]').count(),0);
  for(const tab of ['overview','manage','settings']){await page.setViewportSize({width:390,height:844});await page.evaluate(t=>showView(t),tab);assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);}
  assert.deepEqual(errors,[]);console.log('Schedule UI passed: output → light → two windows → save/reload, overlap rejection, active edit, removal, pulse UI and mobile layout');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
