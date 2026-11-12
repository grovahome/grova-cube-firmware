#include <Arduino.h>
#include <WiFi.h>
#include "modules/wifi_web.h"
#include "modules/wifi_ota.h"

static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>GROVA WLAN einrichten</title><style>
*{box-sizing:border-box}body{margin:0;background:#101917;color:#edf6ef;font:16px system-ui,sans-serif}
main{max-width:560px;margin:32px auto;padding:24px}h1{font-size:28px}p{line-height:1.5;color:#b7cac0}
label{display:block;margin:18px 0 6px}input,select,button{font:inherit;width:100%;padding:12px;border-radius:8px;border:1px solid #53685a;background:#18261e;color:inherit}
button{cursor:pointer;margin-top:16px;background:#b4da8a;color:#18261e;font-weight:600}button:disabled{opacity:.5;cursor:wait}
a{color:#b4da8a}#message{padding:12px;border:1px solid #53685a;border-radius:8px;white-space:pre-line}small{color:#b7cac0}
</style><main><small>GROVA CORE / NETZWERK</small><h1>WLAN einrichten</h1>
<p>Verbinde den Core mit deinem 2,4-GHz-WLAN. Internet und ein GROVA-Server sind dafuer nicht erforderlich.</p>
<p id="message" role="status" aria-live="polite">Status wird geladen ...</p>
<form id="wifiForm"><div id="keyGroup" hidden><label for="key">Passwort des GROVA-WLANs</label>
<input id="key" type="password" maxlength="12" autocomplete="off"><small>Im GROVA-WLAN ist keine erneute Eingabe erforderlich.</small></div>
<button id="scan" type="button">WLAN-Netzwerke suchen</button>
<label for="networks">Gefundene Netzwerke</label><select id="networks"><option value="">Netzwerk auswaehlen</option></select>
<label for="ssid">WLAN-Name (auch fuer versteckte Netzwerke)</label><input id="ssid" autocomplete="off" required>
<label for="password">Passwort deines WLANs</label><input id="password" type="password" maxlength="64" autocomplete="new-password">
<small>Bei einem offenen Netzwerk leer lassen. WPA/WPA2-Personal: 8 bis 63 Zeichen oder 64 Hex-Zeichen. Firmen-WLAN mit Benutzeranmeldung wird hier nicht eingerichtet.</small>
<button id="save" type="submit">Verbindung pruefen und speichern</button></form>
<p>Deine bisherigen Zugangsdaten bleiben bei einem Fehler erhalten. Beim Wechsel kann sich dein Handy kurz vom Setup-WLAN trennen. Verbinde es bei Bedarf erneut.</p>
<p id="destination"></p><p><a id="appLink" hidden>Lokale Webapp oeffnen</a></p></main>
<script>
const el=id=>document.getElementById(id);let busy=false,scanning=false,storageReady=false,scanTimer=null,scanStarted=0,refreshing=false,connectedIp="";
const errors={connection_failed:'Keine Verbindung erhalten. WLAN-Name, Passwort und Reichweite pruefen.',storage_failed:'Speichern fehlgeschlagen. Bisherige Daten bleiben erhalten.',storage_unavailable:'Geraetespeicher nicht verfuegbar. Einrichtung kann nicht gespeichert werden.',rest_storage_failed:'Ruhezustand konnte nicht gespeichert werden. Bitte erneut versuchen.',saved_credentials_invalid:'Gespeicherte WLAN-Daten sind ungueltig. Bitte WLAN neu einrichten.',setup_ap_failed:'Setup-WLAN konnte nicht gestartet werden.'};
async function api(path,options={}){const control=new AbortController(),timer=setTimeout(()=>control.abort(),8000);try{const r=await fetch(path,{...options,signal:control.signal,cache:'no-store',headers:{'X-GROVA-Setup-Request':'1','X-GROVA-Setup-Key':el('key').value,...options.headers}});const data=await r.json();if(!r.ok)throw Error(data.message||'Anfrage fehlgeschlagen');return data;}catch(e){if(e.name==='AbortError')throw Error('Keine Antwort vom Core. Verbindung zum GROVA-WLAN pruefen.');throw e;}finally{clearTimeout(timer);}}
async function refresh(){if(refreshing)return;refreshing=true;try{const s=await api('/api/v1/wifi');busy=s.testing;storageReady=s.storage_ready;el('keyGroup').hidden=!s.requires_key;el('key').required=s.requires_key;el('save').disabled=busy||scanning||!s.storage_ready;el('scan').disabled=busy||scanning;el('save').textContent=busy?'Verbindung wird geprueft ...':'Verbindung pruefen und speichern';if(s.connected&&s.state==='saved')el('password').value='';
const radioErrors={201:'Das WLAN wurde beim Verbindungsversuch nicht gefunden.',202:'Das WLAN hat die Anmeldung abgelehnt. Passwort und WLAN-Sicherheitsmodus pruefen.',203:'Der Router hat die WLAN-Verbindung abgelehnt.',204:'Die WLAN-Schluesselaushandlung hat zu lange gedauert. Passwort und Sicherheitsmodus pruefen.',15:'Die WLAN-Schluesselaushandlung ist fehlgeschlagen. Passwort und Sicherheitsmodus pruefen.'};
if(!scanning)el('message').textContent=s.error?((s.error==='connection_failed'&&radioErrors[s.disconnect_reason])||errors[s.error]||s.error)+(s.disconnect_reason?' (WLAN-Code '+s.disconnect_reason+')':''):s.testing?'Verbindung wird geprueft. Bitte bis zu 30 Sekunden warten.':s.connected?'Verbunden mit '+s.ssid+'\nIP: '+s.ip+(s.state==='saved'?'\nWLAN-Daten gespeichert.':''):'Verbinde dich mit '+s.setup_ssid+' und waehle dein WLAN.';
const dest=el('destination'),link=el('appLink');if(s.connected&&s.ip!=='-'){connectedIp=s.ip;link.href='http://'+s.ip+'/';link.textContent='Webapp oeffnen: '+s.ip;link.hidden=false;dest.textContent=s.requires_key?'Du bist im Heimnetz. Ueber den Link geht es zur Board-Oberflaeche.':'Einrichtung abgeschlossen. Das GROVA-WLAN schliesst nach '+(s.setup_close_delay_s||5)+' Sekunden. Verbinde dein Handy mit deinem Heim-WLAN und oeffne danach den Link.';}
}catch(e){el('message').textContent=connectedIp?'WLAN eingerichtet. Bitte ins Heim-WLAN wechseln und die Webapp unter '+connectedIp+' oeffnen.':'Verbindung zum Core unterbrochen. Bitte im GROVA-WLAN bleiben bzw. erneut damit verbinden. Der Verbindungstest laeuft auf dem Core weiter.';}finally{refreshing=false;}}
function finishScan(){scanning=false;el('scan').disabled=busy;el('save').disabled=busy||!storageReady;el('scan').textContent='WLAN-Netzwerke suchen';}
async function scanResults(){try{if(Date.now()-scanStarted>25000)throw Error('WLAN-Suche dauert zu lange. Bitte erneut versuchen.');const data=await api('/api/v1/wifi/scan');if(data.scanning){scanTimer=setTimeout(scanResults,1000);return;}if(data.error)throw Error(data.error);const list=el('networks');list.replaceChildren(new Option('Netzwerk auswaehlen',''));const seen=new Set();for(const n of data.networks){if(seen.has(n.ssid))continue;seen.add(n.ssid);list.add(new Option(n.ssid+' ('+n.rssi+' dBm)',n.ssid));}el('message').textContent=seen.size?seen.size+' WLAN-Netzwerke gefunden. Bitte auswaehlen.':'Keine 2,4-GHz-Netzwerke gefunden. Erneut suchen oder WLAN-Namen eingeben.';finishScan();}catch(e){el('message').textContent=e.message;finishScan();}}
el('scan').onclick=async()=>{if(busy||scanning)return;clearTimeout(scanTimer);scanning=true;scanStarted=Date.now();el('scan').disabled=true;el('save').disabled=true;el('scan').textContent='Suche laeuft ...';el('message').textContent='Suche nach 2,4-GHz-WLAN-Netzwerken ...';try{await api('/api/v1/wifi/scan',{method:'POST'});await scanResults();}catch(e){el('message').textContent=e.message;finishScan();}};
el('networks').onchange=()=>{if(el('networks').value)el('ssid').value=el('networks').value;};
el('wifiForm').onsubmit=async e=>{e.preventDefault();if(busy||scanning)return;const ssid=el('ssid').value,password=el('password').value;
if(new TextEncoder().encode(ssid).length>32){el('message').textContent='Der WLAN-Name darf maximal 32 UTF-8-Bytes haben.';return;}
busy=true;el('save').disabled=true;el('scan').disabled=true;el('save').textContent='Verbindung wird geprueft ...';el('message').textContent='Verbindung wird geprueft. Bitte bis zu 30 Sekunden warten und im GROVA-WLAN bleiben.';el('message').scrollIntoView({behavior:'smooth',block:'center'});try{await api('/api/v1/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({ssid,password})});await refresh();}catch(e){el('message').textContent=e.message+' Der Auftrag kann bereits angekommen sein; der Status wird weiter geprueft.';}};
refresh();setInterval(()=>{if(busy)refresh();},1500);
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
