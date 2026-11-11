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

## 三、补测结果与未完整项（第二版，2026-10-14）

| 作业 | 结果 | 读集 | 名单外 | 处理 |
|---|---|---:|---|---|
| exec-bridge-linux | **本宿主不可测**：`linuxbridgecheck.py` 需要 `clang -arch arm64`（Apple 工具链），宿主没有，在 0.7 s 内 rc=1 | 2（不完整） | 无 | 不写声明，记为本宿主不可测 |
| c99-ledger | rc=0 | 3 | 无 | 无需声明 |
| ccinterop | rc=0 | 96 | 无 | 无需声明 |
| realprog | rc=0，47 s | 178 | 73：`corpus/*`（已在 declared trees 内）、`.gitattributes`、`.gitignore` | 两个点文件已声明（232fd1b3）；corpus 由 declared tree 覆盖 |
| com-tapedomain | rc=0，候选 `unisacc-next.com`（sha `7e881abb…`） | 2 | 0 | 无 |
| com-latebind | rc=0，同上 | 2 | 0 | 无 |
| com-tools | rc=0，同上，43.9 s | 37 | 31：`corpus/*`（克隆缓存，固定提交，同 tools-1..3） | 同 tools-1..3：外部固定语料，不进声明 |
| gate-infra | 已在 c3d13214 测过 | — | — | 已声明 |

注意：`com-*` 的候选是 cc 指定的 `unisacc-next.com`（sha 256 前缀 `7e881abb`），在 `578ec1cc` 的干净导出上运行。之前用根 `unisacc.com`（引导用的旧字节）的结果作废，未引用。`com-tapedomain` 与 `com-latebind` 的读集只有 2 个文件，因为测试以 `-run` 内嵌程序，读取少；这是实测数字，不是漏记。

### unisa-gold-opinfo-label：真红，修补已在仓外验证，待登记窗

- 定性：**真红，不是环境**。Python 的 `OPINFO_OPS` 为 73 行（`tape.SHAPE` 72 个操作 + other），C 手抄表 72 行，缺 `.libraryfn`（`unisa/tape.py` 由 9645c6db 加入）。与 874a6e42 同类。
- 修补（仓外，干净导出 + 提交 scratch）：`seed/unisa_gold_opinfo_label.c` 在 `.libraryaddr` 后插 `.libraryfn`，注释 71→72、72→73 行；`tests/unisagoldopinfolabelcheck.sh` 文字 72→73 行。**结果：rc=0，same 19，diff 0，neg 2/2。**
- 同类另一处：`seed/unisa_vm.c` 的表也是 SHAPE 副本，缺 `.libraryfn`。修补：加 `{".libraryfn","ri"}`（执行时与 `.libraryaddr` 家族一样以 "operation not covered by this reference" 具名拒绝，不加空实现），头注释 Not covered 列表补上。**结果：`unisavmcheck.sh` rc=0，same 9，diff 0，neg 3/3。** 注意：这些对拍的程序没有使用 `.libraryfn`，所以没有直接证明它被具名拒绝；这一点需要单独的负例。
- 待登记窗（cdx 持有）：gatedeps 声明 `unisa-gold-opinfo-label` 与 unisa-vm 的对应条目，derived-map 加 `unisa/tape.py` 行。当前两个 suite 都未在 gatedeps 中声明。

## 四、rowcov：采用的记账路与边界（cc 定，2026-10-14）

- **采用**：按“源码已核只读名单内 + 有界实测窗口内 0 名单外读取”记账，不拆分片，不改 gate 本体。
- **源码核对**（`tests/rowcov.py`）：它打开的路径是 `tests/rowcov/*`、`examples/*.c`、`tests/c/*.c`、`exec/*` 下的生成与表，全部在名单内。两处非仓库输入：(a) `xdir()` 通过 `exec/stamp.sh` 得到的输出目录（写入，scratch，不入库）；(b) `UA` 二进制，它已被 rowcov 自身的身份摘要（`edges()` 内 `hashlib` 对 UA 的 sha256）计入。
- **边界**：读集**未完整**（parse2-1..24、enc-1..8、parse2-union、lower-union、enc-union、pp-*、pp-shared/locations 等多数超时）。只能说“源码级排除 + 截断前无名单外读取”，不能说“完整读集无名单外读取”。不据此写任何声明。lower-1..8、lower-build、enc-build、lex 为完整读集。

## 五、方法（以后一律如此）

- 限时：`python3 tests/bound.py 55 strace …`，按进程组清理。**不用裸 `timeout`**（`timeout` 只杀直接子进程，孙进程占管道，批次 07–08 的 enc-8 墙钟 438 秒由此而来）。每次运行 ≤55 秒。
- 环境：干净导出（`git archive` 的 HEAD 或指定提交），不用共享工作树。导出中不得留 `.git`（做 k2b/opinfo 时的临时索引须在用完后删除，否则 rowcov 会读到 `.git/*`，曾因此作废一批）。
- 超时（rc=124）读集截断，记为“未完整”，不据此写声明。

## 六、本轮遗留的已知问题

- 运行器（我的 `run.py`）用了裸 `timeout`，批次 07–08 的 enc-8 墙钟 438 秒。以后用 `tests/bound.py`。
- 中途一批因导出误留 `.git` 而作废（已重跑干净版）。
- k2b 的 UA 需要 `build_ref` 构建，不在名单内；这是调用方式，不是读取。
