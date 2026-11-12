# Darwin B139 取证汇总（cc 1349；只读；仓外）

## 范围与身份
- CI run `38111789528`，源码 SHA `82e1a9f06e9e39f805e25f0ad285b7306acb9937`（不跟随主树）。
- 双红：`macos (macos-14)` job `114388705403`；`macos (macos-15)` job `114388705429`；`macos (macos-15-intel)` `114388705401` 仍 in_progress（不推断）。linux×2 success。
- 主树 HEAD = ef35e718；origin/main 采样时为 e88b1ad0（已再前移，不影响本取证的 CI SHA）。

## 原证（已保全，只读）
| 件 | 路径 | sha256 |
|---|---|---|
| macos-14 首红全日志 | `cdx2-ci-macos14-firstred-1302.log` | `0f361b4e…` |
| macos-15 失败全日志（cdx 取） | `cdx-ci-macos15-failure-1314.log` | `0b52165f…`（1314 记录一致） |
| macos-15 同时 cdx2 副本 | `cdx2-ci-macos15-firstred-1314.log` | 待核（未计） |

## 关键原文（两格同形）
- `FAIL bootstrap branches B=139 U=0`（macos-14 05:06:11Z；macos-15 05:11:30Z）。
- `finished acceptance10-Ctape rc=1`（04:34:53Z / 04:35:37Z）；`finished acceptance10-compare rc=1`（04:35:18Z / 04:35:59Z）。
- 两格 bs_B 与 bs_U 均已产出：`2328418 B`，magic `cffaedfe`（Mach-O arm64）；bs_B text 1839624 B / 396703 insn；bs_U text 1832492 B / 402437 insn。
- `##[error] Process completed with exit code 1`（macos-15）。

## 脚本语义（仓内 tests/bootstrap.sh，只读）
- `branch_b`：① `bound 30 $UA $SELF -c`→bs_B.tape；② `bound 45 python3 -m unisa compile … --from-tape -o bs_B`；③ `sign_image`（codesign 结果不检查）；④ `bound 30 bs_B $SELF -c`→bs_C.tape。
- `branch_u`：① `bound 45 python3 -m unisa compile $SELF -o bs_U`；② sign；③ `bound 30 bs_U $SELF -c`→bs_UT.tape。
- 两分支并发（`&`）；`B=$brc U=$urc` 为各分支 `wait` 的 rc。
- `tests/bound.c:133`：子进程被信号杀死时返回 `128+signo`。故 **B=139 = 子进程被 SIGSEGV(11) 杀死**（非 exit 139）。U=0 即 U 分支全部成功。

## 由原证可推的收窄（推断，标注）
- bs_B 与 bs_B.tape 已存在（同一 scratch，非空），说明步①与步②的产物已落盘；故 139 最可能来自 **步④ `bs_B $SELF -c`**（运行 B 镜像）。
  - 限定：步②（python）若在写盘后崩溃也会返回 139，但 CI 未留该步的 rc，**不能排除**。
- 步①（UA 参考二进制 `-c`）若崩溃，bs_B.tape 为空，应走 `empty B tape` 路径（rc 1），与观测不符，故**可排除**（前提：scratch 每次新建，已由脚本保证）。
- U 分支运行同一 SELF 的 bs_U 成功；bs_U 与 bs_B 均为 osx/arm64 Mach-O，体积相同但指令数不同（396703 vs 402437）。
- **Linux 双目标 bootstrap 同 SHA 绿**：B 路径在 lnx 上可跑，故差异与平台（Mach-O 发射 / 签名 / 加载）相关的可能性高于纯逻辑错误。

## 缺失（UNKNOWN）
- 步②/③/④各自的 rc 与 stderr；bound 之下的异常 PC、崩溃栈、DiagnosticReports/core；bs_B 与 bs_U 的字节对比（CI 未上传产物）。
- macos-15-intel 的 conclusion。
- 缺以上无法定位产品或宿主根因。

## 取证结论（一句）
Darwin 双红为同形 B 分支 139=SIGSEGV；最可能是 B 镜像（`--from-tape` 路径产物）在 Mach-O arm64 上运行时崩溃；U 镜像同平台正常；Linux 同源正常。根因未定（产品生成 vs 宿主签名/加载）。
