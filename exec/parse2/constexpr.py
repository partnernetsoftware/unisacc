"""Constant-expression delta assembled from constexpr-manifest.tsv."""
from pathlib import Path
import assemble

def install(E, P, levels, ops, enum_values, enum_defined):
    assemble.run(Path(__file__).parent / "constexpr-manifest.tsv", E, P, {},
                 dict(enum_values=enum_values, enum_defined=enum_defined))
