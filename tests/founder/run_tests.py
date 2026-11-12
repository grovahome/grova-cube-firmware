"""Exercise production Founder drivers with a deterministic bus and GPIO adapter."""
from pathlib import Path
import argparse,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--compiler',default='g++');p.add_argument('--zig',action='store_true');p.add_argument('--status-output',type=Path);p.add_argument('--blank-output',type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='grova-founder-tests-') as temp:
    binary=Path(temp)/'founder_tests.exe'
    command=[a.compiler]+(['c++'] if a.zig else [])
    command+=['-std=c++17','-Wall','-Wextra','-Itests/founder/include','-Isrc','-Iinclude','tests/founder/test.cpp','src/founder/i2c_bus.cpp','src/founder/rtc.cpp','src/founder/outputs.cpp','src/founder/timed_digital.cpp','src/modules/signal_registry.cpp','-o',str(binary)]
    subprocess.run(command,cwd=root,check=True)
    subprocess.run([str(binary)],cwd=root,check=True)

    subprocess.run([str(binary),'fail_timer'],cwd=root,check=True)

    api_binary=Path(temp)/'founder_app_tests.exe'
    api_command=[a.compiler]+(['c++'] if a.zig else [])
    api_command+=['-std=c++17','-Wall','-Wextra','-DARDUINOJSON_ENABLE_ARDUINO_STRING=1','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-DARDUINOJSON_ENABLE_PROGMEM=0','-Itests/founder/include','-Itests/host/include','-Isrc','-Iinclude','-I.pio/libdeps/grova_founder_s3/ArduinoJson/src','tests/founder/app_test.cpp','src/founder/app.cpp','src/founder/equipment.cpp','src/founder/outputs.cpp','src/founder/timed_digital.cpp','src/modules/rest_mode.cpp','src/modules/signal_registry.cpp','-o',str(api_binary)]
    subprocess.run(api_command,cwd=root,check=True)
    result=subprocess.run([str(api_binary)]+(['status'] if a.status_output else []),cwd=root,check=True,text=True,capture_output=bool(a.status_output))
    if a.status_output:
        lines=result.stdout.splitlines();a.status_output.write_text(lines[0],encoding='utf-8');print('\n'.join(lines[1:]))

    blank=subprocess.run([str(api_binary),'--blank'],cwd=root,check=True,text=True,capture_output=True)
    if a.blank_output:a.blank_output.write_text(blank.stdout.strip(),encoding='utf-8')
    snapshot=Path(temp)/'nvs-snapshot.bin'
    for args in [('--driver-failure',),('--app-storage-failure',),('--storage-failure',),('--empty-config',),('--empty-ledger',),('--profile-matrix',),('--reboot-save',str(snapshot)),('--reboot-load',str(snapshot))]:
        subprocess.run([str(api_binary),*args],cwd=root,check=True)
    print('Blank boot, unavailable/corrupt storage and fresh-process restart passed')

    device_binary=Path(temp)/'founder_device_tests.exe'
    device_command=[a.compiler]+(['c++'] if a.zig else [])
    device_command+=['-std=c++17','-Wall','-Wextra','-Itests/founder/include','-Isrc','-Iinclude','tests/founder/devices_test.cpp','src/founder/devices.cpp','src/founder/i2c_bus.cpp','src/modules/signal_registry.cpp','-o',str(device_binary)]
    subprocess.run(device_command,cwd=root,check=True)
    subprocess.run([str(device_binary)],cwd=root,check=True)

    wifi_binary=Path(temp)/'founder_wifi_tests.exe'
    wifi_command=[a.compiler]+(['c++'] if a.zig else [])
    wifi_command+=['-std=c++17','-DGROVA_FOUNDER_APP=1','-Itests/host/wifi_include','-Itests/host/include','-Isrc','-Iinclude','tests/founder/wifi_identity_test.cpp','tests/founder/wifi_glue.cpp','src/modules/wifi_ota.cpp','src/modules/rest_mode.cpp','-o',str(wifi_binary)]
    subprocess.run(wifi_command,cwd=root,check=True)
    subprocess.run([str(wifi_binary)],cwd=root,check=True)
