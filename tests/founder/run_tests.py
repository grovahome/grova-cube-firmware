"""Exercise production Founder drivers with a deterministic bus and GPIO adapter."""
from pathlib import Path
import argparse,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--compiler',default='g++');p.add_argument('--zig',action='store_true');a=p.parse_args()
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='grova-founder-tests-') as temp:
    binary=Path(temp)/'founder_tests.exe'
    command=[a.compiler]+(['c++'] if a.zig else [])
    command+=['-std=c++17','-Wall','-Wextra','-Itests/founder/include','-Isrc','-Iinclude','tests/founder/test.cpp','src/founder/i2c_bus.cpp','src/founder/rtc.cpp','src/founder/outputs.cpp','src/modules/signal_registry.cpp','-o',str(binary)]
    subprocess.run(command,cwd=root,check=True)
    subprocess.run([str(binary)],cwd=root,check=True)

    api_binary=Path(temp)/'founder_app_tests.exe'
    api_command=[a.compiler]+(['c++'] if a.zig else [])
    api_command+=['-std=c++17','-Wall','-Wextra','-DARDUINOJSON_ENABLE_ARDUINO_STRING=1','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-DARDUINOJSON_ENABLE_PROGMEM=0','-Itests/founder/include','-Itests/host/include','-Isrc','-Iinclude','-I.pio/libdeps/grova_founder_s3/ArduinoJson/src','tests/founder/app_test.cpp','src/founder/app.cpp','src/founder/outputs.cpp','src/modules/rest_mode.cpp','src/modules/signal_registry.cpp','-o',str(api_binary)]
    subprocess.run(api_command,cwd=root,check=True)
    subprocess.run([str(api_binary)],cwd=root,check=True)

    device_binary=Path(temp)/'founder_device_tests.exe'
    device_command=[a.compiler]+(['c++'] if a.zig else [])
    device_command+=['-std=c++17','-Wall','-Wextra','-Itests/founder/include','-Isrc','-Iinclude','tests/founder/devices_test.cpp','src/founder/devices.cpp','src/founder/i2c_bus.cpp','src/modules/signal_registry.cpp','-o',str(device_binary)]
    subprocess.run(device_command,cwd=root,check=True)
    subprocess.run([str(device_binary)],cwd=root,check=True)
