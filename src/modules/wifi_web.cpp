#include <Arduino.h>
#include <WiFi.h>
#include "modules/wifi_web.h"
#include "modules/wifi_ota.h"

static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>GROVA WLAN einrichten</title><style>
*{box-sizing:border-box}[hidden]{display:none!important}
:root{color-scheme:light dark;--bg:light-dark(#f4f6f2,#141b17);--panel:light-dark(#fff,#1c2620);--line:light-dark(#d5e0d5,#38473c);--text:light-dark(#17251c,#eaf1e9);--muted:light-dark(#526357,#acbdb0);--accent:light-dark(#245e47,#a8dcb8);--on-accent:light-dark(#fff,#142c1d)}
body{margin:0;background:var(--bg);color:var(--text);font:16px/1.5 system-ui,sans-serif}
main{max-width:560px;margin:20px auto;padding:16px}h1{font-size:29px;letter-spacing:-.04em;margin:8px 0 4px}h2{font-size:20px;margin:0 0 9px}
p,small{color:var(--muted)}small{display:block}label{display:block;margin:12px 0 4px}
input,select,button{font:inherit;padding:9px 11px;border-radius:9px;border:1px solid var(--line);background:var(--panel);color:inherit;min-height:44px}
input,select{width:100%;min-width:0}button{margin-top:12px;cursor:pointer}button:disabled{opacity:.55;cursor:default}
button.primary{background:var(--accent);color:var(--on-accent);border-color:var(--accent);font-weight:650}
a{color:var(--accent)}.card{padding:16px;border:1px solid var(--line);border-radius:14px;background:var(--panel)}
#message{padding:0;white-space:pre-line;overflow-wrap:anywhere}#step{color:var(--accent);margin:14px 0 7px}#selectedNetwork{overflow-wrap:anywhere}
.actions{display:flex;flex-wrap:wrap;gap:9px}
</style><main><small>GROVA CORE</small><h1>WLAN einrichten</h1>
<p>Verbinde den Core mit deinem 2,4-GHz-WLAN. Die Bedienung bleibt lokal.</p>
<p id="step"></p><section class="card">
<p id="message" role="status">Status wird geladen …</p>
<form id="wifiForm"><div id="keyGroup" hidden><label for="key">Passwort des GROVA-WLANs</label>
<input id="key" type="password" maxlength="12" autocomplete="off"></div>
<section id="networkStep"><h2>WLAN auswählen</h2>
<button id="scan" type="button">WLAN-Netzwerke suchen</button>
<label for="networks">Gefundene Netzwerke</label><select id="networks"><option value="">Netzwerk auswählen</option></select>
<label for="ssid">WLAN-Name</label><input id="ssid" autocomplete="off" aria-describedby="ssidHelp" required>
<small id="ssidHelp">Ein verstecktes Netzwerk kannst du von Hand eintragen.</small>
<button id="next" class="primary" type="button">Weiter</button></section>
<section id="passwordStep" hidden><h2>Verbindung prüfen</h2><p id="selectedNetwork"></p>
<label for="password">Passwort deines WLANs</label><input id="password" type="password" maxlength="64" autocomplete="new-password">
<small>Bei einem offenen Netzwerk bleibt das Feld leer. Bei einem Fehler bleiben die bisherigen Zugangsdaten erhalten.</small>
<div class="actions"><button type="button" id="back">Zurück</button><button id="save" class="primary" type="submit" disabled>Prüfen und speichern</button></div></section></form>
<section id="resultStep" hidden><h2>Core öffnen</h2><p id="destination"></p><a id="appLink" hidden>Lokale Weboberfläche öffnen</a></section>
</section><p><a href="/" id="returnLink" hidden>Zurück zur lokalen Weboberfläche</a></p></main>
<script>
const el=id=>document.getElementById(id);
let stage=1,busy=false,scanning=false,storageReady=false,refreshing=false,connectedIp='',pendingSsid='',attemptAt=0;
const errors={
 connection_failed:'Verbindung fehlgeschlagen. Prüfe WLAN-Name, Passwort und Reichweite.',
 storage_failed:'Speichern fehlgeschlagen. Bisherige Zugangsdaten bleiben erhalten.',
 storage_unavailable:'Gerätespeicher nicht verfügbar.',
 rest_storage_failed:'Pause konnte nicht gespeichert werden. Bitte erneut versuchen.',
 saved_credentials_invalid:'Gespeicherte WLAN-Daten sind ungültig.',
 setup_ap_failed:'Das GROVA-WLAN konnte nicht gestartet werden.'
};
const radioErrors={201:'Das WLAN wurde nicht gefunden.',202:'Die Anmeldung wurde abgelehnt. Prüfe das Passwort.',203:'Der Router hat die Verbindung abgelehnt.',204:'Der WLAN-Schlüsselaustausch hat zu lange gedauert.',15:'Der WLAN-Schlüsselaustausch ist fehlgeschlagen.'};
function controls(){
 el('scan').disabled=busy||scanning;el('next').disabled=busy||scanning;
 el('save').disabled=busy||scanning||!storageReady;el('back').disabled=busy;
 el('wifiForm').hidden=stage===3;el('networkStep').hidden=stage!==1;
 el('passwordStep').hidden=stage!==2;el('resultStep').hidden=stage!==3;
 el('step').textContent='Schritt '+stage+' von 3';
 el('save').textContent=busy?'Verbindung wird geprüft …':'Prüfen und speichern';
}
async function api(path,options={}){
 const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),8000);
 try{
  const response=await fetch(path,{...options,signal:controller.signal,cache:'no-store',headers:{'X-GROVA-Setup-Request':'1','X-GROVA-Setup-Key':el('key').value,...options.headers}});
  const data=await response.json();
  if(!response.ok){const error=Error(data.message||'Anfrage fehlgeschlagen');error.rejected=true;throw error;}
  return data;
 }catch(e){if(e.name==='AbortError')throw Error('Keine Antwort vom Core. Prüfe die Verbindung zum GROVA-WLAN.');throw e;}
 finally{clearTimeout(timer);}
}
async function refresh(){
 if(refreshing)return;
 refreshing=true;
 try{
  const s=await api('/api/v1/wifi');
  storageReady=s.storage_ready;el('keyGroup').hidden=!s.requires_key;el('key').required=s.requires_key;
  el('returnLink').hidden=!s.connected||!s.requires_key||stage===3;
  busy=s.testing;
  if(pendingSsid&&s.connected&&s.state==='saved'&&s.ssid===pendingSsid){
   pendingSsid='';connectedIp=s.ip;stage=3;el('password').value='';el('returnLink').hidden=true;
   el('message').textContent='Verbunden mit '+s.ssid+'. Die WLAN-Daten wurden gespeichert.';
   el('destination').textContent=s.requires_key?'Öffne den Core im gewählten WLAN.':'Wechsle mit deinem Handy ins gewählte WLAN. Öffne den Core anschließend über den Link.';
   if(connectedIp&&connectedIp!=='-'){el('appLink').href='http://'+connectedIp+'/';el('appLink').textContent='Core öffnen · '+connectedIp;el('appLink').hidden=false;}
  }else if(s.error){
   pendingSsid='';
   el('message').textContent=((s.error==='connection_failed'&&radioErrors[s.disconnect_reason])||errors[s.error]||s.error);
  }else if(s.testing){
   el('message').textContent='Der Core prüft die Verbindung. Bleib im GROVA-WLAN, bis der Test abgeschlossen ist.';
  }else if(pendingSsid&&Date.now()-attemptAt<12000){
   busy=true;
  }else if(pendingSsid){
   pendingSsid='';el('message').textContent='Keine neue Verbindung bestätigt. Du kannst den Test erneut starten.';
  }else if(!scanning&&stage===1){
   el('message').textContent=s.connected?'Aktuelles WLAN: '+s.ssid:'Wähle dein WLAN oder gib den Namen ein.';
  }
 }catch(e){
  if(connectedIp){el('message').textContent='Wechsle ins gewählte WLAN und öffne den Core unter '+connectedIp+'.';}
  else{
   if(pendingSsid&&Date.now()-attemptAt>=45000){pendingSsid='';busy=false;}
   el('message').textContent=pendingSsid?'Der Core prüft weiter. Verbinde dich bei Bedarf erneut mit dem GROVA-WLAN.':'Core nicht erreichbar. Prüfe die WLAN-Verbindung.';
  }
 }finally{refreshing=false;controls();}
}
async function scanResults(started){
 if(Date.now()-started>25000)throw Error('Die WLAN-Suche dauert zu lange. Bitte erneut suchen.');
 const data=await api('/api/v1/wifi/scan');
 if(data.scanning){await new Promise(resolve=>setTimeout(resolve,1000));return scanResults(started);}
 if(data.error)throw Error(data.error);
 const list=el('networks'),seen=new Set();
 list.replaceChildren(new Option('Netzwerk auswählen',''));
 for(const n of data.networks){if(seen.has(n.ssid))continue;seen.add(n.ssid);list.add(new Option(n.ssid,n.ssid));}
 el('message').textContent=seen.size?'Wähle ein gefundenes WLAN.':'Keine Netzwerke gefunden. Gib den WLAN-Namen von Hand ein.';
}
el('scan').onclick=async()=>{
 if(busy||scanning||!el('key').reportValidity())return;
 scanning=true;controls();el('message').textContent='Suche nach 2,4-GHz-Netzwerken …';
 try{await api('/api/v1/wifi/scan',{method:'POST'});await scanResults(Date.now());}
 catch(e){el('message').textContent=e.message;}
 finally{scanning=false;controls();}
};
el('networks').onchange=()=>{if(el('networks').value)el('ssid').value=el('networks').value;};
el('next').onclick=()=>{
 if(busy||scanning||!el('ssid').reportValidity()||!el('key').reportValidity())return;
 if(new TextEncoder().encode(el('ssid').value).length>32){el('message').textContent='Der WLAN-Name darf maximal 32 UTF-8-Bytes haben.';return;}
 stage=2;el('selectedNetwork').textContent='Gewähltes WLAN: '+el('ssid').value;el('message').textContent='';controls();el('password').focus();
};
el('back').onclick=()=>{stage=1;el('password').value='';el('message').textContent='';controls();el('ssid').focus();};
el('wifiForm').onsubmit=async e=>{
 e.preventDefault();
 if(stage===1){el('next').click();return;}
 if(busy||scanning||!storageReady)return;
 const ssid=el('ssid').value,password=el('password').value;
 pendingSsid=ssid;attemptAt=Date.now();busy=true;controls();
 el('message').textContent='Der Core prüft die Verbindung. Das kann bis zu 30 Sekunden dauern.';
 try{await api('/api/v1/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({ssid,password})});await refresh();}
 catch(e){
  if(e.rejected){pendingSsid='';busy=false;el('message').textContent=e.message;}
  else el('message').textContent='Antwort fehlt. Der Core könnte bereits prüfen. Der Verbindungsstatus wird weiter abgefragt.';
  controls();
 }
};
controls();refresh();setInterval(()=>{if(busy||pendingSsid)refresh();},1500);
</script></html>
)HTML";

static String quoted(const String& value) {
  String out = "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    const unsigned char c = value[i];
    if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
    else if (c < 32) { char esc[7]; snprintf(esc, sizeof(esc), "\\u%04x", c); out += esc; }
    else out += static_cast<char>(c);
  }
  out += '"'; return out;
}

bool wifiWeb_setupRequest(WebServer& server) {
  return wifiOTA_requiresSetup() || (wifiOTA_setupActive() && server.client().localIP() == WiFi.softAPIP());
}

void wifiWeb_page(WebServer& server) {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.send_P(200, "text/html; charset=utf-8", PAGE);
}

static bool apAuthorization(WebServer& server) {
#if defined(GROVA_FOUNDER_APP)
  return wifiOTA_setupActive() && server.client().localIP() == WiFi.softAPIP();
#else
  (void)server;
  return false;
#endif
}
static bool authorize(WebServer& server) {
  // AP clients already know the WPA password. Retain a non-simple header to
  // reject browser cross-origin form requests; do not allow CORS preflights.
  if (apAuthorization(server)) {
    if (server.header("X-GROVA-Setup-Request") == "1") return true;
    server.send(403, "application/json", "{\"message\":\"Bitte die GROVA-Einrichtungsseite neu laden.\"}");
    return false;
  }
  static unsigned failures = 0;
  static uint32_t failedAt = 0;
  if (millis() - failedAt >= 30000UL) failures = 0;
  if (failures >= 5) { server.send(429, "application/json", "{\"message\":\"Zu viele Versuche. Bitte 30 Sekunden warten.\"}"); return false; }
  // Custom header is never permitted by CORS and secrets are not placed in URLs.
  if (server.header("X-GROVA-Setup-Key") != wifiOTA_setupPassword()) {
    ++failures; failedAt = millis();
    server.send(403, "application/json", "{\"message\":\"Setup-Passwort vom OLED eingeben (System-Seite).\"}"); return false;
  }
  failures = 0; return true;
}

void wifiWeb_register(WebServer& server) {
  static const char* headers[] = {"X-GROVA-Setup-Key", "Content-Type", "X-GROVA-Setup-Request"};
  server.collectHeaders(headers, 3);
  server.on("/wifi", HTTP_GET, [&server]() { wifiWeb_page(server); });
  server.on("/api/v1/wifi", HTTP_GET, [&server]() {
    server.sendHeader("Cache-Control", "no-store");
    String json = "{\"requires_key\":"; json += apAuthorization(server) ? "false" : "true";
    json += ",\"state\":"; json += quoted(wifiOTA_setupState());
    json += ",\"error\":"; json += quoted(wifiOTA_setupError());
    json += ",\"connected\":"; json += wifiOTA_isConnected() && !wifiOTA_isTesting() ? "true" : "false";
    json += ",\"disconnect_reason\":"; json += wifiOTA_disconnectReason();
    json += ",\"testing\":"; json += wifiOTA_isTesting() ? "true" : "false";
    json += ",\"storage_ready\":"; json += wifiOTA_storageReady() ? "true" : "false";
    json += ",\"setup_close_delay_s\":"; json += wifiOTA_setupCloseDelayMs() / 1000;
    json += ",\"setup_ssid\":"; json += quoted(wifiOTA_setupSSID());
    json += ",\"ssid\":"; json += quoted(wifiOTA_savedSSID());
    json += ",\"ip\":"; json += quoted(wifiOTA_getIP()); json += "}";
    server.send(200, "application/json", json);
  });
  server.on("/api/v1/wifi", HTTP_POST, [&server]() {
    if (!authorize(server)) return;
    String ssid = server.arg("ssid"), password = server.arg("password");
    if (ssid.length() != strlen(ssid.c_str()) || password.length() != strlen(password.c_str()) ||
        !wifiOTA_submit(ssid.c_str(), password.c_str())) {
      server.send(400, "application/json", "{\"message\":\"Ungueltige WLAN-Daten, Speicher nicht bereit oder Scan/Verbindungstest laeuft bereits.\"}"); return;
    }
    server.send(202, "application/json", "{\"ok\":true}");
  });
  server.on("/api/v1/wifi/scan", HTTP_POST, [&server]() {
    if (!authorize(server)) return;
    if (!wifiOTA_startScan()) { server.send(409, "application/json", "{\"message\":\"Scan/Verbindung laeuft oder Scan zu schnell wiederholt. Bitte warten.\"}"); return; }
    server.send(202, "application/json", "{\"ok\":true}");
  });
  server.on("/api/v1/wifi/scan", HTTP_GET, [&server]() {
    server.sendHeader("Cache-Control", "no-store");
    if (!authorize(server)) return;
    const int count = WiFi.scanComplete();
    String json = "{\"error\":"; json += count < WIFI_SCAN_RUNNING ? "\"WLAN-Suche fehlgeschlagen. Bitte erneut suchen.\"" : "\"\"";
    json += ",\"scanning\":"; json += count == WIFI_SCAN_RUNNING ? "true" : "false";
    json += ",\"networks\":[";
    bool comma = false;
    for (int i = 0; i < count && i < 30; ++i) {
      const String ssid = WiFi.SSID(i);
      if (!ssid.length()) continue;
      if (comma) json += ',';
      comma = true;
      json += "{\"ssid\":"; json += quoted(ssid);
      json += ",\"rssi\":"; json += WiFi.RSSI(i);
      json += ",\"secure\":"; json += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true";
      json += "}";
    }
    json += "]}"; server.send(200, "application/json", json);
  });
}
