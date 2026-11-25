# unisacc 0.1.2 发布说明(前端与系统层合装;2026-11-25)

状态: <CI_STATUS_PLACEHOLDER>

## 1. 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(C9b=起链提交) | `115e5720ded24809c6daadbc9aaa7c72afe08d90` (短 `115e5720de`, `0.1.2-FREEZE base=ec596c55bfef72da7d6bdb4922161a56d537ceb7`, 基线 `main 7b49a9e473`) |
| 链后修补与并入 (`post-chain-merge`) | `C10=4e8aa13c61598429fad27a78d747da1675d0093f` (仅 `tests/daily-compile-smoke.tsv`) + 并入 `origin/main=e3f12440f5f052ee105b0bba6075ec5cc6c4f582` (`d806a5b4e7`, 仅文档/研究 18 文件) + `3dfa49e57` (`research/ccogweco/broken-links.tsv:2` 路径脱敏); 实算 `SOURCES_SID` 均不变 |
| 发布 APE SHA256 (`unisacc.com`) | `e31c413a1287a935c62df9e5c9820f91d7d760ea7e3c2c1480f99f8fc6a7c269` (`8,992,558` 字节) |
| 源快照 `SID6` (`SOURCES_SID`) | `88e190bdb8bc45df16e06ff9b22d959df08adf6105fb23598edc3c25b50bc8b6` |
| `X` 提交 | `<X_COMMIT_PLACEHOLDER>` |
| CI run | `<CI_RUN_PLACEHOLDER>` |
| `pkg.bin` | `4d13b970d75732d61126e67f1f4574a70f48eb9675b95900453dfd370d3b4f1b` |
| `slice-win-arm64` | `4eb534a4bfb838e96a5227a0f1d4520f2f1732cd58ba84cde32b35bab07b219e` |
| `unisacc.com.build.json` / `build.json` | `b3b9f7ad80adc46790751f044b98d32de5edfe876837767ee1dc2ee24854863a` |
| `SHA256SUMS` | `fe6e00319dd6c5401fbb1c0678a72665f666a0bd067c611fee7efae690560b86` |

## 2. 已落产品与测试改动 (`main..4e8aa13c61`)

(1) `C0` (`724ec4890e`):新增 `plans/v0.1.2.md` 与 `build/version-seq.tsv` 第 67 槽 (`0.1.2`, `进行中`)。
(2) `C3` (`ff2d0ad9e0`) 与 `C3c` (`b405bad737`) 已在 `C9` (`ec596c55bf`) 拆出并延后至下一版: `C3` 进程生命周期 7 个产品文件在 `v0.1.2` 预跑与对照中出现两项已证缺陷 (`W0118 processcore-compile` `undefined function 'waitid'` 与 `devsvc-git-c-operations` 热态 `18.4s -> 56.2s`), 按三版整合令拆出回退。
(3) `C1` (`eab6738c09`):前端 B2 `k2-caps` 事实表化与两处漏记消费点校验 (`INCLUDE_SPLICE_MAX=4000` 与 `RELOC_MAX=262144`)。
(4) `C2` (`76a5b050e4`):锁定转发桩容量的现有通过与拒绝合同 (4 路径, 仅检查与登记)。
(5) `C5` (`479c8d0cc2`):`precommit-list-cap` 拆四片 (`precommit-list-cap`、`precommit-list-cap-tight`、`precommit-list-cap-pass`、`precommit-list-cap-deny`)。
(6) `C7` (`719012b5e8`):更新 `tools/check-gen-all-check.cx` `CGAC_GENERATORS` 44 -> 45 (`k2-caps-gen`)。
(7) `C8` (`c18cd1a77a`):为 `real-commit` 接入 `devsvc-tool` 与 `precommit-tool` 材料 (3 轮 `PASS`, 热态 `9.4s`)。
(8) `C9` (`ec596c55bf`) 与 `C9b` (`115e5720de`): 拆出 `C3` 并同步派生物, 刷新 `0.1.2-FREEZE`。
(9) `C10` (`4e8aa13c61`): 同步 `tests/daily-compile-smoke.tsv` 仅测试基线 (`SOURCES_SID` 不变)。
(10) `post-chain-merge` (`d806a5b4e7` + `3dfa49e57`): 并入 `origin/main` (`e3f12440f5`, 仅 `research/*` 与 `software_test_ai.md` 18 文件) 并用 `tools/scrub-home-paths.cx` 脱敏 `research/ccogweco/broken-links.tsv:2`, 并入前后与最终 `HEAD` 实算 `SOURCES_SID` 均为 `88e190bdb8bc45df16e06ff9b22d959df08adf6105fb23598edc3c25b50bc8b6`。

## 3. 整链、pack、warm 与严格预跑实测

- 整链实测 (`notes-evidence.cx` 原行见 `release/evidence/0.1.2/notes-evidence.txt`):
  - `APE_SUMMARY	sha=e31c413a1287a935c62df9e5c9820f91d7d760ea7e3c2c1480f99f8fc6a7c269	bytes=8992558`
  - `CHAIN_TIMES	T1=2026-11-25 19:46:08 +0800 (1795607168)	T2=2026-11-25 20:13:14 +0800 (1795608794)	delta_s=1626	delta_min=27.1`
- 不动点 (`release/evidence/0.1.2/fixedpoint-6targets.tsv`, `6/6 SAME`, `compare_attempt=246`):
  - `lnx/x86_64` (`fixedpoint/g2/lnx-x86_64/entry.elf`): `4542533b550c284189f91de4000f434be74baec4790128aee218c7a9b85402ed` (`1954382` bytes, `SAME`)
  - `lnx/arm64` (`fixedpoint/g2/lnx-arm64/entry.elf`): `f7ab55f6b8cb1466c517e0f3b25c819630317d572202e5fab224593721df055c` (`2105878` bytes, `SAME`)
  - `osx/x86_64` (`fixedpoint/g2/osx-x86_64/slice-osx-x86_64`): `cc9a98e17f2c15f582daebcaa1bd88b3d7099c83ad50aac4291ea10678f81114` (`3203554` bytes, `SAME`)
  - `osx/arm64` (`fixedpoint/g2/osx-arm64/slice-osx-arm64`): `b10fc08cc1436fd57653f245e99547eb87453bff46093b8a1fbaf43d4e0cf3a5` (`4326370` bytes, `SAME`)
  - `win/x86_64` (`fixedpoint/g2/win-x86_64/slice-win-x86_64`): `ff49983e97b4f3f8a58a5235f717704a2792d0ce4310f530395a541e37dcbc46` (`3514368` bytes, `SAME`)
  - `win/arm64` (`fixedpoint/g2/win-arm64/slice-win-arm64`): `7f29174a5232c63a0906496d03bc844b4b2e41bc3944b77ad06bbb6f96f72ee1` (`4708352` bytes, `SAME`)
- pack 与 warm: `RELEASE_RUN RESULT PACK_OK` (`PACK_APE SAME sha256=e31c413a1287a935c62df9e5c9820f91d7d760ea7e3c2c1480f99f8fc6a7c269`), `WARM RESULT READY key=415d35f1214b11233f82f5b45f17c20eaac6059eee27b4177ebab1dcb3ea2c9b targets=5`
- 严格预跑 (`release/evidence/0.1.2/strict.tsv`, `sha256=7988f2b4cdf7b9c5f5b7d25cdbc07d24a6d3d35dbd73ecfde4972ccb6652d8ca`):
  - `PRERUN_MERGE` 原行: `PRERUN_MERGE	windows=251	batches=527	PASS=527	FAIL=0	UNKNOWN=0	SUPERSEDED=2`
  - `SUMMARY` 原行: `SUMMARY	batches=527	PASS=527	FAIL=0	UNKNOWN=0	INVALID=0	total_ms=54267	layout_ms=101	shadow=off	ape_sha256=e31c413a1287a935c62df9e5c9820f91d7d760ea7e3c2c1480f99f8fc6a7c269	version=unisacc 0.1.2`
  - `SUPERSEDED` 双口径说明: `prerun-drive.cx:936/2350` 的 `SUPERSEDED=2` 仅统计 `FAIL -> PASS` (`W0087 -> W9987 slow-split`, `W0219 -> W9919 check-daily-compile-smoke` 共 2 项), 另有 1 项 `UNKNOWN -> PASS` (`W0242.z10 -> W9943 tsv-lint-root`), 合计 3 条原始异常结果行被后行 `PASS` 覆盖, 与 `release-exec STRICT PASS rows=530 expected_fail=0 superseded=3` 一致。补跑窗实测: `W9919 check-daily-compile-smoke PASS 3661ms`, `W9987 slow-split PASS 52588ms`, `W9943 tsv-lint-root PASS 10878ms assertions=2`。

## 4. 已知缺口与未跑范围

(1) `C3` 进程生命周期拆出延后: `C3` 在 `v0.1.2` 出现两项已证缺陷 (`W0118 processcore-compile` `undefined function 'waitid'`; `devsvc-git-c-operations` 热态 `18.4s -> 56.2s`), 已在 `C9` (`ec596c55bf`) 拆出 7 个运行库文件与 `tests/core-interface.tsv` 4 条投影, 延后至 `v0.1.3` 修复后再发。
(2) `cx/compiler/fwdmerge.cx:24` 保留一行 `/* lint:allow R3-die ... */` 纯注释: 无语义变化, 因 `cx-lint R3-die` 规则不可约残留。
(3) 预跑期四次红停加一次未停机批红与修补经过 (`4 prerun stops + 1 keep-going batch failure`, `C6..C10`):
    - 四次 `PRODUCT_RED_STOP` 红停:
      - `W0001` (`lane-key-probes`): `C6` (`218c0a746f`) 同步 `build/lane-inputs.tsv`;
      - `W0081` (`check-gen-all-check`): `C7` (`719012b5e8`) 同步 `CGAC_GENERATORS` `44 -> 45`;
      - `W0118` (`processcore-compile`) 与 `devsvc-git-c-operations`: `C9` (`ec596c55bf`) / `C9b` (`115e5720de`) 拆出 `C3` 并重链;
      - `W0219` (`check-daily-compile-smoke`): `C10` (`4e8aa13c61`) 同步 `tests/daily-compile-smoke.tsv`, 并按先例补跑 `W9919`/`W9987`/`W9943`;
    - 一次未停机 `keep-going` 批红:
      - `W0016` (`real-commit`): 在 `W0081` 红停备份的第二次预跑 (`~/.unisacc-worktree-backups/prerun-v012-stop-W0081-20261125/supervise.log`) 中为 `keep-going` 批红未停机, 由 `C8` (`c18cd1a77a`) 接入 `devsvc-tool` 与 `precommit-tool` 材料修复, 热态墙钟降至 `9.4s`。
(4) `W9942` 原始证据件丢失事故如实披露 (`incident-W9942`):
    - 经过与原因: 针对 `tsv-lint-root` 的首次补跑尝试 `W9942` (`strict-W9942-tsv-lint-root.tsv`, 结果为 `UNKNOWN`, `55631ms`, `load1=2.65`, `transport=-1`, `candidate=e31c413a`) 于 `2026-11-25 21:35:48 +0800` 在 `task-9722` 中切回 `W9941` 且尚未收到 `W9943` 改号令时被人工执行 `rm -f` 误删。核对源码 `tools/dvqualify.cx:705-732` 确认 `dvqualify` 仅 truncate 写文件不 `unlink`, 因此丢失纯系该次人工 `rm -f` 所致。
    - 丢失前取证与佐证留存: 丢失前 `cdx` 已实测原件摘要 `sha256=935cfd93f4b25c0c6752fbd39fae8e0a608b392cb55ad278ef3bfabe88c19537`。两份包含其原始输出行与丢失前 `sha256sum` 的佐证文件私有原件保存在 `~/.local/share/unisacc/evidence-0.1.2-incident/W9942/`, 对应公开脱敏副本归档于 `release/evidence/0.1.2/incident-W9942/` (见第 5 节 `raw_sha -> sanitized_sha` 映射)。
    - 对最终结论影响: 最终合并裁决完全不依赖已丢失的 `W9942`, 由未动过的原始失败窗 `strict-W0242.z10.tsv` (`UNKNOWN`) 与同布局预热后的正式补跑窗 `strict-W9943-tsv-lint-root.tsv` (`PASS`, `10878ms`, `assertions=2`, `sha256=16ab74a2b8fb8be7c87c01ebbabf2c6f47630f2486cf9bb4afa1ff9df7b712a2`) 独立闭环。
(5) 链后并入与 `no-home-path` 脱敏披露 (`post-chain-merge`):
    - `F=C9b=115e5720de` 起链后, 仅追加 `C10=4e8aa13c61` (`tests/daily-compile-smoke.tsv`)、并入 `origin/main=e3f12440f5` (`d806a5b4e7`, 纯 `research/*` 与 `software_test_ai.md` 18 文件)、脱敏提交 `3dfa49e57` 与本发布证据归档, 实算 `SOURCES_SID` 均等于 `88e190bdb8bc45df16e06ff9b22d959df08adf6105fb23598edc3c25b50bc8b6`。
    - 并入的 `origin/main` (`e3f12440f`) 研究页 `research/ccogweco/broken-links.tsv:2` 含未脱敏家目录路径, 触发 `no-home-path` 棘轮 `18/1`, 已用 `tools/scrub-home-paths.cx` 脱敏该一行 (提交号 `3dfa49e57`, 仅改该行路径前缀, `SOURCES_SID` 不变), `GATE 1 RATCHETS` 恢复 `19/0 PASS`。
(6) 未跑范围: 全套 `s9` 未跑; 严格证据所列 `527` 批之外的范围 `UNMEASURED`。

## 5. 证据入口与撤回方案

- 证据目录: `release/evidence/0.1.2/INDEX.tsv`、`release/evidence/0.1.2/strict.tsv`、`release/evidence/0.1.2/fixedpoint-6targets.tsv`、`release/evidence/0.1.2/ape-receipt.txt`、`release/evidence/0.1.2/prerun-windows.tsv`、`release/evidence/0.1.2/prerun-supervise.log`、`release/evidence/0.1.2/cache-manager.tsv`、`release/evidence/0.1.2/strict-W0242.z10.tsv`、`release/evidence/0.1.2/strict-W9943-tsv-lint-root.tsv`、`release/evidence/0.1.2/known-gaps.md`、`release/evidence/0.1.2/g1-dry.txt`、`release/evidence/0.1.2/g1-dry-pre-merge-20261125T2220.txt`、`release/evidence/0.1.2/notes-evidence.txt`、`release/evidence/0.1.2/evidence-rows.tsv`、`release/evidence/0.1.2/incident-W9942/`。
- `W9942` 事故佐证哈希映射 (`~/.local/share/unisacc/evidence-0.1.2-incident/W9942/` 私有原件 -> `release/evidence/0.1.2/incident-W9942/` 公开脱敏副本):
  - `unisacc-session-devsvc-flow-1611311-4coxh8oi.x.out`: `raw=b1ca023e674749d8474054d3ff024b774f98e67ef2e617a1e2edd363413f290f` -> `sanitized=9bb845264cbf6de3ba29d32c8fe8b9cf6d7793ab17f0a40a4050540bc91e8554`
  - `unisacc-session-devsvc-flow-1613789-mv1hm0dg.x.out`: `raw=b5ea6710957ce7cafe6e7b1e876a31c53fd45eef9430c2bf41a6cd7096ae00db` -> `sanitized=3d853e3995ee7ca7acf1cf4843f2c99b46f17306a5101eb031635bd0b70455ae`
  - 丢失前原件 `strict-W9942-tsv-lint-root.tsv` (`cdx` 实算): `sha256=935cfd93f4b25c0c6752fbd39fae8e0a608b392cb55ad278ef3bfabe88c19537`
- 撤回方案: `release/rollback.txt` (回退 Latest 到 `v0.1.1`, 将 `v0.1.2` 转回 draft, 永不删已公开 tag 与 release)。
- 机器提取可复现命令 (`notes-evidence.cx` 位于私有研究目录 `~/.local/share/unisacc/research/agyneon/notes-evidence.cx`, `cxmod.cx:486` 按源路径向上查找 `cxroot.tsv`, 仓外 `.cx` 依赖临时目录软链仓根 `cxroot.tsv`/`cx`/`ext` 执行, 实跑输出落盘于 `release/evidence/0.1.2/notes-evidence.txt`):
```bash
NE="$(mktemp -d)"
ln -s "$PWD/cxroot.tsv" "$NE/cxroot.tsv" && ln -s "$PWD/cx" "$NE/cx" && ln -s "$PWD/ext" "$NE/ext"
ln -s ~/.local/share/unisacc/research/agyneon/notes-evidence.cx "$NE/notes-evidence.cx"
sh ./unisacc.com "$NE/notes-evidence.cx" -- \
  --evidence-dir release/evidence/0.1.2 \
  --chain-dir ~/.cache/unisacc/chains/bg-b1/115e5720ded24809c6daadbc9aaa7c72afe08d90
```
