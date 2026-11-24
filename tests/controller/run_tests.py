"""Exercise production Founder drivers with a deterministic bus and GPIO adapter."""
from pathlib import Path
import argparse,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--compiler',default='g++');p.add_argument('--zig',action='store_true');p.add_argument('--status-output',type=Path);p.add_argument('--blank-output',type=Path);p.add_argument('--boot-frames',type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='grova-founder-tests-') as temp:
    history_binary=Path(temp)/'history_tests.exe'
    subprocess.run([a.compiler]+(['c++'] if a.zig else [])+['-std=c++17','-Wall','-Wextra','-Isrc','tests/controller/history_test.cpp','-o',str(history_binary)],cwd=root,check=True)
    subprocess.run([str(history_binary)],cwd=root,check=True)
    log_binary=Path(temp)/'event_log_tests.exe'
    subprocess.run([a.compiler]+(['c++'] if a.zig else [])+['-std=c++17','-Wall','-Wextra','-Isrc','tests/controller/event_log_test.cpp','-o',str(log_binary)],cwd=root,check=True)
    subprocess.run([str(log_binary)],cwd=root,check=True)
    auto_binary=Path(temp)/'automation_tests.exe'
    subprocess.run([a.compiler]+(['c++'] if a.zig else [])+['-std=c++17','-Wall','-Wextra','-Isrc','tests/controller/automation_test.cpp','-o',str(auto_binary)],cwd=root,check=True)
    subprocess.run([str(auto_binary)],cwd=root,check=True)
    schedule_binary=Path(temp)/'schedule_tests.exe'
    subprocess.run([a.compiler]+(['c++'] if a.zig else [])+['-std=c++17','-Wall','-Wextra','-Isrc','tests/controller/schedule_test.cpp','-o',str(schedule_binary)],cwd=root,check=True)
    subprocess.run([str(schedule_binary)],cwd=root,check=True)
    led_binary=Path(temp)/'led_tests.exe'
    subprocess.run([a.compiler]+(['c++'] if a.zig else [])+['-std=c++17','-Wall','-Wextra','-Isrc','tests/controller/led_test.cpp','-o',str(led_binary)],cwd=root,check=True)
    subprocess.run([str(led_binary)],cwd=root,check=True)
    boot_binary=Path(temp)/'boot_tests.exe'
    boot_command=[a.compiler]+(['c++'] if a.zig else [])
    subprocess.run(boot_command+['-std=c++17','-Wall','-Wextra','-Isrc','tests/controller/boot_test.cpp','-o',str(boot_binary)],cwd=root,check=True)
    subprocess.run([str(boot_binary)]+([str(a.boot_frames)] if a.boot_frames else []),cwd=root,check=True)
    inputs_binary = Path(temp) / 'inputs_tests.exe'
    subprocess.run(boot_command + ['-std=c++17', '-Wall', '-Wextra',
                   '-Itests/controller/include', '-Isrc', '-Iinclude',
                   'tests/controller/inputs_test.cpp', 'src/controller/inputs.cpp',
                   '-o', str(inputs_binary)], cwd=root, check=True)
    subprocess.run([str(inputs_binary)], cwd=root, check=True)
    display_binary=Path(temp)/'display_tests.exe'
    display_command=[a.compiler]+(['c++'] if a.zig else [])
    subprocess.run(display_command+['-std=c++17','-Wall','-Wextra','-Itests/controller/include','-Itests/host/include','-Isrc','-Iinclude','-I.pio/libdeps/grova_founder_s3/ArduinoJson/src','tests/controller/display_test.cpp','src/controller/display.cpp','src/controller/i2c_bus.cpp','src/modules/signal_registry.cpp','-o',str(display_binary)],cwd=root,check=True)
    subprocess.run([str(display_binary)],cwd=root,check=True)
    binary=Path(temp)/'founder_tests.exe'
    command=[a.compiler]+(['c++'] if a.zig else [])
    command+=['-std=c++17','-Wall','-Wextra','-Itests/controller/include','-Isrc','-Iinclude','tests/controller/test.cpp','src/controller/i2c_bus.cpp','src/controller/rtc.cpp','src/controller/outputs.cpp','src/controller/timed_digital.cpp','src/modules/signal_registry.cpp','-o',str(binary)]
    subprocess.run(command,cwd=root,check=True)
    subprocess.run([str(binary)],cwd=root,check=True)

    subprocess.run([str(binary),'fail_timer'],cwd=root,check=True)

    api_binary=Path(temp)/'founder_app_tests.exe'
    api_command=[a.compiler]+(['c++'] if a.zig else [])
    api_command+=['-std=c++17','-Wall','-Wextra','-DARDUINOJSON_ENABLE_ARDUINO_STRING=1','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-DARDUINOJSON_ENABLE_PROGMEM=0','-Itests/controller/include','-Itests/host/include','-Isrc','-Iinclude','-I.pio/libdeps/grova_founder_s3/ArduinoJson/src','tests/controller/app_test.cpp','src/controller/app.cpp','src/controller/equipment.cpp','src/controller/automation.cpp','src/controller/event_log.cpp','src/controller/history.cpp','src/controller/outputs.cpp','src/controller/timed_digital.cpp','src/modules/rest_mode.cpp','src/modules/signal_registry.cpp','-o',str(api_binary)]
    subprocess.run(api_command,cwd=root,check=True)
    result=subprocess.run([str(api_binary)]+(['status'] if a.status_output else []),cwd=root,check=True,text=True,capture_output=bool(a.status_output))
    if a.status_output:
        lines=result.stdout.splitlines();a.status_output.write_text(lines[0],encoding='utf-8');print('\n'.join(lines[1:]))

    blank=subprocess.run([str(api_binary),'--blank'],cwd=root,check=True,text=True,capture_output=True)
    if a.blank_output:a.blank_output.write_text(blank.stdout.strip(),encoding='utf-8')
    snapshot=Path(temp)/'nvs-snapshot.bin'
    for args in [('--driver-failure',),('--app-storage-failure',),('--storage-failure',),('--empty-config',),('--empty-ledger',),('--profile-matrix',),('--small-pulse',),('--reboot-save',str(snapshot)),('--reboot-load',str(snapshot))]:
        subprocess.run([str(api_binary),*args],cwd=root,check=True)
    plan_snapshot=Path(temp)/'plan-nvs.bin'
    for mode in ['--plan-integration','--plan-save','--plan-load','--plan-no-intent-load','--plan-corrupt-intent-load','--plan-paused-save','--plan-paused-load','--plan-release-fail-save','--plan-release-fail-load','--plan-corrupt','--plan-history-save','--plan-history-load','--plan-reserved-save','--plan-reserved-load','--plan-journal-faults','--plan-history-stop','--plan-journal-migration']:
        subprocess.run([str(api_binary),mode,str(plan_snapshot)],cwd=root,check=True)
    for mode in ['--fan-integration','--fan-save','--fan-load','--auto-integration','--auto-save','--auto-load','--backup-integration','--backup-save','--backup-load']:
        subprocess.run([str(api_binary),mode,str(plan_snapshot)],cwd=root,check=True)
    print('Blank boot, unavailable/corrupt storage and fresh-process restart passed')

    device_binary=Path(temp)/'founder_device_tests.exe'
    device_command=[a.compiler]+(['c++'] if a.zig else [])
    device_command+=['-std=c++17','-Wall','-Wextra','-Itests/controller/include','-Isrc','-Iinclude','tests/controller/devices_test.cpp','src/controller/devices.cpp','src/controller/i2c_bus.cpp','src/modules/signal_registry.cpp','-o',str(device_binary)]
    subprocess.run(device_command,cwd=root,check=True)
    subprocess.run([str(device_binary)],cwd=root,check=True)

    wifi_binary=Path(temp)/'founder_wifi_tests.exe'
    wifi_command=[a.compiler]+(['c++'] if a.zig else [])
    wifi_command+=['-std=c++17','-DGROVA_FOUNDER_APP=1','-Itests/host/wifi_include','-Itests/host/include','-Isrc','-Iinclude','tests/controller/wifi_identity_test.cpp','tests/controller/wifi_glue.cpp','src/modules/wifi_ota.cpp','src/modules/rest_mode.cpp','-o',str(wifi_binary)]
    subprocess.run(wifi_command,cwd=root,check=True)
    subprocess.run([str(wifi_binary)],cwd=root,check=True)

    web_binary=Path(temp)/'founder_wifi_web_tests.exe'
    web_command=[a.compiler]+(['c++'] if a.zig else [])
    web_command+=['-std=c++17','-DGROVA_FOUNDER_APP=1','-Itests/host/wifi_include','-Itests/host/include','-Isrc','-Iinclude','tests/host/wifi_web_test.cpp','tests/controller/wifi_glue.cpp','src/modules/wifi_web.cpp','src/modules/wifi_ota.cpp','src/modules/rest_mode.cpp','-o',str(web_binary)]
    subprocess.run(web_command,cwd=root,check=True)
    subprocess.run([str(web_binary)],cwd=root,check=True)
