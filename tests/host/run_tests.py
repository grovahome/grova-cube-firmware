"""Run production run/rest logic on a host; no ESP, network or real NVS needed."""
import argparse
from pathlib import Path
import subprocess
import tempfile

SCENARIOS = (
    "pump_next_day",
    "pump_reservation_failure", "pump_reboot_lock", "pump_crash_before_output", "pump_rejected_start",
    "missing_preset", "missing_preset_boot", "missing_preset_paused",
    "deadline", "pause_write_failure", "resume_write_failure", "pause_clock_failure",
    "boundary", "manual_stop", "reboot_completed", "reboot_stopped", "pause_resume",
    "rest_elapsed", "future_start", "offline_time_wait", "new_explicit_start",
    "save_failure_retry", "power_loss_between_run_and_rest_save", "start_save_failure", "legacy_migration",
)

WIFI_SCENARIOS = (
    "first_boot", "legacy_fallback", "saved_precedence", "success_and_ap_close",
    "failed_trial_preserves_old", "storage_failure", "power_loss_during_trial", "dhcp_required",
    "first_save_failure", "corrupt_record", "recovery_without_run_reset", "input_and_scan",
    "millis_wrap", "ap_failure", "setup_key_write_failure", "key_failure_preserves_saved",
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
        wifi_binary = Path(directory) / "wifi_setup_tests.exe"
        command = [args.compiler] + (["c++"] if args.zig else [])
        command += ["-std=c++17", "-Wall", "-Wextra", "-Itests/host/wifi_include", "-Itests/host/include", "-Isrc", "-Iinclude",
                    "tests/host/wifi_setup_test.cpp", "src/modules/wifi_ota.cpp", "src/modules/rest_mode.cpp", "-o", str(wifi_binary)]
        subprocess.run(command, cwd=root, check=True)
        for scenario in WIFI_SCENARIOS:
            subprocess.run([str(wifi_binary), scenario], cwd=root, check=True)
        web_binary = Path(directory) / "wifi_web_tests.exe"
        command = [args.compiler] + (["c++"] if args.zig else [])
        command += ["-std=c++17", "-Wall", "-Wextra", "-Itests/host/wifi_include", "-Itests/host/include", "-Isrc", "-Iinclude",
                    "tests/host/wifi_web_test.cpp", "src/modules/wifi_web.cpp", "src/modules/wifi_ota.cpp",
                    "src/modules/rest_mode.cpp", "-o", str(web_binary)]
        subprocess.run(command, cwd=root, check=True)
        subprocess.run([str(web_binary)], cwd=root, check=True)
    print(f"{len(SCENARIOS)} firmware lifecycle scenarios passed")
    print(f"{len(WIFI_SCENARIOS)} Wi-Fi setup scenarios passed")


if __name__ == "__main__":
    main()
