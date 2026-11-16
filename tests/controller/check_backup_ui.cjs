// Browser workflow acceptance against production HTML, with no hardware access.
const fs=require('fs'),assert=require('assert');
const args={};for(let i=2;i<process.argv.length;i+=2)args[process.argv[i]]=process.argv[i+1];
const {chromium}=require(args['--playwright']||'playwright');
(async()=>{
 const d=JSON.parse(fs.readFileSync(args['--blank'],'utf8')),packets=[],errors=[];
 d.devices=[{type:'SHT4x',source:'SHT4x@ch0/0x44',channel:0,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:26},{id:'climate.humidity_pct',unit:'%',valid:true,value:70}]},{type:'SHT4x',source:'SHT4x@ch1/0x44',channel:1,address:68,signals:[{id:'climate.temperature_c',unit:'C',valid:true,value:27},{id:'climate.humidity_pct',unit:'%',valid:true,value:75}]}];
 d.epoch_s=Date.UTC(2026,10,13,8,0)/1000;d.time_synced=true;d.schedule_timezone='Europe/Berlin';
 let writeMode='';const backup={format:'grova-core-equipment',version:1,hardware_profile:d.hardware_profile,firmware_version:'test',configuration:{schema:1,revision:17,equipment:[{id:'dev1',profile:'light_switch',name:'Regallicht',output:'OUT12_1',voltage:12,mode:'scheduled',plan:{enabled:true,slots:[{start_minute:480,end_minute:1200,percent:100}]}}],fan_ports:d.fan_ports}};
 const html=fs.readFileSync('src/controller/page.h','utf8').match(/R"HTML\(([\s\S]*)\)HTML"/)[1];
 const browser=await chromium.launch({headless:true,...(args['--browser']?{executablePath:args['--browser']}:{})});
 try{
  const page=await browser.newPage({viewport:{width:1100,height:950}});page.on('pageerror',e=>{errors.push(e.message);console.error('Page:',e.message);});
  await page.route('http://grova.test/**',async route=>{
   const req=route.request();if(req.url().endsWith('/api/v1/configuration'))return route.fulfill({json:backup});if(req.url().endsWith('/api/v1/status'))return route.fulfill({json:d});
   if(req.url().endsWith('/api/v1/control')){
    const p=req.postDataJSON();packets.push(p);if(p.cmd==='restore_equipment'){assert.equal(p.expected_revision,d.equipment_revision);d.rest_mode.enabled=true;if(writeMode==='reject')return route.fulfill({json:{ok:false,message:'Speichern fehlgeschlagen'}});d.equipment=p.backup.configuration.equipment.map(e=>({...e,applied_percent:0,requested_percent:0,startup_remaining_ms:0,remaining_ms:0,max_runtime_ms:0,reason:'REST'}));d.fan_ports=p.backup.configuration.fan_ports;d.equipment_revision++;if(writeMode==='drop-ack')return route.abort();return route.fulfill({json:{ok:true,message:'Restored and paused'}});}
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
  await page.locator('#nav-settings').click();const downloadPromise=page.waitForEvent('download');await page.locator('#exportConfiguration').click();const download=await downloadPromise,saved=JSON.parse(fs.readFileSync(await download.path(),'utf8'));assert.equal(saved.format,'grova-core-equipment');assert.equal(packets.length,0);
  const choose=async value=>{await page.locator('#backupFile').setInputFiles({name:'grova-backup.json',mimeType:'application/json',buffer:Buffer.from(JSON.stringify(value))});await page.waitForFunction(()=>!el('backupPreview').textContent.startsWith('Sicherung wird geprüft'));};
  await choose({...backup,hardware_profile:'wrong'});assert.equal(await page.locator('#restoreConfiguration').isDisabled(),true);assert((await page.locator('#backupPreview').innerText()).includes('Nicht geladen'));assert.equal(packets.length,0);
  await choose({...backup,configuration:{...backup.configuration,equipment:[null]}});assert.equal(await page.locator('#restoreConfiguration').isDisabled(),true);assert.equal(packets.length,0);
  await choose(backup);assert((await page.locator('#backupPreview').innerText()).includes('1 Geräte'));await page.locator('#backupDetails summary').click();assert((await page.locator('#backupItems').innerText()).includes('Regallicht'));assert.equal(await page.locator('#restoreConfiguration').isDisabled(),false);
  d.rest_mode.enabled=false;await page.evaluate(()=>refresh());await page.locator('#restoreConfiguration').click();await idle();assert.equal(packets.length,1);assert.equal(packets[0].cmd,'restore_equipment');assert.equal(packets[0].expected_revision,0);assert.equal(d.rest_mode.enabled,true);assert.equal(d.equipment[0].applied_percent,0);assert((await page.locator('#backupPreview').innerText()).includes('Wiederhergestellt'));assert.equal(await page.locator('#restoreConfiguration').isDisabled(),true);
  await choose(backup);d.equipment_revision++;await page.evaluate(()=>refresh());assert.equal(await page.locator('#restoreConfiguration').isDisabled(),true);await page.evaluate(()=>restoreConfiguration());assert.equal(packets.length,1);
  await choose(backup);writeMode='reject';const name=d.equipment[0].name;await page.locator('#restoreConfiguration').click();await idle();assert.equal(d.equipment[0].name,name);assert((await page.locator('#message').innerText()).includes('fehlgeschlagen'));assert.equal(await page.locator('#restoreConfiguration').isDisabled(),false);
  writeMode='drop-ack';await page.locator('#restoreConfiguration').click();await page.waitForFunction(()=>!busy);writeMode='';await page.evaluate(()=>refresh());assert.equal(packets.filter(p=>p.cmd==='restore_equipment').length,3);assert.equal(d.rest_mode.enabled,true);assert.equal(await page.evaluate(()=>pendingEquipment),null);
  for(const width of [390,768]){await page.setViewportSize({width,height:900});assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1));}
  assert.deepEqual(errors,[]);console.log('Backup UI passed: secret-free JSON download, validated preview, explicit paused restore, live revision guard, failed write, lost ACK without replay and mobile');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
