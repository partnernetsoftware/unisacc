#!/usr/bin/env python3
"""POSIX real-process ownership regression, using the shipped compiler."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import tempfile

APP = Path(__file__).resolve().parents[1]
ROOT = APP.parents[1]
WATCH = [APP / "reload_owner.c", APP / "reload_owner.h",
         APP / "reload_owner_cli.c", ROOT / "unisacc.com"]


def hashes():
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in WATCH}


def run(mode, path, reason=None):
    result = subprocess.run(["/bin/sh", str(ROOT / "unisacc.com"),
                             "reload_owner.c", "reload_owner_cli.c", mode, str(path)],
                            cwd=APP, capture_output=True, timeout=5)
    out = result.stdout.decode()
    assert not result.stderr, result.stderr
    if reason:
        assert result.returncode == 1 and out.strip() == "FAIL: " + reason, (result.returncode, out)
    else:
        assert result.returncode == 0, (result.returncode, out)
        expected = ("PASS: parent holds, fork child refused\n"
                    "PASS: child acquired after parent release\n"
                    "PASS: process exit released ownership\n") if mode == "selftest" else "PASS: acquire/release\n"
        assert out == expected, out
    print("PASS " + mode + " " + str(path), flush=True)


def main():
    before = hashes()
    try:
        with tempfile.TemporaryDirectory(prefix="csih-owner-") as tmp:
            parent = Path(tmp)
            lock = parent / "owner.lock"
            run("selftest", lock)
            assert lock.stat().st_mode & 0o777 == 0o600
            assert lock.read_bytes() == b""
            ino = lock.stat().st_ino
            run("check", lock)
            assert lock.stat().st_ino == ino, "release must retain lock file"
            bad = parent / "bad.lock"
            bad.write_bytes(b"keep")
            bad.chmod(0o644)
            run("check", bad, "lock not owned 0600 regular file")
            assert bad.read_bytes() == b"keep" and bad.stat().st_mode & 0o777 == 0o644
            link = parent / "link.lock"
            link.symlink_to(bad)
            run("check", link, "open lock failed")
            assert link.is_symlink() and bad.read_bytes() == b"keep"
            directory = parent / "directory.lock"
            directory.mkdir(mode=0o700)
            run("check", directory, "open lock failed")
            assert directory.is_dir()
            unsafe = parent / "unsafe"
            unsafe.mkdir(mode=0o755)
            run("check", unsafe / "owner.lock", "parent not owned 0700 directory")
            try:
                (unsafe / "owner.lock").stat()
            except FileNotFoundError:
                pass
            else:
                raise AssertionError("bad parent must not create lock")
            parent_link = parent / "parent-link"
            parent_link.symlink_to(parent, target_is_directory=True)
            run("check", parent_link / "owner.lock", "open parent failed")
            assert parent_link.is_symlink() and lock.stat().st_ino == ino
            run("check", "relative.lock", "lock path not absolute or invalid length")
    finally:
        assert hashes() == before, "source/compiler changed during test"
    print("TOTAL: 8 CLI cases passed; 3 real fork phases; hashes unchanged")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
