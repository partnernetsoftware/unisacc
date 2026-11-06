# .cx 自重启盘点与超时清理边界（2026-11-02）

只读审计。证据均为源码行号 + 本机实测输出；未用源码重建候选复验的条目已标"未核实"。

## 1. 结论

1. 全仓"自重启"（起一个**同一个文件**的产品子进程）只有 4 个，全部依赖 `-run SELF -- child ...` 的位置参数合同，且**没有任何代际/深度守卫**：
   `tools/check-cx-sys.cx`、`tools/check-cx-sys-net.cx`、`tools/check-cx-sys-sig.cx`、`tools/check-cx-fmt.cx`。
2. 当前根产品 `unisacc.com` 在显式 `-run` 下丢 `--` 之后的全部参数（实测 argc=1），默认-run 正常。根因在 `src/driveargs.cx` 把 `runit` 按值传入，早于 `cxc_step` 设置 `-run`。四个文件的子进程 guard 因此全部落空，跑回完整父流程再起子进程。
3. 扇出 >1 的三个文件最坏是**指数无界**（sys≈7、sig≈5、fmt≈3 倍/层）；`check-cx-sys-net` 单点自重启，最坏是**线性深度无界**。事故当时 `ps` 里的 3 个 `check-cx-fmt.cx` 与此一致（父+子+子）。
4. `tools/bound` 的后代清理覆盖 `script(1)`、`setsid`、`nohup`：Linux 上靠 `ps -axo pid,ppid` 树 + `PR_SET_CHILD_SUBREAPER` + `/proc` 收养扫描，且清理后核验"无残留"，失败时以 142 非绿退出。**非 Linux 无 subreaper 兜底**（`sweep_adopted` 直接返回 0），setsid+双 fork 逃逸无覆盖。
5. `tests/run-batch.cx`（经 `cx/sys/process.cx` 的 `np_run`）超时只做 `kill(-pid,SIGKILL); kill(pid,SIGKILL); waitpid(...)`：**不做 /proc 扫描、不核残留、setsid 逃逸清不到**；报告 `subprocess.TimeoutExpired` 后由 run-batch 记 UNKNOWN。
6. `tools/affected.cx` 自带的 `kill_tree` 用 `pgrep -P` 广度优先快照后逆序 killpg+kill，存在"快照后新 fork"和 setsid 逃逸两个缺口，同样不核残留。

## 2. 四个自重启文件（逐个）

共同合同：子进程 argv = `[SELF, "child", MODE, (ARG)]`；`self` 先用字面量常量，再被 `argv[0]` 覆盖（`check-cx-sys`:84、`check-cx-sys-sig`:244、`check-cx-fmt`:319；`check-cx-sys-net` 不覆盖）。

| 文件 | 拼的命令 | 子进程 guard | 每层扇出 | 最坏情况 |
|---|---|---|---|---|
| `tools/check-cx-sys.cx` | `:75` `{"sh", $UNISACC, "-run", self, "--", "child", MODE, ARG}`，经 `sy_run`(:78)；`:198` 同类经 `sy_spawn_inherit`；`:319`/`:322` `script -qec 'sh %s -run %s -- child tty-pgrp|tty-modes -'` | `:83` `argc == 4 && argv[1]=="child"` | 7（4×run_child + 1×spawn_inherit + 2×script） | 7^深度，指数无界 |
| `tools/check-cx-sys-net.cx` | `:195` `cx_fmt(... "script -qec 'stty rows 37 cols 101 && sh %s -run %s -- child tty %s' ...")`，经 `sy_run`(:199) | `:213` `argc == 4 && argv[1]=="child" && argv[2]=="tty"` | 1 | 线性深度无界（每层常驻一条 sh→script→sh→产品 链） |
| `tools/check-cx-sys-sig.cx` | `:131` `{"sh", $UNISACC, "-run", self, "--", "child", MODE}`，经 `sy_spawn_pipe`(:135)；`:196` `... "--", "child", "h"`，经 `sy_run`(:200) | `:243` `argc == 3 && argv[1]=="child"` | 5（4×run_child + 1×h） | 5^深度，指数无界 |
| `tools/check-cx-fmt.cx` | `:284` `{"sh", $UNISACC, "-run", self, "--", "child", "out"}`，经 `sy_run`(:287) 与 `sy_spawn_pipe`(:292)；`:309` `script -qec 'sh %s -run %s -- child tty'` | `:318` `argc == 3 && argv[1]=="child"` | 3（sy_run + spawn_pipe + script） | 3^深度，指数无界 |

其他细节：

- 四条都在 `main` 第一行做 dispatch；guard 不命中就直接往下跑**完整套件**，没有"认不出 child 就拒绝"的兜底。
- `UNISACC` 可被环境覆盖（`sy_env_or("UNISACC", "./unisacc.com")`），拼的是环境值，不是固定路径。
- `check-cx-sys.cx` 与 `check-cx-fmt.cx` 另在 `:284`/`:198` 处用 `spawn_pipe` 再起一次同一子进程，所以一次失败会同时有多条子链。

## 3. 丢参根因（源码位置）

实测（本机，根产品 `unisacc.com` sha256 前缀 `1bfdcd20e5aa60ff`）：

```
$ sh ./unisacc.com -run out/audit/argc.cx -- a b
argc=1 [out/audit/argc.cx]            # 显式 -run：-- 后参数全丢
$ sh ./unisacc.com out/audit/argc.cx -- a b
argc=3 [out/audit/argc.cx] [a] [b]    # 默认-run：正常
$ sh ./unisacc.com -run out/audit/argc.cx -- 
argc=1 [...]
$ sh ./unisacc.com -run out/audit/argc.cx a b
unisacc: error: cannot open a: ENOENT   # 无 -- 时 a 被当成源文件
```

源码链条：

- `src/drive.cx:245` 调 `dv_parse_args(..., opts->runit, opts->defrun, ...)`——`runit` **按值**传入。
- 显式 `-run` 时 `cxc_default_run`（`cx/driver/cli.cx:372-387`）看到 mode word，`modeflag=1`，**不**设 `st->runit`；随后 `cxc_step` 在 `cx/driver/cli.cx:423` 才把 `st->runit` 置 1，但已传出去的那个值仍是 0。
- `src/driveargs.cx:45` 的 `if (kind == 13) { if (runit) *argstart = i + 1; break; }`（`kind==13` 即 `"--"`，见 `src/entrybind.cx:104`）在本地 `runit==0` 时不写 `argstart`，于是 `argstart` 保持初值 `argc`。
- `src/drive.cx:543` `dv_bind_args(argc, argv, argstart, src, ...)` 于是得到 `count = 1 + argc - argc = 1`，程序收到 argv=[SELF]，argc=1（`src/driverun.cx:40-63`）。

默认-run 正常，是因为 `cxc_default_run` 在遍历前就把 `st->runit=1`，传进去的副本也是 1。

未核实：以上为源码静态定位 + 实测行为吻合；未用源码重建候选做对照实跑（候选构建/替换属 bdy 车道）。

## 4. 其余 spawn 点分类（能否自激）

判据：子进程在同一失败条件下是否还会回到同一 spawn 点。

- **链式/批量（非自激，但会放大失控）**
  - `apps/csih/suite.cx:56-58` 起 slice；`apps/csih/check.cx:61,202,241,256,271` 起 `term_cli.cx`/`loop.cx`/`probes/*`；`apps/csih/agent.cx:879` 起 `suite_cli.cx`；`apps/csih/probes/*` 各自起 stub/net/llm/agent 等（`agent_roundtrip.cx:51,63`、`http_roundtrip.cx:30`、`llm_roundtrip.cx:42,49`、`net_cover.cx:16`、`print_mode.cx:118`、`run.cx:24`、`ttyenv.cx:31,60`、`tui_agent_e2e.cx:58`、`tui_run.cx:30`）。子文件均为叶子，未见回到 `suite.cx` 的边 → 有界，但一次失控可同时拉起十几条链（csih 车道，只读记录）。
  - `tools/affected.cx:388` 起 `tests/run-batch.cx`；`tools/census.cx:73`、`tools/all-waves.cx` 起 run-batch（外层再套 bound）。`tests/run-batch.cx:39` 起被测批次。
  - `tests/cx-host-run.cx:329,382,483,562` 直接起驱动器跑 `barrier.cx` 等；`:862,885` 用 `setsid -w` 起协调器；`tests/cx-funit-host.cx:118,123`、`tools/stream-e3-ref-gen.cx:131,138,147,156,165`、`tools/o1-cases-gen.cx:46`、`tools/p1-baseline.cx:165`、`tools/release-ape.cx:66,110`、`tools/build-cxcc-ape.cx:81,230`、`tools/csih-ape.cx:163`、`tests/negative-*.cx`、`tests/resources*.cx`、`tests/route-chains.cx:70`、`tests/ucn-identities.cx:20`、`tests/ri-route-refuse.cx:74`、`tests/runreuse-absent-probe.cx:26`、`tests/user-malloc-null.cx:77`、`tests/loader-faults.cx:74,82` 起 fixture/叶子。均无回到父 spawn 点的边。
  - `ujs/*` 与 `archive/` 未纳入改动，只读记录（论文 B 车道）。
- **叶子（无递归可能）**：`tools/check-driver-cli.cx`、`tools/driver-diagnostics.cx`、`tools/link-malformed-input.cx`、`tools/prune-route-placement.cx`、`tools/tapebin-roundtrip.cx`、`tools/check-cx-entry.cx`，以及 `cx/driver/cli.cx` 的 `-run` 词法（只解析，不起进程）。

没有发现第五个"起自己"的文件。

## 5. `tools/bound` 清理边界

入口 `tools/bound`（shell 薄包装，按 `bound.c` 摘要缓存编译）。关键实现 `tools/bound.c`：

- 启动：`fork` + 子进程 `setsid()`（:456），子进程成为新会话/进程组组长；`bound` 自身 `prctl(PR_SET_CHILD_SUBREAPER,1)`（:416）。
- 超时路径：`reap_until` 轮询到 `command_at = hard - reserve`（reserve=min(2s, 10%)）→ `freeze_tree(child)`：
  - `kill(root, SIGSTOP)`（:223），然后循环 `ps -axo pid=,ppid=`（`:163`，可被 `UNISACC_BOUND_PS` seam 替换）快照；
  - 只把 `ppid` 已在 `tree[]` 的进程加入并 `SIGSTOP`（:224-245），直到一次快照无新增或到 hard deadline；
  - 结束后 `kill(-root, SIGKILL)` + 逐个 `tree[]` `SIGKILL`（:254-255）。
- 清理：`finish_cleanup`（:337）先 `kill(-child)`/`kill(child)`/`tree[]`/`ps_child`，再 `known_gone()`（读 `/proc/PID/stat`，:320-330）等被杀父进程真正退出，再反复 `sweep_adopted(child)`（:283-315）：扫 `/proc`，把 **ppid == bound 自己** 且非僵尸的进程 SIGKILL 并记入 tree，重复到一轮为 0。
- 残留判定：`sweep_adopted` 返回 0 才算 clean；`/proc` 打不开算 CAUSE_ENUM；到 hard deadline 算 CAUSE_DEADLINE。非绿一律 exit 142，并打印 `bound: <reason> (not green); unreaped child pid N; known tree pids: ...`。

覆盖结论：

| 逃逸形态 | Linux | 非 Linux |
|---|---|---|
| `script(1)` 中间层 | 覆盖：仍在 ppid 树内，且与子进程同组（script 不 setsid） | 覆盖（同左） |
| `setsid` 自建会话 | 覆盖：父仍活着时 ppid 树可见；父先死则被 subreaper 收养，`sweep_adopted` 按 `ppid==self` 清到 | **不保证**：`sweep_adopted` 是 `return 0` 空实现（:320），双 fork+setsid 后父死 → 挂到 init，killpg 和 ppid 树都够不到 |
| `nohup` | 覆盖：只忽略 SIGHUP，SIGSTOP/SIGKILL 照常 | 覆盖 |
| 快照后新 fork | 已知成员已被 SIGSTOP，不能再 fork；未知成员只能来自上面两种逃逸 | 同 Linux 的冻结逻辑，但收养缺口存在 |
| 枚举失败/超时 | 具名报 142，不冒充绿 | 同 |

限制：`ps` 快照单次最多占 `(hard-now)/3` 且 ≤0.5s，进程表极大时可能枚举失败——失败会具名报非绿，不会静默。

## 6. `tests/run-batch.cx` / `tools/affected.cx` 清理边界

`tests/run-batch.cx:39` 用 `np_run(args, root, 0, seconds, ...)`；实现 `cx/sys/process.cx:43`：

- 子进程 `setpgid(0,0)`（:49），父也 `setpgid(pid,pid)`（:81）。
- 超时（`idle >= secs*1000`，1ms tick 计墙钟，:99）只做：`kill(-pid,SIGKILL); kill(pid,SIGKILL); waitpid(pid,&st,0)`（:100-101），然后返回 `subprocess.TimeoutExpired`。
- 没有 `/proc` 扫描、没有残留核验、没有 subreaper；子进程若 `setsid`/`setpgid` 另立组，`kill(-pid)` 清不到，`waitpid` 也不等它。
- `waitpid(pid,&st,0)` 是**无 deadline 的阻塞等待**；SIGKILL 后正常会立刻返回，但 D 状态等极端情况会挂住（概率低，未实测）。
- 结果：`run-batch` 把该 transport 记为 `UNKNOWN`（注释 `:42`），不是 PASS——语义正确，但进程是否清干净没有证据。

`tools/affected.cx:323-341` 的 `kill_tree`：`pgrep -P` 广度优先抓后代（最多 256），逆序对每个后代的**进程组和进程**发 SIGKILL，最后杀根。缺口：pgrep 快照之后新 fork 的后代、以及 setsid 另立组的后代；不核残留。`all-waves`/`census` 路径外层有 `tools/bound`，这两处缺口由外层兜住；单独跑 `affected.cx` 时没有兜底。

已知唯一显式残留断言：`tests/resources-limits.cx:129-137`（deadline 杀 `loop.c` 后 `ps -eo args | grep -c '[l]oop\.c'` 必须为 0）。

## 7. 建议（对应三层修复）

1. 根因（bdy）：`dv_parse_args` 改传 `int *runit`（或在 `kind==13` 分支回读 `opts->runit`），并补 `-run FILE -- ARGS` 的回归用例（argc/argv 精确比对）。这只修 `--` 透传，不改变默认-run 行为。
2. 递归守卫（go，抽公共函数放 cx 层）：父进程设 `CX_CHILD_DEPTH`（或 `CX_SELFSPAWN`）再起子进程；`main` 开头若环境已有代际但认不出 `child` 合同，立刻 `_exit(99)` 并打一行具名拒绝，四个文件统一改为调用。守卫必须在**任何**副作用之前。
3. 清理边界（go）：`np_run` 超时路径补齐"冻结→枚举→逐个 SIGKILL→核验无残留"，或让 `run-batch` 固定以 `tools/bound` 启动批次；`affected.cx` 的 `kill_tree` 改用同一套枚举 + 残留核验。非 Linux 需要独立于 subreaper 的枚举兜底（例如启动时记录 session id 并对 `kill` 结果复核）。

## 8. 未核实

- 源码重建候选上的 `-run` 透传行为（只核对源码，未构建复验）。
- macOS/BSD 上 `tools/bound` 的 setsid 逃逸（`sweep_adopted` 空实现，静态可证不覆盖；未在该平台实测）。
- `np_run` 阻塞 `waitpid` 的极端挂起（未构造 D 状态复现）。
- csih/ujs 链上是否存在未读到的隐藏回边（只做文本级 `-run` 扫描 + 关键文件阅读；未做动态调用图）。
