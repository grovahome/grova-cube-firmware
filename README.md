# GROVA Core

### Your environment. Your rules. Local control.

GROVA is an open-source controller project for growing spaces, greenhouses and
other environments where sensors and equipment need to work together. It brings
monitoring, lighting, ventilation and watering into one configurable system,
with control running directly on the device.

Set a schedule, respond to a sensor or operate equipment yourself. Start with a
single sensor and fan, then build a setup that fits your space.

[![Visit GROVA Home](docs/assets/visit-grova-home.svg)](https://grova.carrd.co/)

[Explore the features](#what-you-can-do-with-grova) ·
[Hardware](#built-around-real-hardware) ·
[Build the firmware](#build)

![GROVA Core prototype in its enclosure](docs/hardware/core-s3/photos/core-s3-enclosure.jpg)

*GROVA Core development prototype with its printed enclosure and accessible connections.*

## GROVA at a glance

| | What it means for your setup |
| --- | --- |
| **Monitor** | Read connected sensors and follow changes in temperature, humidity and other supported measurements. |
| **Automate** | Control compatible equipment manually, on a schedule or in response to a specific sensor. |
| **Stay local** | Use the controller's web interface on your phone or computer; automation runs on the controller without a cloud connection. |
| **Adapt** | Assign compatible devices to physical ports and choose their purpose, settings and control mode. |
| **Understand** | See measurements, warnings and rule diagnostics so you can follow why an action is requested or blocked. |
| **Own** | Inspect the source, keep control of your configuration and develop the system around your needs. |

## What you can do with GROVA

### Keep air moving when it is needed

Use fans for circulation, intake or exhaust. Run them at a chosen level, use daily
time windows or let a selected sensor determine demand. Supported variable outputs
allow percentage control, with configurable targets, limits and fallback behavior.

### Give lighting and equipment a routine

Create multiple daily operating windows or repeating intervals for compatible
equipment. Each device has its own control mode, so scheduled lighting can work
alongside sensor-driven ventilation and manually operated outputs.

### Plan controlled watering

Configure bounded pump or valve pulses with minimum pauses between runs. Persistent
execution records help prevent a pulse that already started from being repeated
after a restart. Actual water delivery still depends on the connected plumbing and
hardware; GROVA does not infer water flow from a command.

### See what your system is doing

Check current readings, recent measurement history and events in the local browser
interface. Rule diagnostics show the selected sensor, requested action and any
execution restrictions. Export events and back up device and rule settings for
later restoration.

## A flexible foundation

GROVA is designed around **devices, sensors and rules**, rather than one fixed crop
or growing method. A propagation shelf, an indoor growing cabinet, a small greenhouse
or an environmental test setup can use the same basic approach:

**Connect a supported sensor → configure compatible equipment → choose how it should respond.**

The aim is practical automation that remains understandable as a setup grows:
fewer separate timers, less routine switching and one place to see how equipment
and environmental measurements relate.

## Built around real hardware

The current GROVA Core platform uses an **ESP32-S3**, with **16 MB flash** and
**8 MB PSRAM**. The board provides:

- **12 V and 5 V switched outputs** for compatible equipment.
- **Two PWM fan connections with tachometer inputs** for variable fan control.
- **Two 0–10 V outputs** for compatible controlled devices.
- **Eight multiplexed I²C channels** for sensor expansion.
- **Battery-backed RTC** for timekeeping across power interruptions.
- **USB-C and Wi-Fi**, plus connections for display, encoder and further expansion.

![GROVA Core ESP32-S3 PCB in its open enclosure](docs/hardware/core-s3/photos/core-s3-board.jpg)

*The controller PCB in its open enclosure. [View from above](docs/hardware/core-s3/photos/core-s3-top.jpg).*

[Pinout and hardware notes](docs/hardware/core-s3/README.md) ·
[Circuit schematic (PDF)](docs/hardware/core-s3/SCH_Schematic1_2026-11-01.pdf)

## Development status

GROVA is under active development. The current S3 firmware includes local Wi-Fi
setup, the browser interface, manual/scheduled/sensor-driven device control,
diagnostics, events, measurement history, backup/restore and OTA updates.

Hardware validation is ongoing, including electrical output and load tests,
sensor fault/recovery tests and long-term RTC operation. Display/encoder integration
is deferred. Expansion connectors describe hardware possibilities; driver support
and validation vary by interface. Multi-day growing programs and additional interface
integrations are future work.

Detailed release records and acceptance results live in
[software validation](docs/software-validation.md) and
[board bring-up](docs/board-bringup.md). The legacy V1 firmware remains a separate,
frozen reference target.

## Development branches

| Branch | Purpose |
| --- | --- |
| `main` | Current GROVA Core ESP32-S3 development; default branch |
| `legacy/core-v1` | Frozen legacy ESP32 reference with its validated toolchain |

Hardware targets remain separate build environments. Historical release and freeze
tags retain their original names and commits.

## Build

Install PlatformIO Core and run from the repository root:

```sh
pio run -e grova_founder_s3
```

| Target | Purpose |
| --- | --- |
| `grova_founder_s3` | Current integrated ESP32-S3 application; default build |
| `grova_founder_s3_bringup` | Isolated S3 hardware diagnostic firmware |
| `grova_core_v1` | Frozen legacy ESP32 application |

See [firmware profiles](docs/firmware-profiles.md) for target details and
[Wi-Fi setup](docs/wifi-setup.md) for connecting the controller.

## Repository layout

| Path | Contents |
| --- | --- |
| `src/controller/` | Current S3 application, devices, automation and embedded web interface |
| `src/bringup/` | Isolated hardware diagnostic application |
| `src/modules/` | Shared services and legacy V1 modules |
| `include/boards/` | Hardware pin profiles |
| `docs/` | Firmware contracts, hardware references and validation records |
| `tests/controller/` | Hardware-free controller, policy and web tests |
| `tests/host/` | Shared/legacy lifecycle and Wi-Fi host tests, including common test adapters |
| `tests/embedded/` | PlatformIO board tests; hardware execution requires a prepared bench |
| `tools/` | Local web preview and first-flash packaging tools |

## Tests

```sh
python tests/controller/run_tests.py
python tests/host/run_tests.py
python tests/controller/check_web.py
```

Host tests require a C++17 compiler; web syntax checks require Node.js. Controller
tests also use the ArduinoJson headers installed by the S3 build. For Zig, pass
`--compiler /path/to/zig --zig`. Browser acceptance scripts and the embedded test
procedure are explained in [tests/README.md](tests/README.md).

CI compiles all three firmware targets, compiles the embedded pump tests without
uploading or executing them, runs the host suites, checks embedded JavaScript and runs all eight mocked browser suites.
Automated checks complement physical acceptance; they do not establish water flow,
electrical levels or correct load behavior.

## Documentation

- [Documentation index](docs/README.md)
- [Architecture](docs/architecture.md) and [API](docs/api.md)
- [Hardware and bring-up](docs/board-bringup.md)
- [Software validation](docs/software-validation.md)
- [Legacy V1 freeze](docs/v1-freeze.md)
- [Contributing](CONTRIBUTING.md)
- [License](LICENSE) and [change log](CHANGELOG.md)
