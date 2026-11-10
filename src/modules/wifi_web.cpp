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
<form id="wifiForm"><label for="key">Setup-Passwort vom OLED / Geraetezettel</label>
<input id="key" type="password" maxlength="12" autocomplete="off" required>
<button id="scan" type="button">WLAN-Netzwerke suchen</button>
<label for="networks">Gefundene Netzwerke</label><select id="networks"><option value="">Netzwerk auswaehlen</option></select>
<label for="ssid">WLAN-Name (auch fuer versteckte Netzwerke)</label><input id="ssid" autocomplete="off" required>
<label for="password">Passwort deines WLANs</label><input id="password" type="password" maxlength="64" autocomplete="new-password">
<small>Bei einem offenen Netzwerk leer lassen. WPA/WPA2-Personal: 8 bis 63 Zeichen oder 64 Hex-Zeichen. Firmen-WLAN mit Benutzeranmeldung wird hier nicht eingerichtet.</small>
<button id="save" type="submit">Verbindung pruefen und speichern</button></form>
<p>Deine bisherigen Zugangsdaten bleiben bei einem Fehler erhalten. Beim Wechsel kann sich dein Handy kurz vom Setup-WLAN trennen. Verbinde es bei Bedarf erneut.</p>
<p id="destination"></p><p><a href="/">Zur lokalen Webapp</a></p></main>
<script>
const el=id=>document.getElementById(id);let busy=false,scanTimer=null;
const errors={connection_failed:'Keine Verbindung erhalten. WLAN-Name, Passwort und Reichweite pruefen.',storage_failed:'Speichern fehlgeschlagen. Bisherige Daten bleiben erhalten.',storage_unavailable:'Geraetespeicher nicht verfuegbar. Einrichtung kann nicht gespeichert werden.',rest_storage_failed:'Ruhezustand konnte nicht gespeichert werden. Bitte erneut versuchen.',saved_credentials_invalid:'Gespeicherte WLAN-Daten sind ungueltig. Bitte WLAN neu einrichten.',setup_ap_failed:'Setup-WLAN konnte nicht gestartet werden.'};
async function api(path,options={}){const r=await fetch(path,{...options,cache:'no-store',headers:{'X-GROVA-Setup-Key':el('key').value,...options.headers}});const data=await r.json();if(!r.ok)throw Error(data.message||'Anfrage fehlgeschlagen');return data;}
async function refresh(){try{const s=await api('/api/v1/wifi');busy=s.testing;el('save').disabled=busy||!s.storage_ready;el('scan').disabled=busy;
el('message').textContent=s.error?(errors[s.error]||s.error):s.testing?'Verbindung wird geprueft. Bitte bis zu 30 Sekunden warten.':s.connected?'Verbunden mit '+s.ssid+'\nIP: '+s.ip+(s.state==='saved'?'\nWLAN-Daten gespeichert.':''):'Verbinde dich mit '+s.setup_ssid+' und waehle dein WLAN.';
const dest=el('destination');dest.replaceChildren();if(s.connected&&s.ip!=='-'){const a=document.createElement('a');a.href='http://'+s.ip+'/';a.textContent='Webapp im Heimnetz oeffnen';dest.append('Verbinde Handy/Computer mit deinem Heimnetz. Das Setup-WLAN schliesst nach etwa einer Minute. ',a);}
}catch(e){el('message').textContent='Core gerade nicht erreichbar. Nach dem WLAN-Wechsel bitte neu verbinden; die neue IP steht auf dem OLED.';}}
async function scanResults(){try{const data=await api('/api/v1/wifi/scan');if(data.scanning){scanTimer=setTimeout(scanResults,1200);return;}el('networks').replaceChildren(new Option('Netzwerk auswaehlen',''));for(const n of data.networks){el('networks').add(new Option(n.ssid+' ('+n.rssi+' dBm, '+(n.secure?'geschuetzt':'offen')+')',n.ssid));}el('message').textContent=data.networks.length?'Netzwerk auswaehlen oder Namen manuell eingeben.':'Keine Netzwerke gefunden. Erneut suchen oder Namen manuell eingeben.';}catch(e){el('message').textContent=e.message;}}
el('scan').onclick=async()=>{clearTimeout(scanTimer);try{await api('/api/v1/wifi/scan',{method:'POST'});el('message').textContent='Suche laeuft ...';scanResults();}catch(e){el('message').textContent=e.message;}};
el('networks').onchange=()=>{if(el('networks').value)el('ssid').value=el('networks').value;};
el('wifiForm').onsubmit=async e=>{e.preventDefault();if(busy)return;const ssid=el('ssid').value,password=el('password').value;
if(new TextEncoder().encode(ssid).length>32){el('message').textContent='Der WLAN-Name darf maximal 32 UTF-8-Bytes haben.';return;}
busy=true;el('save').disabled=true;try{await api('/api/v1/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({ssid,password})});el('password').value='';await refresh();}catch(e){busy=false;el('save').disabled=false;el('message').textContent=e.message;}};
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

static bool authorize(WebServer& server) {
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
  static const char* headers[] = {"X-GROVA-Setup-Key", "Content-Type"};
  server.collectHeaders(headers, 2);
  server.on("/wifi", HTTP_GET, [&server]() { wifiWeb_page(server); });
  server.on("/api/v1/wifi", HTTP_GET, [&server]() {
    server.sendHeader("Cache-Control", "no-store");
    String json = "{\"state\":"; json += quoted(wifiOTA_setupState());
    json += ",\"error\":"; json += quoted(wifiOTA_setupError());
    json += ",\"connected\":"; json += wifiOTA_isConnected() && !wifiOTA_isTesting() ? "true" : "false";
    json += ",\"testing\":"; json += wifiOTA_isTesting() ? "true" : "false";
    json += ",\"storage_ready\":"; json += wifiOTA_storageReady() ? "true" : "false";
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
    String json = "{\"scanning\":"; json += count == WIFI_SCAN_RUNNING ? "true" : "false";
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
