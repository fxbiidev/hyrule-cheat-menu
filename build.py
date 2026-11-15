"""Build the standalone ARM64 module using Zig and Python 3.9+."""

import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zipfile

sys.dont_write_bytecode = True
from profile import BUILD_ID, make_patch


ROOT = Path(__file__).resolve().parent
RELEASE = "Hyrule-Cheat-Menu-v1.0.0.zip"
DISTRIBUTION_FILES = (
    "README.md", "CHANGELOG.md", "RELEASE_NOTES.md", "TEST_REPORT.md", "RELEASE_READINESS.md",
    "PROGRESS.md", "third_party/Fast-Travel-LICENSE.txt", "docs/destination-coverage.csv",
    "docs/build177-verification.json",
    "docs/fly-landing-verification.json",
    "docs/screenshots/cheats.png", "docs/screenshots/fast-travel.png", "docs/screenshots/flight.png",
    "docs/screenshots/underwater.png",
    "docs/screenshots/regions.png", "docs/screenshots/noclip.png",
    "docs/screenshots/pause.png",
    "docs/screenshots/build177-cheats.png", "docs/screenshots/build177-travel.png",
    "docs/screenshots/build177-noclip.png", "docs/screenshots/build177-grounded-noclip.png",
    "docs/screenshots/release-cheats.png", "docs/screenshots/release-travel.png",
    "docs/screenshots/release-landing.png",
)


def distribution_bytes(path):
    data = path.read_bytes()
    # Keep archives identical across Windows and CI checkouts. Binary captures
    # remain exact; only text line endings are canonicalized.
    text = path.suffix in (".md", ".txt", ".csv", ".json", ".c", ".h", ".inc", ".S", ".ld", ".py", ".yml") or \
           path.name in (".gitignore", ".gitattributes")
    # The original registry has a pinned byte hash, including its CRLF lines.
    return data.replace(b"\r\n", b"\n") if text and path.name != "destinations.inc" else data


def release_name(value):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*\.zip", value):
        raise argparse.ArgumentTypeError("release name must be a ZIP basename, without directories")
    return value


def source_files():
    # Include maintained source and review material only. Private executable
    # inputs, build products, saves, preferences and Git internals stay outside.
    files = {"build.py", "profile.py", ".gitignore", ".gitattributes",
             ".github/workflows/build.yml", *DISTRIBUTION_FILES}
    for directory, suffixes in (("src", {".c", ".h", ".inc", ".S", ".ld"}),
                                ("tests", {".c", ".py"})):
        files.update(path.relative_to(ROOT).as_posix() for path in (ROOT / directory).rglob("*")
                     if path.is_file() and path.suffix in suffixes)
    return tuple(sorted(files))


def write_archive(package, entries):
    with zipfile.ZipFile(package, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, data in entries:
            entry = zipfile.ZipInfo("Hyrule-Cheat-Menu/" + name, date_time=(1980, 1, 1, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, data)
    package.with_suffix(package.suffix + ".sha256").write_bytes(
        (hashlib.sha256(package.read_bytes()).hexdigest() + "  " + package.name + "\n").encode("ascii"))


def make_nso(elf):
    if elf[:6] != b"\x7fELF\x02\x01" or struct.unpack_from("<H", elf, 18)[0] != 183:
        raise ValueError("expected a little-endian ARM64 ELF")
    phoff = struct.unpack_from("<Q", elf, 32)[0]
    phsize, phnum = struct.unpack_from("<HH", elf, 54)
    loads = []
    for i in range(phnum):
        kind, flags, offset, vaddr, _, size, memsize, _ = struct.unpack_from(
            "<IIQQQQQQ", elf, phoff + i * phsize
        )
        if kind == 1:
            loads.append((flags, vaddr, elf[offset:offset + size], memsize - size))
    if [segment[0] for segment in loads] != [5, 4, 6]:
        raise ValueError("expected RX, R, RW load segments")
    header = bytearray(0x100)
    header[:4] = b"NSO0"
    struct.pack_into("<I", header, 12, 0x38)
    payload = bytearray()
    for i, (_, addr, data, bss) in enumerate(loads):
        if addr % 0x1000 or (i < 2 and bss) or not data:
            raise ValueError("invalid NSO segment")
        struct.pack_into("<IIII", header, 0x10 + 0x10 * i, 0x100 + len(payload), addr,
                         len(data), bss)
        struct.pack_into("<I", header, 0x60 + 4 * i, len(data))
        header[0xa0 + 32 * i:0xc0 + 32 * i] = hashlib.sha256(data).digest()
        payload.extend(data)
    header[0x40:0x54] = hashlib.sha1(payload).digest()
    return header + payload


def build_module(zig, output, header):
    with tempfile.TemporaryDirectory(prefix="hyrule-build-") as temporary:
        elf = Path(temporary) / "hyrule.elf"
        (Path(temporary) / "profile.h").write_text(header, encoding="utf-8")
        objects = []
        for source in ("start.S", "geometry_call.S", "guest.c", "menu.c", "overlay.c", "travel.c", "travel_runtime.c",
                       "preferences.c", "storage.c", "memory.c", "flight.c", "landing.c", "noclip.c", "items.c", "ammo_guard.c", "wallet.c", "geometry.c", "recovery.c", "movement_safety.c", "travel_access.c", "save_guard.c"):
            obj = Path(temporary) / (source + ".o")
            subprocess.run([
                zig, "cc", "-target", "aarch64-linux-none", "-O2", "-fPIC", "-fvisibility=hidden",
                "-ffreestanding", "-fno-math-errno", "-fno-stack-protector", "-fno-unwind-tables", "-mcpu=cortex_a78",
                "-fno-asynchronous-unwind-tables", "-Wall", "-Wextra", "-Werror", "-I", temporary, "-c",
                str(ROOT / "src" / source), "-o", str(obj),
            ], check=True)
            objects.append(str(obj))
        subprocess.run([
            zig, "ld.lld", "-shared", "--hash-style=sysv", "-z", "max-page-size=4096",
            "--no-undefined", "--init=hyrule_init", "-T", str(ROOT / "src/module.ld"),
            "-o", str(elf), *objects,
        ], check=True)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(make_nso(elf.read_bytes()))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--main", type=Path, help="optional private uncompressed supported-game NSO for additional validation")
    parser.add_argument("--zig", default="zig")
    parser.add_argument("--out", type=Path, default=ROOT / "dist/Hyrule-Cheat-Menu")
    parser.add_argument("--release-name", type=release_name, default=RELEASE,
                        help="local ZIP basename; a filename does not establish release readiness")
    parser.add_argument("--source-archive", action="store_true", help="also create a separate source ZIP and checksum")
    args = parser.parse_args()
    ips, header = make_patch(args.main.read_bytes() if args.main else None)
    module = args.out / "exefs/subsdk8"
    patch = args.out / f"exefs/{BUILD_ID}.ips"
    build_module(args.zig, module, header)
    patch.write_bytes(ips)
    documents = []
    for relative in DISTRIBUTION_FILES:
        destination = args.out / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(distribution_bytes(ROOT / relative))
        documents.append(destination)
    package = args.out.parent / args.release_name
    write_archive(package, ((file.relative_to(args.out).as_posix(), file.read_bytes())
                            for file in (module, patch, *documents)))
    if args.source_archive:
        source_package = package.with_name(package.stem + "-source.zip")
        write_archive(source_package, ((name, distribution_bytes(ROOT / name)) for name in source_files()))
    print(args.out)
    print(package)
