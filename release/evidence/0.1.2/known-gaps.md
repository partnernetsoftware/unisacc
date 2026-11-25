# unisacc 0.1.2 已知缺口、未跑范围与事故披露 (`known-gaps`)

版本: `0.1.2` (`C9b=115e5720ded24809c6daadbc9aaa7c72afe08d90`, `C10=4e8aa13c61598429fad27a78d747da1675d0093f`)
候选 APE SHA-256: `e31c413a1287a935c62df9e5c9820f91d7d760ea7e3c2c1480f99f8fc6a7c269` (`8,992,558` 字节)

## 1. 已知缺口与事故记录

(1) `C3` 进程生命周期拆出延后:
- `C3` 在 `v0.1.2` 预跑与对照中出现两项已证缺陷: `W0118` (`processcore-compile`) 报 `undefined function 'waitid'`; `devsvc-git-c-operations` 热态墙钟由 `18.4s` 退化至 `56.2s`。
- 已在 `C9` (`ec596c55bf`) 拆出 7 个运行库文件与 `tests/core-interface.tsv` 4 条投影并刷新 `0.1.2-FREEZE`, 延后至 `v0.1.3` 修复后再发。

(2) `cx/compiler/fwdmerge.cx:24` 保留 `lint:allow R3-die` 纯注释:
- 无语义变化, 属 `cx-lint R3-die` 规则不可约残留。

(3) 预跑期五次红停与修补经过 (`C6..C10`):
- `W0001` (`lane-key-probes`): `C6` (`218c0a746f`) 同步 `build/lane-inputs.tsv`;
- `W0081` (`check-gen-all-check`): `C7` (`719012b5e8`) 同步 `CGAC_GENERATORS` `44 -> 45`;
- `W0016` (`real-commit`): `C8` (`c18cd1a77a`) 接入 `devsvc-tool` 与 `precommit-tool` 材料, 热态墙钟降至 `9.4s`;
- `W0118` (`processcore-compile`) 与 `devsvc-git-c-operations`: `C9` (`ec596c55bf`) / `C9b` (`115e5720de`) 拆出 `C3` 并重跑整链;
- `W0219` (`check-daily-compile-smoke`): `C10` (`4e8aa13c61`) 仅同步 `tests/daily-compile-smoke.tsv` (`SOURCES_SID` 不变), 并按先例补跑 `W9919`/`W9987`/`W9943`。

(4) `W9942` 原始证据件丢失事故如实披露 (`incident-W9942`):
- 经过与原因: 针对 `tsv-lint-root` 的首次补跑尝试 `W9942` (`strict-W9942-tsv-lint-root.tsv`, 结果为 `UNKNOWN`, `55631ms`, `load1=2.65`, `transport=-1`, `candidate=e31c413a`) 于 `2026-11-25 21:35:48 +0800` 在 `task-9722` 中切回 `W9941` 且尚未收到 `W9943` 改号令时被人工执行 `rm -f` 误删。核对源码 `tools/dvqualify.cx:705-732` 确认 `dvqualify` 仅 truncate 写文件不 `unlink`, 丢失纯系该次人工 `rm -f` 所致。
- 丢失前取证与佐证映射: 丢失前 `cdx` 已实算原件 `sha256=935cfd93f4b25c0c6752fbd39fae8e0a608b392cb55ad278ef3bfabe88c19537`。两份包含原始输出行与丢失前 `sha256sum` 的佐证文件私有原件保存于 `~/.local/share/unisacc/evidence-0.1.2-incident/W9942/`, 对应公开脱敏副本归档于 `release/evidence/0.1.2/incident-W9942/`:
  - `unisacc-session-devsvc-flow-1611311-4coxh8oi.x.out`: `raw=b1ca023e674749d8474054d3ff024b774f98e67ef2e617a1e2edd363413f290f` -> `sanitized=9bb845264cbf6de3ba29d32c8fe8b9cf6d7793ab17f0a40a4050540bc91e8554`
  - `unisacc-session-devsvc-flow-1613789-mv1hm0dg.x.out`: `raw=b5ea6710957ce7cafe6e7b1e876a31c53fd45eef9430c2bf41a6cd7096ae00db` -> `sanitized=3d853e3995ee7ca7acf1cf4843f2c99b46f17306a5101eb031635bd0b70455ae`
- 对最终结论影响: 最终合并裁决完全不依赖已丢失的 `W9942`, 由未动过的原始失败窗 `strict-W0242.z10.tsv` (`UNKNOWN`) 与同布局预热后的正式补跑窗 `strict-W9943-tsv-lint-root.tsv` (`PASS`, `10878ms`, `assertions=2`, `sha256=16ab74a2b8fb8be7c87c01ebbabf2c6f47630f2486cf9bb4afa1ff9df7b712a2`) 独立闭环。

(5) 未跑范围:
- 全套 `s9` 未跑; 严格证据 `release/evidence/0.1.2/strict.tsv` 所列 `527` 批之外的范围均为 `UNMEASURED`。

## 2. `SUPERSEDED` 双口径对照说明

- `prerun-drive.cx -- merge` (`tools/prerun-drive.cx:936/2350`) 报告 `PRERUN_MERGE windows=251 batches=527 PASS=527 FAIL=0 UNKNOWN=0 SUPERSEDED=2`: 该计数器仅统计 `FAIL -> PASS` 覆盖的 2 项 (`W0087 slow-split` -> `W9987` 与 `W0219 check-daily-compile-smoke` -> `W9919`), 另含 1 项 `UNKNOWN -> PASS` (`W0242.z10 tsv-lint-root` -> `W9943`), 合计 3 条原始异常行被补跑 `PASS` 覆盖。
- `release-exec.cx -- --execute ... --dry` 报告 `GATE 1 STRICT PASS .../strict.tsv rows=530 expected_fail=0 superseded=3`: 统计全部 `FAIL/UNKNOWN -> PASS` 覆盖的 3 条原始异常行 (`527 + 3 = 530` 行), 两处统计口径完全一致。
