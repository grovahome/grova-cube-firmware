// Exercise the normal wizard and board map against simulated controller responses.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const args = {};
for (let i = 2; i < process.argv.length; i += 2) args[process.argv[i]] = process.argv[i + 1];
const { chromium } = require(args['--playwright'] || 'playwright');

(async () => {
  const d = JSON.parse(fs.readFileSync(args['--blank'], 'utf8'));
  const requests = [], errors = [];
  d.equipment.push({
    id:'dev1', name:'Älterer Dimmer', profile:'light_0_10v', output:'0_10V_OUT_1',
    power_output:'OUT12_1', power_voltage:12, mode:'manual', plan:{enabled:false,slots:[]},
    applied_percent:0, requested_percent:0, reason:'REST',
  });
  const html = fs.readFileSync('src/controller/page.h','utf8').match(/R"HTML\(([\s\S]*)\)HTML"/)[1];
  const browser = await chromium.launch({headless:true, ...(args['--browser'] ? {executablePath:args['--browser']} : {})});
  try {
    const page = await browser.newPage({viewport:{width:1024,height:900}});
    page.on('pageerror', error => errors.push(error.message));
    await page.route('http://grova.test/**', async route => {
      const request=route.request(), path=new URL(request.url()).pathname;
      if (path==='/api/v1/status') return route.fulfill({json:d});
      if (path==='/api/v1/history') return route.fulfill({json:{samples:[],series:[],capacity:120,interval_s:30}});
      if (path==='/api/v1/control') {
        const command=request.postDataJSON();
        requests.push(command);
        if (command.cmd==='configure_equipment') {
          assert.equal(command.expected_revision,d.equipment_revision);
          assert.equal(command.start_plans,false);
          assert.equal(d.rest_mode.enabled,true);
          d.equipment=command.equipment.map(e=>({...e,applied_percent:0,requested_percent:0,reason:'REST'}));
          d.fan_ports=command.fan_ports;
          d.equipment_revision++;
        } else if (command.cmd==='set_rest_mode') {
          d.rest_mode.enabled=command.enabled;
        }
        return route.fulfill({json:{ok:true}});
      }
      return route.fulfill({contentType:'text/html',body:html});
    });
    const idle=()=>page.waitForFunction(()=>connected&&!busy&&!statusBusy&&!pendingEquipment&&!equipmentSaving);
    const next=()=>page.locator('#wizardNext').click();
    const map=id=>page.locator('#controllerMap [data-connector="'+id+'"]');
    await page.goto('http://grova.test/');
    await idle();
    await page.locator('#view-overview [data-add-device]').click();
    assert.equal(await page.locator('#wizardPorts button').count(),5);
    assert.equal(await page.locator('#wizardPorts [data-connector="OUT12_1"]').count(),0);
    assert.equal(await page.locator('#wizardPorts [data-connector="0_10V_OUT_1"]').count(),0);
    assert.equal(await page.locator('#wizardPorts [data-connector="OUT5_1"]').count(),1);
    await page.locator('#wizardCancel').click();
    assert.equal(requests.length,0);
    await page.locator('#nav-overview').click();
    await map('FAN2').click();
    assert.match(await page.locator('#wizardCaption').innerText(),/PWM-Lüfter 2.*Steuerung wählen/);
    await page.locator('#port_FAN2_name').fill('Abluft');
    await page.locator('#port_FAN2_mode').selectOption('scheduled');
    await next();
    const slots=page.locator('#port_FAN2_plan_slots .plan-slot');
    await slots.nth(0).locator('.plan-start').fill('08:00');
    await slots.nth(0).locator('.plan-end').fill('10:00');
    await page.locator('#port_FAN2_plan_add').click();
    await slots.nth(1).locator('.plan-start').fill('09:00');
    await slots.nth(1).locator('.plan-end').fill('11:00');
    await next();
    assert.match(await page.locator('#wizardError').innerText(),/überschneiden/);
    assert.equal(requests.length,0);
    await slots.nth(1).locator('.plan-start').fill('19:30');
    await slots.nth(1).locator('.plan-end').fill('06:00');
    await slots.nth(1).locator('.plan-percent').fill('40');
    await next();
    assert.match(await page.locator('#wizardReview').innerText(),/08:00–10:00.*19:30–06:00.*40 %/s);
    await next();
    await idle();
    assert.equal(d.fan_ports[1].name,'Abluft');
    assert.equal(d.fan_ports[1].plan.slots.length,2);
    assert.equal(d.equipment[0].power_output,'OUT12_1');
    assert.equal(requests.length,1);
    await page.locator('#nav-overview').click();
    await map('0_10V_OUT_2').click();
    assert.match(await page.locator('#wizardIdentityHint').innerText(),/separat versorgt/);
    await page.locator('#port_0_10V_OUT_2_profile').selectOption('fan_0_10v');
    await page.locator('#port_0_10V_OUT_2_name').fill('Externer Lüfter');
    await next();
    await page.locator('#port_0_10V_OUT_2_mode').selectOption('manual');
    await next();
    await next();
    await next();
    await idle();
    assert.equal(d.equipment.length,2);
    assert.equal(d.equipment[1].output,'0_10V_OUT_2');
    assert.ok(!('power_output' in d.equipment[1]));
    for (const width of [320,390,1024]) {
      await page.setViewportSize({width,height:900});
      await page.locator('#nav-overview').click();
      assert.ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
    }
    await page.locator('#rest').click();
    assert.match(await page.locator('#resumeDevices').innerText(),/Abluft/);
    assert.equal(requests.length,2);
    d.equipment_revision++;
    await page.evaluate(()=>refresh());
    assert.equal(await page.locator('#confirmResume').isDisabled(),true);
    await page.getByRole('dialog').getByRole('button',{name:'Abbrechen'}).click();
    await page.locator('#rest').click();
    await page.locator('#confirmResume').click();
    await idle();
    assert.deepEqual(requests.at(-1),{cmd:'set_rest_mode',enabled:false});
    assert.deepEqual(errors,[]);
    console.log('Device wizard passed: free ports, existing wiring, map shortcuts, two daily slots, separate resume and responsive views');
  } finally {
    await browser.close();
  }
})().catch(error => {console.error(error);process.exitCode=1;});
