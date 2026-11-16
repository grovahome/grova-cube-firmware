"""Build and package the first-flash S3 image. Never contacts or writes a device."""
from pathlib import Path
import argparse, hashlib, json, shutil, subprocess, sys
from datetime import datetime, timezone

ROOT=Path(__file__).resolve().parents[1]
ENV='grova_founder_s3'
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--skip-build',action='store_true',help='Only package an already verified local build')
p.add_argument('--platformio-home',type=Path,default=Path.home()/'.platformio')
a=p.parse_args()
if not a.skip_build:
    subprocess.run([sys.executable,'-m','platformio','run','-e',ENV],cwd=ROOT,check=True)
build=ROOT/'.pio/build'/ENV
packages=a.platformio_home/'packages'
parts=[('bootloader.bin',0,build/'bootloader.bin'),('partitions.bin',0x8000,build/'partitions.bin'),
       ('boot_app0.bin',0xe000,packages/'framework-arduinoespressif32/tools/partitions/boot_app0.bin'),
       ('firmware.bin',0x10000,build/'firmware.bin')]
for name,offset,path in parts:
    if not path.is_file(): raise SystemExit(f'Missing build component: {path}')
out=ROOT/'dist/core-s3';out.mkdir(parents=True,exist_ok=True)
for name,offset,path in parts:shutil.copyfile(path,out/name)
factory=out/'grova-core-s3-first-flash.bin'
# Match PlatformIO: ROM bootloader header uses DIO even with QIO application flash.
command=[sys.executable,str(packages/'tool-esptoolpy/esptool.py'),'--chip','esp32s3','merge_bin',
         '--flash_mode','dio','--flash_freq','80m','--flash_size','16MB','-o',str(factory)]
for name,offset,path in parts:command.extend([hex(offset),str(out/name)])
subprocess.run(command,cwd=ROOT,check=True)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
manifest={'environment':ENV,'chip':'esp32s3','module':'ESP32-S3-WROOM-1-N16R8',
          'generated_utc':datetime.now(timezone.utc).isoformat(),
          'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
          'source_dirty':bool(subprocess.check_output(['git','status','--porcelain','--untracked-files=normal'],cwd=ROOT,text=True).strip()),
          'physical_acceptance':'OPEN','factory_image':{'file':factory.name,'offset':'0x0','sha256':sha(factory)},
          'components':[{'file':name,'offset':hex(offset),'sha256':sha(out/name)} for name,offset,path in parts]}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
(out/'README.txt').write_text("""GROVA Founder S3 - first flash only
==================================
Target: ESP32-S3-WROOM-1-N16R8. OLED: SSD1317, 128 SEG x 64 COM.
Hardware acceptance remains OPEN. Resolve GPIO46/BOOT entry before flashing.

Preferred from the firmware repo (COM7 is an example; use the actual board port):
  pio run -e grova_founder_s3 -t upload --upload-port COM7
  pio device monitor --port COM7 --baud 115200

The merged image belongs at offset 0x0, ONLY for the first flash of a blank board.
It also overwrites the NVS range. Do not use it for routine updates of a configured
board; use the normal PlatformIO upload or application OTA instead.

At boot the board is in Rest. With no credentials it opens GROVA-<full MAC>.
OLED shows the setup key. If the screen is blank, enter setup plus newline in USB
serial. Credentials are printed only on that explicit USB request.
Serial commands: status | setup | display <rotation 0..3> <row offset 0..95>
Display default: rotation 0, offset 80 (provisional 64-MUX panel profile).

No device was flashed by this packaging operation. Check manifest.json hashes.
""",encoding='utf-8')
print(f'Packaged {factory} ({factory.stat().st_size} bytes)')
print(f'SHA-256: {manifest["factory_image"]["sha256"]}')
