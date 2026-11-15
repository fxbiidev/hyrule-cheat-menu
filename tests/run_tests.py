"""Run portable state tests and validate a built release without proprietary inputs."""

import argparse
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from profile import BUILD_ID, make_patch
from build import RELEASE, DISTRIBUTION_FILES, distribution_bytes, source_files


def check_checksum(path):
    checksum = path.with_suffix(path.suffix + ".sha256").read_bytes()
    assert checksum == (hashlib.sha256(path.read_bytes()).hexdigest() + "  " + path.name + "\n").encode("ascii"), \
        "package checksum is missing, stale or names another archive"


def check_layout(archive, expected):
    names = archive.namelist()
    assert len(names) == len(set(names)), "duplicate ZIP paths"
    assert set(names) == set(expected), "unexpected or missing ZIP paths"
    assert archive.testzip() is None, "ZIP CRC failure"
    for entry in archive.infolist():
        assert entry.date_time == (1980, 1, 1, 0, 0, 0), "non-deterministic timestamp"
        assert entry.create_system == 3, "non-deterministic ZIP creator platform"
        assert entry.compress_type == zipfile.ZIP_DEFLATED
        assert entry.external_attr >> 16 == 0o100644, "unexpected file type or permissions"


def check_package(path):
    check_checksum(path)
    expected_patch, header = make_patch()
    assert "HCM_PROFILE_CHECKS" in header
    with zipfile.ZipFile(path) as archive:
        base = "Hyrule-Cheat-Menu/"
        module = base + "exefs/subsdk8"
        patch = base + f"exefs/{BUILD_ID}.ips"
        expected = {module, patch, *[base + name for name in DISTRIBUTION_FILES]}
        check_layout(archive, expected)
        for name in DISTRIBUTION_FILES:
            assert archive.read(base + name) == distribution_bytes(ROOT / name), name
        folder = path.parent / "Hyrule-Cheat-Menu"
        if folder.is_dir():
            folder_files = {base + file.relative_to(folder).as_posix() for file in folder.rglob("*") if file.is_file()}
            assert folder_files == expected, "install folder contains stale or unexpected files"
            for name in expected:
                assert archive.read(name) == (folder / name[len(base):]).read_bytes(), name
        assert archive.read(patch) == expected_patch
        nso = archive.read(module)
        assert nso[:4] == b"NSO0"
        assert struct.unpack_from("<I", nso, 12)[0] == 0x38
        for index in range(3):
            offset, address, size, _ = struct.unpack_from("<4I", nso, 0x10 + index * 16)
            assert address % 4096 == 0 and offset + size <= len(nso)
            assert hashlib.sha256(nso[offset:offset + size]).digest() == nso[0xa0 + index * 32:0xc0 + index * 32]
    for malformed in (b"", b"NSO0" + bytes(256)):
        try:
            make_patch(malformed)
        except ValueError:
            pass
        else:
            raise AssertionError("unsupported executable accepted")
    print("Package checksum/structure, verified profile, NSO hashes, and version rejection passed.")


def check_source_package(path):
    check_checksum(path)
    files = source_files()
    with zipfile.ZipFile(path) as archive:
        check_layout(archive, ("Hyrule-Cheat-Menu/" + name for name in files))
        for name in files:
            assert archive.read("Hyrule-Cheat-Menu/" + name) == distribution_bytes(ROOT / name), name
    print("Source ZIP allowlist, exact maintained source/document bytes and checksum passed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zig", default="zig")
    parser.add_argument("--package", type=Path, default=ROOT / "dist" / RELEASE)
    parser.add_argument("--source-package", type=Path, help="also validate the separately generated source ZIP")
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / "tests/package_test.py")], check=True, timeout=15)
    assert hashlib.sha256((ROOT / "src/destinations.inc").read_bytes()).hexdigest() == \
        "eab3838d9626e28f432d97231a1befedb309349096d68fad7cd652186fc5d59d", \
        "RC2 registry changed: review IDs, scenes, ages, spawns and exact position bytes"
    with tempfile.TemporaryDirectory(prefix="hyrule-state-tests-") as temporary:
        output = Path(temporary) / ("menu_test.exe" if sys.platform == "win32" else "menu_test")
        sources = [ROOT / "tests/menu_test.c"] + [ROOT / "src" / name for name in (
            "menu.c", "travel.c", "preferences.c", "inventory_catalog.c")]
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        *map(str, sources), "-o", str(output)], check=True)
        subprocess.run([str(output)], check=True, timeout=15)
        travel = Path(temporary) / ("travel_test.exe" if sys.platform == "win32" else "travel_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        str(ROOT / "tests/travel_test.c"), *[str(ROOT / "src" / name) for name in (
                            "menu.c", "preferences.c", "travel.c", "inventory_catalog.c")], "-o", str(travel)], check=True)
        subprocess.run([str(travel)], check=True, timeout=15)
        native_dir = Path(temporary) / "travel-runtime"
        native_dir.mkdir()
        (native_dir / "profile.h").write_text(make_patch()[1], encoding="utf-8")
        runtime = native_dir / ("travel_runtime_test.exe" if sys.platform == "win32" else "travel_runtime_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", str(native_dir), str(ROOT / "tests/travel_runtime_test.c"),
                        str(ROOT / "src/travel.c"), "-o", str(runtime)], check=True)
        subprocess.run([str(runtime)], check=True, timeout=15)
        flight = native_dir / ("flight_test.exe" if sys.platform == "win32" else "flight_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", str(native_dir), str(ROOT / "tests/flight_test.c"),
                        "-o", str(flight)], check=True)
        subprocess.run([str(flight)], check=True, timeout=15)
        for fixture, extra in (("combat_test", []), ("free_camera_test", []),
                               ("preferences_test", ["preferences.c", "travel.c"])):
            target = native_dir / (fixture + (".exe" if sys.platform == "win32" else ""))
            subprocess.run([args.zig, "cc", "-O2", "-UNDEBUG", "-Wall", "-Wextra", "-Werror",
                            "-I", str(ROOT / "src"), "-I", str(native_dir),
                            str(ROOT / "tests" / (fixture + ".c")),
                            *[str(ROOT / "src" / name) for name in extra], "-o", str(target)], check=True)
            subprocess.run([str(target)], check=True, timeout=30)

        (Path(temporary) / "profile.h").write_text("\n".join(
            f"#define {name} ((uintptr_t){function})" for name, function in (
                ("GAME_noclip_position", "mock_position"),
                ("GAME_noclip_get_position", "mock_get_position"),
                ("GAME_set_linear_velocity", "mock_velocity"),
                ("ORIGINAL_noclip_move", "mock_move"),
                ("GAME_ammo_count", "mock_count"), ("GAME_ammo_capacity", "mock_capacity"),
                ("GAME_give_ammo", "mock_give"), ("GAME_child_link", "mock_child"),
                ("GAME_action_guide_opened", "mock_guide"),
                ("GAME_consume_ammo", "mock_consume"),
                ("GAME_inventory_equipped", "mock_equipped"),
                ("GAME_wallet_cap", "mock_wallet_cap"), ("GAME_read_rupees", "mock_rupees"),
                ("GAME_write_struct_int", "mock_stat_write"),
                ("GAME_read_ui_type", "mock_ui_type"))), encoding="utf-8")
        noclip = Path(temporary) / ("noclip_test.exe" if sys.platform == "win32" else "noclip_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", temporary, str(ROOT / "tests/noclip_test.c"), "-o", str(noclip)], check=True)
        subprocess.run([str(noclip)], check=True, timeout=15)
        recovery = Path(temporary) / ("recovery_test.exe" if sys.platform == "win32" else "recovery_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        str(ROOT / "tests/recovery_test.c"), str(ROOT / "src/recovery.c"),
                        "-o", str(recovery)], check=True)
        subprocess.run([str(recovery)], check=True, timeout=15)
        movement = Path(temporary) / ("movement_safety_test.exe" if sys.platform == "win32" else "movement_safety_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        str(ROOT / "tests/movement_safety_test.c"), "-o", str(movement)], check=True)
        subprocess.run([str(movement)], check=True, timeout=15)
        resources = Path(temporary) / ("guest_resource_test.exe" if sys.platform == "win32" else "guest_resource_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        str(ROOT / "tests/guest_resource_test.c"), "-o", str(resources)], check=True)
        subprocess.run([str(resources)], check=True, timeout=15)
        access_dir = Path(temporary) / "access"
        access_dir.mkdir()
        (access_dir / "profile.h").write_text(
            "#define GAME_read_bool ((uintptr_t)mock_access_bool + GAME_BASE - main_base)\n"
            "#define GAME_ammo_count ((uintptr_t)mock_access_count + GAME_BASE - main_base)\n", encoding="utf-8")
        access = access_dir / ("travel_access_test.exe" if sys.platform == "win32" else "travel_access_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", str(access_dir), str(ROOT / "tests/travel_access_test.c"),
                        "-o", str(access)], check=True)
        subprocess.run([str(access)], check=True, timeout=15)
        items = Path(temporary) / ("items_test.exe" if sys.platform == "win32" else "items_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", temporary, str(ROOT / "tests/items_test.c"), "-o", str(items)], check=True)
        subprocess.run([str(items)], check=True, timeout=15)
        inventory = Path(temporary) / ("inventory_test.exe" if sys.platform == "win32" else "inventory_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", temporary, str(ROOT / "tests/inventory_test.c"),
                        str(ROOT / "src/inventory_catalog.c"), "-o", str(inventory)], check=True)
        subprocess.run([str(inventory)], check=True, timeout=30)

        wallet = Path(temporary) / ("wallet_test.exe" if sys.platform == "win32" else "wallet_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", temporary, str(ROOT / "tests/wallet_test.c"), "-o", str(wallet)], check=True)
        subprocess.run([str(wallet)], check=True, timeout=15)
        landing = Path(temporary) / ("landing_test.exe" if sys.platform == "win32" else "landing_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", temporary, str(ROOT / "tests/landing_test.c"), "-o", str(landing)], check=True)
        subprocess.run([str(landing)], check=True, timeout=15)
        save_guard = Path(temporary) / ("save_guard_test.exe" if sys.platform == "win32" else "save_guard_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        "-I", temporary, str(ROOT / "tests/save_guard_test.c"), "-o", str(save_guard)], check=True)
        subprocess.run([str(save_guard)], check=True, timeout=15)
        overlay = Path(temporary) / ("overlay_test.exe" if sys.platform == "win32" else "overlay_test")
        subprocess.run([args.zig, "cc", "-O2", "-UNDEBUG", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        str(ROOT / "tests/overlay_test.c"), *[str(ROOT / "src" / name) for name in (
                            "menu.c", "preferences.c", "travel.c", "inventory_catalog.c")], "-o", str(overlay)], check=True)
        subprocess.run([str(overlay)], check=True, timeout=30)
        storage = Path(temporary) / ("storage_test.exe" if sys.platform == "win32" else "storage_test")
        subprocess.run([args.zig, "cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        str(ROOT / "tests/storage_test.c"), *[str(ROOT / "src" / name) for name in (
                            "storage.c", "preferences.c", "travel.c")], "-o", str(storage)], check=True)
        subprocess.run([str(storage)], check=True, timeout=15)
        subprocess.run([str(storage), "unavailable"], check=True, timeout=15)
    check_package(args.package)
    if args.source_package:
        check_source_package(args.source_package)
