// Browser acceptance for the production embedded page. No device or network service is contacted.
const fs=require('fs'),assert=require('assert');
const args={};for(let i=2;i<process.argv.length;i+=2)args[process.argv[i]]=process.argv[i+1];
const {chromium}=require(args['--playwright']||'playwright');
(async()=>{
 const blank=JSON.parse(fs.readFileSync(args['--blank'],'utf8'));
 const html=fs.readFileSync('src/controller/page.h','utf8').match(/R"HTML\(([\s\S]*)\)HTML"/)[1];
 const d=structuredClone(blank),packets=[],errors=[];let mode='',offline=false;
 const browser=await chromium.launch({headless:true,...(args['--browser']?{executablePath:args['--browser']}:{})});
 try{
  const page=await browser.newPage({viewport:{width:1180,height:1000}});page.on('pageerror',e=>errors.push(e.message));
  await page.route('http://grova.test/**',async route=>{
   const req=route.request();
   if(req.url().endsWith('/api/v1/status')){
    if(mode==='hang-status'){await new Promise(r=>setTimeout(r,5600));try{await route.fulfill({json:d});}catch{}return;}
    return route.fulfill({status:offline?503:200,json:offline?{error:'offline'}:d});
   }
   if(req.url().endsWith('/api/v1/control')){
    const p=req.postDataJSON();packets.push(p);
    if(mode==='reject-write')return route.fulfill({status:400,json:{ok:false,message:'Speichern fehlgeschlagen'}});
    if(p.cmd==='configure_equipment'){
     if(p.expected_revision!==d.equipment_revision)return route.fulfill({status:400,json:{ok:false,message:'Configuration changed'}});
     d.equipment=p.equipment.map(e=>({...e,max_runtime_ms:e.max_runtime_ms||0,startup_remaining_ms:0,remaining_ms:0,requested_percent:0,applied_percent:0,reason:'REST'}));
     d.fan_ports=p.fan_ports;d.equipment_revision++;
     for(const o of d.outputs)o.assigned=d.equipment.some(e=>e.output===o.id||e.power_output===o.id);
    }
    if(p.cmd==='set_rest_mode')d.rest_mode.enabled=p.enabled;
    if(mode==='drop-ack')return route.abort();
    if(mode==='ack-offline')offline=true;
    return route.fulfill({json:{ok:true,message:'Applied'}});
   }
   return route.fulfill({contentType:'text/html',body:html});
  });
  const idle=()=>page.waitForFunction(()=>!busy&&!statusBusy);
  await page.goto('http://grova.test/');await page.waitForSelector('#port_OUT5_1_active',{state:'attached'});await idle();
  assert.equal(await page.locator('#equipment article').count(),0);assert.equal(await page.locator('#fanCard1').isVisible(),false);assert.equal(await page.locator('#emptyEquipment').isVisible(),true);
  assert.equal(await page.locator('#lockState').innerText(),'Ausgänge gesperrt');assert.equal(packets.length,0);
  await page.locator('#nav-manage').click();await page.locator('#connectorSelect').selectOption('all');
  assert.equal(await page.locator('#equipmentEditor .port').count(),7);assert.equal(await page.locator('select[id$="_voltage"]').count(),0);
  for(const cb of await page.locator('#portEditorFields input[type="checkbox"]').all())assert.equal(await cb.isChecked(),false);
  await page.locator('#port_OUT5_1_active').check();await page.locator('#port_OUT5_1_profile').selectOption('pump');await page.locator('#port_OUT5_1_name').fill('Pumpe A');await page.locator('#port_OUT5_1_runtime').fill('1.5');
  assert((await page.locator('#port_OUT5_1_protection').innerText()).includes('Trockenlauferkennung'));
  await page.evaluate(()=>refresh());assert.equal(await page.locator('#port_OUT5_1_name').inputValue(),'Pumpe A');
  await page.locator('#saveEquipment').click();await idle();assert.equal(d.equipment[0].voltage,5);assert.equal(d.equipment[0].max_runtime_ms,1500);assert.equal(await page.locator('#saveEquipment').isDisabled(),false);
  // Another client changes the saved configuration while this browser has a draft.
  await page.locator('#port_OUT5_1_name').fill('Mein Entwurf');d.equipment_revision++;d.equipment[0].name='Extern';await page.evaluate(()=>refresh());
  assert.equal(await page.locator('#port_OUT5_1_name').inputValue(),'Mein Entwurf');assert.equal(await page.locator('#saveEquipment').isDisabled(),true);
  await page.locator('#discardEquipment').click();assert.equal(await page.locator('#port_OUT5_1_name').inputValue(),'Extern');
  // Applied POST whose acknowledgement was lost: reconcile by status, never repeat the POST.
  await page.locator('#port_OUT5_1_name').fill('Antwort verloren');mode='drop-ack';let before=packets.length;
  await page.locator('#saveEquipment').click();await idle();assert.equal(packets.length,before+1);assert.equal(await page.locator('#saveEquipment').isDisabled(),true);
  mode='';await page.evaluate(()=>refresh());assert.equal(await page.locator('#port_OUT5_1_name').inputValue(),'Antwort verloren');assert.equal(await page.evaluate(()=>pendingEquipment),null);assert.equal(packets.length,before+1);
  // Failed durable save retains both the old overview and the unsaved draft.
  await page.locator('#port_OUT5_1_name').fill('Nicht gespeichert');mode='reject-write';await page.locator('#saveEquipment').click();await idle();
  assert.equal(d.equipment[0].name,'Antwort verloren');assert.equal(await page.locator('#port_OUT5_1_name').inputValue(),'Nicht gespeichert');mode='';await page.locator('#discardEquipment').click();
  // ACK succeeded, but subsequent status refresh failed. Do not rebuild from stale state.
  await page.locator('#port_OUT5_1_name').fill('Status fehlt');mode='ack-offline';await page.locator('#saveEquipment').click();await idle();assert.equal(await page.locator('#port_OUT5_1_name').inputValue(),'Status fehlt');assert.equal(await page.locator('#discardEquipment').isDisabled(),true);
  offline=false;mode='';await page.evaluate(()=>refresh());assert.equal(await page.evaluate(()=>pendingEquipment),null);assert.equal(await page.locator('#discardEquipment').isDisabled(),false);
  // A bounded read timeout must release the poll lock and disable activation.
  mode='hang-status';const started=Date.now();await page.evaluate(()=>refresh());assert(Date.now()-started<6500);assert.equal(await page.locator('#rest').isDisabled(),true);assert.equal(await page.evaluate(()=>statusBusy),false);mode='';await page.evaluate(()=>refresh());
  // Map navigation is read-only; unsupported interfaces remain placeholders.
  before=packets.length;await page.locator('#controllerMap [data-connector="adc1"]').click();assert.equal(await page.locator('#connectorLive').innerText(),'Noch nicht verfügbar');await page.locator('#controllerMap [data-connector="sensors"]').click();assert.equal(await page.locator('#sensorCard').isVisible(),true);assert.equal(packets.length,before);
  await page.locator('#connectorSelect').selectOption('all');await page.locator('#port_OUT5_1_active').uncheck();await page.locator('#saveEquipment').click();await idle();assert.equal(d.equipment.length,0);
  await page.locator('#nav-overview').click();assert.equal(await page.locator('#emptyEquipment').isVisible(),true);
  await page.reload();await page.waitForSelector('#port_OUT5_1_active',{state:'attached'});assert.equal(await page.locator('#equipment article').count(),0);
  await page.setViewportSize({width:390,height:844});for(const tab of ['overview','manage','settings']){await page.evaluate(t=>showView(t),tab);assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);}
  if(args['--preview']){const preview=await browser.newPage();preview.on('pageerror',e=>errors.push(e.message));await preview.setContent(fs.readFileSync(args['--preview'],'utf8'));await preview.waitForSelector('#emptyEquipment');assert((await preview.locator('body').innerText()).includes('KEINE HARDWARE'));await preview.locator('select[onchange="previewPreset(this.value)"]').selectOption('example');await preview.waitForFunction(()=>state?.equipment.length===2);await preview.locator('select[onchange="previewPreset(this.value)"]').selectOption('blank');await preview.waitForFunction(()=>state?.equipment.length===0);await preview.close();}
  assert.deepEqual(errors,[]);console.log('Browser acceptance passed: empty boot, fixed ports, profile limits, draft retention, version conflict, lost ACK, write failure, stale status, timeout, removal/reload, map and mobile layout');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
