# P2/P3 精确补丁方案：超时分支切到 sy_kill_tree

日期：2026-11-02。状态：只读方案，未动 `np_run`/`affected.cx`（等 cc 放行）。P1（`sy_kill_tree`/`sy_child_subreaper`）与 P5（`tools/check-reaper.cx`，24+12 断言绿）已落地。

## 0. 现状（证据，行号为当前工作区）

| 位置 | 现状 | 缺口 |
|---|---|---|
| `cx/sys/process.cx:100` | `kill(-pid, SIGKILL); kill(pid, SIGKILL); waitpid(pid, &st, 0);` | 组外后代（`setsid`、双 fork 丢父）不在组里，杀不到；杀完不复核，超时被当成"已清理" |
| `cx/sys/process.cx:8` | `np_result { rc, out, outn, err, errn }` | 没有残留字段，调用方拿不到"还活着几个" |
| `tests/run-batch.cx:39` | `np_run(args, root, 0, seconds, &result, reason, ...)` | 每个批次（budget 1..55 s）都走这条超时路径 |
| `tests/run-batch.cx:43-44` | `timed_out` → `status=UNKNOWN`、rc 3 | 有残留时仍是 UNKNOWN（不是 FAIL），残留证据丢失 |
| `tools/affected.cx:325-341` | `kill_tree()`：`pgrep -P` 广度优先 ≤256 个 pid，再逐个 `sy_killpg`+`sy_signal`+`sy_kill` | 外部 `pgrep` 依赖；`setsid` 后已丢父的后代扫不到；256 上限；无残留复核 |
| `tools/affected.cx:403-410` | `hard_deadline` 到点 → `kill_tree` → `DEFERRED id killed-at-deadline` | 同上；日志里看不出是否收干净 |
| `tools/cxlib/netpack.cx:754-826` | **同签名 `np_run` 的完整副本**（工具库内自带 `np_result`），超时分支 `kill(-pid)` 同上 | 同一缺口，另一份代码 |
| `tools/cxlib/e2stream.cx:143-180` | 自带 fork+轮询超时，只 `kill(pid, SIGKILL)` | 连进程组都没杀，更漏 |

依赖超时文本的现有契约（改动必须保留前缀）：
- `tests/run-batch.cx:43` `strncmp(reason, "subprocess.TimeoutExpired", 25)`
- `tools/cxlib/allsession.cx:320` 按 `run-batch: child transport subprocess.TimeoutExpired` 计数
- `tools/cxlib/netpack.cx:814`、`tools/cxlib/e2stream.cx:175` 生成同样前缀

## 1. P2：`cx/sys/process.cx` + `tests/run-batch.cx`

### 1.1 `np_result` 增加残留字段（唯一的接口形状变化）

```c
typedef struct np_result { int rc; char *out; size_t outn; char *err; size_t errn; int residue; } np_result;
```

- `residue`：正常退出 = 0；超时 = 杀完复核到的存活数（`sy_kill_tree` 返回值）；非 Linux（`-ENOSYS`）或回退组杀 = -1（"未复核"，不冒充 0）。
- `np_free` 里清零。224 个 `.cx` 引用 `np_run`，全部按名访问字段，`{0}` 初始化仍合法；无二进制 ABI（每次随产品重编）。

### 1.2 超时分支（`cx/sys/process.cx:98-117`）

```c
if (idle >= (long)secs * 1000) {
    size_t i;
    char rep[256];
    int sv = sy_kill_tree(pid, 2000, 0, 0, rep, sizeof rep);   /* 冻结→全杀→孤儿 sweep→复核 */
    if (sv < 0) { kill(-pid, SIGKILL); kill(pid, SIGKILL); }    /* 无 /proc：保留旧行为，residue=-1 */
    { int w; do { w = waitpid(pid, &st, WNOHANG); } while (w < 0 && errno == EINTR); }  /* 已由 reaper 收掉则 ECHILD，忽略 */
    res->residue = sv;
    if (open_o) close(po[0]);
    if (open_e) close(pe[0]);
    free(o.p); free(e.p);
    {   np_buf l = { 0, 0, 0 };
        ... 现有文本不变 ...
        if (sv > 0) { np_puts(&l, " ("); np_puts(&l, rep); np_puts(&l, " survived)"); }   /* 只追加，前缀不动 */
        snprintf(why, wl, "%s", l.p); free(l.p);
    }
    return -1;
}
```

要点：
- 前缀 `subprocess.TimeoutExpired: Command '[...]' timed out after N seconds` 一字不改；残留只在句尾追加 ` (pid(start) ... survived)`。
- 残留数进 `res->residue`，调用方不需要解析文本。
- `keep` 传 `0,0`：`np_run` 是单线程（工作区 `.cx` 无 `pthread_*`，并行靠多进程），同一进程里没有第二个在跑的根；同进程更早作业的遗留子进程 starttime 小于本次根，被 sweep 的 `starttime >= root` 下界排除，不会被误杀。
- `sy_kill_tree` 内部已 `waitpid` 根（≤500 ms），这里再等一次只为回收；`ECHILD` 忽略。

### 1.3 `tests/run-batch.cx`

1. `main` 里参数校验之后、`np_run` 之前：`sy_child_subreaper();`（整进程一次；让批次的 `setsid`/双 fork 孤儿被本进程收养，sweep 才看得见）。需要 `#pragma unisa use sys.process`。
2. 判定（`:43-46`）：

```c
int timed_out = transport && !strncmp(reason, "subprocess.TimeoutExpired", 25);
int leaked    = result.residue > 0;              /* 超时后仍活着 */
int ok        = !leaked && !transport && !result.rc && ... ;
printf("BATCH_RUN\tid=%s\tstatus=%s\trc=%d\ttransport=%d\tmarker=%s\tresidue=%d\tcandidate=%s\tscope=...\n",
       ..., ok ? "PASS" : (timed_out && !leaked) ? "UNKNOWN" : "FAIL", ..., result.residue, ...);
return ok ? 0 : leaked ? 1 : timed_out ? 3 : 1;
```

即：**有残留 → FAIL（不是 UNKNOWN）**；干净超时仍是 UNKNOWN/rc 3（"不是判决，要重跑"的既有语义不动）。`finish()` 只匹配 `\tstatus=PASS\t`，多一个 `residue=` 字段不影响解析。

## 2. P3：`tools/affected.cx`

1. `run()` 开头（`sy_mkdir(outparent...)` 之后、首个 `sy_spawn_pipe` 之前）调 `sy_child_subreaper()`。
2. 删 `kill_tree()`（`325-341`）、`parse_long` 里只为它服务的用法；`read_all` 仍被 `--users`/`--since` 用（`:447`、`:515`），保留。
3. `hard_deadline` 分支（`403-410`）：

```c
int keep[MAXJOBS], nk = 0, j, sv; char rep[256];
for (j = 0; j < nrun; j++) if (j != k) keep[nk++] = ch[j].pid;   /* 同时跑的其他批次根必须留 */
sv = sy_kill_tree(ch[k].pid, 5000, keep, nk, rep, sizeof rep);
if (sv < 0) { kill_tree_fallback(ch[k].pid); }                    /* 非 Linux：旧 pgrep 路径降级成 sy_killpg+sy_signal+sy_kill */
if (ch[k].ofd >= 0) sy_close(ch[k].ofd);
if (ch[k].efd >= 0) sy_close(ch[k].efd);
printf("DEFERRED\t%s\tkilled-at-deadline\tresidue=%d\n", batch[ch[k].bi].id, sv);
```

4. `-j>1` 时 `keep` 是必需的：其他批次根也是本进程的直接子进程，`ppid==self`，若不带 keep 会被孤儿 sweep 当作垃圾杀掉。这是 P3 唯一容易写错的地方。

## 3. 回归用例清单

| # | 命令 | 期望 | 覆盖的合同 |
|---|---|---|---|
| 1 | `sh ./unisacc.com tools/check-reaper.cx` | 12 PASS | 冻结树/逃逸链/keep 不动，残留 0 |
| 2 | 新增：check-reaper 第 3 组"np_run 超时"夹具：`np_run(["sh","-c","setsid sleep 300 & echo $! > f; sleep 300"], 1, ...)` | 返回 -1；`why` 以 `subprocess.TimeoutExpired: Command '` 开头；`res.residue == 0`；`f` 里的 setsid 后代 `sy_signal(pid,0)!=0`；超时前后 `/proc/self/fd` 数不变 | 组外后代被杀、残留字段、管道无泄漏 |
| 3 | 反向：把夹具改成"只冻不杀的残留"（`SIGKILL` 被 `SIGSTOP` 挡住不可能，用 `sy_kill_tree(pid, 0, keep=... 含该 pid)` 直调） | `residue >= 1`，`why` 追加 ` survived)` | 残留不是 0 可被看见 |
| 4 | `sh ./unisacc.com tools/check-procguard.cx` | 24 PASS | 杀路径与残留字段互不影响 |
| 5 | `tools/affected.cx -- --files cx/sys/process.cx tests/run-batch.cx tools/affected.cx --run -j 4` | `AFFECTED_RUN pass=… fail=0`（cx-package-refusals 的过期断言按既有裁定单列） | 批次执行路径整体 |
| 6 | `tests/run.cx -- --smoke` | 全绿 | 既有 SOURCE 批次的 rc/stdout/marker 解析 |
| 7 | 人为让一个批次超 budget（budget=1 的临时行，不提交） | 干净超时：`status=UNKNOWN`、rc 3；同时 `residue=0` | UNKNOWN 语义不被残留改造吞掉 |
| 8 | `sh ./unisacc.com tools/check-reaper.cx` 之后 `ps` 查 `setsid` 逃逸 pid | 不存在 | 无跨进程残留 |

不重复测：`all-waves`（禁跑）、`check-cx-sys*`（等 bdy 放行）。

## 4. 未纳入本批（需 cc 裁定）

1. `tools/cxlib/netpack.cx:754-826` 是 `np_run` 的第二份拷贝（工具库自己的 `np_result`）。切到 `sy_kill_tree` 需要在工具库 `#pragma unisa use sys.process`，并接受"库内 runner 依赖 L0 进程清理"；它有独立批次，属另一份改动。
2. `tools/cxlib/e2stream.cx:143-180` 只 `kill(pid)`，连进程组都没杀；同样改法，但 `e2stream` 的 `SECS` 是 2 ms 轮询语义，残留判定要单独定。
3. `np_run` 正常退出（非超时）时发现的遗留孤儿：目前 `np_result.residue` 仍填 0，不主动扫。要不要在正常退出后也做一次 `sy_kill_tree(pid, 500, keep, ...)` 收尾（会让"进程退出码 0 但留了后台子进程"从静默变成可见），由 cc 定。

## 5. 风险

- `np_result` 加字段：224 个 `.cx` 重编，源码级兼容，无 ABI；`{0}` 初始化仍合法。
- `sy_child_subreaper()` 是进程级：`run-batch`/`affected` 里除被测子进程外没有别人的子进程，收养范围可控；但一旦某工具在设了 subreaper 的进程里 fork 长驻子进程，它死后会被本进程收养为僵尸，需要显式 `waitpid`——本批两个入口都在退出前关/收全部子进程。
- `why` 文本追加只发生在 `residue > 0`；`allsession.cx:320` 的按行计数不受影响。
- 非 Linux 一律 `-ENOSYS`：回退旧组杀，`residue=-1`（未知 ≠ 0），不假称干净。

## 草稿实测补充（2026-11-02，out/p2-draft，未改 cx/、tools/）

`out/p2-draft/` 复制一份 `sys/process.cx` 打上 P2 补丁（`np_result.residue`、超时分支切 `sy_kill_tree(pid,2000,0,0,...)`、`-ENOSYS` 回退组杀、`waitpid WNOHANG`），配 `np-contract.cx` 24 断言全绿（`sh ./unisacc.com out/p2-draft/np-contract.cx`，9.0s）：

- A：`setsid` 逃逸子进程被杀且 `residue=0`；`subprocess.TimeoutExpired: Command '['…']' timed out after 1 seconds` 前缀与时长文本逐字不变；`/proc/self/fd` 计数前后相等（无 fd 泄漏）。
- B：双 fork 孤儿（`( setsid sleep 300 & )`）在**未设 subreaper** 时被 init 收走，扫描看不见 → `sy_kill_tree` 返回 `residue=0` 而进程仍活。**这就是 residue=0 会说谎的场景**，run-batch 必须 `sy_child_subreaper()`，否则超时后的“干净”判定不可信。
- B2：同一形状在设了 subreaper 后成为本进程子进程，被 sweep 杀掉，`residue=0` 才是真干净。
- B3：`keep` 列表内的 pid 不被停/杀/扫，也不计入 residue。
- D：正常路径不变：退出码 `rc=7` 透传、stdout/stderr 分别捕获、错误 cwd 走 `FileNotFoundError:`、被信号杀死报 `-9`。

结论：P2 的超时分支不必改协议；新增的 `residue` 只有在 `run-batch` 设了 subreaper 后才有“0 即干净”的含义，两者同一提交落地。

## 草稿实测补充 2（2026-11-02，P2b 抽取 + e2stream + netpack）

抽取方案已在 `out/p2-draft/` 落地并各自实跑：`sys/reaper.cx` 承载原 `kt_*` 块（导出 `rp_kill_tree`/`rp_subreaper`），`sys/process.cx` 只留两行包装 `sy_kill_tree`/`sy_child_subreaper`（对外接口不变），`cxlib.netpack.cx` 与 `cxlib.e2stream.cx` 各 `#pragma unisa use reaper`。

- **为什么必须抽模块**：`cxlib.netpack` 与 `sys.process` 同程序不可并存（`np_buf/np_result/np_run` 重复定义，已实测报错），所以 netpack 只能引一个不含 np_* 的第三方模块。裸名 `use reaper` 在 `cx/sys/` 内解析为同目录 `reaper.cx`；`tools/cxlib/netpack.cx` 用文件符号链接 `tools/cxlib/reaper.cx -> ../../cx/sys/reaper.cx`（先例 `tools/cxlib/codec.cx`）。
- **重复编译无风险**：两模块本就互斥，任何程序只经一条路径引入 reaper；`rp_kill_tree` 不会被链两次。
- **`check-reaper.cx` 对草稿全绿**：`sh ./unisacc.com out/p2-draft/check-reaper.cx` → `CX_REAPER ok=12 failed=0`，说明抽取是 drop-in。
- **netpack 草稿**：`out/p2-draft/netpack-contract.cx` 12 断言全绿——子进程 `setpgid`、超时 `rp_kill_tree(pid,2000,0,0,…)`、`np_result.residue`、前缀/时长文本逐字不变、`residue>0` 才追加 ` (pid(start) survived)`、`setsid` 后代被杀、cwd 错误仍走 `FileNotFoundError:`。
- **e2stream 草稿**：`out/p2-draft/e2-contract.cx` 9 断言全绿（经 `out/p2-draft/e2-child.cx` 实跑）：超时 `e2s_raise` exit 1、前缀/时长不变、无 survivor 后缀、`setsid` 后代被杀、正常路径 rc/stdout/stderr 与"子进程非零 rc 是数据"都不变。**e2stream 原来连进程组都没杀（只有 `kill(pid)`）**，本次同时加上 `setpgid`。
- **`e2s_*` 前置条件**：`e2s_run` 前必须 `e2s_start(file, prefix, root, cap, td, cap)`；未初始化时捕获文件落在 `/.capture-out` → `PermissionError`。

## 草稿实测补充 3（2026-11-02，P3 affected + 默认并行）

- **默认并行 = 核数（上限 8）**：新增 L0 `sy_ncpu()`（`cx/sys/process.cx` 内 `np_ncpu_os()` 读 `/sys/devices/system/cpu/cpu[0-9]*`，非 Linux 返回 -1，调用方自定回退 4）。实测 `ncpu=16` → `RUN_DIR … jobs=8`，`-j N` 仍是 1..8。
- **`kill_tree` 换实现**：删掉 `pgrep -P` 广度遍历（`tools/affected.cx:322-341`），改 `sy_kill_tree(pid, 2000, keep, nkeep, rep, cap)`，`keep` = 其余在跑子进程 pid（兄弟作业不会被一次截止误杀），`<0` 回退旧组杀；`hard_deadline` 行的输出只在有残留时多两列：`DEFERRED\tID\tkilled-at-deadline\tresidue=N\tsurvived=pid(start)`，无残留时形状与现在逐字相同（`check-affected.cx` 的 `start-deadline` 断言不受影响）。
- **`run()` 开头 `sy_child_subreaper()`**：`out/p2-draft/subreaper-probe.cx` 5 断言证明——设了 subreaper 时任务内双 fork 的后代（`setsid sh -c 'sleep 300 &'`）被 reparent 到本进程，被 sweep 杀掉且 `residue=0`；`--no-subreaper` 反向用例给出 `residue=0` 而后代仍活，正是"0 说谎"的实证。
- **affected 端到端**：`out/p2-draft/affected.cx -- --ids cx-register cx-reaper --run` → `pass=2 fail=0 jobs=8`；`--hard-deadline-ms 1000` → `DEFERRED cx-register killed-at-deadline`（无残留列）+ `AFFECTED_RUN pass=1 fail=1 deferred=1`。
- **环境噪声（非本改动）**：一次并行跑里 `cx-register` FAIL，原因是**别的车道同时写 `tests/*.tsv`**，`check-register` 的"启动快照逐字节复原"被第三方写入打断；同一命令重跑 `pass=2 fail=0`。切换后跑该批次时不得与其他注册写者并发。
- **`residue>0` 的输出分支无法构造**（SIGKILL 停不住；冻结后集合不再变大），该分支只有 `sy_kill_tree` 直调用例覆盖（见回归清单 #3），日志列保持 **UNMEASURED**。
