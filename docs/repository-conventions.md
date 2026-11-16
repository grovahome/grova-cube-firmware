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

## Branch organization

- `main`: current S3 development and GitHub default branch.
- `legacy/core-v1`: frozen V1 reference; keep its history and freeze tags intact.
- Short-lived development branches: `codex/<topic>` for Codex work; use descriptive
  topic names and remove merged branches when no longer needed.
- Tags identify historical releases/baselines; do not move or rename published tags.

On 16 November 2026, GitHub branches were renamed from `founder-pcb-bringup` to
`main` and from `grova-core-v1` to `legacy/core-v1`. Commit history was preserved.
The obsolete local `main` at `736e886` was already an ancestor of the current line;
only its redundant local branch reference was removed. Stale remote-tracking
references were pruned and `origin/HEAD` was updated to `origin/main`.
Historical engineering records retain the branch names used at those checkpoints.
The separate internal GROVA repository retains its own branch organization.

## Cleanup verification — 16 November 2026

- All three PlatformIO firmware targets compiled successfully.
- Embedded pump safety suite compiled successfully with both uploading and execution disabled; no hardware test cases were executed.
- Controller host suite, 25 lifecycle and 17 Wi-Fi scenarios passed.
- Embedded JavaScript syntax and all eight mocked browser acceptance scripts passed.
- All local Markdown documentation links resolve; Python scripts parse successfully.
- All 164 C++ source/header files match the previous contents after only the expected include/path substitutions.
- README rendered and visually inspected with the supplied original prototype photographs.
- No device access, firmware upload, configuration change or internal GROVA worktree edits during cleanup.
