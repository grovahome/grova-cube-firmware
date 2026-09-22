"""Run production run/rest logic on a host; no ESP, network or real NVS needed."""
import argparse
from pathlib import Path
import subprocess
import tempfile

SCENARIOS = (
    "boundary", "manual_stop", "reboot_completed", "reboot_stopped", "pause_resume",
    "rest_elapsed", "future_start", "offline_time_wait", "new_explicit_start",
    "save_failure_retry", "power_loss_between_run_and_rest_save", "start_save_failure", "legacy_migration",
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--zig", action="store_true", help="compiler is a Zig executable")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="grova-run-tests-") as directory:
        binary = Path(directory) / "local_run_tests.exe"
        command = [args.compiler] + (["c++"] if args.zig else [])
        command += ["-std=c++17", "-Wall", "-Wextra", "-Itests/host/include", "-Isrc", "-Iinclude",
                    "tests/host/local_run_test.cpp", "src/modules/local_run.cpp", "src/modules/rest_mode.cpp", "-o", str(binary)]
        subprocess.run(command, cwd=root, check=True)
        for scenario in SCENARIOS:
            subprocess.run([str(binary), scenario], cwd=root, check=True)
    print(f"{len(SCENARIOS)} firmware lifecycle scenarios passed")


if __name__ == "__main__":
    main()
