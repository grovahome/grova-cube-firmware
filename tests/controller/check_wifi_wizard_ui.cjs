// Setup-page acceptance with simulated networking and generated credentials only.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const args = {};
for (let i=2; i<process.argv.length; i+=2) args[process.argv[i]]=process.argv[i+1];
const {chromium}=require(args['--playwright']||'playwright');

(async()=>{
  const html=fs.readFileSync('src/modules/wifi_web.cpp','utf8').match(/R"HTML\(([\s\S]*?)\)HTML"/)[1];
  const browser=await chromium.launch({headless:true,...(args['--browser']?{executablePath:args['--browser']}:{})});
  try {
    const page=await browser.newPage({viewport:{width:320,height:800}});
    const errors=[],submissions=[];
    let status={state:'setup',error:'',connected:false,requires_key:false,testing:false,storage_ready:true,ssid:'',ip:'-'};
    page.on('pageerror',error=>errors.push(error.message));
    await page.route('http://grova.test/**',route=>{
      const request=route.request(), pathname=new URL(request.url()).pathname;
      if(pathname==='/api/v1/wifi/scan'){
        if(request.method()==='POST')return route.fulfill({status:202,json:{ok:true}});
        return route.fulfill({json:{scanning:false,networks:[{ssid:'Werkstatt'},{ssid:'Werkstatt'}]}});
      }
      if(pathname==='/api/v1/wifi'){
        if(request.method()==='POST'){
          submissions.push(new URLSearchParams(request.postData()));
          status={...status,state:'testing',testing:true,error:''};
          return route.fulfill({status:202,json:{ok:true}});
        }
        return route.fulfill({json:status});
      }
      return route.fulfill({contentType:'text/html',body:html});
    });
    await page.goto('http://grova.test/wifi');
    await page.waitForFunction(()=>storageReady);
    assert.equal(await page.locator('#passwordStep').isVisible(),false);
    await page.locator('#scan').click();
    await page.locator('#networks').selectOption('Werkstatt');
    await page.locator('#next').click();
    assert.match(await page.locator('#step').innerText(),/2 von 3/);
    assert.match(await page.locator('#selectedNetwork').innerText(),/Werkstatt/);
    await page.locator('#password').fill('test-only-password');
    await page.locator('#save').click();
    assert.equal(submissions.length,1);
    assert.equal(submissions[0].get('ssid'),'Werkstatt');
    await page.waitForFunction(()=>busy&&!refreshing);
    status={...status,state:'failed',testing:false,error:'connection_failed',disconnect_reason:202};
    await page.evaluate(()=>refresh());
    assert.match(await page.locator('#message').innerText(),/abgelehnt/);
    assert.equal(await page.locator('#passwordStep').isVisible(),true);
    assert.equal(await page.locator('#save').isEnabled(),true);
    await page.locator('#password').fill('test-only-corrected');
    await page.locator('#save').click();
    await page.waitForFunction(()=>busy&&!refreshing);
    status={...status,state:'saved',error:'',testing:false,connected:true,ssid:'Werkstatt',ip:'192.0.2.8'};
    await page.evaluate(()=>refresh());
    assert.match(await page.locator('#step').innerText(),/3 von 3/);
    assert.equal(await page.locator('#password').inputValue(),'');
    assert.equal(await page.locator('#appLink').getAttribute('href'),'http://192.0.2.8/');
    assert.equal(submissions.length,2);
    assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
    assert.deepEqual(errors,[]);
    console.log('Wi-Fi wizard passed: network scan, credentials, failed trial, retry, saved connection and local link');
  }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
