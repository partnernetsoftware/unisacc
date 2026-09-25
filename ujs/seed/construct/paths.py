"""Locate ``ujs/`` root from ``seed/construct`` (resolve symlinks)."""
from __future__ import annotations

import os

# realpath: .../ujs/seed/construct/paths.py → construct/ → seed/ → ujs/
_HERE = os.path.dirname(os.path.realpath(__file__))
_UJS_ROOT = os.path.dirname(os.path.dirname(_HERE))


def ujs_root() -> str:
    return _UJS_ROOT


def weights_dir() -> str:
    return os.path.join(_UJS_ROOT, "weights")


def practice_core() -> str:
    return os.path.join(_UJS_ROOT, "practice", "core")


def seed_vm() -> str:
    return os.path.join(_UJS_ROOT, "seed", "vm")
