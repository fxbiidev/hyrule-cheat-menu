"""Exercise archive reproducibility and rejection without a game or compiler."""

import argparse
import hashlib
from pathlib import Path
import sys
import tempfile
import warnings

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from build import release_name, source_files, write_archive
from run_tests import check_layout
import zipfile


def rejected(entries, expected, path):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", UserWarning)
        write_archive(path, entries)
    with zipfile.ZipFile(path) as archive:
        try:
            check_layout(archive, expected)
        except AssertionError:
            return
    raise AssertionError("unsafe archive layout accepted")


if __name__ == "__main__":
    for name in ("../outside.zip", "..\\outside.zip", "/outside.zip", "C:outside.zip", "plain", ".zip"):
        try:
            release_name(name)
        except argparse.ArgumentTypeError:
            pass
        else:
            raise AssertionError("unsafe package filename accepted: " + name)
    assert release_name("Hyrule-Cheat-Menu-owner-test.zip") == "Hyrule-Cheat-Menu-owner-test.zip"
    files = source_files()
    assert all(Path(name).suffix not in {".nso", ".nsp", ".xci", ".keys", ".bin", ".elf", ".exe"}
               and not name.startswith(("dist/", ".git/")) for name in files)
    with tempfile.TemporaryDirectory(prefix="hyrule-package-tests-") as temporary:
        first, second = (Path(temporary) / name for name in ("first.zip", "second.zip"))
        entries = (("README.md", b"Owner review\n"), ("src/example.c", b"void example(void) {}\n"))
        expected = {"Hyrule-Cheat-Menu/" + name for name, _ in entries}
        write_archive(first, entries)
        write_archive(second, entries)
        assert hashlib.sha256(first.read_bytes()).digest() == hashlib.sha256(second.read_bytes()).digest()
        with zipfile.ZipFile(first) as archive:
            check_layout(archive, expected)
        rejected(entries + (entries[0],), expected, first)
        rejected(entries + (("exefs/stale.ips", b"old patch"),), expected, first)
    print("Archive reproducibility, duplicate/stale path rejection, source allowlist and ZIP filename guards passed.")
