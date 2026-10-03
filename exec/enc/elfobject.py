"""ELF64 ET_REL writer delta, consuming a checked object plan.

install(E, regions, arch) creates EO.write, returning through RET. Caller supplies
registers eo_textlen, eo_nzend, eo_datalen, eo_nrel, eo_nsym, eo_nlocal,
eo_strsize and eo_tapelen. nsym counts named symbols, excluding four anchors.
Sparse byte regions: text/data/strings/tape. strings includes initial NUL.
Symbols are already in local/global/UND order; each 4-cell row is
(name string offset, ELF info, section index, section-relative value).
Relocations are 4-cell rows (offset, ELF type, ELF symbol index, signed addend).
All names, symbol/relocation decisions belong to the preceding plan delta.
No executor primitive or host writer is introduced here. Input extent/plan
validation is a caller precondition; this module guards nonnegative data
extents, nzend <= datalen, and relocation symbol indices < 4 + nsym.
"""


def install(E, regions, arch="x86_64"):
    # Stage control lives in elfobject-result.tsv / elfobject-byte.tsv (finite_rules,
    # section s1); fresh labels are declared in elfobject-fresh.tsv and allocated in
    # recorded order via E.P.fresh on an unregistered scope. Python binds only the
    # dynamic facts: the region base addresses and the ELF machine number (arch).
    # Residue: none.
    assert arch in ("x86_64", "arm64")
    assert set(regions) == {"text", "data", "strings", "tape", "symbols", "relocs"}
    from pathlib import Path
    from finite_rules import install as rules
    class _Scope:
        def __init__(self, cur): self.cur = cur
    root = Path(__file__).parent
    bindings = dict(regions, machine=183 if arch == "arm64" else 62)
    for line in (root / "elfobject-fresh.tsv").read_text().splitlines()[1:]:
        part, key, kind, prefix = line.split("\t")
        bindings[key] = E.P.fresh(_Scope(prefix), kind)
    rules(E.g, root, "elfobject", bindings, section="s1")
    return "EO.write"
