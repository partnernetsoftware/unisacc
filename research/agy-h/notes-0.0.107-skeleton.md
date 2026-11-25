# unisacc 0.0.107 发布说明骨架 (对标 origin/main release/notes-0.0.106.md v4)

用途: 作为第二车 (`v0.0.107`) 在 `release/notes-0.0.107.md` 生成与 `Y1` 回填时的结构模板、变化占位、数据来源与门禁核对清单。
结构真源: `origin/main:release/notes-0.0.106.md` (261 行, Y1 提交 `32f26cd024`)、`tools/release-prep.cx:303-326`、`tools/notes-fill.cx:56-70`。
生成态裁剪规则: 正式落盘 `release/notes-0.0.107.md` 时只保留第 1 节展示的七章结构(如无链后补跑则精简第 5 章), 去除本骨架中的元说明与回填标注括号。

---

## 1. 对标 notes-0.0.106.md (v4) 的章节结构与固定文本占位

以下直接给出 `release/notes-0.0.107.md` 的成文骨架与必须原样保留的固定句、占位符:

```markdown
# unisacc 0.0.107 发布说明

状态：同一候选通过六格原生 CI，结论为 QUALIFIED（run <CI_RUN_ID>）。本版未执行全套 s9，未取得全套 s9 资格；同候选六格原生 CI 与全套 s9 是两件事，不得以 CI 六格替代全套 s9 资格裁决。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(构造输入、发布标签目标) | <COMMIT_40HEX> |
| 发布 APE SHA256(unisacc.com) | <APE_SHA256> |
| CI run | <CI_RUN_ID> |
| (可选扩展行: 若照 notes-0.0.105 附全量资产) 源快照 SID | <SOURCES_SID_64HEX> |
| (可选扩展行) pkg.bin | <PACKAGE_SHA256> |
| (可选扩展行) slice-win-arm64 | <WIN_ARM64_SHA256> |
| (可选扩展行) unisacc.com.build.json / build.json | <RECORD_SHA256> |
| (可选扩展行) SHA256SUMS | <CHECKSUMS_SHA256> |

## 产品改动

### A. 既有车二底盘 (v0.0.107 分支已落 7 笔提交, baseline d750ad123c)

(1) B1 前端声明符与重声明 (1/3..3/3): 全局对象重声明类型/尺寸冲突与函数/对象双向种类冲突(ad5c3e72a6、1028de9b92), 7.2 文件域不完整数组暂定定义跨定义前瞻与末尾 1 元素补齐以及 t10 typedef 数组形参退化与 SH.PREFIX 维度平移(a183bd4e5d), B1 重放与 v2 覆盖增量(ac483247c7); t04/t05 保持守卫拒绝。
(2) 前置容量扩展: CXM_MAXU 256 补丁合入(9199f8dbbf, 38ea37666a)。
(3) 转发桩边界与测试拆片计数: F1 转发桩边界与覆盖增量(ac483247c7), module-units 拆片与 count-v2 增量(a7b2dd86c5), cxmod-tri 拆片与 count-v2 增量(351be964ef)。
(4) 不动点证据工具 v4: 合入不动点收据工具更新(d750ad123c)。

### B. 合入 0.0.106 发布提交与车上六笔 (阶段 A 同步占位)

(5) 同步 origin/main (v0.0.106 发布与车上修复): <0.0.106 公开提交与六笔非 SID 修复合入 commit, 含 tests/env-semantics.tsv 等活表同步与派生表重算结果 stale=0 hard=0 drift=0>。

### C. 107 与 106 之间追加集变化占位 (待逐笔入车后按实际合入状态保留或删除)

(6) 提交门截断与保护: precommit-list-cap v7 (<COMMIT_OR_SKIP>, 8b9e7eec...)。
(7) 进程回收 P0 修复: chain-step-reaped-kill (<COMMIT_OR_SKIP>, 1271d5c8...)。
(8) 选择器复用: dvqualify-selector-reuse-v3 (<COMMIT_OR_SKIP>, 61ba39ee...)。
(9) 发布工具长路径修复: r39c-release-exec-sha (<COMMIT_OR_SKIP>, 2e9e41e4..., 解除 0.0.106 W0150 200B sha_file 截断)。
(10) 无生产者挂账清零候选: producer-cx-booter-ape (3a) 与 producer-tools-default-regression (3b) (<COMMIT_OR_SKIP>, 4a3aa9ca...)。
(11) 封口与宏扫描: seal-precheck-v2 (<COMMIT_OR_SKIP>) 与 check-tool-material-macro v3 (<COMMIT_OR_SKIP>, 依赖 106 pinscan)。
(12) WP4 与生成器负控夹具: wp4-install-ape v4 (2e4a07d6...)、wp4-material v2 (8958b5bf...)、u08-source-missing v3 (32affee7...)、gen-all-drift-fixture-tests (<COMMIT_OR_SKIP>)。
(13) r39e 候选与 cc 遮蔽三批修复及 0.0.107 准入收口: r39e/cc 遮蔽三批 (<COMMIT_OR_SKIP>) 与 v0107-criteria (7d3dc50c...)。

### D. 覆盖声明固定句

(<NEXT_IDX>) 覆盖声明: 本版整链全绿(REV=<COMMIT_40HEX>); 预跑原 merge 为 <PRE_BATCHES> 批 PASS=<PRE_PASS> FAIL=<PRE_FAIL> UNKNOWN=<PRE_UNKNOWN>, <若无后补则写无后补; 若有同候选后补则写同候选后补 N 条原 FAIL 行后合并为 PASS=<FINAL_PASS> FAIL=<FINAL_FAIL> UNKNOWN=<FINAL_UNKNOWN>>(见严格证据与链后补跑节); 此段是覆盖事实, 不构成全批资格声明。

## 严格证据

证据 release/evidence/v0.0.107-strict-<DATE>.tsv:rows=<ROWS>,expected_fail=<EXPECTED_FAIL>,superseded=<SUPERSEDED>; 预跑原 merge 为 <PRE_BATCHES> 批; <合并补跑 PASS 行后或原表> SUMMARY 为 batches=<FINAL_BATCHES>、PASS=<FINAL_PASS>、FAIL=<FINAL_FAIL>、UNKNOWN=<FINAL_UNKNOWN>、INVALID=<FINAL_INVALID>。FAIL 批与上一版对照见 release/evidence/0.0.107/expected-fail.tsv, 标 NEW 的不自动放行。

## 已知缺口与未验证范围

(1) 严格证据批次 cx-booter-ape 固定条目: 若 3a 未入车或仍缺生产者, 保持 "严格证据批次 cx-booter-ape 状态 UNKNOWN(无 RAW BATCH_RUN 行), 未覆盖, 不记 PASS; 0.0.106/0.0.105 同形, 属预期 UNKNOWN, 不是 INVALID" (若 3a 已入车且严格表实跑转 PASS, 改入产品改动节记录消缺并在此注明已清零, 不伪造缺口)。
(2) 严格证据批次 tools-default-regression 固定条目: 若 3b 未入车或仍缺生产者, 保持 "严格证据批次 tools-default-regression 状态 UNKNOWN(无 RAW BATCH_RUN 行), 未覆盖, 不记 PASS; 0.0.106/0.0.105 同形, 属预期 UNKNOWN, 不是 INVALID" (若 3b 已入车且严格表实跑转 PASS, 同上据实切换)。
(3) s9 全套本版未跑; 严格证据所列 <FINAL_BATCHES> 批之外的批次 UNMEASURED。
(4)..(M) build/pkg-gaps.tsv 全部 OPEN 行(由 tools/release-prep.cx:316-322 自动抽取生成, 格式 `(%d) <task>:<need>(build/pkg-gaps.tsv,OPEN)` )。
(M+1) U05(provenance) 未测量固定条目: 若本车 CI osx 格仍无带 sha 对照的 cache-manager 实跑行, 保留 "U05(provenance)在本车 workflow 无取证步骤, 记未测(UNMEASURED): 0.0.105 的 cache-manager.tsv 只有 sha 与备份路径, 无 macOS 实跑; 关闭条件为 CI osx 格出现一次带 sha 对照的实跑行, 本车未做。(build/state-unverified.tsv U05 行; research/ccogweco/unverified-classification.md U05 行)"。
(M+2, 按实触发) 本车 CI 重试或非一次通过披露(如有, 对标 0.0.106 第 (192) 条写出首个 run ID、失败格、源码行与重派发后 run ID)。

## 链后补跑与后补证据(R40 B-t / R36 路径 B)

<若无链后补跑: 本版零 chain_rev 之后合入、零 SUPERSEDED, 无此节或只保留"无(chain_rev=<COMMIT_40HEX> 即最终 HEAD, 零 SUPERSEDED)">。
<若有链后合入非 SID 修复与补跑: 照 notes-0.0.106.md:233-251 写 chain_rev -> post_chain_rev、放行路径列表、SOURCES_SID 与 APE 逐字不变核对、各批 RAW FAIL 原行定因与 SUPERSEDED 计数、串行/并发复跑文件清单>。

## 不动点证据

fixedpoint-6targets.tsv 含六目标行与原链 compare 原行; 链 compare 相位 slices=same count=6、objects=same count=102、kernels=same count=2、product=same, 六目标 g2/g3 终片 6/6 逐对 sha256 相同。证据: release/evidence/0.0.107/fixedpoint-6targets.tsv 与 ape-receipt.txt。

## 证据入口

release/evidence/0.0.107/INDEX.tsv; release/evidence/v0.0.107-strict-<DATE>.tsv; release/evidence/0.0.107/expected-fail.tsv。撤回方案: release/rollback.txt (回退 Latest 到 v0.0.106, 不删标签与 release)。
```

---

## 2. 各章节数据来源与提取/生成命令原行

| 章节 | 主要数据来源文件 | 获取/生成命令原行 | 备注 |
|---|---|---|---|
| 顶头状态行 | `release/candidate.tsv` (`ci_run_id`)、`release/evidence/0.0.107/ci-<CI_RUN_ID>.json`、`release/decision.txt` | `sh ./unisacc.com tools/release-decide.cx -- --candidate release/candidate.tsv --evidence release/evidence.tsv --assets <LAYOUT>` | 未跑全套 s9 时固定携带 `同候选六格原生 CI 与全套 s9 是两件事` |
| 真实身份 | `release/candidate.tsv`、`release/evidence/0.0.107/fixedpoint-6targets.tsv`、`release/evidence/0.0.107/ape-receipt.txt`、`OUT/layout/SHA256SUMS` | `sh ./unisacc.com tools/release-run.cx -- --pack . release/candidate.tsv <SRC_DIR> <OUT> --chain-out <CHAIN> --commit <COMMIT_40HEX>` | 回填前由 `tools/release-prep.cx:307-308` 填 `commit`、`ape_sha256`、`CI run` 占位 |
| 产品改动 (A/B/C/D) | `v0.0.107` 提交历史、`origin/main..v0.0.107`、车二 14 份补丁实际合入记录 | `MB=$(git merge-base origin/main v0.0.107) && git log --oneline "$MB..v0.0.107"` | 未合入车二的补丁须从改动清单删除或标未入车, 不得虚报 |
| 严格证据 | `release/evidence/v0.0.107-strict-<DATE>.tsv`、`release/evidence/0.0.107/expected-fail.tsv` | `sh ./unisacc.com tools/release-run.cx -- --prepare 0.0.107 --layout <OUT>/layout --strict release/evidence/v0.0.107-strict-<DATE>.tsv --commit <COMMIT_40HEX> --prev 0.0.106` | `tools/release-prep.cx:143-216, 309-312` 读取 `SUMMARY` 与全表 9 列及 `RAW BATCH_RUN` 行 |
| 已知缺口与未验证范围 | `release/evidence/v0.0.107-strict-<DATE>.tsv` (`UNKNOWN` 行)、`build/pkg-gaps.tsv` (`OPEN` 行)、`build/state-unverified.tsv` (`U05`)、`research/ccogweco/unverified-classification.md` | 上同 `release-run.cx -- --prepare` (`tools/release-prep.cx:313-322` 自动生成 `(1)..(M)` 条目) + `grep -n '^U05' build/state-unverified.tsv` | 检查 `cx-booter-ape`、`tools-default-regression` 与 `U05` 固定条目 |
| 链后补跑与后补证据 | `release/evidence/0.0.107/path-b-proof.md`、`release/evidence/0.0.107/post-chain-merge-provenance.txt` | `sh ./unisacc.com tools/release-exec.cx -- --dry release/candidate.tsv --layout <OUT>/layout` (或实跑 `Y1` 日志) | 仅当 `HEAD != chain_rev` 且发生 `SUPERSEDED` 补跑时填报 |
| 不动点证据 | `release/evidence/0.0.107/fixedpoint-6targets.tsv`、`release/evidence/0.0.107/ape-receipt.txt`、`chain-run` stdout `compare` 4 行 | `sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev <COMMIT_40HEX>` | 核 `slices=same count=6`、`objects=same count=102`、`kernels=same count=2`、`product=same` |
| 证据入口与回滚 | `release/evidence/0.0.107/INDEX.tsv`、`release/rollback.txt`、`release/notes-0.0.106.md` | `sh ./unisacc.com tools/notes-fill.cx -- --notes release/notes-0.0.107.md --candidate release/candidate.tsv --decision release/decision.txt [--evidence release/evidence.tsv] --out <OUT_NOTES>` | 回滚目标 APE sha256 从 `release/notes-0.0.106.md` 抽取 (`tools/release-prep.cx:291-302`) |

---

## 3. 107 与 106 之间的变化占位 (车二底盘 + 追加集 14 份清单)

注: 队列文件 `research/agyneon/car2-review-queue.md` (镜像 `~/.local/share/unisacc/research/agyneon/car2-review-queue.md`) 送审总表列 `#1..#13` 共 13 序号, 其中第 13 条合并了 `r39e` 候选与 `cc` 遮蔽三批修复; 同目录 `car2-commit-sequence.md` 阶段 B 为 12 个执行序号(第 3 项含 `3a`+`3b` 共 13 份)加阶段 C `v0107-criteria.patch`, 合计 14 份交付项/补丁。

### (1) 车二既有底盘 (7 笔已在 v0.0.107 HEAD d750ad123c)

- `ad5c3e72a6` + `1028de9b92`: 全局对象重声明类型/尺寸冲突与函数/对象双向种类冲突修复与 `c99-global-redecl-conflict` 产物输入登记
- `9199f8dbbf`: 合入 `CXM_MAXU 256` 补丁 (`38ea37666a`)
- `a183bd4e5d`: B1 补丁 `2/3` (`7.2` 文件域不完整数组暂定定义与 `t10` 形参退化)
- `ac483247c7`: `F1` 转发桩边界+覆盖增量 与 B1 `2/3`、`3/3` 重放+`v2` 增量
- `a7b2dd86c5`: `module-units` 拆片与 `count-v2` 增量
- `351be964ef`: `cxmod-tri` 拆片与 `count-v2` 增量并重生表
- `d750ad123c`: 合入不动点证据工具 `v4`

### (2) 追加集 14 份清单占位对照表 (逐笔入车后回填合入 sha 或删除)

| 骨架序号 | 队列来源与编号 | 补丁/交付路径 | 已知 sha256 (来源: `car2-review-queue.md` / `car2-commit-sequence.md`) | 变化定位 | 必须同步核对项 |
|---|---|---|---|---|---|
| 1 | 队列 `#1` / 阶段 B `(5)` | `research/agyneon/precommit-list-cap.patch` (v7) | `8b9e7eec6bbb2fcc0901287220e71172529523834c0b2b73a1a7d35769e5b765` | 提交门 `from-run`/`runlist` 上限截断与 fail-closed | 绿 `8/8 PASS` + `PRECOMMIT blocked=0` |
| 2 | 队列 `#2` / 阶段 B `(9)` | `research/ompgpt/chain-step-reaped-kill.patch` | `1271d5c82623cbeab0504ee1b6001936c57fbe73c1b997955ff3e92942a12b0c` | P0 进程信号 `reaped` 后不再 `sy_kill(pid)` | 正负控 `REAPED_GIT case=fixed rc=0` / `mutant rc=1` |
| 3 | 队列 `#3` / 阶段 B `(4)` | `research/ompgpt/dvqualify-selector-reuse-v3.patch` | `61ba39ee11739cf2702feff37abc09e0e585e61ae52fe36e94c95fe81ddaf055` | `dvqualify` 选择器复用与页脚拒键 | `CHECK_DVQUALIFY fail=0` |
| 4 | 队列 `#4` / 阶段 B `(11)` | `research/cdx/r39c-release-exec-sha.patch` | `2e9e41e43ac11664dccfb9a72bfb66eead6635e2d419691812d347fd401764c9` | 修复 `release-exec.cx` `sha_file` 200B 长路径截断 (0.0.106 `W0150` 暴露项) | `CHECK_RELEASE_EXEC PASS part=c failed=0` |
| 5 | 队列 `#5` / 阶段 B `(3a)` | `research/cdx/producer-cx-booter-ape.patch` (3a) | 待核 (彩排 `rc=0`) | 为 `cx-booter-ape` 补材料生产者 | 若入车转实跑绿, 更新已知缺口 (1) |
| 6 | 队列 `#6` / 阶段 B `(3b)` | `research/ompgpt/producer-tools-default-regression.patch` (3b) | `4a3aa9ca16048801824a7b8ab696f0762e2ff197c598aea6917b2de81ede0208` | 为 `tools-default-regression` 补生产者并修 `checker_old_sha`; 注意 `tools/affected.cx:876` 与 3a 并集冲突 | 若入车转实跑绿, 更新已知缺口 (2) |
| 7 | 队列 `#7` / 阶段 B `(8)` | `research/cdx/seal-precheck-v2.patch` | 待核 (排除 `build/links.tsv` 应用 + `derive --only links`) | 封口缺项预检工具 | `PRECOMMIT blocked=0` + `drift=0` |
| 8 | 队列 `#8` / 阶段 B `(10)` | `research/ompgpt/check-tool-material-macro` (v3) | 待落盘核 sha | 依赖 106 `pinscan`, 工具材料路径宏拒绝与 `register.cx` 登记 | 核 `tests/tools-compile-sources.tsv` |
| 9 | 队列 `#9` / 阶段 B `(2)` | `research/agyneon/wp4-install-ape.patch` (v4) | `2e4a07d6e722df91f811b3874a22505032f70584d039f4a5061cf894284d1c79` | WP4 安装 APE 缺件 fail-closed 夹具 | `PRECOMMIT staged_cx=1 blocked=0` |
| 10 | 队列 `#10` / 阶段 B `(1)` | `research/agyneon/wp4-material.patch` (v2) | `8958b5bf0ada2dd3a521355ddac98232583a36a27b981c3cd58c71060bcdbc75` | WP4 材料与 `tsv-lint`/`ratchets-tool` 断言 | `TABLES_GEN stale=0 hard=0` |
| 11 | 队列 `#11` / 阶段 B `(6)` | `research/agyneon/u08-source-missing.patch` (v3) | `32affee775882f290ad57ec16bbbf3e294ae2c020a747a10dccbc38dd8045f6b` | `check-slice-migration-gen.cx` 缺源负控 | `source-missing-refused` 负控实证 |
| 12 | 队列 `#12` / 阶段 B `(7)` | `research/ompgpt/gen-all-drift-fixture-tests.patch` | 待核 (彩排 `rc=0`) | `gen-all-check` 漂移夹具测试 | 漂移未拒转红负控 |
| 13 | 队列 `#13` / 阶段 B `(12)` | `r39e` 候选 + `cc` 遮蔽三批修复 | 待定稿落盘 | `r39e` 候选及 `cc` 遮蔽三批真实修复 | 落盘 sha 核对 + 三批真绿 |
| 14 | 阶段 C `1` | `research/agyneon/v0107-criteria.patch` | `7d3dc50cb8bc868141aa641cdc6b596c5d2738a9ade8087fda19043491ba566c` | `0.0.107` 准入与复核表项收口 | `DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --version 0.0.107` 报 `READY=yes` |

---

## 4. 必须回填的位置清单 (Y1 前人工/工具核验关)

(1) `<CI_RUN_ID>`:
- 出处位置: 状态行 `run <CI_RUN_ID>` 与 `## 真实身份` 表 `CI run` 行(若出现 CI 重试, 并在 `## 已知缺口与未验证范围` 补写失败 run 与重派 run)。
- 来源: `release/candidate.tsv` 第 7 列 `ci_run_id` (`tools/notes-fill.cx:63` 读 `cand_col(cand, "ci_run_id")`, 对应 `tools/release-exec.cx:1398` `set_ci_run_id`)。
- 检查: 提交 `Y1` 前不得保留 `<CI_RUN_ID>` 或 `待填` 字样。

(2) 候选 `sha` 与冻结提交 `commit`:
- `<COMMIT_40HEX>`: `## 真实身份` `冻结提交(构造输入、发布标签目标)` 与产品改动/链后补跑里的 `REV`/`chain_rev` (`release/candidate.tsv` 列 3 `commit`, 对应 `tools/notes-fill.cx:61`)。
- `<APE_SHA256>`: `## 真实身份` `发布 APE SHA256(unisacc.com)` (`release/candidate.tsv` 列 9 `ape_sha256`, 对应 `tools/notes-fill.cx:62`)。
- 其余候选 sha (若沿用 `notes-0.0.105.md` 7 行身份表或写入回滚方案): `SOURCES_SID`、`pkg.bin` (`package_sha256`)、`slice-win-arm64` (`win_arm64_sha256`)、`unisacc.com.build.json` / `build.json` (`record_sha256`)、`SHA256SUMS` (`checksums_sha256`), 逐字对齐 `release/candidate.tsv` 与 `release/evidence/0.0.107/ape-receipt.txt`。

(3) 严格计数与表路径:
- `<DATE>`、`rows`、`expected_fail`、`superseded`、原 merge 计数 (`<PRE_BATCHES>`, `<PRE_PASS>`, `<PRE_FAIL>`, `<PRE_UNKNOWN>`) 与最终合并 `SUMMARY` (`<FINAL_BATCHES>`, `<FINAL_PASS>`, `<FINAL_FAIL>`, `<FINAL_UNKNOWN>`, `<FINAL_INVALID>`)。
- 出处: `## 产品改动` 覆盖声明、`## 严格证据` 段、`## 已知缺口与未验证范围` 第 (3) 项 (`<FINAL_BATCHES>`) 与 `## 证据入口`。
- 铁律校验: 若无后补, `<PRE_COUNT>` 与 `<FINAL_COUNT>` 一致且 `superseded=0`; 若有后补, 保留原 FAIL 行并标 `SUPERSEDED`, 必须如实同时披露预跑原 merge 计数与合并后 `SUMMARY` (对标 `notes-0.0.106.md:30, 34, 249`)。

(4) 状态行措辞固定句:
- 位置: 文件最顶部状态行 (`# unisacc 0.0.107 发布说明` 下一行)。
- 必须原样保留的固定句: `同候选六格原生 CI 与全套 s9 是两件事` (完整推荐固定句: `状态：同一候选通过六格原生 CI，结论为 QUALIFIED（run <CI_RUN_ID>）。本版未执行全套 s9，未取得全套 s9 资格；同候选六格原生 CI 与全套 s9 是两件事，不得以 CI 六格替代全套 s9 资格裁决。`)。
- 如实际运行了全套 `s9`, 也只能依据 `release/decision.txt` 的 `STEP	4	s9` 与 `QUALIFIED` 实测行改写, 不得仅因 CI 六格通过改写或删去边界声明。

---

## 5. 已知缺口章的固定条目与核对口

(1) `U05` (`provenance`) 未测量:
- 事实真源: `build/state-unverified.tsv:3` (`U05	0.0.65	provenance	UNVERIFIED	osx cache-manager	借用副本来源与 macOS 实跑	候选上首次 osx 实跑并核来源	tools/cache-retain.cx`) 与 `research/ccogweco/unverified-classification.md:11`。
- 固定条目条件: 除非 `0.0.107` CI `osx` 格新增并实跑一次带 sha 对照的 `cache-manager` 实跑行且通过, 否则必须照 `notes-0.0.106.md:231` 保留 `(M+1)` 的 `U05(provenance)` 未测条目。

(2) 两个 `UNKNOWN` 无生产者批次 (`cx-booter-ape`、`tools-default-regression`):
- 事实真源: `prd_mempalace.md:152` ("cx-booter-ape 与 tools-default-regression 已连续 11 版 UNKNOWN")、`notes-0.0.105.md:46`、`notes-0.0.106.md:38-39`。
- 固定条目条件: `notes-0.0.107.md` 在 `3a`/`3b` 补丁实际入车并产生 `RAW BATCH_RUN` 且实跑 `PASS` 之前, 必须把这两批保留在 `## 已知缺口与未验证范围` 第 `(1)(2)` 条, 状态严格写 `UNKNOWN(无 RAW BATCH_RUN 行)`, 不记 `PASS`; 一旦入车并在 `v0.0.107-strict-<DATE>.tsv` 实测转 `PASS`, 则按真实 `SUMMARY` 更新 `UNKNOWN` 计数并把消缺写进产品改动节, 不保留过时缺口假条目。

(3) 全套 `s9` 未跑与 `pkg-gaps` 展开:
- 固定条 `(3)`: `s9 全套本版未跑; 严格证据所列 <FINAL_BATCHES> 批之外的批次 UNMEASURED。`
- 固定条 `(4)..(M)`: 随 `release-run.cx -- --prepare` 从当时最新冻结提交的 `build/pkg-gaps.tsv` 自动读取全部 `OPEN` 行 (截至编写本骨架时共有 `188` 条 `OPEN`, 同 `notes-0.0.106.md` 的 `(4)..(191)` 共 188 条); 若车二修复关闭了其中某条 `OPEN`, 需先更新 `build/pkg-gaps.tsv` 并随 `release-prep` 自动同步编号, 不可手工硬拷贝 106 编号。

(4) 无绝对家目录路径、无 `emoji`、无特殊符号自检命令:

```sh
sh ./unisacc.com tools/doc-links.cx -- release/notes-0.0.107.md && \
sh ./unisacc.com tools/no-home-path.cx -- --check && \
! grep -n '<CI_RUN_ID>\|<COMMIT_40HEX>\|<APE_SHA256>\|<FINAL_BATCHES>\|待填\|待人工填写' release/notes-0.0.107.md
```
