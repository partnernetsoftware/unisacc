# c43 名单外读取审计：一页汇总（2026-10-14，只读）

## 一、方法

- 对象：gate 中每个作业自己的命令（`tests/gate.sh` 的 `job` 行，rowcov 分片按循环展开）。
- 环境：干净导出（`git archive` 的 HEAD，不用共享工作树）。
- 记录：`strace -f -qq -e trace=openat`，只记 openat 成功、位于导出根下、普通文件的路径。
- 比对：**名单外** = 不在 `gatequeue.py` 的全局名单里（`git ls-tree` 同前缀：src exec tests include kernel weights unisa examples unisacc.c README.md ARCHITECTURE.md AGENTS.md prd.tree.md prd.map.md research/referee.tsv iterate release scripts Makefile seed）。
- **限时方式（以后一律用这个）**：`python3 tests/bound.py 55 strace …`，按进程组清理。**不再用裸 `timeout`**：`timeout` 只杀直接子进程，孙进程占着管道，运行器会一直等（批次 07–08 的 `rowcov-enc-8` 墙钟 438 秒即由此而来）。每次运行 ≤55 秒。
- 超时（rc=124）的作业，读集**截断**，记为“未完整”，不据此写声明。

## 二、七批 + 两次记录的全部结论

| 批次 | 提交 | 覆盖 | 名单外读取 | 处理 |
|---|---|---|---|---|
| exec- 批次 00 | 4cde534e | 10 个未声明 | 无 | 无需声明；exec-srcelf-1 超时（不完整） |
| exec-/unisa- 批次 01 | d91d6c44 | 未声明 | 无 | 无需声明 |
| unisa- 批次 02 | 64604049 | 未声明 | 无 | 无需声明；两项 rc1 是真实失败（`DIFF encode`、`DIFF render`），不是读取问题 |
| unisa- 批次 03 | 4f6fa20a | 10 个未声明 | 无 | 无需声明；opinfo-label rc1 未分析 |
| 声明批次 1 | c44cf763 | decision-ledger、exittable、k2b | 有，已在声明内 | 已写入 gatedeps 的 files/trees |
| 声明批次 2 | c3d13214 | gate-infra、com-tapedomain、com-latebind | 有（根产品） | 已写入声明；refresh_gatedeps 同笔 |
| 本轮批次 05 | bc6dae9e | tools-1/2/3、decision-ledger、exittable、h1source、precommithook、seedpy、subtract-safety-selftest、k2b | 见下 | 见下 |
| 本轮批次 06 | bf5e27ba | rowcov-pp-locations/-shared/-lex/-build/-union | 无（窗口内） | 6 条超时（不完整） |
| 本轮批次 07–08 | d4f00651 | rowcov parse2-1..24、lower-1..8、enc-1..8 | 无（窗口内） | parse2、enc 大部分超时（不完整） |

### 批次 05 与 06 的具体处理

1. **tools-1/2/3：名单外读取 `corpus/crypto-algorithms/*`（约 9–12 个文件）。** 这是 `tests/tools.sh` 的克隆缓存，按固定提交号检出，`.gitignore` 忽略。**处理**：不进声明（外部固定语料，内容由固定提交号决定）；`tests/tools.sh` 已改为：缓存 HEAD 不等于固定提交号、或工作树有已跟踪文件被改，都明确拒绝（bf5e27ba、64eee4a8）。
2. **precommithook：名单外读取 `.gitattributes`、`.gitignore`。** 已补声明（bf5e27ba）；摘要证明：改 `.gitignore` 会改变其输入摘要。
3. **k2b：此前 rc1（未设 UA）。** 用 `build_ref` 构建 UA 后在干净导出上 **rc=0**（bf5e27ba 的研究稿）。k2b 需要 `git ls-files`，导出里为此临时建过索引，不提交。
4. **decision-ledger、exittable、subtract-safety-selftest：名单外读取都在已声明的 files 或 trees 内。** 已覆盖。
5. **h1source、seedpy：无名单外读取。**

## 三、尚未测或未完整

| 作业 | 状态 | 原因 |
|---|---|---|
| com-tapedomain、com-latebind、com-tools | 未测 | 命令依赖产品二进制 `$PRODUCT`，需要产品就绪后测 |
| gate-infra | 已测（c3d13214） | 结论已在声明中 |
| exec-bridge-linux、c99-ledger、ccinterop、realprog | 未测 | 静态表中标了 archive/、research/，尚未 strace；建议下一批 |
| rowcov-parse2-1..24（24 条） | 不完整 | 每条超时（55 s），读集截断 |
| rowcov-enc-1..7、enc-8 | 不完整 | 超时；enc-8 墙钟 438 秒（运行器缺陷，见第一节） |
| rowcov-lower-1..8、lower-build、enc-build、lower-union、enc-union、parse2-union、pp-*、lex | 见批次记录 | lower-1..8 与 lex、lower-build、enc-build 完整；其余超时 |

## 四、完整化的办法（交 cc 定夺）

1. **拆分片（改 gate 本体，需要登记窗）。** rowcov 的 parse2 与 enc 分片在 55 秒窗口内放不下，拆成更多分片即可各自完整。代价是更多作业、更多 rowcov 树（每棵约 2 GB，有并发限制）。
2. **不改分片、只补一次完整读集（研究，不入库）。** 在干净导出上对每条分片用 `python3 tests/bound.py 55 strace …` 逐条跑，把读集取并集；但这仍被 55 秒截断，不能得到完整读集。因此这条路**不可行**，除非放宽限时（政策上不允许）。
3. **以超时窗口内的结论作为有界证据（当前做法）。** 窗口内 rowcov 对分片的读取都在名单内，且此前 `research/` 的静态误报已被实测否定（pp 作业）。这只能证明“截断前没有名单外读取”，不能证明完整。若采用此做法，需在声明里写明“有界”，不据此声明任何文件。

**建议**：先按 3 记账；若 parse2/enc 的名单外读取需要完整证明，走 1（拆分片），这需要登记窗。

## 五、本轮遗留的已知问题

- 运行器（我的 `run.py`）用了裸 `timeout`，批次 07–08 的 enc-8 墙钟 438 秒。以后用 `tests/bound.py`。
- 中途一批因导出误留 `.git` 而作废（已重跑干净版）。
- k2b 的 UA 需要 `build_ref` 构建，不在名单内；这是调用方式，不是读取。
