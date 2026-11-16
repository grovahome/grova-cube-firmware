# Repository conventions

This repository contains firmware, board references, firmware documentation,
regression tests and engineering tools. Keep dashboards, backend services,
marketing sites and internal handoff notes in their own repositories.

- Documentation uses lowercase kebab-case names describing its topic.
- C++ and Python filenames use lowercase snake_case; application code is in `src/controller/`.
- Hardware folders use platform names (`core-s3`); board headers use `grova_core_s3.h`.
- Tests live under `tests/`, separated by execution target; PlatformIO uses `tests/embedded/`.
- Generated `.pio/`, `dist/`, Python bytecode and local caches are ignored.
- Never commit credentials or local board overrides.

The November 2026 structure cleanup renames `src/founder/` and `tests/founder/`
to `controller/`, replaces the `founder-` documentation prefix with topic names,
and renames packaging/preview tools by purpose. No tests or firmware functionality
are removed. Existing build environments (`grova_founder_s3` and its diagnostic
variant), preprocessor switches, C++ namespaces, firmware versions and persisted
protocol/profile identifiers are retained. Changing those is a separate compatibility
migration, not a cosmetic filename operation. Existing historical evidence and commit
identifiers remain historical.

The installed board is unaffected by repository reorganization. Do not infer a new
installed version or completed hardware acceptance from these changes.

## Cleanup verification — 16 November 2026

- All three PlatformIO firmware targets compiled successfully.
- Embedded pump safety suite compiled successfully with both uploading and execution disabled; no hardware test cases were executed.
- Controller host suite, 25 lifecycle and 17 Wi-Fi scenarios passed.
- Embedded JavaScript syntax and all eight mocked browser acceptance scripts passed.
- All local Markdown documentation links resolve; Python scripts parse successfully.
- All 164 C++ source/header files match the previous contents after only the expected include/path substitutions.
- README rendered and visually inspected with the supplied original prototype photographs.
- No device access, firmware upload, configuration change or internal GROVA worktree edits during cleanup.
