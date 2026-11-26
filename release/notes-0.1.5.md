# unisacc 0.1.5 发布说明

状态：降级版公开发布。整体裁决 `NOT_QUALIFIED`：全套 s9 本版未跑，严格证据只覆盖慢车道分片与逐批补跑，不得称全批资格。nn 迁移起步未达成（能力就绪）：私有候选已验证（agyneon3 私有安装根 APE：stream e2 与 G5，`w-agy-h/.work/release-evidence-nn-start.md:47-51` 的 PASS 行）；公开候选及分发不据此升格；m53 影子对拍默认关。公开上传为 0.1.3 的五件被哈希实体加 `SHA256SUMS` 本身，共六件；本版不随公开发布资产分发 `tape/tapemode.so` 与 `tape/m53.tapeslice`，最终以正式资产单回读为准。

## 真实身份

| 对象 | 身份 | 来源 |
| --- | --- | --- |
| 候选基线 | `e9ce30ee0`（集成实测）/ `01d77868c`（第二轮复现）；冻结提交待 `freeze-cut` 回填 | `w-agyneon/.work/release-notes-0.1.5.md:3` |
| nn 起步私有验证 APE（历史件，自报 0.1.3） | `candidate_ape_sha=0a27a652d446c53d53d15eaac029a5df6a8478a01b7813cc8caf4f4f6a15a39e`, `3364110`B | `w-agy-h/.work/release-evidence-nn-start.md:11`、`w-agyneon3/.work/report-merged-candidate-v2.md` |
| 旁置 tapemode.so（私有根） | `sha256=f387e22ed90d6efd9f1e717da9900a250afc8982e9c124777bcce6b6f350809a`, `14368`B | 同上 |
| 确定性旁置 so（两目录重建，修后） | `sha256=941583e6e81f48cadbd917dbaa52fd1c5799529caf64bfc8c9d0d5c16e9f3d05`, `13552`B, `SO_DET_CMP=EQUAL` | `w-agyneon/.work/report-merged-candidate-repro2.md:8-11` |
| m53 切片（集成验证件） | `9f9294c428e5a797f5ce926440c2cba78857f0a3223201ef7d1ef5ff35a02544` | `w-agyneon3/.work/report-merged-candidate-v2.md` |
| m53 切片（独立另打件） | `c47cdd0610f5dbb471ddc3da59a64a523374108b08b46418a0ac9a9f822e1d2f` | `w-agyneon/.work/report-merged-candidate-repro2.md:16` |
| entry-build 中间产物 | `candidate=7826db82fe539612bafe363dbaa1198f7eac20ef6d5b74115abe440fde4d5719`, `4984060`B（与 agyneon3 同一中间产物一致；非发行 APE） | `w-agyneon/.work/report-merged-candidate-repro2.md:25` |
| 发布 APE、F、`SOURCES_SID`、CI | 待正式链与 `release-exec` 回执 | `release/candidate.tsv` |

身份口径：`0a27a652` 是历史私有验证件（自报 0.1.3）；`e9ce30ee0` 是 agyneon3 的集成实测基线；`01d77868c` 是第二轮复现基线；`c47cdd06` 是独立另打容器，不是 `9f9294c4` 那一件。

## 产品改动

(1) **nn 迁移起步（m53 S1 与旁置 tapemode.so，默认关）**：运行时取决于显式开启 `UNISACC_M53_S1` 并提供可信旁置文件；启用后从旁置 `tape/` 自装载零重定位 `tapemode.so`（`driver.tape_soload`，零 `sys.ffi`），对拍 `tape/m53.tapeslice`；结果始终采用原生，旁置缺失、损坏、非零重定位或权限不符时 fail-open 回退原生并在 stderr 具名报告。`release-ape --with-tape-sidecar` 是构造侧显式开关，默认关闭且未接入公开发布链；公开分发旁置资产需发布工具改造，仅为后续版本计划，以正式资产单为准。
(2) **tape 减步四层**（`c558db67b`）：`opt6_v2` 16 档 `445,252,559 -> 254,035,160` 步（`-42.95%`），64 档 `3,355,321,542 -> 2,017,795,326` 步（`-39.86%`）（`research/agyneon/v014/tape-step-reduction/report.md:18-29`）；G1 实测落地计数 `254,364,857` / `2,021,202,433`（`research/agyneon2/v014/g1-measure-v015.md:16、18`）。
(3) **L4 粘性写错误与 overlap-safe compact**（`3d9efd33d`）：`L4V2_CMP16=EQUAL`（`cut=0`）、`L4V2_CMP64=EQUAL`（`cut=0`）、`DONE u=102 bytes=9516745 cut=1478`、`L4V2_CMP102=EQUAL`（`research/agyneon2/v014/review-l4-sticky-r2.md:15-20`）。
(4) **函数级调用入口（增量 0）**（`c72fdcd6a`）：`tapepack.cx` 将导出名称放入 `DATA`、16B 条目放入 `SYMS`；`tape_x86_64.S mode=5`；`CHECK_FUNC_ENTRY passed=12 failed=0`、`CHECK_TAPEPACK passed=19 failed=0`（`research/agyneon/v014/review-func-entry.md:19、21`）。
(5) **工具与门**：`28136e237`（check-tape-grade-gate 29 例）、`6ead29acd`（root-current 门 24 与 release-post）、`dff492cce`（check-prerun-rebase-plan 3 冷片）、`af3e98239`（T1(b) v3 `CHECK_INSTALL_APE failed=0 not_run=0`，覆盖缺口见 `report-t1b-v3.md:44-48`）。
(6) **结构 ID 容量修复**：第二次彩排的 win/arm64 gen2 e3 触及结构 ID 容量——整品 parse2 对 win/arm64 的 105 个单元需要 2085 个 ID，而当时容量 2048（常量 2049 为排他上界），0.1.3 只用了 2047。`99e6f0b61` 将前端 `STRUCT_ID_MAX` 从 2049 调到 4096（排他上界，不是可用 ID 个数）；`855fa3dfd` 在两处驱动 `tape_soload` 与 `stream_s1` 移除三个结构体定义（struct-free driver units）。两提交已在版本分支；证据 `report-win-arm64-struct-cap.md`、`reh2.txt`、`exec/facts/k2-caps.tsv:10`；新候选是否通过以完整回执为准。

## 严格证据

- m53 影子对拍：`CHECK_SLICE_NN_M53_S1	PASS	vectors=28	ok=29	fail=0	mutations=3	candidate_ape_sha=0a27a652d446c53d53d15eaac029a5df6a8478a01b7813cc8caf4f4f6a15a39e	tape_container_sha=9f9294c428e5a797f5ce926440c2cba78857f0a3223201ef7d1ef5ff35a02544`（`w-agy-h/.work/release-evidence-nn-start.md:21-37`）。
- G5 子模式（`f8dc7a0740` 修复后，非 ELF APE 改经 `sh` 执行）：`CHECK_SLICE_NN_M53_S1_G5	PASS	cases=4	ok=5	fail=0`，仍用同一 `0a27a652...` 与 `f387e22e...`（`w-agy-h/.work/release-evidence-nn-start.md:55-56`）。
- 确定性：修前同树两次构建摘要不同（agyneon `4ba6ff69...` 对 `4acef2a1...`，agyneon3 私有集成 `f387e22e...`，均 `14368`B）；仅去 build-id 仍有临时对象名 `STT_FILE` 差异，加 `-Wl,-s` 剥离后才通过；修后 `01d77868c` 两目录重建逐字节一致，`941583e6...`、`13552`B、`SO_DET_CMP=EQUAL`（`w-agyneon/.work/report-merged-candidate-repro2.md:8-11`、`w-agyneon3/.work/report-build-id-det.md:9-23`）。
- stream e2 复现：`OFF` 是基线（`out_sha=85dd3bfa...`，`err=` 空，无 `eq_off` 字段）；其余四态均 `rc=0`、`eq_off=YES`、`reason=absent`（`w-agyneon/.work/report-merged-candidate-repro2.md:17-21`）。源码 `01d77868c:cx/driver/stream_s1.cx:174-192`：无 `UNISACC_APE_SELF` 时由自身 ELF 定位旁置，缺路径报 `absent`，权限拒绝报 `untrusted`；`dev-lnx` 薄壳定位到 `unisacc.com.dev` 下的旁置路径，复现原行未记录精准缺失层级。agyneon3 独立一轮有真实 `ON` 无回退及不同的 `NOSO`/损坏/不可信诊断（`w-agy-h/.work/release-evidence-nn-start.md:47-51`）。
- G1 三档（`lnx/x86_64` predecode，`research/agyneon2/v014/g1-measure-v015.md:16、18、19、44`）：16 档 `2358 ms / 42 ms = 56.14x STOP`；64 档 `18863 ms / 263 ms = 71.72x STOP`；102 档 `wall_ms=100716 / 1326 ms`（参考 `75.95x`，`cut=1478`），`load_post=4.02` 超洁净线记 `UNMEASURED`，且超 `55s` 绝对时限。
- G2/G3：`UNMEASURED`；osx/win/arm64 的 tape 功能路径 `NOT_APPLICABLE`（`28136e237` 合同）。
- 27 批声明命中：`24 PASS / 0 FAIL / 3 UNMEASURED`（缺材料 `optimization-closure-0-4`、`optimization-closure-5-9`、`release-prep-ratchets`）；三批材料已 PREPARED，但 0.1.5 候选未实跑，仍记 `UNMEASURED`（`research/agyneon/v014/tape-step-reduction/slow-lane-v015.md:63-67`）。
- 698 批分片原数（历史基线 `c558db67b`，不是冻结候选逐项实测）：`PASS=89 / FAIL=29 / UNMEASURED=580`，22 个 FAIL 为超时截断，无去重终汇总，其余记 `NOT_QUALIFIED`（`research/agyneon/v014/tape-step-reduction/slow-lane-v015.md:32-45`）。

## 已知缺口与未验证范围

(1) 全套 s9 本版未跑；慢车道 698 批大部分 `NOT_QUALIFIED`（历史基线 `c558db67b`）；G2/G3 `UNMEASURED`。
(2) nn 迁移起步未达成（能力就绪）；S1 默认关，公开候选不能直接启用；旁置件本版不分发，公开分发旁置资产仅为下一版计划。独立复现只记录了 `reason=absent`：这是复现环境设置错误（`dev-lnx` 薄壳未设 `UNISACC_APE_SELF`，旁置由进程自身 ELF 定位，缺路径报 `absent`；`cx/driver/stream_s1.cx:174-192`），不是产品缺陷；上游说明记为定位到 `unisacc.com.dev` 下的旁置路径，复现原行未记录精准缺失层级；独立开关 `ON` 用真实容器仍未复现，待彩排链后用 `release-ape pack --with-tape-sidecar` 完整 APE 补测（`w-agyneon/.work/release-notes-0.1.5.md:17`）。
(3) 配对改名未验证：`tapemode.so` 与 `manifest.tsv` 是两次连续改名，中断下的同时性与恢复结果未核（`w-agyneon3/.work/report-p2-pp-fix.md:27-28`）；安装时 APE 与 sidecar 同步是另一个时序问题。双门名来源在此候选仍开，v0.1.6 私有跟进未替换本版；product-map 读集边界未全验。
(4) `seen` 只比路径身份；`tapecall` 客体指针回传留增量 2；方案 A 的 `A1`/`A2`/`A5`/`A6` 部署前置以冻结决策为准。
(5) 链 bootstrap：彩排已选 0.0.105 开发根（`934c7c0bc...`，0.1.3 链 vault）作版本工作树实体 `unisacc.com`。`report-rehearsal-67p4.md` 第 3-4 节记录该旧根可编译 25 个 controller 文件、0.1.2/0.1.3 根拒绝；0.1.3 正式链历史回执记 `bootstrap_sha=934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1`；`ad5c3e72a` 引入拒绝门。该报告不提供 0.1.5 最终正式链引导摘要，也不单凭报错证明内部比较分支；0.1.5 终态等该轮回执。
(6) 链事实：彩排链两次失败后通过（第一次 bootstrap 根 0.0.105，第二次结构 ID 容量）；失败目录保留在 `chains/bg-b1/failed-1` 与 `chains/bg-b1/failed-2`；彩排终态约 02:05 回填。

## 回退办法

- 运行开关：清除显式 `UNISACC_M53_S1` 即回默认原生路径。
- 源码回退：撤回 S1 提交是源码级方案，仅限列明提交与依赖，未经用户验证；安装回滚以 `release/rollback.txt` 与发行时的产品身份为准。详见 `release/rollback.txt`。
