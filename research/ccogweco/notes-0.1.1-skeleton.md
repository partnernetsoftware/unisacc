# unisacc 0.1.1 发布说明骨架(产品车:诊断定位;2026-11-25 ccogweco)

状态:同一候选通过六格原生 CI,结论为 QUALIFIED(run <CI_RUN_ID>)。本版未执行全套 s9,未取得全套 s9 资格;
同候选六格原生 CI 与全套 s9 是两件事,不得混称。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(F6=起链提交) | <COMMIT_40HEX>(0.1.1 车,基线 main 864de15886) |
| 发布 APE SHA256 | <APE6_SHA256>(本车为产品车,APE 与 0.1.0 有产品字节差异) |
| 源快照 SID | <SID6_64HEX>(必与 0.1.0 的 37ff46cd… 不同;差异即本车四组诊断改动的闭包) |
| CI run | <CI_RUN_ID> |
| pkg.bin / slice-win-arm64 / 两个 build.json / SHA256SUMS | <按 pack 后核> |

回填:`git -C .claude/worktrees/v0.1.1 rev-parse HEAD`;`sha256sum <pack>/layout/unisacc.com`;pack 的
`RELEASE_RUN SOURCES_SID VERIFIED` 与 `FIXEDPOINT VERIFIED`;`gh run list` 按 headSha。

## 产品改动(本版唯一大改:诊断定位)

(1) c15:未定义函数前移编译期,由 e2/e1 拒绝接定位。
(2) c16:`no file` 候选路径(searched/package-root)与 `does not export` 带 `use` 行并入 cxmod 双源诊断。
(3) c17:e2/e1 拒绝接定位的文案与来源行。
(4) `-o` 目标为已有源文件时告警,源文件字节不变。
(5) diag-contract 基线:接入 `tools/check-diag-contract.cx` 合同夹具并消减对应 KNOWN-GAP 计数
    (`DC_GAP_BASELINE` 等于实测值,不先降基线)。
- SID 变化说明:本车改 `cx/compiler` 与诊断事实表,产品闭包与 0.1.0 不同,SID 必变;不适用零改动封存口径。
- 证据:各组的先红后绿夹具与 check-diag-contract 原行(链后补)。

## 准入七条(占位;产品车口径)

| # | 条件 | 本版预期 |
|---|---|---|
| 1 | 六目标不动点连续 3 版 | 106/107/0.1.0 已归档;0.1.1 链止后归档第四套(产品字节变化仍须四同) |
| 2 | CI 六格 QUALIFIED 且 strict FAIL/UNKNOWN=0 | 按实际 run 与 strict 汇总;UNKNOWN 仅限已登记且证据可核项 |
| 3 | 零已知 P0 | P0 账本清零;U33 为 P1 随 0.0.125 |
| 4 | state-unverified 声称类 <=3 | <按当轮计数>;不搬账 |
| 5 | 前端覆盖三期 | 未满足(B2 顺延 0.1.2) |
| 6 | 起链到公开 <=120 分钟 | 未满足预期;目标线 130 分钟(fable 流程),超线记未达标并复盘,不单独停车 |
| 7 | 能用清单 30 过 25 | 清单沿用,release-chain 缺口登记 0.1.x |

## fable 流程新增条款(0.1.1 专用)

- 目标线 **130 分钟**(120 硬线自 0.1.2 起);超线记未达标,不单独停车;硬停手线仅:同车第 2 次产品红、
  尾段(预跑止到 X)超 20 分钟。
- **静默窗**:冻结提交前 60 分钟门工具路径零变化;入场回执附两条命令原输出
  (`git log --since="60 minutes ago" --format="%h %ci %s" origin/main -- <清单路径>` 与
  `git diff --name-only 彩排提交..origin/main -- <清单路径>`),cc 复看、gatekeeper 二签。
- **人工补救计数**(对照 0.1.0 实测 3 次):本车目标不超 1;四列:补救分钟、分类、自动重试收敛窗数、
  keep-going 待补批数。前三类(起跑单、冷首窗、贴限)任一非 0 即判对应 P0 项未生效。
- 入场条件:P0 出口彩排全绿(P0-Y/P0-Z 与静默窗均过)。

## 严格证据(占位)

    证据 release/evidence/v0.1.1-strict-<DATE>.tsv:rows=<ROWS>,expected_fail=<N>,superseded=<N>;
    预跑原 merge 为 <PRE_BATCHES> 批 PASS=<P> FAIL=<F> UNKNOWN=<U>;
    合并后 SUMMARY <FINAL>/<PASS>/<FAIL>/<UNKNOWN>/<INVALID>。

回填:`grep -E 'SUMMARY|rows=' release/evidence/v0.1.1-strict-*.tsv`。

## 已知缺口

- (1) 转发调用上限:8192 仅表级证据;实测约 **355** 个以上不同被调用转发符号即生成物畸形失败、
  **>=413** 段错误;0.0.126 的 F1 容量拒绝已入 0.0.107,但产品级 355/413 的实际上限与拒绝文案须在本车
  或后续候选上复核,未测写 UNMEASURED。
- (2) U05(provenance)未测;全套 s9 未跑;严格证据范围外 UNMEASURED。
- (3) 条件 4/5/6 按当轮实测如实写;B2 顺延 0.1.2。

## 链后取值命令(预留)

| 值 | 命令 |
|---|---|
| T1/T2 | `head -1 <chain>/owner.tsv`、`cat <chain>/exit.tsv` |
| 四同/package | `grep -h 'CHAIN	fixedpoint' <chain>/*.stdout` |
| 六目标 g2/g3 | `lnx-*/entry.elf`、`osx-*/slice-osx-*`、`win-*/slice-win-*` 逐对 sha |
| APE6 | `sha256sum <chain>/ape-final/unisacc-next.com` |
| SID6 | pack `RELEASE_RUN SOURCES_SID VERIFIED` |
| 公开/Y1/Y2 | INDEX `exec-5-publish`、`exec-4-Y1`、`exec-7-Y2` |
| 人工补救 | agy-h 账本四列与 supervise 日志停/重起时刻 |

## 落盘规则

- 本页是骨架;正式 release/notes-0.1.1.md 只保留成文章节;占位未回填不过 Y1;不把未测写成通过。
- 不写零改动封存口径(本车为产品车);SID 必须与 0.1.0 不同并说明差异来源。
