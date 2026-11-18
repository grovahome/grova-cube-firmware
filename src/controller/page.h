#pragma once
#include <Arduino.h>
static const char FOUNDER_PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>GROVA Core</title><style>
:root{color-scheme:dark;--bg:#101915;--panel:#19261f;--line:#33473a;--text:#eef4ee;--muted:#acbcb0;--accent:#c3eea8}*{box-sizing:border-box}[hidden]{display:none!important}body{margin:0;background:var(--bg);color:var(--text);font:16px system-ui,sans-serif;line-height:1.5}main{max-width:1080px;margin:auto;padding:32px 24px 64px}header,.heading,.actions,.safety{display:flex;justify-content:space-between;align-items:center;gap:16px}brand{display:block;font-size:13px;letter-spacing:.2em;color:var(--accent)}h1{font-size:30px;letter-spacing:-.04em;margin:4px 0}h2{font-size:21px;margin:0}h3{font-size:17px;margin:0}p{margin:8px 0 16px}.sub,small{color:var(--muted);font-size:14px}.sub:empty{display:none}.pill{border:1px solid var(--line);border-radius:30px;padding:7px 12px;font-size:13px;color:var(--accent)}nav{display:flex;gap:6px;border-bottom:1px solid var(--line);margin:24px 0}button,input,select,textarea{font:inherit;border:1px solid #53664f;border-radius:9px;background:var(--bg);color:inherit;padding:10px 14px}button{cursor:pointer}button.primary{background:var(--accent);border-color:var(--accent);color:#192617}button:disabled{opacity:.4;cursor:default}button:focus-visible,a:focus-visible{outline:2px solid var(--accent);outline-offset:4px}nav button{border:0;border-radius:0;background:none;color:var(--muted);padding:14px 12px;border-bottom:2px solid transparent}nav button[aria-current="page"]{color:var(--text);border-bottom-color:var(--accent)}a{color:var(--accent)}input,textarea,select{max-width:100%;width:100%}.port{border-top:1px solid var(--line);padding:16px 0}.port-toggle{display:flex;align-items:center;gap:12px;font-weight:600;font-size:17px}.port-toggle input{width:22px;height:22px;accent-color:var(--accent);flex-shrink:0}#portEditorFields{border:0;margin:0;padding:0}#portEditorFields:disabled{opacity:.6}label{display:block;margin:14px 0 5px;font-size:14px}fieldset{min-width:0;border:1px solid var(--line);border-radius:12px;padding:18px;margin:14px 0}legend{color:var(--accent);padding:0 6px}.safety{padding:18px 20px;border:1px solid var(--line);border-radius:12px;margin-bottom:30px}.safety p{margin:3px 0 0}.actions{justify-content:flex-start;flex-wrap:wrap;gap:8px}.heading{margin:28px 0 16px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,220px),1fr));gap:16px}.card{min-width:0;background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:22px}.device-status{font-size:24px;margin:12px 0 0}.card details{border-top:1px solid var(--line);padding-top:14px;margin-top:18px}.card button{margin-top:12px}.card .actions button{margin-top:0}.row{display:flex;justify-content:space-between;gap:16px;padding:10px 0;border-bottom:1px solid var(--line);overflow-wrap:anywhere}.metric{font-size:28px;margin:10px 0 0}.metric small{margin-left:5px}.stack{display:grid;gap:16px}.empty{color:var(--muted);padding:20px 0}.settings{max-width:720px}details{margin-top:16px}summary{cursor:pointer;font-weight:500}pre{white-space:pre-wrap;overflow-wrap:anywhere;font-size:12px}#message{position:static;background:var(--bg);white-space:pre-wrap;padding:10px 0}#message:empty{display:none}#fans{display:contents}.sensor-source{margin-bottom:0}.notice{border-left:3px solid #c5ad78;padding-left:12px;color:var(--muted);font-size:14px}@media(max-width:600px){main{padding:20px 16px 40px}header{align-items:flex-start}h1{font-size:26px}.safety{align-items:flex-start;flex-direction:column}nav{gap:0}nav button{font-size:14px;padding:12px 9px}.heading{align-items:flex-start;flex-wrap:wrap}.card{padding:18px}}
.controller-card{margin-bottom:20px}.controller-card>.heading{margin:0 0 12px}.controller-layout{display:grid;grid-template-columns:minmax(0,1fr) 240px;gap:24px;align-items:center}#controllerMap{display:block;width:100%;max-width:650px;margin:auto}.connector{cursor:pointer;outline:none}.connector .hit{fill:#1f2c24;stroke:#52614f;stroke-width:3}.connector text{font:14px system-ui,sans-serif;fill:#dce8d4;pointer-events:none}.connector .screen-label{font-size:32px;fill:#c3eea8;letter-spacing:6px}.connector .screen-small{font-size:14px;fill:#c3eea8;letter-spacing:5px}.connector .occupied{fill:#c3eea8;stroke:#263927;stroke-width:2;visibility:hidden}.connector.is-occupied .occupied{visibility:visible}.connector.is-occupied .hit{stroke:#729d57}.connector.future .hit{stroke-dasharray:6 4}.connector:hover .hit,.connector:focus-visible .hit,.connector.is-selected .hit{stroke:#b4da82;stroke-width:6}.connector[data-connector=encoder] .hit{fill:#27352c}.connector[data-connector=encoder] text{fill:#263927}.connector .sensor-label{font-size:16px;fill:#dce8d4}#controllerMap>text{font:14px system-ui,sans-serif;fill:#59654f}#controllerMap .case-wordmark{font-size:28px;letter-spacing:6px}.map-legend{display:flex;flex-wrap:wrap;gap:8px 16px;font-size:13px;margin:24px 0}.port.is-selected{border-left:3px solid var(--accent);padding-left:16px}#connectorHeading{margin:8px 0}#connectorLive:empty{display:none}@media(max-width:760px){.controller-layout{grid-template-columns:1fr}.controller-layout aside{border-top:1px solid var(--line);padding-top:16px}#controllerMap{max-width:500px}}#backupPreview,#trendSummary{overflow-wrap:anywhere}
:root{color-scheme:light dark;--bg:light-dark(#f4f6f2,#141b17);--panel:light-dark(#fff,#1c2620);--line:light-dark(#d5e0d5,#38473c);--text:light-dark(#17251c,#eaf1e9);--muted:light-dark(#526357,#acbdb0);--accent:light-dark(#245e47,#a8dcb8);--accent-text:light-dark(#fff,#142c1d);--selected:light-dark(#e0efe5,#293f30);--pause:light-dark(#f9efdc,#342c1c);--pause-text:light-dark(#6b471c,#f2cf9a)}
main{max-width:1160px;padding:22px 24px 45px}
header{padding:0 0 16px;border-bottom:1px solid var(--line)}
header h1{font-size:23px;margin:0;letter-spacing:-.025em}header h1 span{font-weight:500;color:var(--muted)}
.app-layout{display:grid;grid-template-columns:145px minmax(0,1fr);gap:22px;margin-top:14px}
.app-content{min-width:0}nav{align-self:start;flex-direction:column;margin:0;border:0;gap:5px}
nav button{min-height:44px;text-align:left;border:1px solid transparent;border-radius:10px;padding:9px 12px;overflow-wrap:anywhere}
nav button[aria-current="page"]{color:var(--accent);background:var(--selected);border-color:transparent;font-weight:650}
button,input,select,textarea{border-color:var(--line)}button{min-height:44px}
button.primary{color:var(--accent-text)}button:not(:disabled):hover{border-color:var(--accent)}
input:focus-visible,select:focus-visible,summary:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
.safety{margin:0 0 14px;padding:12px 15px;background:var(--selected)}
.safety[data-paused="true"]{color:var(--pause-text);background:var(--pause);border-color:transparent}
.safety[data-paused="true"] .sub{color:inherit}
#warningPanel{display:block;border-color:var(--line);background:var(--panel)}
#warningList{gap:6px}#warningList p{margin:0}
.heading{margin:20px 0 12px}h2{font-size:clamp(23px,4vw,31px);letter-spacing:-.04em}
.heading .sub{margin:3px 0 0}.card{padding:16px}.controller-card{margin:0}
.overview-stack{display:grid;gap:13px}#trendHistory{margin:0}
#trendCanvas{display:block;width:100%;height:170px}
.controller-layout{grid-template-columns:minmax(0,1fr) minmax(175px,.7fr);gap:12px;align-items:start}
#controllerMap{max-width:470px;background:var(--bg);border:1px solid var(--line);border-radius:12px}
.map-ports,.device-links,.wizard-ports{display:grid;gap:6px}
.map-ports button,.device-links button,.wizard-ports button{margin:0;text-align:left;padding:7px 11px;overflow-wrap:anywhere}
.map-ports small,.device-links small,.wizard-ports small{display:block}
.map-ports button[data-occupied="true"]{background:var(--selected)}
.map-ports button:hover,.map-ports button:focus-visible{border-color:var(--accent)}
.map-note{margin:8px 0 0}.wizard-bar{display:flex;gap:5px;margin:12px 0}
.wizard-bar span{flex:1;height:4px;background:var(--line);border-radius:4px}.wizard-bar .done{background:var(--accent)}
body[data-wizard-step] #bulkPanel,body[data-wizard-step] #deviceLinks,body[data-wizard-step] #manageActions,body[data-wizard-step] #assignment > summary,body[data-wizard-step] #assignmentHelp,body[data-wizard-step] #bulkSave,body[data-wizard-step] #assignment > .sub,body[data-wizard-step] .port-toggle,body[data-wizard-step] .port > .sub{display:none}
body[data-wizard-step] #assignmentCard{margin-top:0}body[data-wizard-step] #assignment{border:0;margin:0;padding:0}body[data-wizard-step] .port{border:0;padding:0}
body[data-wizard-step="identity"] .operation,body[data-wizard-step="identity"] .protection,
body[data-wizard-step="mode"] .identity-field,body[data-wizard-step="mode"] .operation-details,body[data-wizard-step="mode"] .protection,
body[data-wizard-step="details"] .identity-field,body[data-wizard-step="details"] .mode-field,body[data-wizard-step="details"] .mode-hint,
body[data-wizard-step="review"] #equipmentEditor,body[data-wizard-step="port"] #equipmentEditor{display:none}
body[data-wizard-step="identity"] #wizardIdentityHint:not(:empty){display:block}
body[data-wizard-quick="true"][data-wizard-step="mode"] .identity-field{display:block}#wizardIdentityHint{display:none}#wizardReview dl{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,2fr);gap:9px}
#wizardReview dd{margin:0;overflow-wrap:anywhere}#wizardReview dt{color:var(--muted)}
#wizardError{color:light-dark(#a1232d,#ffadb2);margin:8px 0}#wizardError:empty{display:none}
#wizardActions{margin-top:15px}.plan-slot{border:1px solid var(--line);border-radius:10px;padding:10px}
.plan-slot label{flex:1 1 120px;min-width:0;margin:0}.plan-slot button{margin-top:0}
dialog{max-width:min(480px,calc(100% - 24px));color:var(--text);background:var(--panel);border:1px solid var(--line);border-radius:12px;padding:18px}
dialog::backdrop{background:#0008}dialog li{overflow-wrap:anywhere}
@media(max-width:740px){.app-layout{display:block}nav{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));margin-bottom:12px}nav button{text-align:center;padding:7px 2px;font-size:13px}}
@media(max-width:520px){main{padding:12px}.controller-layout{grid-template-columns:minmax(0,1fr)}.map-ports{grid-template-columns:repeat(2,minmax(0,1fr))}.map-ports button{padding:6px}.card{padding:12px}}
</style></head><body><main>
<header><div><h1>GROVA <span>Core</span></h1><span id="connection" class="sub">Verbindung wird hergestellt …</span></div><span class="pill" id="boardHealth">Verbinden …</span></header>
<div class="app-layout">
<nav aria-label="Hauptnavigation"><button id="nav-overview" aria-current="page" onclick="showView('overview')">Übersicht</button><button id="nav-manage" onclick="showView('manage')">Geräte</button><button id="nav-sensors" onclick="showView('sensors')">Sensoren</button><button id="nav-settings" onclick="showView('settings')">System</button></nav>
<div class="app-content">
<div id="message" role="status" aria-live="polite"></div>
<section class="safety" id="operationState" aria-label="Ausgangssicherheit"><div><strong id="lockState">Ausgangszustand wird geladen</strong><p id="lockHelp" class="sub"></p></div><div class="actions"><button id="rest" onclick="toggleRest()" disabled>Ausgänge freigeben</button></div></section>
<dialog id="resumeDialog" aria-labelledby="resumeTitle"><form method="dialog"><h3 id="resumeTitle">Betrieb fortsetzen?</h3><p id="resumeDescription"></p><ul id="resumeDevices"></ul><div class="actions"><button value="cancel" autofocus>Abbrechen</button><button id="confirmResume" type="button" class="primary" onclick="resumeOperation()">Betrieb freigeben</button></div></form></dialog>
<section id="warningPanel" class="safety" aria-labelledby="warningHeading" hidden><h2 id="warningHeading">Warnungen <span id="warningCount"></span></h2><p id="warningStatus" class="sub"></p><div id="warningList" class="stack"></div></section>
<section id="view-overview" aria-label="Übersicht">
<div class="heading"><div><h2>Übersicht</h2><p class="sub">Messwerte und Gerätezustände auf einen Blick.</p></div><button class="primary" data-add-device onclick="startWizard()" disabled>Gerät hinzufügen</button></div>
<div class="overview-stack">
<div class="grid" id="signals"></div>
<details id="trendHistory" class="card" open><summary>Messwertverlauf</summary><p class="sub">Letzte Stunde · Messpunkte alle 30 Sekunden · seit dem letzten Neustart</p><label>Messreihe<select id="trendSeries" onchange="drawHistory()"></select></label><p id="trendStatus" class="sub">Warte auf Verbindung …</p><canvas id="trendCanvas" role="img" aria-label="Verlauf der gewählten Messreihe"></canvas><p id="trendSummary" class="sub"></p></details>
<section class="card controller-card" aria-label="Controller-Anschlussplan"><div class="heading"><div><h3>Anschlussplan</h3><p class="sub">GROVA DC022B · Gehäuse-Draufsicht</p></div></div><div class="controller-layout"><svg id="controllerMap" viewBox="0 0 1000 1000" aria-labelledby="controllerTitle controllerDesc"><title id="controllerTitle">GROVA DC022B · vereinfachte Draufsicht</title><desc id="controllerDesc">Ausgänge oben von links nach rechts: 12 Volt 1, 12 Volt 2, 5 Volt, 0 bis 10 Volt 1 und 2. Sensoren unten links, Lüfter rechts neben der Anschlusswanne.</desc>
<rect x="50" y="50" width="900" height="900" rx="48" fill="#718367"/><rect x="63" y="63" width="874" height="874" rx="36" fill="#d9ccb0"/>
<path d="M88 405 Q80 405 80 415 V515 Q80 540 100 540 V845 Q100 860 125 870 V905 Q125 915 140 915 H420 Q430 915 430 900 V595 Q430 580 465 580 V445 Q465 430 450 430 H160 Q155 430 155 415 V405Z" fill="#17231e"/>

<g class="connector" data-connector="OUT12_1" role="button" tabindex="0" aria-label="12 V · 1" aria-pressed="false"><title>12 V · 1</title><rect class="hit" x="125" y="55" width="96" height="157" rx="9"/><rect x="133" y="68" width="80" height="116" rx="5" fill="#414944"/><rect x="143" y="62" width="23" height="88" rx="5" fill="#b2834b"/><rect x="173" y="62" width="23" height="88" rx="5" fill="#b2834b"/><text x="173" y="200" text-anchor="middle">12 V · 1</text><circle class="occupied" cx="206" cy="74" r="7"/></g>
<g class="connector" data-connector="OUT12_2" role="button" tabindex="0" aria-label="12 V · 2" aria-pressed="false"><title>12 V · 2</title><rect class="hit" x="257" y="55" width="96" height="157" rx="9"/><rect x="265" y="68" width="80" height="116" rx="5" fill="#414944"/><rect x="275" y="62" width="23" height="88" rx="5" fill="#b2834b"/><rect x="305" y="62" width="23" height="88" rx="5" fill="#b2834b"/><text x="305" y="200" text-anchor="middle">12 V · 2</text><circle class="occupied" cx="338" cy="74" r="7"/></g>
<g class="connector" data-connector="OUT5_1" role="button" tabindex="0" aria-label="5 V" aria-pressed="false"><title>5 V</title><rect class="hit" x="389" y="55" width="96" height="157" rx="9"/><rect x="397" y="68" width="80" height="116" rx="5" fill="#414944"/><rect x="407" y="62" width="23" height="88" rx="5" fill="#b2834b"/><rect x="437" y="62" width="23" height="88" rx="5" fill="#b2834b"/><text x="437" y="200" text-anchor="middle">5 V</text><circle class="occupied" cx="470" cy="74" r="7"/></g>
<g class="connector" data-connector="0_10V_OUT_1" role="button" tabindex="0" aria-label="0–10 V · 1" aria-pressed="false"><title>0–10 V · 1</title><rect class="hit" x="521" y="55" width="96" height="157" rx="9"/><rect x="529" y="68" width="80" height="116" rx="5" fill="#414944"/><rect x="539" y="62" width="23" height="88" rx="5" fill="#b2834b"/><rect x="569" y="62" width="23" height="88" rx="5" fill="#b2834b"/><text x="569" y="200" text-anchor="middle">0–10 V · 1</text><circle class="occupied" cx="602" cy="74" r="7"/></g>
<g class="connector" data-connector="0_10V_OUT_2" role="button" tabindex="0" aria-label="0–10 V · 2" aria-pressed="false"><title>0–10 V · 2</title><rect class="hit" x="653" y="55" width="96" height="157" rx="9"/><rect x="661" y="68" width="80" height="116" rx="5" fill="#414944"/><rect x="671" y="62" width="23" height="88" rx="5" fill="#b2834b"/><rect x="701" y="62" width="23" height="88" rx="5" fill="#b2834b"/><text x="701" y="200" text-anchor="middle">0–10 V · 2</text><circle class="occupied" cx="734" cy="74" r="7"/></g>
<g class="connector" data-connector="display" role="button" tabindex="0" aria-label="Display einstellen" aria-pressed="false"><rect class="hit screen" x="322" y="227" width="304" height="176" rx="10"/><text x="474" y="300" text-anchor="middle" class="screen-label">GROVA</text><text x="474" y="328" text-anchor="middle" class="screen-small">CORE</text></g>
<g class="connector" data-connector="encoder" role="button" tabindex="0" aria-label="Drehgeber – Hardwarebedienung" aria-pressed="false"><circle class="hit" cx="812" cy="313" r="96"/><circle cx="812" cy="313" r="73" fill="#414944"/><path d="M812 248 V265" stroke="#a7b49e" stroke-width="5"/><text x="812" y="427" text-anchor="middle">Drehgeber</text></g>
<g class="connector" data-connector="FAN1" role="button" tabindex="0" aria-label="PWM-Lüfter 1" aria-pressed="false"><rect class="hit" x="582" y="445" width="111" height="65" rx="6"/><rect x="590" y="453" width="95" height="25" fill="#9ca495"/><circle cx="603" cy="475" r="4" fill="#d8be7d"/><circle cx="626" cy="475" r="4" fill="#d8be7d"/><circle cx="649" cy="475" r="4" fill="#d8be7d"/><circle cx="672" cy="475" r="4" fill="#d8be7d"/><text x="638" y="500" text-anchor="middle">FAN 1</text><circle class="occupied" cx="680" cy="456" r="6"/></g>
<g class="connector" data-connector="FAN2" role="button" tabindex="0" aria-label="PWM-Lüfter 2" aria-pressed="false"><rect class="hit" x="582" y="520" width="111" height="65" rx="6"/><rect x="590" y="528" width="95" height="25" fill="#9ca495"/><circle cx="603" cy="550" r="4" fill="#d8be7d"/><circle cx="626" cy="550" r="4" fill="#d8be7d"/><circle cx="649" cy="550" r="4" fill="#d8be7d"/><circle cx="672" cy="550" r="4" fill="#d8be7d"/><text x="638" y="575" text-anchor="middle">FAN 2</text><circle class="occupied" cx="680" cy="531" r="6"/></g>
<g class="connector future" data-connector="onewire" role="button" tabindex="0" aria-label="1W – noch nicht verfügbar" aria-pressed="false"><rect class="hit" x="195" y="445" width="50" height="85" rx="5"/><rect x="203" y="453" width="34" height="51" fill="#657164"/><text x="220.0" y="520" text-anchor="middle">1W</text></g>
<g class="connector future" data-connector="safety" role="button" tabindex="0" aria-label="SAFE – noch nicht verfügbar" aria-pressed="false"><rect class="hit" x="255" y="445" width="50" height="85" rx="5"/><rect x="263" y="453" width="34" height="51" fill="#657164"/><text x="280.0" y="520" text-anchor="middle">SAFE</text></g>
<g class="connector future" data-connector="water" role="button" tabindex="0" aria-label="H₂O – noch nicht verfügbar" aria-pressed="false"><rect class="hit" x="315" y="445" width="50" height="85" rx="5"/><rect x="323" y="453" width="34" height="51" fill="#657164"/><text x="340.0" y="520" text-anchor="middle">H₂O</text></g>
<g class="connector future" data-connector="adc1" role="button" tabindex="0" aria-label="ADC1 – noch nicht verfügbar" aria-pressed="false"><rect class="hit" x="195" y="545" width="50" height="80" rx="5"/><rect x="203" y="553" width="34" height="46" fill="#657164"/><text x="220.0" y="615" text-anchor="middle">ADC1</text></g>
<g class="connector future" data-connector="adc2" role="button" tabindex="0" aria-label="ADC2 – noch nicht verfügbar" aria-pressed="false"><rect class="hit" x="255" y="545" width="50" height="80" rx="5"/><rect x="263" y="553" width="34" height="46" fill="#657164"/><text x="280.0" y="615" text-anchor="middle">ADC2</text></g>
<g class="connector future" data-connector="uart" role="button" tabindex="0" aria-label="UART – noch nicht verfügbar" aria-pressed="false"><rect class="hit" x="410" y="445" width="48" height="130" rx="5"/><rect x="418" y="453" width="32" height="96" fill="#657164"/><text x="434.0" y="565" text-anchor="middle">UART</text></g>
<g class="connector future" data-connector="rs485" role="button" tabindex="0" aria-label="RS485 – noch nicht verfügbar" aria-pressed="false"><rect class="hit" x="83" y="445" width="63" height="67" rx="5"/><rect x="91" y="453" width="47" height="33" fill="#657164"/><text x="114.5" y="502" text-anchor="middle">RS485</text></g>
<g class="connector" data-connector="sensors" role="button" tabindex="0" aria-label="Sensoren – acht I²C-Ports" aria-pressed="false"><rect class="hit" x="144" y="645" width="280" height="248" rx="10"/>
<rect x="156" y="660" width="50" height="95" rx="4" fill="#a7afa1"/><circle cx="181" cy="676" r="3" fill="#414e44"/><circle cx="181" cy="696" r="3" fill="#414e44"/><circle cx="181" cy="716" r="3" fill="#414e44"/><circle cx="181" cy="736" r="3" fill="#414e44"/>
<rect x="220" y="660" width="50" height="95" rx="4" fill="#a7afa1"/><circle cx="245" cy="676" r="3" fill="#414e44"/><circle cx="245" cy="696" r="3" fill="#414e44"/><circle cx="245" cy="716" r="3" fill="#414e44"/><circle cx="245" cy="736" r="3" fill="#414e44"/>
<rect x="284" y="660" width="50" height="95" rx="4" fill="#a7afa1"/><circle cx="309" cy="676" r="3" fill="#414e44"/><circle cx="309" cy="696" r="3" fill="#414e44"/><circle cx="309" cy="716" r="3" fill="#414e44"/><circle cx="309" cy="736" r="3" fill="#414e44"/>
<rect x="348" y="660" width="50" height="95" rx="4" fill="#a7afa1"/><circle cx="373" cy="676" r="3" fill="#414e44"/><circle cx="373" cy="696" r="3" fill="#414e44"/><circle cx="373" cy="716" r="3" fill="#414e44"/><circle cx="373" cy="736" r="3" fill="#414e44"/>
<rect x="156" y="770" width="50" height="95" rx="4" fill="#a7afa1"/><circle cx="181" cy="786" r="3" fill="#414e44"/><circle cx="181" cy="806" r="3" fill="#414e44"/><circle cx="181" cy="826" r="3" fill="#414e44"/><circle cx="181" cy="846" r="3" fill="#414e44"/>
<rect x="220" y="770" width="50" height="95" rx="4" fill="#a7afa1"/><circle cx="245" cy="786" r="3" fill="#414e44"/><circle cx="245" cy="806" r="3" fill="#414e44"/><circle cx="245" cy="826" r="3" fill="#414e44"/><circle cx="245" cy="846" r="3" fill="#414e44"/>
<rect x="284" y="770" width="50" height="95" rx="4" fill="#a7afa1"/><circle cx="309" cy="786" r="3" fill="#414e44"/><circle cx="309" cy="806" r="3" fill="#414e44"/><circle cx="309" cy="826" r="3" fill="#414e44"/><circle cx="309" cy="846" r="3" fill="#414e44"/>
<rect x="348" y="770" width="50" height="95" rx="4" fill="#a7afa1"/><circle cx="373" cy="786" r="3" fill="#414e44"/><circle cx="373" cy="806" r="3" fill="#414e44"/><circle cx="373" cy="826" r="3" fill="#414e44"/><circle cx="373" cy="846" r="3" fill="#414e44"/>
<text x="284" y="915" text-anchor="middle" class="sensor-label">SENSOREN · I²C</text></g>
<circle cx="703" cy="666" r="15" fill="#5b6656"/><circle cx="760" cy="666" r="8" fill="#414944"/><circle cx="794" cy="666" r="8" fill="#414944"/><circle cx="828" cy="666" r="8" fill="#414944"/>
<circle cx="106" cy="895" r="17" fill="#718367"/><circle cx="501" cy="895" r="17" fill="#718367"/>
<text x="744" y="822" class="case-wordmark" text-anchor="middle">GROVA</text><text x="744" y="850" text-anchor="middle">CORE · DC022B</text></svg>
<div><div id="mapPorts" class="map-ports" role="group" aria-label="Anschlüsse und Belegung"></div><p id="mapInfo" class="sub map-note" role="status"></p></div></div></section>
<section aria-label="Gerätezustände"><div class="heading"><h3>Gerätezustand</h3></div><p class="sub" id="emptyEquipment" hidden>Keine Geräte eingerichtet. Wähle „Gerät hinzufügen“ oder einen freien Anschluss im Plan.</p><div class="grid"><div id="equipment" style="display:contents"></div><div id="fans"></div></div></section>
<details id="eventHistory" class="card"><summary>Letzte Ereignisse</summary><p class="sub">Die letzten 64 Ereignisse seit diesem Neustart. Im RAM gespeichert, ohne zusätzliche Flash-Schreibzyklen. Nach einem Neustart beginnt das Protokoll neu.</p><p id="eventMeta" class="sub"></p><button id="downloadEvents" onclick="downloadEventLog()">Protokoll herunterladen</button><div id="eventList"></div></details>
</div></section>
<section id="view-manage" aria-label="Geräte" hidden>
<div id="manageActions" class="heading"><div><h2>Geräte</h2><p class="sub">Eingerichtete Geräte und ihre Steuerung.</p></div><button class="primary" data-add-device onclick="startWizard()" disabled>Gerät hinzufügen</button></div>
<div id="deviceLinks" class="device-links"></div>
<div id="bulkPanel" class="actions"><button id="bulkEquipment" onclick="openBulk()">Alle Anschlüsse bearbeiten</button></div>
<section class="card" id="assignmentCard" hidden>
<div id="wizardHeader" class="heading" hidden><div><h2 id="wizardTitle">Gerät hinzufügen</h2><p id="wizardCaption" class="sub"></p></div></div>
<div id="wizardProgress" class="wizard-bar" aria-hidden="true" hidden></div>
<div id="wizardPorts" class="wizard-ports" hidden></div>
<p id="wizardIdentityHint" class="sub"></p>
<div id="bulkSelect" hidden><label>Gehe zu Anschluss<select id="connectorSelect" onchange="selectConnector(this.value)"><option value="all">Alle Anschlüsse</option></select></label></div>
<p class="notice" id="assignmentHelp"></p>
<details id="assignment" open><summary>Belegung bearbeiten</summary><div id="equipmentEditor"></div><div class="actions" id="bulkSave"><button id="saveEquipment" class="primary" onclick="saveEquipment()" disabled>Pausiert speichern</button><button id="discardEquipment" onclick="buildEquipmentEditor(state)">Entwurf verwerfen / neu laden</button></div><p class="sub">0–10 V liefert das Steuersignal. Das Gerät wird separat versorgt.</p></details>
<div id="wizardReview" hidden></div><p id="wizardError" role="alert"></p>
<div class="actions" id="wizardActions" hidden><button id="wizardCancel" onclick="cancelWizard()">Abbrechen</button><button id="wizardBack" onclick="advanceWizard(-1)">Zurück</button><button id="wizardRemove" onclick="removeWizardDevice()" hidden>Gerät entfernen</button><button id="wizardNext" class="primary" onclick="advanceWizard(1)">Weiter</button></div>
</section></section>
<section id="view-sensors" aria-label="Sensoren" hidden><div class="heading"><div><h2>Sensoren</h2><p class="sub">Unterstützte I²C-Sensoren erkennt der Core automatisch.</p></div></div><section class="card" id="sensorCard"><h3>Angeschlossene Sensoren</h3><div id="sensorList"></div></section></section>
<section id="view-settings" aria-label="Einstellungen" hidden><div class="heading"><h2>System</h2></div><div class="settings stack">
<section class="card"><h3>WLAN</h3><p id="wifiStatus" class="sub"></p><a href="/wifi">WLAN einrichten oder wechseln →</a></section>
<section class="card"><h3>Uhrzeit</h3><div id="clock"></div><button id="rtcToggle" onclick="send({cmd:'set_rtc_config',enabled:!state.rtc.enabled})" disabled>Hardware-Uhr umschalten</button></section>
<section class="card"><h3>Geräte und Regeln sichern</h3><p class="sub">Sichert Zuordnungen, Betriebsarten und Regeln als JSON-Datei. Ohne WLAN-/MQTT-Passwörter, aktuelle Ein-/Auszustände, Messwertverlauf oder Laufjournale.</p><button id="exportConfiguration" onclick="downloadConfiguration()">Konfiguration herunterladen</button><label>Sicherung auswählen<input id="backupFile" type="file" accept="application/json,.json" onchange="previewConfiguration(this.files[0])"></label><p id="backupPreview" class="sub">Noch keine Sicherung ausgewählt.</p><details id="backupDetails" hidden><summary>Geräte in der Sicherung</summary><div id="backupItems"></div></details><p class="notice">Wiederherstellen pausiert alle Geräte. Danach bleiben die Ausgänge gesperrt, bis du den Betrieb ausdrücklich fortsetzt. Ungespeicherte Entwürfe werden ersetzt.</p><button id="restoreConfiguration" onclick="restoreConfiguration()" disabled>Ausgewählte Sicherung wiederherstellen</button></section>
<section class="card"><h3>Display</h3><p id="displayStatus" class="sub"></p><details><summary>Ausrichtung anpassen</summary><label>Ausrichtung<select id="displayRotation"><option value="0">0°</option><option value="1">90°</option><option value="2">180°</option><option value="3">270°</option></select></label><label>Zeilenoffset (0–95)<input id="displayOffset" type="number" min="0" max="95" value="80"></label><button onclick="send({cmd:'set_display_config',rotation:Number(el('displayRotation').value),offset:Number(el('displayOffset').value)})">Display speichern</button></details></section>
<section class="card"><details><summary>Erweitert</summary><details><summary>MQTT-Verbindung</summary><label>Broker<input id="mqttHost" maxlength="128" placeholder="IP-Adresse oder Hostname"></label><label>Port<input id="mqttPort" type="number" min="1" max="65535" value="1883"></label><label>Benutzer<input id="mqttUser" maxlength="64" autocomplete="username"></label><label>Passwort<input id="mqttPassword" type="password" maxlength="128" autocomplete="new-password"></label><button onclick="saveMqtt()">MQTT speichern</button><p class="sub">Optional. Passwort beim Speichern erneut eingeben. Leerer Broker deaktiviert MQTT.</p></details>

<details><summary>Diagnose &amp; Ausgangstests</summary><div id="system"></div><details><summary>Freie Ausgänge testen</summary><p class="notice">Nur ohne schutzbedürftige Last testen. Zugeordnete Anschlüsse sind gesperrt. 0–10 V sind Sollwerte, keine Spannungsmessung.</p><div id="outputs"></div></details><details><summary>Sensor-Rohdaten</summary><pre id="devices"></pre></details></details></details></section></div></section>
<script>
const el=id=>document.getElementById(id);
let selectedConnector='all', currentView='overview', bulkOpen=false, wizard=null, equipmentSaving=false;
let state=null,initialized=false,busy=false,connected=false,equipmentRevision=-1,equipmentDirty=false;
let pendingEquipment=null,statusBusy=false,lastCommandUncertain=false;
let backupDraft=null,backupRevision=-1,backupLoadSerial=0,historyData=null,historyBusy=false;
const signalNames={'climate.temperature_c':'Temperatur','climate.humidity_pct':'Luftfeuchtigkeit','climate.pressure_hpa':'Luftdruck','climate.co2_ppm':'CO₂','light.lux':'Helligkeit','light.uv_index':'UV-Index'};
const reasons={REST:'Ausgänge gesperrt',OTA:'Firmware-Update',STORAGE:'Speicher prüfen',OUTPUT_INIT:'Ausgang nicht bereit',MANUAL:'Manuelle Bedienung',RUNTIME_COMPLETE:'Laufzeit beendet',SCHEDULE:'Zeitplan aktiv',WAIT_TIME:'Warte auf gültige Uhrzeit',OFF:'Aus',FAULT:'Fehler',AUTO:'Vorhandene Regel aktiv'};
function showView(view){
 if(view!==currentView&&(pendingEquipment||equipmentSaving)&&wizard)return;
 if(view!==currentView&&wizard){if(equipmentDirty&&!confirm('Ungespeicherten Entwurf verwerfen?'))return;cancelWizard(false);}
 currentView=view;
 for(const name of ['overview','manage','sensors','settings']){
  el('view-'+name).hidden=name!==view;
  el('nav-'+name).setAttribute('aria-current',name===view?'page':'false');
 }
 if(view==='overview'&&connected)refreshHistory();
}
const row=(parent,name,value)=>{const d=document.createElement('div');d.className='row';const a=document.createElement('span'),b=document.createElement('span');a.textContent=name;b.textContent=value;d.append(a,b);parent.append(d);};
function button(parent,text,handler,energize=false){const b=document.createElement('button');b.textContent=text;b.onclick=handler;if(energize)b.dataset.energize='1';parent.append(b);return b;}
function operatingPanel(box,label='Bedienen'){const details=document.createElement('details'),summary=document.createElement('summary');summary.textContent=label;details.append(summary);box.append(details);return details;}
function numberField(parent,id,label,value,min,max,step=1){const l=document.createElement('label');l.textContent=label;const input=document.createElement('input');Object.assign(input,{id,type:'number',value,min,max,step});l.append(input);parent.append(l);return input;}
async function apiRequest(url,options={}){
 const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),5000);
 try{const response=await fetch(url,{...options,signal:controller.signal});const data=await response.json();return {response,data};}finally{clearTimeout(timer);}
}
function showOffline(){connected=false;el('connection').textContent='Board nicht erreichbar';el('boardHealth').textContent='Offline';el('lockState').textContent='Ausgangszustand unbekannt';delete el('operationState').dataset.paused;el('lockHelp').textContent='Angezeigte Werte sind zuletzt empfangen. Verbindung prüfen.';el('warningPanel').hidden=false;el('warningCount').textContent='(Stand unbekannt)';el('warningStatus').textContent='Warnungsstand unbekannt: Board nicht erreichbar. Angezeigte Diagnosen sind zuletzt empfangen.';el('eventMeta').textContent='Board nicht erreichbar. Letzter empfangener Protokollstand.';el('trendStatus').textContent='Board nicht erreichbar. Verlauf zeigt den zuletzt geladenen Stand.';gateControls();}
function settleEquipment(d){if(!pendingEquipment||busy)return;
 if(d.equipment_revision>pendingEquipment.expected){
  const wasWizard=pendingEquipment.wizard;
  pendingEquipment=null;
  if(wasWizard)closeWizard();
  buildEquipmentEditor(d);
  if(wasWizard)el('view-manage').querySelector('.device-links button, [data-add-device]:enabled')?.focus();
  el('message').textContent='Belegung vom Core bestätigt. Die Ausgänge bleiben pausiert.';
 }
 else if(pendingEquipment.uncertain){pendingEquipment=null;el('message').textContent='Keine neue Belegung bestätigt. Entwurf bleibt erhalten; bei Bedarf erneut übernehmen.';}
}
async function send(payload){lastCommandUncertain=false;if(!connected){el('message').textContent='Board nicht erreichbar. Der tatsächliche Gerätezustand ist unbekannt.';return false;}if(busy){el('message').textContent='Ein Befehl wird noch verarbeitet.';return false;}busy=true;gateControls();let acknowledged=false;
 try{const {response:r,data:result}=await apiRequest('/api/v1/control',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});acknowledged=typeof result.ok==='boolean';if(!acknowledged)throw Error('Ungültige Antwort');if(!r.ok||!result.ok)throw Error(result.message||'Befehl abgelehnt');el('message').textContent=payload.cmd==='configure_equipment'?'Gespeichert; warte auf bestätigten Boardstand.':payload.cmd==='stop_all'?'Alle Geräte und Zeitpläne pausiert. Bleibt auch nach Neustart erhalten.':payload.cmd==='set_rest_mode'?(payload.enabled?'Ausgänge gesperrt.':'Betrieb freigegeben. Gespeicherte Zeitpläne laufen wieder.'):'Gespeichert / übernommen.';await refresh();return true;
 }catch(e){if(!acknowledged){lastCommandUncertain=true;showOffline();el('message').textContent='Antwort fehlt. Ausführung unklar; Status wird neu abgefragt. Keine automatische Wiederholung.';}else el('message').textContent=e.message;return false;}finally{busy=false;gateControls();}}

function toggleRest(){
 if(!state||el('rest').disabled)return;
 if(!state.rest_mode.enabled){send({cmd:'set_rest_mode',enabled:true});return;}
 const dialog=el('resumeDialog');
 dialog.dataset.revision=String(state.equipment_revision);
 updateResumeDialog();
 el('confirmResume').disabled=false;
 dialog.showModal();
}
function updateResumeDialog(){
 const devices=[...(state?.equipment||[]),...(state?.fan_ports||[]).filter(f=>f.enabled)].filter(e=>e.plan?.enabled||e.mode==='automatic');
 const list=el('resumeDevices'),signature=JSON.stringify(devices.map(e=>e.name));
 if(list.dataset.signature!==signature){
  list.dataset.signature=signature;list.replaceChildren();
  for(const e of devices){const item=document.createElement('li');item.textContent=e.name;list.append(item);}
 }
 el('resumeDescription').textContent=devices.length?'Diese Geräte können je nach Zeitplan oder Messwert anlaufen.':'Die Ausgänge werden für die manuelle Bedienung freigegeben.';
}
function resumeOperation(){
 if(el('confirmResume').disabled||el('rest').disabled||!state?.rest_mode.enabled)return;
 el('resumeDialog').close();
 send({cmd:'set_rest_mode',enabled:false});
}
function saveMqtt(){send({cmd:'set_mqtt_config',host:el('mqttHost').value,port:Number(el('mqttPort').value),user:el('mqttUser').value,password:el('mqttPassword').value});el('mqttPassword').value='';}
function buildControls(d){
 for(const o of d.outputs){const box=document.createElement('div'),title=document.createElement('label'),live=document.createElement('p');title.textContent=o.id;live.id='live_'+o.id;live.className='sub';box.append(title,live);
 if(o.kind==='switch'){for(const on of [true,false]){const b=button(box,on?'Ein':'Aus',()=>send({cmd:'test_output',output:o.id,state:on}),on);b.dataset.output=o.id;}}
 else{const input=numberField(box,'value_'+o.id,'Sollspannung / V',0,0,10,.1);const b=button(box,'Spannung setzen',()=>send({cmd:'test_output',output:o.id,percent:Number(input.value)*10}),true);b.dataset.output=o.id;}el('outputs').append(box);}

 el('displayRotation').value=d.display.rotation;el('displayOffset').value=d.display.offset;
 el('mqttHost').value=d.mqtt.host||'';el('mqttPort').value=d.mqtt.port||1883;el('mqttUser').value=d.mqtt.user||'';
}
function renderSensors(d){el('signals').replaceChildren();for(const s of d.signals.filter(s=>s.valid)){const box=document.createElement('article'),title=document.createElement('h3'),value=document.createElement('p');box.className='card';title.textContent=signalNames[s.id]||'Messwert';value.className='metric';value.textContent=s.value.toLocaleString('de-DE',{maximumFractionDigits:1})+' '+(s.unit==='C'?'°C':s.unit);box.append(title,value);el('signals').append(box);}if(!d.signals.some(s=>s.valid))el('signals').textContent='Noch keine aktuellen Messwerte. Sensoren anschließen und Erkennung prüfen.';
 el('sensorList').replaceChildren();if(!d.devices.length)el('sensorList').textContent='Keine unterstützten Sensoren erkannt.';for(const sensor of d.devices){const box=document.createElement('div');row(box,sensor.type,sensor.valid?'Messbereit':sensor.ready?'Warte auf Messwert':'Nicht bereit');const p=document.createElement('p');p.className='sub';p.textContent=(sensor.channel<0?'Systembus':'I²C-Port '+sensor.channel)+' · Adresse 0x'+sensor.address.toString(16).toUpperCase();box.append(p);el('sensorList').append(box);}}
function gateControls(){const locked=!connected||!state||state.rest_mode.enabled;document.querySelectorAll('[data-energize="1"]').forEach(b=>{const o=state?.outputs.find(o=>o.id===b.dataset.output),e=state?.equipment.find(e=>e.id===b.dataset.device);b.disabled=locked||!state?.outputs_ready||!!(b.dataset.fan&&(!state?.equipment_settings_ok||!state?.fan_ports?.[Number(b.dataset.fan)-1]?.enabled))||!!(b.dataset.output&&(!o?.ready||o.assigned))||!!(b.dataset.device&&(!state.equipment_settings_ok||e?.startup_remaining_ms>0));});document.querySelectorAll('[data-output]').forEach(b=>{const o=state?.outputs.find(o=>o.id===b.dataset.output);if(!connected||o?.assigned)b.disabled=true;else if(b.dataset.energize!=='1')b.disabled=false;});const conflict=!!state&&equipmentRevision!==state.equipment_revision;el('saveEquipment').disabled=!connected||!state?.settings_ok||busy||!!pendingEquipment||conflict;if(el('portEditorFields'))el('portEditorFields').disabled=!connected||busy||!!pendingEquipment;el('discardEquipment').disabled=busy||!!pendingEquipment||!connected;if(conflict&&equipmentDirty&&!pendingEquipment)el('assignmentHelp').textContent='Belegung wurde anderweitig geändert. Entwurf verwerfen / neu laden, bevor du speicherst.';el('rest').disabled=!connected||busy||equipmentSaving||!!pendingEquipment||(state?.rest_mode.enabled&&(!state.settings_ok||!state.outputs_ready));
 const resumeDialog=el('resumeDialog');
 const resumeChanged=resumeDialog.open&&resumeDialog.dataset.revision!==String(state?.equipment_revision);
 el('confirmResume').disabled=el('rest').disabled||!state?.rest_mode.enabled||resumeChanged;
 if(resumeChanged)el('resumeDescription').textContent='Die Belegung wurde geändert. Schließe den Dialog und prüfe die Freigabe erneut.';
 else if(resumeDialog.open&&connected)updateResumeDialog();
 document.querySelectorAll('[data-add-device]').forEach(b=>b.disabled=!connected||busy||equipmentSaving||!!pendingEquipment||!state?.settings_ok||!availablePorts().length);
 el('bulkEquipment').disabled=!connected||busy||equipmentSaving||!!pendingEquipment;
 el('wizardNext').disabled=!connected||!state?.settings_ok||busy||equipmentSaving||!!pendingEquipment||conflict;
 for(const id of ['wizardBack','wizardCancel','wizardRemove'])el(id).disabled=busy||equipmentSaving||!!pendingEquipment;
 if(wizard&&conflict&&!pendingEquipment)el('wizardError').textContent='Belegung wurde auf dem Core geändert. Brich den Entwurf ab und öffne das Gerät erneut.';el('rtcToggle').disabled=!connected;el('exportConfiguration').disabled=!connected||busy;el('backupFile').disabled=busy||!!pendingEquipment;el('restoreConfiguration').disabled=!connected||busy||!!pendingEquipment||!backupDraft||backupRevision!==state?.equipment_revision;}
async function refresh(){if(statusBusy)return false;statusBusy=true;try{const wasConnected=connected;const {response:r,data:d}=await apiRequest('/api/v1/status',{cache:'no-store'});if(!r.ok)throw Error('Status nicht erreichbar');state=d;connected=true;if(!initialized){buildControls(d);initialized=true;}
 el('connection').textContent=d.wifi_connected?'WLAN verbunden · '+d.ip:'WLAN nicht verbunden';el('boardHealth').textContent=d.healthy?'Board bereit':'Board prüfen';el('rest').textContent=d.rest_mode.enabled?'Betrieb fortsetzen':'Alles pausieren';el('lockState').textContent=d.rest_mode.enabled?'Ausgänge pausiert':'Ausgänge freigegeben';el('operationState').dataset.paused=String(d.rest_mode.enabled);el('lockHelp').textContent=d.rest_mode.enabled?'Betrieb pausiert. Einstellungen bleiben gespeichert; zum Fortsetzen freigeben.':d.equipment.some(e=>e.plan?.enabled||e.mode==='automatic')?'Automatisierung aktiv. Zeit- und Sensorregeln laufen selbstständig weiter.':d.fan_ports?.some(f=>f.enabled&&(f.plan?.enabled||f.mode==='automatic'))?'Automatisierung aktiv. Zeit- und Sensorregeln laufen selbstständig weiter.':'Manuelle Bedienung freigegeben.';
 renderSensors(d);renderEquipment(d);refreshSensorOptions();settleEquipment(d);updateControllerMap();el('emptyEquipment').hidden=!!d.equipment.length||!!d.fan_ports?.some(f=>f.enabled);
 for(const o of d.outputs)el('live_'+o.id).textContent=(o.assigned?'Zugeordnet · ':'Frei · ')+(o.kind==='switch'?(o.percent>0?'Ein':'Aus'):o.voltage_setpoint.toFixed(2)+' V Soll');
 renderFans(d);renderWarnings(d);renderEvents(d);
 el('wifiStatus').textContent=d.wifi_connected?'Verbunden · '+d.ip:'Nicht verbunden';el('displayStatus').textContent=d.oled_present?'Display erreichbar':'Kein Display erreichbar';el('clock').replaceChildren();row(el('clock'),'Uhrzeit',d.time_synced?new Date(d.epoch_s*1000).toLocaleString('de-DE',{timeZone:d.schedule_timezone||'Europe/Berlin'}):'Noch nicht synchronisiert');row(el('clock'),'Hardware-Uhr',!d.rtc.enabled?'Deaktiviert':d.rtc.present?(d.rtc.valid?'Bereit':'Zeit prüfen'):'Nicht erkannt');el('rtcToggle').textContent=d.rtc.enabled?'Hardware-Uhr deaktivieren':'Hardware-Uhr aktivieren';
 el('system').replaceChildren();row(el('system'),'Board-ID',d.cube_id);row(el('system'),'Firmware',d.firmware_version||'Unbekannt');row(el('system'),'Build',(d.firmware_build_date||'')+' '+(d.firmware_build_time||''));row(el('system'),'OTA',d.ota_ready?'Bereit im Heimnetz':'Nicht bereit');row(el('system'),'Softwarestatus',d.warning);row(el('system'),'Ausgangstreiber',d.outputs_ready?'Bereit':'Initialisierung fehlgeschlagen');row(el('system'),'Zeitquelle',d.time_source);row(el('system'),'Batterieumschaltung',d.rtc.backup_configured?'Konfiguriert':'Nicht konfiguriert');row(el('system'),'I²C-Multiplexer',d.mux_present?'Erkannt':'Nicht erkannt');row(el('system'),'MQTT',d.mqtt_connected?'Verbunden':'Getrennt');row(el('system'),'Speicher',Math.round(d.flash_bytes/1048576)+' MB Flash / '+Math.round(d.psram_bytes/1048576)+' MB PSRAM');
 el('devices').textContent=JSON.stringify(d.devices,null,2);gateControls();if(!wasConnected&&currentView==='overview')refreshHistory();
 }catch(e){showOffline();return false;}finally{statusBusy=false;}}
function selectField(parent,id,label,options,value){const l=document.createElement('label');l.textContent=label;const input=document.createElement('select');input.id=id;for(const [v,text] of options){const o=document.createElement('option');o.value=v;o.textContent=text;input.append(o);}input.value=value??'';input.onchange=()=>{equipmentDirty=true;};l.append(input);parent.append(l);return input;}
function inputField(parent,id,label,value,type='text'){const l=document.createElement('label');l.textContent=label;const input=document.createElement('input');input.id=id;input.type=type;input.value=value;input.oninput=()=>{equipmentDirty=true;};l.append(input);parent.append(l);return input;}

const futureConnectors={adc1:['ADC 1','Analoger Eingang – noch nicht in der Firmware verfügbar.'],adc2:['ADC 2','Analoger Eingang – noch nicht in der Firmware verfügbar.'],onewire:['1-Wire','Vorbereitet für eine spätere Firmwareversion.'],safety:['Safety','Schutzkontakt – Auswertung noch nicht implementiert.'],water:['Wasser-Schutzkontakt','Auswertung und Schutzlogik noch nicht implementiert.'],rs485:['RS485','Schnittstelle noch nicht in der Firmware verfügbar.'],uart:['UART','Schnittstelle noch nicht zur Gerätezuordnung verfügbar.'],encoder:['Drehgeber','Lokale Bedienung am Gehäuse. Keine frei zuweisbare Geräteschnittstelle.']};
function portDevice(id){
 if(/^FAN[12]$/.test(id)){
  const fan=state?.fan_ports?.[Number(id.slice(-1))-1];
  return fan?.enabled?fan:null;
 }
 return state?.equipment.find(e=>e.output===id||e.power_output===id)||null;
}
function mapPortName(id){
 const capability=state?.output_capabilities.find(c=>c.id===id);
 return capability?portLabel(capability):'PWM-Lüfter '+id.slice(-1);
}
function outputIds(){return [...orderedPorts(state).map(c=>c.id),'FAN1','FAN2'];}
function availablePorts(){return state?outputIds().filter(id=>!portDevice(id)&&!state.outputs.some(o=>o.id===id&&o.assigned)):[];}
function selectConnector(id){
 selectedConnector=id;
 el('connectorSelect').value=id;
 filterConnectorEditor();
 updateControllerMap();
}
function filterConnectorEditor(){
 for(const card of document.querySelectorAll('#equipmentEditor .port')){
  const id=card.dataset.port;
  card.hidden=selectedConnector!=='all'&&id!==selectedConnector;
 }
}
function mapAction(id){
 if(id==='display'){showView('settings');el('displayRotation').closest('details').open=true;el('displayRotation').focus();return;}
 if(id==='sensors'){showView('sensors');return;}
 if(futureConnectors[id]){el('mapInfo').textContent=futureConnectors[id].join(' · ');return;}
 if(!connected||!state)return;
 const existing=portDevice(id);
 startWizard(existing?.output||id);
}
function addPortLink(parent,id,name,description,occupied,action){
 const control=document.createElement('button'),title=document.createElement('strong'),status=document.createElement('small');
 title.textContent=name;status.textContent=description;
 control.append(title,status);control.dataset.occupied=String(occupied);control.dataset.connector=id;control.onclick=action;
 parent.append(control);
}
function updateControllerMap(){
 if(!state)return;
 const ids=outputIds(),available=new Set(availablePorts());
 const signature=JSON.stringify([ids.map(id=>[id,available.has(id),portDevice(id)?.name,portDevice(id)?.output,portDevice(id)?.mode,portDevice(id)?.plan?.enabled]),state.devices.map(d=>[d.type,d.channel])]);
 if(el('mapPorts').dataset.signature===signature)return;
 el('mapPorts').dataset.signature=signature;
 el('mapPorts').replaceChildren();el('deviceLinks').replaceChildren();
 for(const id of ids){
  const device=portDevice(id),occupied=!available.has(id);
  const description=device?'Belegt · '+device.name:occupied?'Belegt':'Frei · Gerät hinzufügen';
  const node=document.querySelector('#controllerMap [data-connector="'+id+'"]');
  node?.classList.toggle('is-occupied',occupied);
  node?.setAttribute('aria-label',mapPortName(id)+' · '+description);
  addPortLink(el('mapPorts'),id,mapPortName(id),description,occupied,()=>mapAction(id));
  if(device&&(!device.output||device.output===id)){
   const mode=device.mode==='automatic'?'Sensorgesteuert':device.plan?.enabled?'Zeitgesteuert':'Manuell';
   addPortLink(el('deviceLinks'),id,device.name,mapPortName(id)+' · '+mode,true,()=>mapAction(id));
  }
 }
 const sensorText=state.devices.length?state.devices.map(d=>d.type+' · '+(d.channel<0?'Systembus':'Kanal '+d.channel)).join(', '):'Keine Sensoren erkannt';
 document.querySelector('#controllerMap [data-connector="sensors"]').classList.toggle('is-occupied',state.devices.length>0);
 addPortLink(el('mapPorts'),'sensors','I²C-Sensoren',sensorText,false,()=>showView('sensors'));
 if(!el('deviceLinks').childElementCount){const empty=document.createElement('p');empty.className='sub';empty.textContent='Noch keine Geräte eingerichtet.';el('deviceLinks').append(empty);}
}
for(const node of document.querySelectorAll('#controllerMap [data-connector]')){
 const id=node.dataset.connector;
 if(/^(OUT|0_10V|FAN)/.test(id)){const option=document.createElement('option');option.value=id;option.textContent=node.getAttribute('aria-label');el('connectorSelect').append(option);}
 node.onclick=()=>mapAction(id);
 node.onkeydown=e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();mapAction(id);}};
}

const protectionNames={current_monitoring:'Stromüberwachung',overtemperature_monitoring:'Übertemperaturüberwachung',dry_run_detection:'Trockenlauferkennung',flow_monitoring:'Durchflussmessung',water_level_detection:'Wasserstandserkennung',tacho_feedback:'Drehzahlrückmeldung'};
function protectionSummary(p){if(!p)return 'Gerätetyp auswählen, um Schutzfunktionen und Grenzen zu sehen.';let text='Vorhanden: Ausgangssperre, Setup-/Update-Sperre und exklusive Belegung.';if(p.timed)text+=' Zeitabschaltung, höchstens '+p.max_runtime_ms/1000+' s pro Impuls; letzte 16 Ereignisse gegen Wiederholung gespeichert.';if(p.startup_lock_ms)text+=' Startsperre nach Neustart: '+p.startup_lock_ms/60000+' min.';text+=' Nicht vorhanden: '+(p.missing_protections||['current_monitoring','overtemperature_monitoring']).map(v=>protectionNames[v]||v).join(', ')+'.';return text+' Physische Abnahme offen.';}
function portLabel(c){return c.kind==='dc_switch'?c.supply_voltage+' V · Ausgang '+(c.id.endsWith('_2')?'2':'1'):'0–10 V · Ausgang '+(c.id.endsWith('_2')?'2':'1');}
function orderedPorts(d){return ['OUT12_1','OUT12_2','OUT5_1','0_10V_OUT_1','0_10V_OUT_2'].map(id=>d.output_capabilities.find(c=>c.id===id)).filter(Boolean);}
function markEquipmentDirty(){equipmentDirty=true;el('assignmentHelp').textContent='Änderungen noch nicht gespeichert. Mit „Speichern“ übernehmen.';}
function portShell(id,title,subtitle,active){const card=document.createElement('article');card.className='port';card.dataset.port=id;const heading=document.createElement('label');heading.className='port-toggle';const toggle=document.createElement('input');toggle.id='port_'+id+'_active';toggle.type='checkbox';toggle.checked=active;const text=document.createElement('span');text.textContent=title;heading.append(toggle,text);const info=document.createElement('p');info.className='sub';info.textContent=subtitle;const linked=document.createElement('p');linked.id='port_'+id+'_linked';linked.className='sub';linked.hidden=true;const fields=document.createElement('div');fields.id='port_'+id+'_fields';fields.hidden=!active;card.append(heading,info,linked,fields);el('equipmentEditor').append(card);return {toggle,fields};}
function timeText(minute){return String(Math.floor(minute/60)).padStart(2,'0')+':'+String(minute%60).padStart(2,'0');}
function minuteValue(value,end=false){if(!/^\d{2}:\d{2}$/.test(value))throw Error('Bitte eine gültige Uhrzeit eingeben.');const [h,m]=value.split(':').map(Number);if(h>23||m>59)throw Error('Ungültige Uhrzeit.');const n=h*60+m;return end&&n===0?1440:n;}
function makePlanEditor(parent,id,profile,initial){
 const box=document.createElement('fieldset'),legend=document.createElement('legend');legend.textContent='Täglicher Ablauf';box.append(legend);parent.append(box);
 const label=document.createElement('label'),enabled=document.createElement('input');enabled.type='checkbox';enabled.id=id+'_plan_enabled';enabled.style.width='auto';label.append(enabled,' Zeitplan verwenden');box.append(label);
 const content=document.createElement('div'),rows=document.createElement('div'),hint=document.createElement('p');content.id=id+'_plan_fields';rows.id=id+'_plan_slots';hint.className='sub';content.append(hint,rows);box.append(content);
 let activeProfile=profile;
 const pattern=selectField(content,id+'_pattern','Zeitsteuerung',[['windows','Tägliche Zeiten'],['interval','Wiederkehrendes Intervall']],initial?.interval?'interval':'windows');
 const interval=document.createElement('div');content.insertBefore(pattern.parentElement,rows);content.insertBefore(interval,rows);
 const every=inputField(interval,id+'_every','Alle / Minuten',initial?.interval?.every_minutes??180,'number');every.min='1';every.max='1440';
 const anchor=inputField(interval,id+'_anchor','Täglicher Beginn',timeText(initial?.interval?.anchor_minute??0),'time');
 const duration=inputField(interval,id+'_interval_duration','Laufzeit',initial?.interval?(profile?.timed?initial.interval.runtime_ms/1000:initial.interval.duration_minutes):profile?.timed?1:10,'number');
 const percent=inputField(interval,id+'_interval_percent','Leistung / %',initial?.interval?.percent??100,'number');percent.min='1';percent.max='100';
 for(const input of [every,anchor,duration,percent])input.oninput=markEquipmentDirty;
 function patternView(){const pulse=!!activeProfile?.timed;interval.hidden=pattern.value!=='interval';rows.hidden=pattern.value==='interval';add.hidden=pattern.value==='interval';duration.parentElement.firstChild.textContent=pulse?'Laufzeit / Sekunden':'Laufzeit / Minuten';duration.min=pulse?'.001':'1';duration.max=pulse?activeProfile.max_runtime_ms/1000:1440;duration.step=pulse?'.001':'1';percent.parentElement.hidden=pulse||!['pwm','0_10v'].includes(activeProfile?.connection);}
 pattern.onchange=()=>{patternView();markEquipmentDirty();};
 const catchUp=selectField(content,id+'_catchup','Verpasster Bewässerungstermin',[['0','Nicht nachholen'],['5','Einmal bis 5 Minuten später'],['15','Einmal bis 15 Minuten später'],['30','Einmal bis 30 Minuten später'],['60','Einmal bis 60 Minuten später']],0);catchUp.onchange=markEquipmentDirty;
 const add=button(content,'Zeit hinzufügen',()=>{addSlot({});markEquipmentDirty();});add.id=id+'_plan_add';
 function addSlot(slot){if(rows.children.length>=8)return;const pulse=!!activeProfile?.timed,row=document.createElement('div');row.className='plan-slot';row.style.cssText='display:flex;gap:10px;flex-wrap:wrap;align-items:end;margin:12px 0';
  const previous=rows.lastElementChild?.querySelector('.plan-end')?.value;const begin=slot.start_minute??(previous?(minuteValue(previous,true)+60)%1440:480);
  const start=inputField(row,'','Von',timeText(begin),'time');start.className='plan-start';start.oninput=markEquipmentDirty;
  if(pulse){const duration=inputField(row,'','Dauer / Sekunden',(slot.runtime_ms??Math.min(1000,activeProfile.max_runtime_ms))/1000,'number');duration.className='plan-duration';duration.min='.001';duration.max=activeProfile.max_runtime_ms/1000;duration.step='.001';duration.oninput=markEquipmentDirty;}
  else {const end=inputField(row,'','Bis',timeText((slot.end_minute??Math.min(begin+120,1440))%1440),'time');end.className='plan-end';end.oninput=markEquipmentDirty;if((activeProfile?.connection==='0_10v'||activeProfile?.connection==='pwm')){const value=inputField(row,'','Leistung / %',slot.percent??100,'number');value.className='plan-percent';value.min='1';value.max='100';value.oninput=markEquipmentDirty;}}
  button(row,'Entfernen',()=>{row.remove();add.disabled=rows.children.length>=8;markEquipmentDirty();});rows.append(row);add.disabled=rows.children.length>=8;
 }
 function render(p,plan={}){activeProfile=p;box.hidden=!p;catchUp.parentElement.hidden=!p?.timed;catchUp.value=String(plan.catch_up_minutes||0);enabled.checked=!!plan.enabled;content.hidden=!enabled.checked;rows.replaceChildren();for(const slot of plan.slots||[])addSlot(slot);hint.textContent=p?.timed?'Mitteleuropäische Ortszeit: täglich zu diesen Uhrzeiten ein kurzer Impuls. Die Nachholregel ist wählbar. Es wird höchstens der letzte Termin nachgeholt, nie ein bereits gestarteter Impuls. Pumpen haben nach Neustart 10 Minuten Startsperre.':'Mitteleuropäische Ortszeit: täglich in diesen Zeiten an, dazwischen aus. Mehrere Zeitfenster und Zeiten über Mitternacht sind möglich. 00:00 als Ende bedeutet Mitternacht.';add.textContent=p?.timed?'Bewässerungszeit hinzufügen':'Zeitfenster hinzufügen';add.disabled=rows.children.length>=8;patternView();}
 enabled.onchange=()=>{content.hidden=!enabled.checked;if(enabled.checked&&!rows.children.length)addSlot({});markEquipmentDirty();};
 render(profile,initial);box.renderPlan=render;return box;
}
function readPlan(id,p){const enabled=el(id+'_plan_enabled').checked,slots=[];
 if(el(id+'_pattern')?.value==='interval'){
  const interval={every_minutes:Number(el(id+'_every').value),anchor_minute:minuteValue(el(id+'_anchor').value)};
  if(!Number.isInteger(interval.every_minutes)||interval.every_minutes<1||interval.every_minutes>1440)throw Error('Intervall muss 1 bis 1440 Minuten betragen.');
  if(p.timed){interval.runtime_ms=Math.round(Number(el(id+'_interval_duration').value)*1000);if(!Number.isInteger(interval.runtime_ms)||interval.runtime_ms<1||interval.runtime_ms>p.max_runtime_ms)throw Error('Impulsdauer außerhalb der Schutzgrenze.');}
  else{interval.duration_minutes=Number(el(id+'_interval_duration').value);interval.percent=['pwm','0_10v'].includes(p.connection)?Number(el(id+'_interval_percent').value):100;if(!Number.isInteger(interval.duration_minutes)||interval.duration_minutes<1||interval.duration_minutes>interval.every_minutes||!Number.isInteger(interval.percent)||interval.percent<1||interval.percent>100)throw Error('Intervall-Laufzeit oder Leistung ungültig.');}
  return {enabled,slots:[],interval,...(p.timed?{catch_up_minutes:Number(el(id+'_catchup').value)}:{})};
 }
 for(const row of el(id+'_plan_slots').children){const start=minuteValue(row.querySelector('.plan-start').value);const item={start_minute:start};if(p.timed)item.runtime_ms=Math.round(Number(row.querySelector('.plan-duration').value)*1000);else{item.end_minute=minuteValue(row.querySelector('.plan-end').value,true);item.percent=Number(row.querySelector('.plan-percent')?.value??100);}slots.push(item);}
 if(enabled&&!slots.length)throw Error('Bitte mindestens eine Zeit hinzufügen.');
 for(let i=0;i<slots.length;i++){const a=slots[i];if(p.timed){if(!Number.isInteger(a.runtime_ms)||a.runtime_ms<1||a.runtime_ms>p.max_runtime_ms)throw Error('Impulsdauer außerhalb der Schutzgrenze.');if(slots.slice(0,i).some(b=>a.start_minute===b.start_minute))throw Error('Diese Bewässerungszeit ist doppelt.');}
 else{if(a.start_minute===a.end_minute||!Number.isInteger(a.percent)||a.percent<1||a.percent>100)throw Error('Zeitfenster oder Leistung ungültig.');const inside=(s,m)=>s.start_minute<s.end_minute?m>=s.start_minute&&m<s.end_minute:m>=s.start_minute||m<s.end_minute;for(const b of slots.slice(0,i))for(let m=0;m<1440;m++)if(inside(a,m)&&inside(b,m))throw Error('Zeitfenster dürfen sich nicht überschneiden.');}}
 return {enabled,slots,...(p.timed?{catch_up_minutes:Number(el(id+'_catchup').value)}:{})};
}
let operationEditors={};
function sensorOptions(){const all=[];for(const d of (state?.devices||[]).filter(d=>d.source))for(const v of d.signals||[]){const key=d.source+'|'+v.id;if(!all.some(x=>x[0]===key))all.push([key,(signalNames[v.id]||v.id)+' ['+v.unit+'] · '+d.type+' · '+(d.channel<0?'Systembus':'I²C '+d.channel)+' · 0x'+d.address.toString(16)+(v.valid?'':' · kein gültiger Wert')]);}return all;}
function refreshSensorOptions(){const available=sensorOptions();for(const select of document.querySelectorAll('.rule-source')){const selected=select.value,options=[...available];if(selected&&!options.some(o=>o[0]===selected)){const [source,signal]=selected.split('|');options.push([selected,'Nicht verbunden · '+source+' · '+(signalNames[signal]||signal)]);}if(!selected)options.unshift(['','Bitte Messwert auswählen']);const signature=JSON.stringify(options);if(select.dataset.options===signature)continue;select.dataset.options=signature;select.replaceChildren();for(const [value,label] of options){const option=document.createElement('option');option.value=value;option.textContent=label;select.append(option);}select.value=selected;}}
function makeSensorEditor(parent,id,p,initial={}){
 const box=document.createElement('fieldset');box.id=id+'_sensor_fields';parent.append(box);const legend=document.createElement('legend');legend.textContent='Sensorregelung';box.append(legend);
 const hint=document.createElement('p');hint.className='sub';hint.textContent='Zielwert: normale Regelung. Minimum/Maximum: Warnung und optional eine besondere Leistung. Oben stehende Grenzwertaktionen haben Vorrang. Bis zu sechs Messwerte. Normale Anforderungen nutzen die höchste benötigte Leistung. Kurze Messaussetzer werden mit dem letzten gültigen Wert überbrückt. Ab 10 Sekunden ohne gültigen Messwert folgen Warnung und Ersatzverhalten.';box.append(hint);
 const analog=['0_10v','pwm'].includes(p?.connection),pulse=!!p?.timed;
 const fallback=numberField(box,id+'_fallback','Bei fehlendem Messwert: Leistung / %',initial.fallback_percent||0,0,pulse?0:100,analog?1:100);if(pulse){fallback.parentElement.hidden=true;const note=document.createElement('p');note.className='notice';note.textContent='Bei fehlendem Sensor keine Bewässerung. Jede Anforderung löst nur einen begrenzten Impuls aus; danach gilt die gespeicherte Mindestpause. Besteht die Anforderung weiterhin, folgt nach der Pause ein neuer Impuls.';box.append(note);}
 const pulseDuration=numberField(box,id+'_sensor_pulse','Impuls / Sekunden',(initial.pulse_ms||Math.min(p?.max_runtime_ms||1000,1000))/1000,.001,(p?.max_runtime_ms||10000)/1000,.001);pulseDuration.parentElement.hidden=!pulse;
 const cooldown=numberField(box,id+'_sensor_cooldown','Mindestpause nach Lauf / Minuten',(initial.cooldown_s||3600)/60,1,1440,1);cooldown.parentElement.hidden=!pulse;
 const rows=document.createElement('div');rows.id=id+'_sensor_rules';box.append(rows);
 function addRule(rule={}){if(rows.children.length>=6)return;const row=document.createElement('fieldset');row.className='sensor-rule';const order=document.createElement('legend');order.textContent='Regel';row.append(order);
  const options=sensorOptions(),selected=rule.source?rule.source+'|'+rule.signal:options[0]?.[0]||'';if(selected&&!options.some(o=>o[0]===selected))options.push([selected,'Nicht verbunden · '+rule.source+' · '+(signalNames[rule.signal]||rule.signal)]);if(!options.length)options.push(['','Noch keine Sensoren erkannt']);
  const source=selectField(row,'','Messwert / Sensor',options,selected);source.className='rule-source';
  const direction=selectField(row,'','Dieses Gerät soll den Messwert',[['lower','Senken'],['raise','Erhöhen']],rule.direction||'lower');direction.className='rule-direction';
  const defaults=(selected.endsWith('humidity_pct')?[65,10,2]:selected.endsWith('co2_ppm')?[800,400,50]:selected.endsWith('lux')?[10000,5000,500]:[24,4,.5]);
  function field(cls,label,value,step='.1'){const i=inputField(row,'',label,value,'number');i.className=cls;i.step=step;i.oninput=markEquipmentDirty;return i;}
  field('rule-target','Zielwert',rule.target??defaults[0]);
  const limits=document.createElement('details'),limitsTitle=document.createElement('summary');
  limitsTitle.textContent='Grenzwerte und Aktionen';limits.className='rule-limits';limits.open=!wizard;
  limits.append(limitsTitle);row.append(limits);
  const minimum=field('rule-min','Minimum (optional)',rule.minimum??'');
  const maximum=field('rule-max','Maximum (optional)',rule.maximum??'');
  const low=field('rule-below','Unter Minimum: Leistung / % (leer = nur Warnung)',rule.below_percent??'',analog?'1':'100');const high=field('rule-above','Über Maximum: Leistung / % (leer = nur Warnung)',rule.above_percent??'',analog?'1':'100');low.min=high.min='0';low.max=high.max='100';limits.append(minimum.parentElement,maximum.parentElement,low.parentElement,high.parentElement);
  const more=document.createElement('details'),summary=document.createElement('summary');summary.textContent='Regelverhalten';more.append(summary);row.append(more);
  const band=field('rule-band','Regelbereich ab Zielwert',rule.band??defaults[1]);const hys=field('rule-hysteresis','Hysterese',rule.hysteresis??defaults[2]);const delay=field('rule-delay','Verzögerung / Sekunden',rule.delay_s??5,'1');more.append(band.parentElement,hys.parentElement,delay.parentElement);
  source.onchange=()=>markEquipmentDirty();direction.onchange=markEquipmentDirty;
  const actions=document.createElement('div');actions.className='actions';button(actions,'Höher priorisieren',()=>{if(row.previousElementSibling)rows.insertBefore(row,row.previousElementSibling);renumber();markEquipmentDirty();});button(actions,'Niedriger priorisieren',()=>{if(row.nextElementSibling)rows.insertBefore(row.nextElementSibling,row);renumber();markEquipmentDirty();});button(actions,'Regel entfernen',()=>{row.remove();renumber();markEquipmentDirty();});row.append(actions);rows.append(row);renumber();
 }
 function renumber(){[...rows.children].forEach((r,i)=>r.querySelector('legend').textContent='Priorität '+(i+1));add.disabled=rows.children.length>=6;}
 const add=button(box,'Messwert hinzufügen',()=>{addRule();markEquipmentDirty();});add.id=id+'_sensor_add';for(const r of initial.rules||[])addRule(r);
 for(const input of [fallback,pulseDuration,cooldown])input.oninput=markEquipmentDirty;
 return {box,read(){const config={fallback_percent:pulse?0:Number(fallback.value),rules:[]};if(!Number.isInteger(config.fallback_percent)||config.fallback_percent<0||config.fallback_percent>100||(!analog&&![0,100].includes(config.fallback_percent)))throw Error('Ersatzleistung passt nicht zum Ausgang.');
  if(pulse){config.pulse_ms=Math.round(Number(pulseDuration.value)*1000);config.cooldown_s=Number(cooldown.value)*60;if(!Number.isInteger(config.pulse_ms)||config.pulse_ms<1||config.pulse_ms>p.max_runtime_ms||!Number.isInteger(config.cooldown_s)||config.cooldown_s<60||config.cooldown_s>86400)throw Error('Impulsdauer oder Mindestpause ungültig.');}
  for(const row of rows.children){const get=c=>row.querySelector('.rule-'+c).value,key=get('source').split('|');if(key.length!==2||!key[0])throw Error('Bitte einen erkannten Messwert auswählen.');const r={source:key[0],signal:key[1],direction:get('direction'),target:Number(get('target')),band:Number(get('band')),hysteresis:Number(get('hysteresis')),delay_s:Number(get('delay'))};
   if(get('target')===''||!Number.isFinite(r.target)||!Number.isFinite(r.band)||r.band<=0||!Number.isFinite(r.hysteresis)||r.hysteresis<0||r.hysteresis>r.band||!Number.isInteger(r.delay_s)||r.delay_s<0||r.delay_s>300)throw Error('Zielwert oder Regelverhalten ungültig.');
   for(const [field,key] of [['min','minimum'],['max','maximum'],['below','below_percent'],['above','above_percent']])if(get(field)!==''){r[key]=Number(get(field));if(!Number.isFinite(r[key]))throw Error('Ungültiger Grenzwert.');}
   if((r.minimum!==undefined&&r.minimum>=r.target)||(r.maximum!==undefined&&r.maximum<=r.target)||(r.minimum!==undefined&&r.maximum!==undefined&&r.hysteresis>=r.maximum-r.minimum))throw Error('Minimum muss unter und Maximum über dem Zielwert liegen.');
   for(const [action,limit] of [['below_percent','minimum'],['above_percent','maximum']])if(r[action]!==undefined&&(r[limit]===undefined||!Number.isInteger(r[action])||r[action]<0||r[action]>100||(!analog&&![0,100].includes(r[action]))))throw Error('Grenzwertaktion passt nicht zum Gerät. Schaltausgänge und Impulse unterstützen 0 oder 100 %.');
   config.rules.push(r);
  }
  if(!config.rules.length)throw Error('Bitte mindestens einen Messwert für die Sensorregelung hinzufügen.');return config;
 }};
}
function makeOperationEditor(parent,id,p,old={}){
 const fan=p?.id?.startsWith('fan_')||p?.connection==='pwm';
 const initial=old.mode==='automatic'?'automatic':old.plan?.enabled?'scheduled':'manual';
 const role=selectField(parent,id+'_role','Lüfterfunktion',[['circulation','Umluft'],['intake','Zuluft'],['exhaust','Abluft']],old.fan_role||'circulation');role.parentElement.hidden=!fan;role.onchange=markEquipmentDirty;
 const mode=selectField(parent,id+'_mode','Betriebsart',[['manual','Manuell'],['scheduled','Zeitgesteuert'],['automatic','Sensorgesteuert']],initial);
 mode.parentElement.classList.add('mode-field');role.parentElement.classList.add('operation-details');
 const modeHint=document.createElement('p');modeHint.className='sub mode-hint';mode.parentElement.after(modeHint);
 const modeHelp={manual:'Über die Gerätekarte bedienen.',scheduled:'Nach Uhrzeit oder in einem wiederkehrenden Intervall steuern.',automatic:'Auf eine ausgewählte Messgröße reagieren.'};
 const plan=makePlanEditor(parent,id,p,old.plan||{});plan.classList.add('operation-details');el(id+'_plan_enabled').parentElement.hidden=true;
 const sensors=makeSensorEditor(parent,id,p,old.automation||{});sensors.box.classList.add('operation-details');
 function update(){modeHint.textContent=modeHelp[mode.value];plan.hidden=mode.value!=='scheduled'||!p;sensors.box.hidden=mode.value!=='automatic'||!p;const enabled=el(id+'_plan_enabled');if(mode.value==='scheduled'&&!enabled.checked){enabled.checked=true;enabled.onchange();}else if(mode.value!=='scheduled')enabled.checked=false;}
 mode.onchange=()=>{update();markEquipmentDirty();};update();
 return {read(){const selected=mode.value;return {mode:selected,...(fan?{fan_role:role.value}:{}),plan:selected==='scheduled'?readPlan(id,p):{...(old.plan||{slots:[]}),enabled:false},...(selected==='automatic'?{automation:sensors.read()}:old.automation?{automation:old.automation}:{})};}};
}
function buildEquipmentEditor(d){if(!d)return;operationEditors={};el('equipmentEditor').replaceChildren();const group=document.createElement('fieldset');group.id='portEditorFields';el('equipmentEditor').append(group);
 for(const c of orderedPorts(d)){
  const old=d.equipment.find(v=>v.output===c.id)||{},id='port_'+c.id,{toggle,fields}=portShell(c.id,portLabel(c),c.id+(c.kind==='dc_switch'?' · feste Versorgungsspannung':' · Steuersignal, externe Versorgung'),!!old.profile);group.append(fields.parentElement);
  const compatible=d.device_catalog.filter(p=>p.connection===c.kind),profile=selectField(fields,id+'_profile','Angeschlossenes Gerät',[['','Bitte Gerätetyp auswählen'],...compatible.map(p=>[p.id,({light_switch:'Licht',light_0_10v:'Licht (dimmbar)',pump:'Pumpe',valve_pulse:'Ventil (Impuls)',fan_switch:'Lüfter',fan_0_10v:'Lüfter (0–10 V)'})[p.id]||p.label])],old.profile||'');
  const name=inputField(fields,id+'_name','Name (frei wählbar)',old.name||'');name.maxLength=39;
  profile.parentElement.classList.add('identity-field');name.parentElement.classList.add('identity-field');
  if(old.power_output)profile.disabled=true;
  const runtime=inputField(fields,id+'_runtime','Maximale Laufzeit / Sekunden',(old.max_runtime_ms||10000)/1000,'number');runtime.min='.001';runtime.max='10';runtime.step='.001';runtime.parentElement.classList.add('identity-field');
  const operation=document.createElement('div');operation.className='operation';fields.append(operation);const protection=document.createElement('p');protection.id=id+'_protection';protection.className='notice protection';fields.append(protection);
  function selected(){return compatible.find(p=>p.id===profile.value);}
  function controls(value){operation.replaceChildren();operationEditors[id]=makeOperationEditor(operation,id,selected(),value);}
  controls(old);
  function update(){const p=selected();fields.hidden=!toggle.checked;runtime.parentElement.hidden=!p?.timed;runtime.max=(p?.max_runtime_ms||10000)/1000;protection.textContent=protectionSummary(p);refreshPortUse();}
  toggle.onchange=()=>{if(toggle.checked&&!name.value)name.value=selected()?.label||'';update();markEquipmentDirty();};
  profile.onchange=()=>{const p=selected();if(!name.value)name.value=profile.selectedOptions[0]?.textContent||'';const light=p?.id.startsWith('light_');controls({plan:{enabled:!!light,slots:light?[{start_minute:480,end_minute:1200,percent:100}]:[]}});update();markEquipmentDirty();};
  name.oninput=runtime.oninput=markEquipmentDirty;update();
 }
 for(let n=1;n<=2;n++){const old=d.fan_ports?.[n-1]||{enabled:false,name:'Lüfter '+n},id='port_FAN'+n,{toggle,fields}=portShell('FAN'+n,'PWM-Lüfter · Anschluss '+n,'Fester Lüfteranschluss mit Drehzahlrückmeldung',old.enabled);group.append(fields.parentElement);const name=inputField(fields,id+'_name','Name',old.name);name.maxLength=39;name.parentElement.classList.add('identity-field');name.oninput=markEquipmentDirty;const operation=document.createElement('div');operation.className='operation';fields.append(operation);operationEditors[id]=makeOperationEditor(operation,id,{id:'fan_pwm',connection:'pwm',timed:false},old);toggle.onchange=()=>{fields.hidden=!toggle.checked;markEquipmentDirty();};}
 equipmentRevision=d.equipment_revision;equipmentDirty=false;el('assignmentHelp').textContent='Speichern pausiert alle Ausgänge. Starte den Betrieb anschließend bei Bedarf wieder.';refreshPortUse();gateControls();filterConnectorEditor();}
function refreshPortUse(){
 if(!state)return;
 const claims=new Map();
 for(const e of state.equipment){
  if(e.power_output&&el('port_'+e.output+'_active')?.checked)claims.set(e.power_output,e);
 }
 for(const c of orderedPorts(state).filter(c=>c.kind==='dc_switch')){
  const id='port_'+c.id,active=el(id+'_active');
  if(!active)continue;
  const owner=claims.get(c.id),info=el(id+'_linked');
  active.disabled=!!owner;
  if(owner){info.textContent='Teil der vorhandenen Belegung für '+owner.name+'.';}
  info.hidden=!owner;
  el(id+'_fields').hidden=!!owner||!active.checked;
 }
}
function equipmentPayload(){
 const active=orderedPorts(state).filter(c=>el('port_'+c.id+'_active').checked&&!el('port_'+c.id+'_active').disabled),kept=state.equipment.filter(e=>active.some(c=>c.id===e.output)),used=new Set(kept.map(e=>e.id)),free=['dev1','dev2','dev3','dev4','dev5'].filter(id=>!used.has(id)),equipment=[],fan_ports=[];
 for(const c of active){const id='port_'+c.id,profile=state.device_catalog.find(p=>p.id===el(id+'_profile').value);if(!profile)throw Error('Bitte für '+portLabel(c)+' einen Gerätetyp auswählen.');const name=el(id+'_name').value;if(!name.trim()||new TextEncoder().encode(name).length>39)throw Error('Bitte einen gültigen, kürzeren Gerätenamen eingeben.');
   const row={id:kept.find(e=>e.output===c.id)?.id||free.shift(),name,profile:profile.id,output:c.id,...operationEditors[id].read()};if(c.kind==='dc_switch')row.voltage=c.supply_voltage;
   if(profile.timed){row.max_runtime_ms=Math.round(Number(el(id+'_runtime').value)*1000);if(!Number.isInteger(row.max_runtime_ms)||row.max_runtime_ms<1||row.max_runtime_ms>profile.max_runtime_ms)throw Error('Maximale Laufzeit liegt außerhalb der Schutzgrenze.');if(!row.plan.enabled){row.plan=JSON.parse(JSON.stringify(row.plan));for(const slot of row.plan.slots)slot.runtime_ms=Math.min(slot.runtime_ms,row.max_runtime_ms);if(row.plan.interval)row.plan.interval.runtime_ms=Math.min(row.plan.interval.runtime_ms,row.max_runtime_ms);}if(row.mode!=='automatic'&&row.automation?.pulse_ms>row.max_runtime_ms)row.automation={...row.automation,pulse_ms:row.max_runtime_ms};if(row.plan.slots.some(s=>s.runtime_ms>row.max_runtime_ms)||row.plan.interval?.runtime_ms>row.max_runtime_ms||row.automation?.pulse_ms>row.max_runtime_ms)throw Error('Impuls überschreitet die maximale Laufzeit.');}
    const existing=kept.find(e=>e.output===c.id);
   if(existing?.power_output){row.power_output=existing.power_output;row.power_voltage=existing.power_voltage;}
   equipment.push(row);
  }
  for(let n=1;n<=2;n++){const id='port_FAN'+n,enabled=el(id+'_active').checked,name=el(id+'_name').value||'Lüfter '+n;if(new TextEncoder().encode(name).length>39||!name.trim())throw Error('Lüftername ist ungültig.');fan_ports.push({enabled,name,...(enabled?operationEditors[id].read():{mode:'manual',fan_role:el(id+'_role').value,plan:{enabled:false,slots:[]}})});}
 const payload={cmd:'configure_equipment',expected_revision:equipmentRevision,equipment,fan_ports,start_plans:false};
 if(new TextEncoder().encode(JSON.stringify(payload)).length>12288)throw Error('Konfiguration zu groß. Bitte weniger Regeln oder Zeiten verwenden.');
 return payload;
}
async function saveEquipment(){
 if(!connected||!state?.settings_ok||busy||pendingEquipment||equipmentSaving||equipmentRevision!==state.equipment_revision)return;
 let payload;
 try{payload=equipmentPayload();}catch(e){el(wizard?'wizardError':'message').textContent=e.message;return;}
 equipmentSaving=true;gateControls();
 try{
  if(!state.rest_mode.enabled&&!await send({cmd:'set_rest_mode',enabled:true}))return;
  pendingEquipment={expected:payload.expected_revision,uncertain:false,wizard:!!wizard};
  const saved=await send(payload);
  if(saved){if(connected)settleEquipment(state);}
  else if(lastCommandUncertain)pendingEquipment.uncertain=true;
  else{pendingEquipment=null;await refresh();}
 }finally{equipmentSaving=false;gateControls();}
}

const wizardLabels={
 port:'Anschluss wählen',identity:'Gerät benennen',mode:'Steuerung wählen',
 details:'Steuerung einstellen',review:'Prüfen und speichern'
};
function wizardSteps(){
 const id=wizard.id;
 if(!id)return ['port'];
 const start=wizard.entry==='choose'?['port','identity']:wizard.entry==='quick'?[]:['identity'];
 const mode=el('port_'+id+'_mode')?.value;
 const needsDetails=mode!=='manual'||!el('port_'+id+'_role').parentElement.hidden;
 return [...start,'mode',...(needsDetails?['details']:[]),'review'];
}
function openBulk(){
 if(!state||equipmentSaving||pendingEquipment||wizard)return;
 bulkOpen=!bulkOpen;
 el('bulkEquipment').textContent=bulkOpen?'Anschlussbearbeitung schließen':'Alle Anschlüsse bearbeiten';
 el('bulkSelect').hidden=!bulkOpen;
 el('assignmentCard').hidden=!bulkOpen;
 if(bulkOpen)selectConnector('all');
}
function startWizard(id){
 if(!connected||!state?.settings_ok||busy||pendingEquipment||equipmentSaving)return;
 const existing=id&&portDevice(id);
 if((id&&!existing&&!availablePorts().includes(id))||(!id&&!availablePorts().length))return;
 if(wizard||equipmentDirty){if(!confirm('Ungespeicherten Entwurf verwerfen?'))return;}
 if(wizard)closeWizard();
 if(bulkOpen)openBulk();
 buildEquipmentEditor(state);
 showView('manage');
 wizard={id:null,step:'port',entry:id?'direct':'choose',editing:!!existing,removing:false};
 if(id)chooseWizardPort(id);
 else {equipmentDirty=true;renderWizard();}
}
function chooseWizardPort(id){
 if(!wizard||(!wizard.editing&&!availablePorts().includes(id)))return;
 if(wizard.id!==id){
  if(wizard.id)el('port_'+wizard.id+'_active').checked=false;
  wizard.id=id;
  const toggle=el('port_'+id+'_active');
  if(!toggle.checked){toggle.checked=true;toggle.onchange();}
 }
 if(wizard.entry==='direct'&&!wizard.editing&&/^FAN[12]$/.test(id))wizard.entry='quick';
 wizard.step=wizard.entry==='quick'?'mode':'identity';
 equipmentDirty=true;
 renderWizard();
}
function closeWizard(){
 wizard=null;delete document.body.dataset.wizardStep;delete document.body.dataset.wizardQuick;
 for(const id of ['wizardHeader','wizardProgress','wizardPorts','wizardReview','wizardActions','assignmentCard'])el(id).hidden=true;
 el('wizardError').textContent='';el('wizardIdentityHint').textContent='';
}
function cancelWizard(focus=true){
 if(!wizard||pendingEquipment||equipmentSaving)return;
 closeWizard();buildEquipmentEditor(state);
 if(focus)el('view-manage').querySelector('[data-add-device]:enabled, .device-links button')?.focus();
}
function renderWizard(){
 const {id,step,editing,removing}=wizard,steps=wizardSteps();
 const position=steps.indexOf(step)+1;
 document.body.dataset.wizardStep=step;document.body.dataset.wizardQuick=String(wizard.entry==='quick');
 if(step==='details'){
  for(const limits of el('port_'+id+'_fields').querySelectorAll('.rule-limits:not([data-wizard-ready])')){
   limits.open=false;limits.dataset.wizardReady='1';
  }
 }
 selectConnector(id||'all');
 for(const key of ['assignmentCard','wizardHeader','wizardProgress','wizardActions'])el(key).hidden=false;
 el('assignment').open=true;
 el('wizardTitle').textContent=removing?'Gerät entfernen':editing?'Gerät bearbeiten':'Gerät hinzufügen';
 el('wizardCaption').textContent=(id?mapPortName(id)+' · ':'')+wizardLabels[step]+(position>0?' · Schritt '+position+(steps.length>1?' von '+steps.length:''):'');
 const progress=el('wizardProgress');progress.replaceChildren();
 for(let i=0;i<steps.length;i++){const bar=document.createElement('span');if(i<position)bar.className='done';progress.append(bar);}
 const choices=el('wizardPorts');choices.hidden=step!=='port';choices.replaceChildren();
 if(!choices.hidden){
  for(const port of availablePorts()){
   const c=state.output_capabilities.find(c=>c.id===port);
   const purpose=!c?'Versorgung und Drehzahlsteuerung':c.kind==='dc_switch'?'Geschaltete '+c.supply_voltage+'-V-Versorgung':'Steuersignal · Gerät separat versorgt';
   addPortLink(choices,port,mapPortName(port),purpose,false,()=>chooseWizardPort(port));
  }
  if(!choices.childElementCount)choices.textContent='Keine freien Anschlüsse verfügbar.';
 }
 const existing=id?portDevice(id):null;
 el('wizardIdentityHint').textContent=existing?.power_output
  ?'Die vorhandene Belegung verwendet zusätzlich '+mapPortName(existing.power_output)+'. Neue Geräte werden mit einem Anschluss eingerichtet.'
  :id?.startsWith('0_10V')?'0–10 V liefert das Steuersignal. Das Gerät wird separat versorgt.':'';
 el('wizardBack').hidden=position<=1||removing;
 el('wizardNext').hidden=step==='port';
 el('wizardNext').textContent=step==='review'?'Pausiert speichern':'Weiter';
 el('wizardRemove').hidden=!editing||step!=='identity';
 el('wizardReview').hidden=step!=='review';
 el('wizardError').textContent='';
 gateControls();
 const card=el('assignmentCard');
 card.scrollIntoView({block:'nearest'});
 const focus=step==='port'?choices.querySelector('button'):step==='review'?el('wizardNext'):
  [...card.querySelectorAll('input,select,button')].find(node=>node!==el('wizardNext')&&!node.disabled&&node.getClientRects().length&&node.closest('.port'));
 focus?.focus({preventScroll:true});
}
function describeWizard(payload){
 const id=wizard.id,review=el('wizardReview'),isFan=/^FAN[12]$/.test(id);
 review.replaceChildren();
 const p=document.createElement('p');p.className='notice';
 p.textContent=(state.rest_mode.enabled?'Die Ausgänge bleiben nach dem Speichern pausiert.':'Beim Speichern werden alle Ausgänge pausiert. Danach kannst du den Betrieb separat freigeben.');
 if(wizard.removing){
  const removed=document.createElement('p');removed.textContent=(portDevice(id)?.name||mapPortName(id))+' wird aus der Belegung entfernt.';
  review.append(removed,p);return;
 }
 const device=isFan?payload.fan_ports[Number(id.slice(-1))-1]:payload.equipment.find(e=>e.output===id);
 const profile=isFan?{connection:'pwm',label:'PWM-Lüfter'}:state.device_catalog.find(p=>p.id===device.profile);
 const fields=[['Name',device.name],['Anschluss',mapPortName(id)],['Gerätetyp',profile.label]];
 if(device.plan.enabled)fields.push(['Zeitplan',planText(device,profile)]);
 else if(device.mode==='automatic'){
  for(const [index,rule] of device.automation.rules.entries()){
   const sensor=rule.source;
   const signal=signalNames[rule.signal]||rule.signal;
   const direction=rule.direction==='raise'?'Unterhalb des Zielwerts':'Oberhalb des Zielwerts';
   fields.push(['Regel '+(index+1),signal+' · '+sensor+' · Ziel '+rule.target+' · '+direction]);
  }
  fields.push(['Bei Sensorausfall',profile.timed?'Kein Impuls':device.automation.fallback_percent+' %']);
 }else fields.push(['Steuerung','Manuell']);
 if(profile.timed)fields.push(['Maximale Laufzeit',device.max_runtime_ms/1000+' s']);
 const list=document.createElement('dl');
 for(const [label,value] of fields){const term=document.createElement('dt'),description=document.createElement('dd');term.textContent=label;description.textContent=value;list.append(term,description);}
 review.append(list,p);
}
function advanceWizard(delta){
 if(!wizard||busy||pendingEquipment||equipmentSaving||(delta>0&&el('wizardNext').disabled))return;
 const steps=wizardSteps(),index=steps.indexOf(wizard.step);
 if(delta<0){wizard.step=steps[index-1];renderWizard();return;}
 if(wizard.step==='review'){saveEquipment();return;}
 const id='port_'+wizard.id;
 if(wizard.step==='identity'||(wizard.step==='mode'&&wizard.entry==='quick')){
  const name=el(id+'_name').value,profile=el(id+'_profile');
  if((profile&&!profile.value)||!name.trim()||new TextEncoder().encode(name).length>39){
   el('wizardError').textContent='Wähle einen Gerätetyp und gib einen gültigen Namen ein.';return;
  }
 }
 const next=steps[index+1];
 let payload;
 if(next==='review'){
  try{payload=equipmentPayload();}
  catch(e){el('wizardError').textContent=e.message;return;}
 }
 wizard.step=next;
 renderWizard();
 if(payload)describeWizard(payload);
}
function removeWizardDevice(){
 if(!wizard?.editing||pendingEquipment||equipmentSaving)return;
 wizard.removing=true;
 const toggle=el('port_'+wizard.id+'_active');
 toggle.checked=false;toggle.onchange();
 wizard.step='review';renderWizard();
 try{describeWizard(equipmentPayload());}
 catch(e){el('wizardError').textContent=e.message;el('wizardNext').disabled=true;}
}

function eventId(){const a=new Uint32Array(4);crypto.getRandomValues(a);return Array.from(a,v=>v.toString(16).padStart(8,'0')).join('');}
function nextPlanAction(d,e,p){
 if(!d.time_synced||!d.epoch_s)return 'Warte auf gültige Uhrzeit.';
 const parts=Object.fromEntries(new Intl.DateTimeFormat('en-GB',{timeZone:d.schedule_timezone||'Europe/Berlin',year:'numeric',month:'2-digit',day:'2-digit',hour:'2-digit',minute:'2-digit',hourCycle:'h23'}).formatToParts(new Date(d.epoch_s*1000)).filter(v=>v.type!=='literal').map(v=>[v.type,Number(v.value)]));
 const minute=parts.hour*60+parts.minute,slots=[...(e.plan.slots||[])],changes=[];
 if(e.plan.interval){const i=e.plan.interval;for(let start=i.anchor_minute;start<1440;start+=i.every_minutes)slots.push(p.timed?{start_minute:start,runtime_ms:i.runtime_ms}:{start_minute:start,end_minute:(start+i.duration_minutes)%1440,percent:i.percent});}
 if(p.timed){for(const slot of slots)changes.push({minute:slot.start_minute,label:'Impuls · '+slot.runtime_ms/1000+' s'});}
 else{
  const at=m=>{for(const slot of slots){if(slot.start_minute<slot.end_minute?m>=slot.start_minute&&m<slot.end_minute:m>=slot.start_minute||m<slot.end_minute)return slot.percent;}return 0;};
  for(const boundary of new Set(slots.flatMap(slot=>[slot.start_minute,slot.end_minute%1440]))){const value=at(boundary);if(value!==at((boundary+1439)%1440))changes.push({minute:boundary,label:value?(p.connection==='0_10v'?value+' %':'Ein'):'Aus'});}
 }
 for(const c of changes)c.delta=(c.minute-minute+1440)%1440||1440;
 changes.sort((a,b)=>a.delta-b.delta);
 if(!changes.length)return 'Plan: durchgehend '+(slots.length?'eingeschaltet.':'aus.');
 const next=changes[0],day=next.minute>minute&&next.delta<1440?'heute':'morgen';
 return 'Nächster regulärer Termin: '+day+' '+timeText(next.minute)+' · '+next.label;
}
const diagnosticLabels={NONE:'Keine Sperre',REST:'Betrieb pausiert',SETUP:'WLAN-Einrichtung aktiv',OTA:'Firmwareupdate aktiv',STORAGE:'Speicher prüfen',OUTPUT_INIT:'Ausgangstreiber nicht bereit',WAIT_TIME:'Uhrzeit fehlt',STARTUP_LOCK:'Startsperre nach Neustart',DISABLED:'Anschluss deaktiviert',TACHO_FAULT:'Drehzahlfehler',WAIT_EVALUATION:'Noch nicht ausgewertet',MISSING:'Sensor nicht erreichbar',STALE:'Messwert veraltet',INVALID:'Messwert ungültig',FALLBACK:'Anderer Messwert fehlt – Ersatzverhalten',DELAY:'Zustandswechsel verzögert',SAME_DEMAND:'Gleiche Anforderung – Reihenfolge entscheidet',SELECTED:'Bestimmt die Geräteanforderung',LOWER_PRIORITY:'Grenzwertaktion mit höherer Priorität hat Vorrang',LOWER_DEMAND:'Eine andere Regel fordert mehr Leistung',TARGET_REACHED:'Zielbereich erreicht'};
function measure(value,unit=''){return value===null||value===undefined?'Kein gültiger Messwert':Number(value).toLocaleString('de-DE',{maximumFractionDigits:2})+(unit?' '+(unit==='C'?'°C':unit):'');}
function demandText(value,e,p){if(value===null||value===undefined)return 'Nicht ausgewertet';return p?.timed?(value?'Begrenzter Impuls':'Kein Impuls'):['pwm','0_10v'].includes(p?.connection)?value+' %':value?'Ein':'Aus';}
function diagnosticShell(parent,id){const details=document.createElement('details'),summary=document.createElement('summary'),content=document.createElement('div');details.id='diagnosis_'+id;summary.textContent='Regeldiagnose';content.id='diagnosisContent_'+id;details.append(summary,content);parent.append(details);}
function renderDiagnostics(e,id,p,applied){const content=el('diagnosisContent_'+id);if(!content)return;content.replaceChildren();const status=e.automation_status||{},block=status.execution_block||'NONE',headline=document.createElement('p');headline.className='notice';headline.textContent=(block!=='NONE'?(diagnosticLabels[block]||block):'Aktuelle Regelentscheidung')+' · Ansteuerung: '+demandText(applied,e,p);content.append(headline);
 row(content,'Geräteanforderung',status.evaluated_age_ms===null||status.evaluated_age_ms===undefined?'Noch nicht ausgewertet':demandText(status.demand_percent,e,p)+(block!=='NONE'?' (letzte Auswertung)':''));
 if(status.cooldown_remaining_s&&!e.remaining_ms)row(content,'Mindestpause','Noch '+status.cooldown_remaining_s+' s');
 if(!status.rules?.length){const hint=document.createElement('p');hint.className='sub';hint.textContent=e.automation?.rules?.length?'Diagnose wird geladen.':'Keine Sensorregeln eingerichtet.';content.append(hint);return;}
 for(const [n,d] of status.rules.entries()){const r=e.automation?.rules?.[n]||{},box=document.createElement('section'),title=document.createElement('h4');title.textContent='Priorität '+(n+1)+' · '+(signalNames[d.signal]||d.signal)+(d.selected?' · maßgeblich':'');box.append(title);row(box,'Sensor',d.source);row(box,'Messwert',measure(d.value,d.unit));row(box,'Messwertalter',d.age_ms===null||d.age_ms===undefined?'Unbekannt':measure(d.age_ms/1000,'s'));row(box,'Messwertstatus',d.input_state==='OK'?'Gültig':diagnosticLabels[d.input_state]||'Unbekannt');row(box,'Zielwert',measure(r.target,d.unit)+' · '+(r.direction==='raise'?'erhöhen':'senken'));
 row(box,'Grenzen',(r.minimum!==undefined?'Min '+measure(r.minimum,d.unit):'Kein Minimum')+' / '+(r.maximum!==undefined?'Max '+measure(r.maximum,d.unit):'Kein Maximum'));row(box,'Normale Anforderung',demandText(d.normal_demand_percent,e,p));
 const limit=d.observed_limit!=='NONE'?d.observed_limit:d.active_limit,action=limit==='BELOW_MINIMUM'?r.below_percent:limit==='ABOVE_MAXIMUM'?r.above_percent:undefined;
 row(box,'Grenzwertaktion',!limit||limit==='NONE'?'Keine aktiv':action===undefined?'Nur Warnung; normale Zielregelung':demandText(action,e,p));row(box,'Regelanforderung',demandText(d.requested_percent,e,p));row(box,'Entscheidung',(diagnosticLabels[d.decision]||d.decision||'Unbekannt')+(d.pending_ms?' · noch '+Math.ceil(d.pending_ms/1000)+' s':''));content.append(box);}
}
function warningEntries(d){const entries=[];const devices=[...d.equipment.map(e=>({e,id:e.id,p:d.device_catalog.find(p=>p.id===e.profile)})),...(d.fan_ports||[]).map((e,i)=>({e,id:'FAN'+(i+1),p:{connection:'pwm'}})).filter(x=>x.e.enabled)];for(const {e,id,p} of devices){if(e.mode!=='automatic')continue;const status=e.automation_status||{};for(const [n,r] of (status.rules||[]).entries()){const rule=e.automation?.rules?.[n]||{},label=e.name+' · '+(signalNames[r.signal]||r.signal)+' · '+r.source;
 if(r.input_state!=='OK'){entries.push({id,text:label+': '+(diagnosticLabels[r.input_state]||'Messwert fehlt')+'. Ersatzverhalten: '+demandText(e.automation?.fallback_percent??0,e,p)+(status.execution_block&&status.execution_block!=='NONE'?' · '+(diagnosticLabels[status.execution_block]||status.execution_block):'')+'.'});continue;}
 const observed=r.observed_limit&&r.observed_limit!=='NONE'?r.observed_limit:null,active=r.active_limit&&r.active_limit!=='NONE'?r.active_limit:null,limit=observed||active;if(!limit)continue;const low=limit==='BELOW_MINIMUM',action=low?rule.below_percent:rule.above_percent;const state=observed?(low?'unter Minimum':'über Maximum'):'Grenzwertwarnung bleibt durch Hysterese aktiv';
 let text=label+': '+measure(r.value,r.unit)+' · '+state+' ('+measure(low?rule.minimum:rule.maximum,r.unit)+'). '+(action===undefined?'Nur Warnung.':'Grenzwertaktion: '+demandText(action,e,p)+'.');if(status.execution_block&&status.execution_block!=='NONE')text+=' '+(diagnosticLabels[status.execution_block]||status.execution_block)+'.';else if(status.reason==='SENSOR_MISSING')text+=' Ersatzverhalten wegen eines fehlenden Messwerts hat Vorrang.';else if(!active)text+=' Grenzwertaktion noch nicht aktiv'+(r.pending_ms?' · Verzögerung '+Math.ceil(r.pending_ms/1000)+' s':'')+'.';else if(action!==undefined&&!r.selected)text+=' Höhere Priorität oder Ersatzverhalten hat Vorrang.';entries.push({id,text});}}
 if(d.warning&&d.warning!=='OK')entries.push({text:'Board prüfen · '+(({SETTINGS:'Einstellungen oder Speicher prüfen',OUTPUT_INIT:'Ausgangstreiber nicht bereit',FAN_FAULT:'Drehzahlfehler',RESUME_FAILED:'Automatischer Wiederanlauf fehlgeschlagen'})[d.warning]||d.warning)});return entries;}
function renderWarnings(d){const entries=warningEntries(d);el('warningPanel').hidden=!entries.length;el('warningCount').textContent='('+entries.length+')';el('warningStatus').textContent='Aktueller Boardstand · '+new Date().toLocaleTimeString('de-DE');el('warningList').replaceChildren();for(const w of entries){const box=document.createElement('div'),text=document.createElement('p');text.textContent=w.text;box.append(text);if(w.id)button(box,'Zur Regeldiagnose',()=>{showView('overview');const target=el('diagnosis_'+w.id);if(target){target.open=true;target.scrollIntoView({behavior:'smooth'});}});el('warningList').append(box);}if(d.healthy&&entries.length)el('boardHealth').textContent=entries.length+' Warnung'+(entries.length===1?'':'en');}
function downloadJson(payload,name){const url=URL.createObjectURL(new Blob([JSON.stringify(payload,null,2)],{type:'application/json'})),a=document.createElement('a');a.href=url;a.download=name;document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);}
async function downloadConfiguration(){if(!connected||busy)return;try{const {response:r,data:b}=await apiRequest('/api/v1/configuration',{cache:'no-store'});if(!r.ok||b.format!=='grova-core-equipment')throw Error(b.message||'Sicherung nicht verfügbar.');downloadJson(b,'grova-geraete-regeln.json');el('message').textContent='Konfiguration heruntergeladen. Auf dem Core wurde nichts gespeichert.';}catch(e){el('message').textContent=e.message;}}
async function previewConfiguration(file){const generation=++backupLoadSerial;el('backupDetails').hidden=true;el('backupItems').replaceChildren();backupDraft=null;backupRevision=-1;gateControls();if(!file){el('backupPreview').textContent='Noch keine Sicherung ausgewählt.';return;}el('backupPreview').textContent='Sicherung wird geprüft …';try{if(file.size>16384)throw Error('Sicherung ist zu groß (maximal 16 KiB).');const text=await file.text();if(generation!==backupLoadSerial)return;const b=JSON.parse(text),c=b.configuration;if(b.format!=='grova-core-equipment'||b.version!==1||b.hardware_profile!==state?.hardware_profile||c?.schema!==1||!Array.isArray(c.equipment)||c.equipment.length>5||!Array.isArray(c.fan_ports)||c.fan_ports.length!==2)throw Error('Dateiformat, Version oder Hardwareprofil passen nicht zu diesem Board.');if(![...c.equipment,...c.fan_ports].every(e=>e&&typeof e==='object'&&typeof e.name==='string'&&(e.automation==null||Array.isArray(e.automation.rules)&&e.automation.rules.length<=6))||!c.equipment.every(e=>typeof e.output==='string'&&typeof e.profile==='string')||!c.fan_ports.every(e=>typeof e.enabled==='boolean'))throw Error('Geräte- oder Regelstruktur ist ungültig.');backupDraft=b;backupRevision=state.equipment_revision;for(const e of c.equipment){const port=state.output_capabilities.find(p=>p.id===e.output);row(el('backupItems'),e.name,(port?portLabel(port):e.output)+' · '+(state.device_catalog.find(p=>p.id===e.profile)?.label||e.profile)+' · '+(e.mode==='automatic'?'Sensorgesteuert':e.plan?.enabled?'Zeitgesteuert':'Manuell'));}for(const [n,f] of c.fan_ports.entries())if(f.enabled)row(el('backupItems'),f.name,'PWM-Lüfter '+(n+1)+' · '+(f.mode==='automatic'?'Sensorgesteuert':f.plan?.enabled?'Zeitgesteuert':'Manuell'));el('backupDetails').hidden=false;const rules=[...c.equipment,...c.fan_ports].reduce((n,e)=>n+(e.automation?.rules?.length||0),0);el('backupPreview').textContent=file.name+' · '+c.equipment.length+' Geräte · '+c.fan_ports.filter(f=>f.enabled).length+' PWM-Lüfter · '+rules+' Sensorregeln. Der Core prüft vor dem Speichern zusätzlich alle Ausgangstypen, Spannungen und Schutzgrenzen.';}catch(e){if(generation!==backupLoadSerial)return;backupDraft=null;backupRevision=-1;el('backupDetails').hidden=true;el('backupPreview').textContent='Nicht geladen: '+e.message;}gateControls();}
async function restoreConfiguration(){if(!backupDraft||!connected||busy||pendingEquipment)return;if(backupRevision!==state.equipment_revision){el('message').textContent='Belegung wurde geändert. Sicherung erneut auswählen.';return;}pendingEquipment={expected:backupRevision,uncertain:false};const saved=await send({cmd:'restore_equipment',expected_revision:backupRevision,backup:backupDraft});if(saved){backupDraft=null;el('backupFile').value='';el('backupPreview').textContent='Wiederhergestellt. Betrieb bleibt pausiert; Zuordnungen prüfen und bei Bedarf fortsetzen.';}else if(lastCommandUncertain){pendingEquipment.uncertain=true;}else{pendingEquipment=null;await refresh();}gateControls();}
function seriesLabel(s){if(s.kind==='sensor')return (signalNames[s.signal]||s.signal)+' · '+s.source;const port=state?.output_capabilities?.find(c=>c.id===s.source);return (port?portLabel(port):s.source==='FAN1'?'PWM-Lüfter 1':s.source==='FAN2'?'PWM-Lüfter 2':s.source)+' · Ansteuerung';}
function seriesKey(s){return s.source+'|'+s.signal;}
async function refreshHistory(){if(historyBusy||currentView!=='overview'||!el('trendHistory').open)return;if(!connected){el('trendStatus').textContent='Board nicht erreichbar. Verlauf zeigt den zuletzt geladenen Stand.';return;}historyBusy=true;try{const {response:r,data:h}=await apiRequest('/api/v1/history',{cache:'no-store'});if(!r.ok||!Array.isArray(h.samples)||!Array.isArray(h.series))throw Error('Verlauf momentan nicht verfügbar.');historyData=h;const select=el('trendSeries'),selected=select.value;select.replaceChildren();for(const s of [...h.series].sort((a,b)=>(a.kind==='sensor'?0:1)-(b.kind==='sensor'?0:1))){const option=document.createElement('option');option.value=seriesKey(s);option.textContent=seriesLabel(s);select.append(option);}if([...select.options].some(o=>o.value===selected))select.value=selected;el('trendStatus').textContent=h.samples.length+' / '+h.capacity+' Messpunkte · alle '+h.interval_s+' s · zuletzt geladen '+new Date().toLocaleTimeString('de-DE')+(h.omitted?' · '+h.omitted+' Messreihen wegen Speichergrenze nicht aufgezeichnet':'');drawHistory();}catch(e){el('trendStatus').textContent=historyData?'Verlauf konnte nicht aktualisiert werden. Angezeigt wird der zuletzt geladene Stand.':'Verlauf momentan nicht verfügbar.';}finally{historyBusy=false;}}
function drawHistory(){const canvas=el('trendCanvas'),ctx=canvas.getContext('2d'),width=Math.max(240,canvas.clientWidth),height=170,scale=Math.min(devicePixelRatio||1,2);canvas.width=width*scale;canvas.height=height*scale;ctx.scale(scale,scale);ctx.clearRect(0,0,width,height);const palette=getComputedStyle(el('trendHistory'));const s=historyData?.series.find(s=>seriesKey(s)===el('trendSeries').value),samples=historyData?.samples||[],values=s?.values||[],finite=values.filter(v=>typeof v==='number'&&Number.isFinite(v));if(!s||!finite.length){el('trendSummary').textContent='Noch keine gültigen Messpunkte für diese Auswahl.';ctx.fillStyle=palette.color;ctx.font='13px system-ui';ctx.fillText('Noch keine gültigen Messpunkte',12,40);return;}
 const low=Math.min(...finite),high=Math.max(...finite),pad=low===high?Math.max(1,Math.abs(low)*.05):(high-low)*.1,min=s.unit==='%'&&low>=0&&high<=100?Math.max(0,low-pad):low-pad,max=s.unit==='%'&&low>=0&&high<=100?Math.min(100,high+pad):high+pad,left=60,right=12,top=16,bottom=30,w=width-left-right,h=height-top-bottom;ctx.font='11px system-ui';ctx.strokeStyle=palette.borderTopColor;ctx.fillStyle=palette.color;for(let n=0;n<=4;n++){const y=top+n*h/4;ctx.beginPath();ctx.moveTo(left,y);ctx.lineTo(width-right,y);ctx.stroke();ctx.fillText(measure(max-(max-min)*n/4),2,y+4);}
 const origin=samples[0]?.uptime_ms||0,last=samples[samples.length-1],duration=last?((last.uptime_ms-origin)>>>0):0;ctx.strokeStyle=getComputedStyle(el('boardHealth')).color;ctx.lineWidth=2;ctx.beginPath();let down=false;for(let n=0;n<samples.length;n++){const value=values[n];if(typeof value!=='number'||!Number.isFinite(value)){down=false;continue;}const x=left+(duration?(((samples[n].uptime_ms-origin)>>>0)/duration)*w:w/2),y=top+(max-value)/(max-min)*h;if(down)ctx.lineTo(x,y);else ctx.moveTo(x,y);down=true;}ctx.stroke();ctx.fillStyle=getComputedStyle(el('boardHealth')).color;for(let n=0;n<samples.length;n++){const value=values[n];if(typeof value!=='number'||!Number.isFinite(value))continue;const x=left+(duration?(((samples[n].uptime_ms-origin)>>>0)/duration)*w:w/2),y=top+(max-value)/(max-min)*h;ctx.beginPath();ctx.arc(x,y,2,0,Math.PI*2);ctx.fill();}
 const time=t=>t?.epoch_s?new Date(t.epoch_s*1000).toLocaleTimeString('de-DE',{hour:'2-digit',minute:'2-digit',timeZone:state?.schedule_timezone||'Europe/Berlin'}):t?'+'+measure(t.uptime_ms/60000,'min'):'';ctx.fillStyle=palette.color;ctx.fillText(time(samples[0]),left,height-8);ctx.textAlign='right';ctx.fillText(time(last),width-right,height-8);ctx.textAlign='left';const latest=values[samples.length-1];el('trendSummary').textContent=seriesLabel(s)+' · letzter Messpunkt: '+measure(latest,s.unit)+' · Min '+measure(low,s.unit)+' / Max '+measure(high,s.unit)+'. Fehlende Messwerte bleiben Lücken.';canvas.setAttribute('aria-label',el('trendSummary').textContent);}
el('trendHistory').addEventListener('toggle',()=>{if(el('trendHistory').open)refreshHistory();});
window.addEventListener('resize',()=>{if(el('trendHistory').open)drawHistory();});setInterval(()=>{if(el('trendHistory').open&&!el('view-overview').hidden)refreshHistory();},30000);
const eventLabels={BOOT:'Core gestartet – RAM-Protokoll neu begonnen',CONFIGURATION_SAVED:'Geräte und Regeln gespeichert',PAUSED:'Betrieb pausiert',RELEASED:'Betrieb freigegeben',INPUT_LOST:'Messwert nicht verfügbar',INPUT_RECOVERED:'Messwert wieder verfügbar',BELOW_MINIMUM:'Minimum unterschritten',ABOVE_MAXIMUM:'Maximum überschritten',LIMIT_CLEARED:'Vorherige Grenzwertwarnung beendet',FALLBACK_REQUIRED:'Ersatzverhalten erforderlich; Betrieb gesperrt',FALLBACK_ACTIVE:'Ersatzverhalten aktiv',FALLBACK_ENDED:'Ersatzverhalten nicht mehr erforderlich'};
function eventText(e){let text=eventLabels[e.type]||e.type;if(e.input_state)text+=' · '+(diagnosticLabels[e.input_state]||e.input_state);if(e.name)text+=' · '+e.name;if(e.priority)text+=' · Priorität '+e.priority;if(e.source)text+=' · '+(signalNames[e.signal]||e.signal)+' · '+e.source;if(e.value!==null&&e.value!==undefined)text+=' · '+measure(e.value,e.unit);if(e.limit!==undefined)text+=' · Grenze: '+measure(e.limit,e.unit);if(e.percent!==undefined)text+=' · '+(e.percent===0?'Aus':e.percent===100?'Ein':e.percent+' %');if(e.revision!==undefined)text+=' · Revision '+e.revision;return text;}
function renderEvents(d){const log=d.event_log||{events:[],count:0,overwritten:0};el('eventMeta').textContent=log.count+' Einträge'+(log.overwritten?' · '+log.overwritten+' ältere Einträge ersetzt':'')+' · nur seit diesem Neustart';const signature=JSON.stringify(log);if(el('eventList').dataset.signature===signature)return;el('eventList').dataset.signature=signature;el('eventList').replaceChildren();if(!log.events.length){el('eventList').textContent='Noch keine Ereignisse.';return;}for(const e of log.events){const box=document.createElement('div'),date=document.createElement('p'),text=document.createElement('p');date.className='sub';date.textContent='#'+e.sequence+' · '+(e.epoch_s?new Date(e.epoch_s*1000).toLocaleString('de-DE',{timeZone:d.schedule_timezone||'Europe/Berlin'}):'Uhrzeit unbekannt · '+measure(e.uptime_ms/1000,'s')+' seit Start');text.textContent=eventText(e);box.append(date,text);el('eventList').append(box);}}
function downloadEventLog(){if(!state?.event_log)return;const snapshot={cube_id:state.cube_id,firmware_version:state.firmware_version,downloaded_at:new Date().toISOString(),connection_live:connected,...state.event_log};const url=URL.createObjectURL(new Blob([JSON.stringify(snapshot,null,2)],{type:'application/json'})),a=document.createElement('a');a.href=url;a.download='grova-ereignisse.json';document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function automationText(e){const a=e.automation_status||{},rules=e.automation?.rules||[],rule=rules[a.winning_rule];const name=rule?(signalNames[rule.signal]||rule.signal)+' · '+rule.source:'';const labels={NOT_CONFIGURED:'Keine Sensorregel eingerichtet',SENSOR_MISSING:'Messwert fehlt / veraltet · Ersatzverhalten',TARGET_REACHED:'Zielbereich erreicht',TARGET_CONTROL:'Zielregelung',BELOW_MINIMUM:'Warnung: unter Minimum',ABOVE_MAXIMUM:'Warnung: über Maximum'};return (a.cooldown_remaining_s&&a.demand_percent&&!e.remaining_ms?'Mindestpause · noch '+a.cooldown_remaining_s+' s · ':'')+(labels[a.reason]||'Sensorregelung bereit')+(name?' · '+name:'')+(a.warning_mask&& !['BELOW_MINIMUM','ABOVE_MAXIMUM'].includes(a.reason)?' · Grenzwertwarnung':'');}
function automationInfo(e){if(!e.automation?.rules?.length)return 'Noch keine Sensorregel eingerichtet. Gerät bleibt aus.';return e.automation.rules.map((r,i)=>(i+1)+'. '+(signalNames[r.signal]||r.signal)+' → Ziel '+r.target+(r.minimum!==undefined?' · Min '+r.minimum:'')+(r.maximum!==undefined?' · Max '+r.maximum:'')).join(' / ');}
function planText(e,p){if(e.plan?.interval){const i=e.plan.interval;return 'Alle '+i.every_minutes+' min ab '+timeText(i.anchor_minute)+' · '+(p.timed?i.runtime_ms/1000+' s':i.duration_minutes+' min'+(['0_10v','pwm'].includes(p.connection)?' · '+i.percent+' %':''));}return 'Täglich · '+(e.plan?.slots||[]).map(s=>timeText(s.start_minute)+(p.timed?' für '+s.runtime_ms/1000+' s':'–'+timeText(s.end_minute)+(['0_10v','pwm'].includes(p.connection)?' · '+s.percent+' %':''))).join(' / ');}
function percentSlider(parent,id,initial=0){const input=numberField(parent,id,'Leistung / %',initial,0,100),value=document.createElement('output');input.type='range';input.step='1';value.textContent=initial+' %';input.oninput=()=>value.textContent=input.value+' %';input.parentElement.append(value);return input;}
function renderFans(d){const signature=JSON.stringify(d.fan_ports?.map(({automation_status,...config})=>config));if(el('fans').dataset.signature!==signature){el('fans').dataset.signature=signature;el('fans').replaceChildren();for(let n=1;n<=2;n++){const f=d.fan_ports?.[n-1];if(!f?.enabled)continue;const box=document.createElement('article');box.className='card';box.id='fanCard'+n;const title=document.createElement('h3');title.textContent=f.name;const role=document.createElement('p');role.className='sub';role.textContent=({circulation:'Umluft',intake:'Zuluft',exhaust:'Abluft'})[f.fan_role]||'Lüfter';box.append(role);const live=document.createElement('p'),note=document.createElement('p');live.id='fan'+n;live.className='device-status';note.id='fanReason'+n;note.className='sub';box.append(title,live,note);if(f.mode==='automatic'){const hint=document.createElement('p');hint.className='notice';hint.textContent=automationInfo(f);box.append(hint);diagnosticShell(box,'FAN'+n);}else if(f.plan?.enabled){const summary=document.createElement('p');summary.textContent=planText(f,{connection:'pwm',timed:false});const next=document.createElement('p');next.id='fan'+n+'_next';next.className='notice';box.append(summary,next);}else{const panel=operatingPanel(box),input=percentSlider(panel,'fanvalue'+n,d['fan'+n]?.target_pct||0);panel.open=true;const set=button(panel,'Übernehmen',()=>send({cmd:'set_fan_manual',fan:n,percent:Number(input.value)}),true);set.dataset.fan=n;button(panel,'Stopp',()=>send({cmd:'set_fan_manual',fan:n,percent:0}));}button(box,'Betriebsart / Zeitplan bearbeiten',()=>{startWizard('FAN'+n);});el('fans').append(box);}}
 for(let n=1;n<=2;n++){const config=d.fan_ports?.[n-1],f=d['fan'+n];if(!config?.enabled)continue;el('fan'+n).textContent=f.current_pct+' %';el('fanReason'+n).textContent=f.tacho_fault?'Drehzahlfehler – Anschluss prüfen':d.rest_mode.enabled?'Betrieb pausiert':config.mode==='automatic'?automationText(config):config.plan?.enabled?'Zeitplan · '+(d.time_synced?'aktiv':'warte auf Uhrzeit'):'Manuelle Bedienung';if(config.mode==='automatic')renderDiagnostics(config,'FAN'+n,{connection:'pwm'},f.current_pct);if(config.mode!=='automatic'&&config.plan?.enabled)el('fan'+n+'_next').textContent=nextPlanAction(d,config,{connection:'0_10v',timed:false});}
}
function renderEquipment(d){if(equipmentRevision<0||(!equipmentDirty&&equipmentRevision!==d.equipment_revision))buildEquipmentEditor(d);const signature=JSON.stringify(d.equipment.map(e=>[e.id,e.name,e.profile,e.output,e.max_runtime_ms,e.plan,e.mode,e.automation,e.fan_role]));if(el('equipment').dataset.signature!==signature){el('equipment').replaceChildren();el('equipment').dataset.signature=signature;
 for(const e of d.equipment){const p=d.device_catalog.find(p=>p.id===e.profile),box=document.createElement('article'),label=document.createElement('h3'),live=document.createElement('p'),reason=document.createElement('p');box.className='card';label.textContent=e.name;live.id=e.id+'_live';live.className='device-status';reason.id=e.id+'_reason';reason.className='sub';box.append(label,live,reason);if(e.fan_role){const role=document.createElement('p');role.className='sub';role.textContent=({circulation:'Umluft',intake:'Zuluft',exhaust:'Abluft'})[e.fan_role]||'Lüfter';box.append(role);}if(p.timed&&(e.plan?.enabled||e.mode==='automatic')){const history=document.createElement('p');history.id=e.id+'_history';history.className='notice';box.append(history);}const panel=operatingPanel(box),actions=document.createElement('div');if(p.id.startsWith('fan_'))panel.open=true;actions.className='actions';let start;
 if(e.mode==='automatic'){panel.remove();const note=document.createElement('p');note.className='notice';note.textContent=automationInfo(e);box.append(note);diagnosticShell(box,e.id);button(actions,'Betriebsart bearbeiten',()=>{startWizard(e.output);});box.append(actions);el('equipment').append(box);continue;}
 if(e.plan?.enabled){const summary=document.createElement('p');summary.className='sub';summary.textContent=planText(e,p);box.append(summary);const next=document.createElement('p');next.id=e.id+'_next';next.className='notice';box.append(next);button(actions,'Zeitplan bearbeiten',()=>{startWizard(e.output);});panel.remove();box.append(actions);el('equipment').append(box);continue;}
 if(p.timed){const input=numberField(panel,e.id+'_duration','Laufzeit / Sekunden',Math.min(1000,e.max_runtime_ms)/1000,.001,e.max_runtime_ms/1000,.001);start=button(actions,'Für diese Zeit starten',()=>send({cmd:'run_device',id:e.id,runtime_ms:Math.round(Number(input.value)*1000),event_id:eventId()}),true);}
 else if(p.connection==='0_10v'){const input=p.id==='fan_0_10v'?percentSlider(panel,e.id+'_percent',e.requested_percent||0):numberField(panel,e.id+'_percent',p.id==='light_0_10v'?'Helligkeit / %':'Leistung / %',0,0,100);start=button(actions,'Übernehmen',()=>send({cmd:'set_device',id:e.id,percent:Number(input.value)}),true);}
 else start=button(actions,'Einschalten',()=>send({cmd:'set_device',id:e.id,state:true}),true);
 start.dataset.device=e.id;button(actions,'Stopp',()=>send({cmd:'stop_device',id:e.id}));panel.append(actions);const info=document.createElement('p');info.className='sub';info.textContent=e.output+(p.timed?' · maximal '+e.max_runtime_ms/1000+' s':'');panel.append(info);el('equipment').append(box);}}
 for(const e of d.equipment){const p=d.device_catalog.find(p=>p.id===e.profile);if(e.mode==='automatic')renderDiagnostics(e,e.id,p,e.applied_percent);if(p.timed&&(e.plan?.enabled||e.mode==='automatic')){const r=e.last_run,labels={RESERVED:'Start gespeichert, Ausführung noch nicht bestätigt',STARTED:'Ausgang angesteuert',COMPLETED:'Ansteuerung regulär beendet',INTERRUPTED:'Ansteuerung unterbrochen',INTERRUPTED_UNKNOWN:'Neustart: Abschluss unklar; wird nicht wiederholt',START_FAILED:'Start abgelehnt',SKIPPED:'Nicht gestartet: Termin außerhalb der Nachholfrist',LEGACY_UNKNOWN:'Alter Lauf: Ausführung nicht nachgewiesen'};let text='Noch kein automatischer Lauf erfasst.';if(r&&r.state!=='NONE'){const year=Math.floor(r.scheduled_date_key/1000),day=r.scheduled_date_key%1000,date=new Date(Date.UTC(year,0,day+1)).toLocaleDateString('de-DE',{timeZone:'UTC'});text='Letzter automatischer Impuls · '+date+' '+timeText(r.scheduled_minute)+' · '+(r.durable===false?'Speicherfehler: Abschluss unklar':labels[r.state]||'Status unklar')+'.';}el(e.id+'_history').textContent=text+' Wasserfluss wird nicht gemessen.';}if(e.plan?.enabled)el(e.id+'_next').textContent=nextPlanAction(d,e,p);el(e.id+'_live').textContent=e.applied_percent>0?(p.connection==='0_10v'?e.applied_percent+' %':'Ein'):'Aus';el(e.id+'_reason').textContent=d.rest_mode.enabled?'Betrieb pausiert':!d.equipment_settings_ok||!d.outputs_ready?'Betrieb blockiert – Einstellungen prüfen':(e.plan?.enabled||e.mode==='automatic')&&!d.time_synced?'Warte auf gültige Uhrzeit':e.startup_remaining_ms?'Startsperre · noch '+Math.ceil(e.startup_remaining_ms/1000)+' s':e.remaining_ms?'Noch '+(e.remaining_ms/1000).toLocaleString('de-DE')+' s':e.mode==='automatic'?automationText(e):reasons[e.reason]||'Status prüfen';}}
refresh();setInterval(refresh,3000);
</script></div></div></main></body></html>
)HTML";
