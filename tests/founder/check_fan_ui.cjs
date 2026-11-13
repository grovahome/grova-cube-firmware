// Browser workflow acceptance against production HTML, with no hardware access.
const fs=require('fs'),assert=require('assert');
const args={};for(let i=2;i<process.argv.length;i+=2)args[process.argv[i]]=process.argv[i+1];
const {chromium}=require(args['--playwright']||'playwright');
(async()=>{
 const d=JSON.parse(fs.readFileSync(args['--blank'],'utf8')),packets=[],errors=[];
 d.devices=[{type:'SHT4x',source:'SHT4x@ch0/0x44',channel:0,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:26}]}];
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
  const manage=async id=>{await page.locator('#nav-manage').click();await page.locator('#controllerMap [data-connector="'+id+'"]').click();};
  const save=async()=>{await page.locator('#saveEquipment').click();await idle();};
  const overview=async()=>{await page.locator('#nav-overview').click();await idle();};
  await manage('FAN1');await page.locator('#port_FAN1_active').check();
  await page.locator('#port_FAN1_name').fill('Umluft');
  await page.locator('#port_FAN1_mode').selectOption('scheduled');
  const slots=page.locator('#port_FAN1_plan_slots .plan-slot');
  await slots.nth(0).locator('.plan-start').fill('08:00');await slots.nth(0).locator('.plan-end').fill('10:00');await slots.nth(0).locator('.plan-percent').fill('35');
  await page.locator('#port_FAN1_plan_add').click();await slots.nth(1).locator('.plan-start').fill('12:00');await slots.nth(1).locator('.plan-end').fill('13:00');await slots.nth(1).locator('.plan-percent').fill('60');
  await save();assert.equal(d.fan_ports[0].mode,'scheduled');assert.equal(d.fan_ports[0].plan.slots.length,2);assert.equal(d.rest_mode.enabled,false);
  await overview();assert((await page.locator('#fanCard1').innerText()).includes('35 %'));assert((await page.locator('#fan1_next').innerText()).includes('10:00'));assert.equal(await page.locator('#fanvalue1').count(),0);
  await page.reload();await idle();await manage('FAN1');assert.equal(await slots.nth(1).locator('.plan-percent').inputValue(),'60');
  await page.locator('#port_FAN1_mode').selectOption('manual');await save();await overview();assert.equal(await page.locator('#fanvalue1').getAttribute('type'),'range');
  await page.locator('#rest').click();await idle();await page.locator('#fanvalue1').fill('47');await page.locator('#fanCard1').getByRole('button',{name:'Übernehmen',exact:true}).click();await idle();assert(packets.some(p=>p.cmd==='set_fan_manual'&&p.fan===1&&p.percent===47));
  await manage('FAN1');await page.locator('#port_FAN1_mode').selectOption('automatic');await page.locator('#port_FAN1_sensor_add').click();assert.equal(await page.locator('#port_FAN1_plan_enabled').isVisible(),false);await save();await overview();assert.equal(await page.locator('#fanvalue1').count(),0);assert((await page.locator('#fanCard1').innerText()).includes('Ziel 24'));
  // Either physical PWM connector can take either role.
  await manage('FAN2');await page.locator('#port_FAN2_active').check();await page.locator('#port_FAN2_mode').selectOption('scheduled');await page.locator('#port_FAN2_plan_slots .plan-percent').fill('55');await save();assert.equal(d.fan_ports[1].plan.slots[0].percent,55);assert.equal(d.fan_ports[0].mode,'automatic');
  // 0–10 V gets the same mode choice and a real slider; DC switches stay on/off.
  await manage('0_10V_OUT_1');await page.locator('#port_0_10V_OUT_1_active').check();await page.locator('#port_0_10V_OUT_1_profile').selectOption('fan_0_10v');await save();await overview();assert.equal(await page.locator('#dev1_percent').getAttribute('type'),'range');
  await manage('0_10V_OUT_1');await page.locator('#port_0_10V_OUT_1_mode').selectOption('automatic');await page.locator('#port_0_10V_OUT_1_sensor_add').click();await save();await overview();assert.equal(await page.locator('#dev1_percent').count(),0);assert((await page.locator('#equipment').innerText()).includes('Ziel 24'));
  await manage('OUT12_1');await page.locator('#port_OUT12_1_active').check();await page.locator('#port_OUT12_1_profile').selectOption('fan_switch');await page.locator('#port_OUT12_1_mode').selectOption('scheduled');assert.equal(await page.locator('#port_OUT12_1_plan_slots .plan-percent').count(),0);await save();
  await manage('FAN1');await page.locator('#port_FAN1_active').uncheck();await save();await overview();assert.equal(await page.locator('#fanCard1').count(),0);
  for(const width of [390,768]){await page.setViewportSize({width,height:900});assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1));}
  assert.deepEqual(errors,[]);console.log('Fan UI passed: independent roles, PWM percentage windows/reload, slider, auto pending, 0–10 V, switch limitations, removal and mobile layout');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
