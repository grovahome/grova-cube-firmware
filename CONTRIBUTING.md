# Contributing to GROVA Core

GROVA is an open-source controller project. Keep changes focused, describe the
user-visible behavior and distinguish software verification from physical acceptance.

## Set up

- Install PlatformIO Core, Python 3, a C++17 host compiler, and Node.js 22 with npm.
- Build `pio run -e grova_founder_s3` to compile the current application and install
  its dependencies, including the ArduinoJson headers used by host tests.
- Run `npm ci` and `npx playwright install chromium` for browser tests. Linux may
  require `npx playwright install --with-deps chromium` for system dependencies.

The diagnostic target is `grova_founder_s3_bringup`; `grova_core_v1` is the frozen
legacy reference. These existing build names are retained for compatibility.

## Check a change

```sh
pio run -e grova_founder_s3
python tests/controller/run_tests.py --blank-output blank.json
python tests/host/run_tests.py
python tests/controller/check_web.py
npm run test:browser -- --blank blank.json
git diff --check
```

Write the generated blank fixture to an ignored local directory when keeping it,
or delete it after testing. Host runners default to `g++`; Zig users can pass
`--compiler /path/to/zig --zig`. The browser runner accepts `--browser /path/to/browser`
and `--playwright /path/to/playwright` for existing local installations.

CI builds all three targets and also compiles the embedded pump suite with
`--without-uploading --without-testing`. Host and browser checks are hardware-free.
See [tests/README.md](tests/README.md) before working with embedded tests. Firmware
uploads, load tests and restore operations on a real controller require a separate,
deliberate hardware procedure; do not use them as routine software checks.

## Organization and style

- Current application: `src/controller/`; shared and legacy services: `src/modules/`.
- Use lowercase snake_case for C++/Python filenames and kebab-case for documentation.
- Keep text files in UTF-8 with LF line endings; `.gitattributes` enforces LF.
- Preserve the separation between board drivers, devices, automation and interface.
- Preserve configuration compatibility, sensor identity and restart protection.
- Add or update meaningful regression coverage when behavior changes; pure prose or
  filename changes do not need tests that merely repeat the implementation.
- Keep generated files, credentials and board overrides out of Git.

See [repository conventions](docs/repository-conventions.md) for details.

## Propose a change

Use a focused branch and pull request. Explain the problem, resulting behavior and
checks performed. Update the relevant documentation and the Unreleased section of
the [changelog](CHANGELOG.md). Target `main` for current controller work. The frozen `legacy/core-v1` branch
is retained as a reference and does not receive routine feature changes.
Keep physical acceptance claims limited to tests that were actually performed.

The firmware is distributed under the [GPL-3.0 license](LICENSE).
