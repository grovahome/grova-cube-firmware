# Tests

All test sources live under `tests/`. The suites have different execution targets;
they are not duplicate folders and are required by the continuous integration workflow.

| Directory | Purpose | Execution |
| --- | --- | --- |
| `controller/` | Current S3 drivers, sensor source/cache behavior, automation, scheduling, safety, persistence, restart handling, backup, history and web acceptance | PC; deterministic Arduino, bus and storage adapters |
| `host/` | Shared/legacy run lifecycle and Wi-Fi onboarding regressions; common adapters also used by controller tests | PC |
| `embedded/test_pump_safety/` | Real legacy pump driver timer cutoff and failure paths | Compile in CI; execute only on a prepared hardware bench |

## Hardware-free checks

From the repository root, after building the S3 target to install ArduinoJson:

```sh
python tests/controller/run_tests.py
python tests/host/run_tests.py
python tests/controller/check_web.py
```

The host runners require a C++17 compiler (`g++` by default). For Zig use
`--compiler /path/to/zig --zig`. The JavaScript check requires Node.js and accepts
`--node /path/to/node`. The runners create temporary executables and do not contact
or control a board. Fake Arduino/sensor/storage headers are test adapters, not firmware.

Install the pinned browser dependency with `npm ci`, then install Chromium with
`npx playwright install chromium`. Run the browser suites through the shared runner:

```sh
python tests/controller/run_tests.py --blank-output blank.json
npm run test:browser -- --blank blank.json
```

CI uses this same runner and generates the fixture under `.cache/browser/`.
Keep generated fixture files local. See [contributor setup](../CONTRIBUTING.md).

The `controller/check_*_ui.cjs` scripts exercise the embedded HTML with Playwright
and mocked HTTP responses. They require Playwright and a compatible Chromium browser.
Generate fixtures with `run_tests.py --blank-output blank.json --status-output status.json`,
then inspect each script's `--blank`, `--status`, `--playwright` and `--browser` options
as applicable. Fixture JSON and browser dependencies are local generated files.

## Embedded pump tests

`platformio.ini` sets `test_dir = tests/embedded`. CI only compiles:

```sh
pio test -e grova_core_v1 -f test_pump_safety --without-uploading --without-testing
```

Do not run a bare `pio test` against a connected controller during routine software
verification. Executing this suite uploads test firmware and exercises a pump output.
It requires explicit hardware-test authorization, the correct legacy board and a
disconnected pump/load. It is not the S3 pump acceptance procedure.

## Why retain these tests?

They guard against regressions such as repeated watering after restart, losing a
saved pause, accepting invalid configuration, selecting the wrong sensor or ignoring
an output cutoff. Passing tests do not replace physical sensor, electrical or load tests.
