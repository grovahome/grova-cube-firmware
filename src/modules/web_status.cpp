#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <string.h>

#include "config.h"
#include "firmware_info.h"
#include "modules/alarms.h"
#include "modules/climate.h"
#include "modules/control.h"
#include "modules/fan.h"
#include "modules/fan_control.h"
#include "modules/grow_mode.h"
#include "modules/i2c_discovery.h"
#include "modules/light.h"
#include "modules/local_run.h"
#include "modules/outputs.h"
#include "modules/preset_store.h"
#include "modules/pump_scheduler.h"
#include "modules/rest_mode.h"
#include "modules/runtime_config.h"
#include "modules/sensors.h"
#include "modules/stability.h"
#include "modules/time_sync.h"
#include "modules/ui.h"
#include "modules/wifi_ota.h"

static WebServer server(80);
static bool serverStarted = false;

static const char STATUS_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>vGrow Status</title>
  <style>
    :root{color-scheme:dark;--bg:#101418;--panel:#1a2027;--muted:#93a1af;--text:#edf3f8;--ok:#54d17a;--warn:#f4be4f;--line:#2b3540}
    *{box-sizing:border-box}
    body{margin:0;background:var(--bg);color:var(--text);font-family:system-ui,-apple-system,Segoe UI,sans-serif}
    main{max-width:920px;margin:0 auto;padding:18px}
    header{display:flex;align-items:flex-end;justify-content:space-between;gap:12px;margin-bottom:14px}
    h1{font-size:24px;margin:0}
    .sub{color:var(--muted);font-size:13px}
    .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:10px}
    .card{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:12px}
    .card h2{font-size:13px;letter-spacing:.04em;text-transform:uppercase;color:var(--muted);margin:0 0 10px}
    .row{display:flex;justify-content:space-between;gap:12px;border-top:1px solid var(--line);padding:7px 0;font-size:14px}
    .row:first-of-type{border-top:0}
    .key{color:var(--muted)}
    .value{text-align:right;font-variant-numeric:tabular-nums}
    .pill{display:inline-block;border-radius:999px;padding:3px 8px;font-size:12px;background:#25303a;color:var(--text)}
    .buttons{display:flex;flex-wrap:wrap;gap:6px}
    button{border:1px solid var(--line);border-radius:7px;background:#26313b;color:var(--text);padding:7px 9px;font:inherit;font-size:13px}
    input,select{width:100%;border:1px solid var(--line);border-radius:6px;background:#101820;color:var(--text);padding:6px;font:inherit;font-size:13px}
    input{width:70px;text-align:right}
    .config-grid{display:grid;grid-template-columns:1fr auto auto auto;gap:7px;align-items:center;font-size:13px}
    .config-grid span{color:var(--muted)}
    .config-actions{margin-top:10px}
    button:active{transform:translateY(1px)}
    .ok{color:var(--ok)}
    .warn{color:var(--warn)}
  </style>
</head>
<body>
<main>
  <header>
    <div>
      <h1>vGrow Status</h1>
      <div class="sub" id="updated">waiting for data</div>
    </div>
    <div class="pill" id="health">...</div>
  </header>
  <section class="grid">
    <div class="card"><h2>Controls</h2><div id="controls"></div><div class="sub" id="controlStatus"></div></div>
    <div class="card"><h2>Warning Limits + Fan Curve</h2><div id="configEditor"></div><div class="sub" id="configStatus"></div></div>
    <div class="card"><h2>Climate</h2><div id="climate"></div><div id="climateTargets"></div><div class="sub" id="climateStatus"></div></div>
    <div class="card"><h2>Grow Mode</h2><div id="grow"></div></div>
    <div class="card"><h2>Fan</h2><div id="fan"></div></div>
    <div class="card"><h2>Light</h2><div id="light"></div></div>
    <div class="card"><h2>Pump</h2><div id="pump"></div></div>
    <div class="card"><h2>Sensors</h2><div id="sensors"></div></div>
    <div class="card"><h2>I2C Discovery</h2><div id="i2c"></div></div>
    <div class="card"><h2>System</h2><div id="system"></div></div>
    <div class="card"><h2>Local Presets</h2><div id="localPresets"></div><div class="sub" id="localPresetStatus"></div></div>
  </section>
</main>
<script>
const row=(k,v)=>`<div class="row"><span class="key">${k}</span><span class="value">${v}</span></div>`;
const val=(v,d,u)=>Number.isFinite(v)?v.toFixed(d)+u:'-';
const yn=v=>v?'YES':'NO';
function setRows(id, rows){document.getElementById(id).innerHTML=rows.map(x=>row(x[0],x[1])).join('');}
async function refresh(){
  try{
    const r=await fetch('/api/v1/status',{cache:'no-store'});
    const d=await r.json();
    const warn=d.warning==='OK'?`<span class="ok">${d.warning}</span>`:`<span class="warn">${d.warning}</span>`;
    document.getElementById('health').innerHTML=d.rest_mode.enabled?'<span class="warn">REST MODE</span>':(d.healthy?'<span class="ok">HEALTH OK</span>':'<span class="warn">WARN</span>');
    document.getElementById('updated').textContent=d.cube_id+' | IP '+d.ip+' | uptime '+d.uptime_s+'s';
    setRows('climate',[
      ['Temp',val(d.temp_c,1,' C')],
      ['Active target temp',d.target_temp_c.toFixed(1)+' C'],
      ['Hum',val(d.hum_pct,1,' %')],
      ['Active target hum',d.target_hum_pct.toFixed(0)+' %'],
      ['Warning',warn]
    ]);
    renderClimateTargets(d.climate_targets);
    setRows('grow',[
      ['Rest Mode',d.rest_mode.enabled?'<span class="warn">ACTIVE</span>':'OFF'],
      ['Mode',d.grow.mode],
      ['Local Run',d.local_run.active?d.local_run.status:'OFF'],
      ['Preset',d.local_run.preset_name||'-'],
      ['Phase',d.local_run.phase_label||'-'],
      ['Effect',d.grow.effect],
      ['Germination',yn(d.grow.germination)],
      ['Harvest',yn(d.grow.harvest)]
    ]);
    setRows('fan',[
      ['Mode',d.fan.mode],
      ['Now',d.fan.current_pct+' %'],
      ['Target',d.fan.target_pct+' %'],
      ['Reason',d.fan.reason],
      ['Tacho',d.fan.tacho],
      ['RPM Fan 1',d.fan.rpm],
      ['RPM Fan 2',d.fan.rpm2]
    ]);
    setRows('light',[
      ['State',d.light.on?'ON':'OFF'],
      ['Mode',d.light.mode],
      ['Reason',d.light.reason],
      ['On',String(d.light.on_hour).padStart(2,'0')+':00'],
      ['Off',String(d.light.off_hour).padStart(2,'0')+':00']
    ]);
    setRows('pump',[
      ['Mode',d.pump.mode],
      ['Reason',d.pump.reason],
      ['Running',yn(d.pump.running)],
      ['Remaining',d.pump.remaining_s+' s'],
      ['Schedule',String(d.pump.hour).padStart(2,'0')+':'+String(d.pump.minute).padStart(2,'0')],
      ['Duration',d.pump.duration_s+' s'],
      ['Events Today',d.pump.runs_today],
      ['Safety',d.pump.safety_mode],
      ['Boot Lock',yn(d.pump.startup_locked)]
    ]);
    setRows('sensors',[
      ['Status',d.sensor.status],
      ['Source',d.sensor.source],
      ['Pressure',val(d.sensor.pressure_hpa,0,' hPa')],
      ['Pressure sensor',d.sensor.pressure_source],
      ['CO2',val(d.environment&&d.environment.co2_ppm,0,' ppm')],
      ['Lux',val(d.environment&&d.environment.lux,0,' lx')],
      ['UV index',val(d.environment&&d.environment.uv_index,1,'')],
      ['Sources',(d.sensor.sources||[]).map(s=>s.label+': '+s.status).join('<br>')||'-'],
      ['Fails',d.sensor.fail_count],
      ['Consecutive fails',d.sensor.consecutive_fail_count],
      ['Fault',yn(d.sensor.fault)]
    ]);
    const found=(d.i2c&&d.i2c.devices?d.i2c.devices:[]).filter(x=>x.present).map(x=>x.module+' '+x.name+' '+x.addr);
    setRows('i2c',[
      ['Bus',d.i2c?'SDA '+d.i2c.sda+' / SCL '+d.i2c.scl:'-'],
      ['Known found',d.i2c?d.i2c.found_count:0],
      ['Detected',found.length?found.join('<br>'):'none']
    ]);
    setRows('system',[
      ['Cube ID',d.cube_id],
      ['Firmware',d.firmware_version],
      ['Build',d.firmware_build_date+' '+d.firmware_build_time],
      ['WiFi',d.wifi_connected?'OK':'FAIL'],
      ['Time',d.time_synced?'OK':'SYNC'],
      ['Time source',d.time_source],
      ['Clock',String(d.hour).padStart(2,'0')+':'+String(d.minute).padStart(2,'0')],
      ['Rest Mode',d.rest_mode.enabled?'ACTIVE':'OFF'],
      ['Local presets',d.local_presets.active_slot>=0?'Slot '+d.local_presets.active_slot:'none'],
      ['RTC',d.rtc.enabled?(d.rtc.present?(d.rtc.valid?'OK':'INVALID'):'MISSING'):'OFF'],
      ['Settings',d.settings_ok?'OK':'FAIL'],
      ['Free heap',d.free_heap]
    ]);
    renderLocalPresets(d.local_presets,d.local_run);
    renderRtc(d.rtc);
    renderConfig(d.config);
  }catch(e){
    document.getElementById('health').innerHTML='<span class="warn">OFFLINE</span>';
  }
}
function numberInput(id,value,step){
  return `<input id="${id}" type="number" step="${step}" value="${value}">`;
}
function renderConfig(c){
  if(!c || document.activeElement.closest('#configEditor')) return;
  let html='<div class="config-grid">';
  html+='<span>Temp warning min/max</span>'+numberInput('cfg_temp_min',c.temp_min_c.toFixed(1),'0.1')+numberInput('cfg_temp_max',c.temp_max_c.toFixed(1),'0.1')+'<span>C</span>';
  html+='<span>Hum warning min/max</span>'+numberInput('cfg_hum_min',c.hum_min_pct,'1')+numberInput('cfg_hum_max',c.hum_max_pct,'1')+'<span>%</span>';
  html+='<span>Fan curve</span><span>T over target</span><span>H over target</span><span>Fan</span>';
  c.fan_curve.forEach((p,i)=>{
    html+=`<span>P${i+1}</span>${numberInput('cfg_to_'+i,p.temp_over_c.toFixed(1),'0.1')}${numberInput('cfg_ho_'+i,p.hum_over_pct,'1')}${numberInput('cfg_fp_'+i,p.fan_pct,'1')}`;
  });
  html+='</div><div class="config-actions"><button onclick="saveConfig()">Save to Cube</button></div>';
  document.getElementById('configEditor').innerHTML=html;
}
function renderClimateTargets(c){
  if(!c || document.activeElement.closest('#climateTargets')) return;
  let html='<div class="config-grid" style="margin-top:10px">';
  html+='<span>Day target</span>'+numberInput('clim_day_temp',c.day_temp_c.toFixed(1),'0.1')+numberInput('clim_day_hum',c.day_hum_pct,'1')+'<span>C / %</span>';
  html+='<span>Night target</span>'+numberInput('clim_night_temp',c.night_temp_c.toFixed(1),'0.1')+numberInput('clim_night_hum',c.night_hum_pct,'1')+'<span>C / %</span>';
  html+='</div><div class="config-actions"><button onclick="saveClimateTargets()">Save Targets</button></div>';
  document.getElementById('climateTargets').innerHTML=html;
}
function renderRtc(rtc){
  if(!rtc) return;
  const button=rtc.enabled
    ? `<button onclick="sendControl({cmd:'set_rtc_config',enabled:false})">Disable RTC</button>`
    : `<button onclick="sendControl({cmd:'set_rtc_config',enabled:true})">Enable RTC</button>`;
  document.getElementById('system').innerHTML+=`<div class="config-actions">${button}</div>`;
}
function renderLocalPresets(presets,run){
  const slots=(presets&&presets.slots)||[];
  const options=slots.map(s=>`<option value="${s.slot}" ${run&&run.slot===s.slot?'selected':''}>Slot ${s.slot+1}: ${s.saved?(s.name||s.id):'empty'}</option>`).join('');
  const rows=[
    ['Status',run&&run.active?run.status:'idle'],
    ['Active slot',run&&run.slot>=0?'Slot '+(run.slot+1):'-'],
    ['Preset',run&&run.preset_name?run.preset_name:'-'],
    ['Phase',run&&run.phase_label?run.phase_label:'-'],
    ['Progress',run&&run.active?run.total_progress_pct.toFixed(1)+' %':'-']
  ];
  document.getElementById('localPresets').innerHTML=rows.map(x=>row(x[0],x[1])).join('')+
    `<div class="config-actions"><select id="localRunSlot">${options}</select></div>
    <div class="buttons">
      <button onclick="startLocalRun()">Start Slot</button>
      <button onclick="sendControl({cmd:'pause_local_run'})">Pause</button>
      <button onclick="sendControl({cmd:'resume_local_run'})">Resume</button>
      <button onclick="sendControl({cmd:'stop_local_run'})">Stop</button>
    </div>`;
}
async function startLocalRun(){
  const status=document.getElementById('localPresetStatus');
  const slot=Number(document.getElementById('localRunSlot').value);
  status.textContent='starting...';
  await sendControl({cmd:'set_rest_mode',enabled:false});
  await sendControl({cmd:'start_local_run',slot:slot,run_id:'esp-web-'+Date.now(),revision:(Date.now()>>>0)});
  status.textContent='start requested';
}
async function saveClimateTargets(){
  const payload={cmd:'set_climate_targets',
    day_temp_c:Number(document.getElementById('clim_day_temp').value),
    night_temp_c:Number(document.getElementById('clim_night_temp').value),
    day_hum_pct:Number(document.getElementById('clim_day_hum').value),
    night_hum_pct:Number(document.getElementById('clim_night_hum').value)
  };
  const status=document.getElementById('climateStatus');
  status.textContent='saving...';
  try{
    const r=await fetch('/api/v1/control',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
    const d=await r.json();
    status.textContent=d.message||'saved';
    document.activeElement.blur();
    await refresh();
  }catch(e){
    status.textContent='save failed';
  }
}
async function saveConfig(){
  const payload={cmd:'set_config',
    temp_min_c:Number(document.getElementById('cfg_temp_min').value),
    temp_max_c:Number(document.getElementById('cfg_temp_max').value),
    hum_min_pct:Number(document.getElementById('cfg_hum_min').value),
    hum_max_pct:Number(document.getElementById('cfg_hum_max').value)
  };
  for(let i=0;i<5;i++){
    payload['temp_over_c_'+i]=Number(document.getElementById('cfg_to_'+i).value);
    payload['hum_over_pct_'+i]=Number(document.getElementById('cfg_ho_'+i).value);
    payload['fan_pct_'+i]=Number(document.getElementById('cfg_fp_'+i).value);
  }
  const status=document.getElementById('configStatus');
  status.textContent='saving...';
  try{
    const r=await fetch('/api/v1/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
    const d=await r.json();
    status.textContent=d.message||'saved';
    document.activeElement.blur();
    await refresh();
  }catch(e){
    status.textContent='save failed';
  }
}
async function sendControl(payload){
  const status=document.getElementById('controlStatus');
  status.textContent='sending...';
  try{
    const r=await fetch('/api/v1/control',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
    const d=await r.json();
    status.textContent=d.message||'done';
    await refresh();
  }catch(e){
    status.textContent='request failed';
  }
}
function drawControls(){
  document.getElementById('controls').innerHTML=`
    <div class="buttons">
      <button onclick="sendControl({cmd:'set_grow_mode',mode:'GERM'})">Germ</button>
      <button onclick="sendControl({cmd:'set_grow_mode',mode:'GROWTH'})">Growth</button>
      <button onclick="sendControl({cmd:'set_grow_mode',mode:'HARVEST'})">Harvest</button>
      <button onclick="sendControl({cmd:'set_rest_mode',enabled:true})">Rest On</button>
      <button onclick="sendControl({cmd:'set_rest_mode',enabled:false})">Rest Off</button>
      <button onclick="sendControl({cmd:'set_light_mode',mode:'AUTO'})">Light Auto</button>
      <button onclick="sendControl({cmd:'set_light_mode',mode:'ON'})">Light On</button>
      <button onclick="sendControl({cmd:'set_light_mode',mode:'OFF'})">Light Off</button>
      <button onclick="sendControl({cmd:'set_fan_auto',fan:1})">Fan 1 Auto</button>
      <button onclick="sendControl({cmd:'set_fan_manual',fan:1,percent:50})">Fan 1 50%</button>
      <button onclick="sendControl({cmd:'set_fan_manual',fan:1,percent:75})">Fan 1 75%</button>
      <button onclick="sendControl({cmd:'set_fan_manual',fan:2,percent:0})">Fan 2 Off</button>
      <button onclick="sendControl({cmd:'set_fan_manual',fan:2,percent:25})">Fan 2 25%</button>
      <button onclick="sendControl({cmd:'set_fan_manual',fan:2,percent:50})">Fan 2 50%</button>
      <button onclick="sendControl({cmd:'set_fan_manual',fan:2,percent:75})">Fan 2 75%</button>
      <button onclick="sendControl({cmd:'set_fan_manual',fan:2,percent:100})">Fan 2 100%</button>
      <button onclick="sendControl({cmd:'pump_test',action:'start'})">Pump Test</button>
      <button onclick="sendControl({cmd:'pump_test',action:'stop'})">Pump Stop</button>
    </div>`;
}
drawControls();
refresh();
setInterval(refresh,2000);
</script>
</body>
</html>
)HTML";

static void appendJsonString(String& json, const char* key, const char* value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":\"";
  json += value;
  json += "\"";
  if (comma) json += ",";
}

static void appendJsonBool(String& json, const char* key, bool value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value ? "true" : "false";
  if (comma) json += ",";
}

static void appendJsonInt(String& json, const char* key, long value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value;
  if (comma) json += ",";
}

static void appendJsonFloat(String& json, const char* key, float value, int decimals, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  if (isnan(value)) {
    json += "null";
  } else {
    json += String(value, decimals);
  }
  if (comma) json += ",";
}

static void handleRoot() {
  server.send_P(200, "text/html", STATUS_PAGE);
}

static void handleStatus() {
  float temp = getTemp();
  float hum = getHum();
  bool settingsOk =
    ui_settingsReady() &&
    growMode_settingsReady() &&
    climate_settingsReady() &&
    light_settingsReady() &&
    pumpScheduler_settingsReady() &&
    restMode_settingsReady() &&
    presetStore_settingsReady() &&
    localRun_settingsReady() &&
    runtimeConfig_settingsReady() &&
    time_settingsReady();

  String json;
  json.reserve(3600);
  json += "{";
  appendJsonString(json, "cube_id", MQTT_CUBE_ID);
  appendJsonFloat(json, "temp_c", temp, 1);
  appendJsonFloat(json, "hum_pct", hum, 1);
  appendJsonFloat(json, "target_temp_c", getTargetTemp(), 1);
  appendJsonFloat(json, "target_hum_pct", getTargetHum(), 0);
  appendJsonString(json, "warning", alarms_getPrimaryWarning(temp, hum));
  appendJsonBool(json, "healthy", isSystemHealthy());
  appendJsonBool(json, "wifi_connected", wifiOTA_isConnected());
  appendJsonString(json, "ip", wifiOTA_getIP());
  appendJsonBool(json, "time_synced", isTimeSynced());
  appendJsonString(json, "time_source", time_getSourceName());
  appendJsonInt(json, "hour", getHour());
  appendJsonInt(json, "minute", getMinute());
  appendJsonInt(json, "uptime_s", millis() / 1000UL);
  appendJsonInt(json, "free_heap", ESP.getFreeHeap());
  appendJsonBool(json, "settings_ok", settingsOk);
  appendJsonString(json, "firmware_name", GROVA_FIRMWARE_NAME);
  appendJsonString(json, "firmware_version", GROVA_FIRMWARE_VERSION);
  appendJsonString(json, "firmware_build_date", GROVA_BUILD_DATE);
  appendJsonString(json, "firmware_build_time", GROVA_BUILD_TIME);
  appendJsonString(json, "firmware_build_env", GROVA_BUILD_ENV);

  json += "\"environment\":{";
  appendJsonFloat(json, "temperature_c", temp, 1);
  appendJsonFloat(json, "humidity_pct", hum, 1);
  appendJsonFloat(json, "pressure_hpa", sensors_getPressureHpa(), 0);
  appendJsonFloat(json, "co2_ppm", sensors_getCo2Ppm(), 0);
  appendJsonFloat(json, "lux", sensors_getLux(), 0);
  appendJsonFloat(json, "uv_index", sensors_getUvIndex(), 1);
  appendJsonString(json, "temperature_source", sensors_getTemperatureSourceName());
  appendJsonString(json, "humidity_source", sensors_getHumiditySourceName());
  appendJsonString(json, "pressure_source", sensors_getPressureSourceName());
  appendJsonString(json, "co2_source", sensors_getCo2SourceName());
  appendJsonString(json, "lux_source", sensors_getLuxSourceName());
  appendJsonString(json, "uv_source", sensors_getUvSourceName(), false);
  json += "},";

  runtimeConfig_appendJson(json);
  json += ",";
  fanControl_appendJson(json);
  json += ",";
  presetStore_appendSummaryJson(json);
  json += ",";
  localRun_appendJson(json);
  json += ",";

  json += "\"rest_mode\":{";
  appendJsonBool(json, "enabled", restMode_isEnabled());
  appendJsonString(json, "mode", restMode_getName());
  appendJsonString(json, "reason", restMode_getReasonName(), false);
  json += "},";

  json += "\"rtc\":{";
  appendJsonBool(json, "enabled", rtc_isEnabled());
  appendJsonBool(json, "present", rtc_isPresent());
  appendJsonBool(json, "valid", rtc_hasValidTime());
  appendJsonBool(json, "used_for_boot", rtc_wasUsedForBoot());
  appendJsonBool(json, "last_read_ok", rtc_lastReadOk());
  appendJsonBool(json, "last_write_ok", rtc_lastWriteOk(), false);
  json += "},";

  json += "\"climate_targets\":{";
  appendJsonFloat(json, "day_temp_c", climate_getDayTemp(), 1);
  appendJsonFloat(json, "night_temp_c", climate_getNightTemp(), 1);
  appendJsonFloat(json, "day_hum_pct", climate_getDayHum(), 0);
  appendJsonFloat(json, "night_hum_pct", climate_getNightHum(), 0, false);
  json += "},";

  json += "\"grow\":{";
  appendJsonString(json, "mode", growMode_getName());
  appendJsonString(json, "effect", growMode_getEffectName());
  appendJsonBool(json, "germination", growMode_isGermination());
  appendJsonBool(json, "harvest", growMode_isHarvest(), false);
  json += "},";

  json += "\"fan\":{";
  appendJsonString(json, "mode", fan_getModeName(1));
  appendJsonInt(json, "current_pct", getFanPercent());
  appendJsonInt(json, "target_pct", getFanTargetPercent());
  appendJsonString(json, "reason", fan_getReasonName(1));
  appendJsonString(json, "tacho", fan_getTachoStatusName());
  appendJsonInt(json, "rpm", getFanRPM());
  appendJsonInt(json, "rpm2", getFan2RPM(), false);
  json += "},";

  json += "\"fan1\":{";
  appendJsonBool(json, "enabled", fan_isEnabled(1));
  appendJsonString(json, "mode", fan_getModeName(1));
  appendJsonInt(json, "current_pct", getFanPercent());
  appendJsonInt(json, "target_pct", getFanTargetPercent());
  appendJsonString(json, "reason", fan_getReasonName(1));
  appendJsonString(json, "decision_priority", fan_getDecisionPriorityName(1));
  appendJsonString(json, "winning_rule", fan_getWinningRuleId(1));
  appendJsonInt(json, "temperature_demand_pct", fan_getTemperatureDemandPercent(1));
  appendJsonInt(json, "humidity_demand_pct", fan_getHumidityDemandPercent(1));
  appendJsonInt(json, "rpm", getFanRPM());
  appendJsonBool(json, "tacho_fault", fan_getTachoFault(1), false);
  json += "},";

  json += "\"fan2\":{";
  appendJsonBool(json, "enabled", fan_isEnabled(2));
  appendJsonString(json, "mode", fan_getModeName(2));
  appendJsonInt(json, "current_pct", getFan2Percent());
  appendJsonInt(json, "target_pct", getFan2TargetPercent());
  appendJsonString(json, "reason", fan_getReasonName(2));
  appendJsonString(json, "decision_priority", fan_getDecisionPriorityName(2));
  appendJsonString(json, "winning_rule", fan_getWinningRuleId(2));
  appendJsonInt(json, "temperature_demand_pct", fan_getTemperatureDemandPercent(2));
  appendJsonInt(json, "humidity_demand_pct", fan_getHumidityDemandPercent(2));
  appendJsonInt(json, "rpm", getFan2RPM());
  appendJsonBool(json, "tacho_fault", fan_getTachoFault(2), false);
  json += "},";

  json += "\"light\":{";
  appendJsonBool(json, "on", isLightOn());
  appendJsonString(json, "mode", ui_getLightModeName());
  appendJsonString(json, "reason", light_getReasonName());
  appendJsonInt(json, "on_hour", light_getOnHour());
  appendJsonInt(json, "off_hour", light_getOffHour(), false);
  json += "},";

  json += "\"pump\":{";
  appendJsonString(json, "mode", pumpScheduler_getModeName());
  appendJsonString(json, "reason", pumpScheduler_getReasonName());
  appendJsonBool(json, "running", pumpScheduler_isRunning());
  appendJsonInt(json, "remaining_s", pumpScheduler_getRemainingSeconds());
  appendJsonInt(json, "hour", pumpScheduler_getRunHour());
  appendJsonInt(json, "minute", pumpScheduler_getRunMinute());
  appendJsonInt(json, "duration_s", pumpScheduler_getRunDurationSeconds());
  appendJsonInt(json, "max_duration_s", pumpScheduler_getMaxRunDurationSeconds());
  appendJsonInt(json, "runs_today", pumpScheduler_getRunsToday());
  appendJsonString(json, "safety_mode", "event_lock");
  appendJsonBool(json, "startup_locked", pumpScheduler_isStartupLocked(), false);
  json += "},";

  json += "\"outputs\":{";
  json += "\"aux_12v\":{";
  appendJsonBool(json, "available", outputs_hasAux12v());
  appendJsonBool(json, "on", outputs_isAux12vOn(), false);
  json += "},";
  json += "\"aux_5v\":{";
  appendJsonBool(json, "available", outputs_hasAux5v());
  appendJsonBool(json, "on", outputs_isAux5vOn(), false);
  json += "}";
  json += "},";

  json += "\"sensor\":{";
  appendJsonString(json, "status", sensors_getStatusName());
  appendJsonString(json, "source", sensors_getSourceName());
  appendJsonFloat(json, "pressure_hpa", sensors_getPressureHpa(), 0);
  appendJsonString(json, "pressure_source", sensors_getPressureSourceName());
  appendJsonFloat(json, "bosch_temp_c", sensors_getBoschTemp(), 1);
  appendJsonInt(json, "fail_count", sensors_getFailCount());
  appendJsonInt(json, "consecutive_fail_count", sensors_getConsecutiveFailCount());
  appendJsonBool(json, "fault", sensors_hasFault());
  sensors_appendSourcesJson(json);
  json += "}";

  json += ",";
  i2cDiscovery_appendJson(json);

  json += "}";

  server.send(200, "application/json", json);
}

static void handleControl() {
  String body = server.arg("plain");
  String response;
  bool ok = control_handleJson(body, response);
  server.send(ok ? 200 : 400, "application/json", response);
}

static void handleConfig() {
  String json;
  json.reserve(700);
  json += "{";
  runtimeConfig_appendJson(json);
  json += ",";
  fanControl_appendJson(json);
  json += "}";
  server.send(200, "application/json", json);
}

static void handleConfigPost() {
  String body = server.arg("plain");
  String response;
  bool ok = runtimeConfig_applyJson(body, response);
  server.send(ok ? 200 : 400, "application/json", response);
}

static void handleLocalPresets() {
  String json;
  json.reserve(6200);
  presetStore_appendFullJson(json);
  server.send(200, "application/json", json);
}

void webStatus_begin() {
  if (strlen(WIFI_SSID) == 0) {
    Serial.println("Web status disabled");
    return;
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/v1/status", HTTP_GET, handleStatus);
  server.on("/api/v1/control", HTTP_POST, handleControl);
  server.on("/api/v1/local-presets", HTTP_GET, handleLocalPresets);
  server.on("/api/v1/config", HTTP_GET, handleConfig);
  server.on("/api/v1/config", HTTP_POST, handleConfigPost);
  server.begin();
  serverStarted = true;
  Serial.println("Web status ready");
}

void webStatus_loop() {
  if (!serverStarted || WiFi.status() != WL_CONNECTED) return;
  server.handleClient();
}
