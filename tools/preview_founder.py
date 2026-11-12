"""Create a clearly marked, hardware-free preview; always starts with an empty board."""
from pathlib import Path
import argparse,json,re
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--blank-status',type=Path,required=True)
p.add_argument('--example-status',type=Path)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
blank=json.loads(a.blank_status.read_text(encoding='utf-8'))
assert not blank['equipment'] and not any(f['enabled'] for f in blank['fan_ports'])
assert blank['rest_mode']['enabled'] and all(o['percent']==0 for o in blank['outputs'])
snapshots={'blank':blank}
if a.example_status:snapshots['example']=json.loads(a.example_status.read_text(encoding='utf-8'))
html=re.search(r'R"HTML\(([\s\S]*)\)HTML"',(root/'src/founder/page.h').read_text(encoding='utf-8'))[1]
mock=r'''<script>
const previewSnapshots=__DATA__;let previewState=structuredClone(previewSnapshots.blank);
window.fetch=async(url,opts)=>{
 if(opts?.method!=='POST')return {ok:true,json:async()=>structuredClone(previewState)};
 const p=JSON.parse(opts.body);let ok=true,message='Vorschau aktualisiert';
 if(p.cmd==='configure_equipment'){
  if(p.expected_revision!==previewState.equipment_revision){ok=false;message='Belegung veraltet';}
  else {previewState.equipment=p.equipment.map(e=>({...e,max_runtime_ms:e.max_runtime_ms||0,startup_remaining_ms:0,remaining_ms:0,applied_percent:0,requested_percent:0,reason:'REST'}));previewState.fan_ports=p.fan_ports;previewState.equipment_revision++;if(p.start_plans)previewState.rest_mode.enabled=false;for(const o of previewState.outputs)o.assigned=p.equipment.some(e=>e.output===o.id||e.power_output===o.id);}
 }else if(p.cmd==='set_rest_mode')previewState.rest_mode.enabled=p.enabled;
 else if(p.cmd==='stop_all'){previewState.rest_mode.enabled=true;}else{ok=false;message='Diese Vorschau führt keine Gerätebefehle aus.';}
 return {ok,json:async()=>({ok,message})};
};
async function previewPreset(value){previewState=structuredClone(previewSnapshots[value]);equipmentDirty=false;equipmentRevision=-1;pendingEquipment=null;await refresh();}
</script>'''.replace('__DATA__',json.dumps(snapshots,ensure_ascii=True).replace('<','\u003c'))
options='<option value="blank">Leeres Board (Erststart)</option>'
if 'example' in snapshots:options+='<option value="example">Beispielbelegung laden</option>'
banner='<section class="notice"><strong>INTERAKTIVE VORSCHAU · KEINE HARDWARE</strong><p>Alle Werte sind simuliert. Es werden keine Daten an einen Controller gesendet.</p><label>Ansicht<select onchange="previewPreset(this.value)">'+options+'</select></label></section>'
html=html.replace('<script>',mock+'<script>',1).replace('<nav aria-label="Hauptnavigation">',banner+'<nav aria-label="Hauptnavigation">',1)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(html,encoding='utf-8')
print(a.output)
