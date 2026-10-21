#!/usr/bin/env python3
"""Seal paradigm modules: manifest + x流×y网适用性（只读结构检查，不 reseal、不跑封回放）。

真源：tests/SEAL-MODULES.md、tests/seal-modules.manifest.tsv
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/seal-modules.manifest.tsv"
IMAGE_STAGES = ROOT / "exec/pipeline/image-stages.tsv"
GOLD_DIR = ROOT / "weights/gold"

# prd §3.0b 十八网（含 pp）；lexcls/lexword 非独立 STAGES
NETS = (
    "pp", "lex", "parse", "type", "scope", "irsel", "enc", "reloc", "tyinfo", "pfconv",
    "regmap", "isel", "abi", "combo", "binsel", "prec", "opinfo", "peep",
)
FLOWS = ("e2", "e1", "e3", "e4", "prune", "lower", "elf")

STREAM_WIRED = frozenset({"e1", "e4", "prune", "elf"})
SHAPE_GAP_STREAM = frozenset({"e2", "e3", "lower"})
GRID_NETS = frozenset(n for n in NETS if n != "pp")


def load_image_product_nets() -> dict[str, str]:
    out: dict[str, str] = {}
    for line in IMAGE_STAGES.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) >= 4:
            out[parts[0]] = parts[3]
    return out


def parse_manifest() -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for line in MANIFEST.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if parts[0] == "schema":
            continue
        if parts[0] == "paradigm" and parts[1] == "id":
            continue
        if len(parts) < 6:
            raise SystemExit(f"sealmodulescheck: bad manifest row: {line}")
        rows.append(
            {
                "paradigm": parts[0],
                "id": parts[1],
                "leaf_dir": parts[2],
                "checker": parts[3],
                "gate_job": parts[4],
                "note": parts[5] if len(parts) > 5 else "",
            }
        )
    return rows


def stream_cell_capture_net(leaf_dir: Path) -> str | None:
    cell = leaf_dir / "cell.tsv"
    if not cell.is_file():
        return None
    for line in cell.read_text().splitlines():
        f = line.split("\t")
        if len(f) >= 2 and f[0] == "capture_net":
            return f[1].strip()
    return None


def check_manifest(rows: list[dict[str, str]]) -> int:
    bad = 0
    product = load_image_product_nets()
    by_paradigm: dict[str, list[dict[str, str]]] = {}
    for r in rows:
        by_paradigm.setdefault(r["paradigm"], []).append(r)

    for pid in STREAM_WIRED:
        if not any(r["paradigm"] == "stream" and r["id"] == pid for r in rows):
            print(f"sealmodulescheck: FAIL manifest missing stream row for {pid}")
            bad += 1

    for net in sorted(GRID_NETS):
        if not any(r["paradigm"] == "grid" and r["id"] == net for r in rows):
            print(f"sealmodulescheck: FAIL manifest missing grid row for {net}")
            bad += 1

    for r in rows:
        leaf = ROOT / r["leaf_dir"]
        if not leaf.is_dir():
            print(f"sealmodulescheck: FAIL missing leaf_dir {r['leaf_dir']}")
            bad += 1
            continue
        chk = ROOT / r["checker"]
        if r["checker"].endswith(".cx") and not chk.is_file():
            print(f"sealmodulescheck: FAIL missing checker {r['checker']}")
            bad += 1
        if r["paradigm"] == "stream":
            cap = stream_cell_capture_net(leaf)
            want = product.get(r["id"])
            if want and cap != want:
                print(
                    f"sealmodulescheck: FAIL {r['id']} capture_net {cap!r} != image-stages {want!r}"
                )
                bad += 1
            if not (leaf / "cell.tsv").is_file():
                print(f"sealmodulescheck: FAIL {r['leaf_dir']} missing cell.tsv")
                bad += 1
        if r["paradigm"] == "grid":
            if not (leaf / "net.tsv").is_file():
                print(f"sealmodulescheck: FAIL {r['leaf_dir']} missing net.tsv")
                bad += 1
            gold = GOLD_DIR / f"{r['id']}.tsv"
            if not gold.is_file():
                print(f"sealmodulescheck: FAIL missing gold table {gold}")
                bad += 1

    extra = sorted(ROOT.glob("tests/seals/exec-net-*"))
    manifest_grid_dirs = {r["leaf_dir"] for r in rows if r["paradigm"] == "grid"}
    for p in extra:
        rel = str(p.relative_to(ROOT))
        if rel not in manifest_grid_dirs:
            print(f"sealmodulescheck: FAIL exec-net leaf not in manifest: {rel}")
            bad += 1

    if bad:
        print(f"sealmodulescheck: manifest {bad} problem(s)")
    else:
        n_stream = len(by_paradigm.get("stream", []))
        n_grid = len(by_paradigm.get("grid", []))
        n_gap = len(by_paradigm.get("shape-gap", []))
        print(
            f"sealmodulescheck: manifest ok ({n_stream} stream, {n_grid} grid, {n_gap} shape-gap rows)"
        )
    return bad


def stream_module_cell(flow: str, net: str, product_nets: dict[str, str]) -> tuple[str, str]:
    """streamseal 能否参数化吃下该 (流, 网) 组合（结构口径，非行为覆盖）。"""
    prod = product_nets.get(flow, "")
    # 产品网名是 e1.net 等；金表网名是 lex 等。流格只绑 stage 产品网，不绑任意金表网。
    if flow in STREAM_WIRED:
        if net == "pp":
            return "n/a", "stream leaf 不消费 pp 金表"
        return "n/a", f"streamseal 已挂 {flow} 叶，capture 产品网 {prod}，非金表网 {net}"
    if flow in SHAPE_GAP_STREAM:
        if flow == "e2" and net == "pp":
            return "shape-gap", "ppleafseal 叶，非 streamseal.cx"
        return "shape-gap", "暂不合 streamseal 形状（见 SEAL-MODULES.md）"
    return "n/a", "unknown flow"


def grid_module_cell(flow: str, net: str) -> tuple[str, str]:
    """netseal 能否参数化吃下该网（与流无关，记录流列仅作 x×y 表）。"""
    if net == "pp":
        return "shape-gap", "无 exec-net-pp；pp 走 ppleafseal"
    if net in GRID_NETS:
        leaf = ROOT / f"tests/seals/exec-net-{net}"
        if leaf.is_dir() and (leaf / "net.tsv").is_file():
            return "wired", "netseal.cx + net.tsv"
        return "gap", "金表网缺 exec-net 叶"
    return "n/a", "unknown net"


def check_xy_matrix() -> int:
    product = load_image_product_nets()
    bad = 0
    gaps: list[str] = []
    wired_stream = wired_grid = 0
    for flow in FLOWS:
        for net in NETS:
            sm, snote = stream_module_cell(flow, net, product)
            gm, gnote = grid_module_cell(flow, net)
            if sm == "shape-gap" or gm == "shape-gap":
                gaps.append(f"{flow}\t{net}\tstream={sm}\tgrid={gm}\t{snote}; {gnote}")
            if sm == "wired":
                wired_stream += 1
            if gm == "wired":
                wired_grid += 1
            if sm == "gap" or gm == "gap":
                print(f"sealmodulescheck: FAIL unexpected gap {flow}×{net} stream={sm} grid={gm}")
                bad += 1

    # 适用性摘要：任意流叶 -> 仅 4 叶 streamseal；任意网 -> 17 叶 netseal（pp 除外）
    print(
        "sealmodulescheck: xy matrix "
        f"({len(FLOWS)} flows × {len(NETS)} nets = {len(FLOWS)*len(NETS)} cells)"
    )
    print(
        f"  streamseal parameterized leaves: {len(STREAM_WIRED)}/{len(FLOWS)} flows "
        f"(e1,e4,prune,elf); shape-gap: {','.join(sorted(SHAPE_GAP_STREAM))}, pp paradigm separate"
    )
    print(
        f"  netseal parameterized nets: {len(GRID_NETS)}/{len(NETS)} gold nets "
        f"(all except pp); grid wired cells per column: {len(FLOWS)}×{len(GRID_NETS)}"
    )
    if gaps:
        print(f"  shape-gap samples ({min(8, len(gaps))} of {len(gaps)}):")
        for line in gaps[:8]:
            print(f"    {line}")
    if bad:
        print(f"sealmodulescheck: xy {bad} problem(s)")
    else:
        print("sealmodulescheck: xy applicability ok (no silent gaps)")
    return bad


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true", help="manifest + xy structure (default)")
    ap.add_argument("--manifest-only", action="store_true")
    ap.add_argument("--xy-only", action="store_true")
    args = ap.parse_args()
    if not (args.manifest_only or args.xy_only):
        args.check = True
    bad = 0
    rows = parse_manifest()
    if args.check or args.manifest_only:
        bad += check_manifest(rows)
    if args.check or args.xy_only:
        bad += check_xy_matrix()
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
