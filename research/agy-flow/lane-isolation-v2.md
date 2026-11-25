# 预跑车道与开发车道隔离 v2 设计说明 (按三轮静态审查闭环定稿)

## 1. 背景与审查阻断认领

`docs/agy-flow` 原草案补丁 (`abca047c`, `lane-isolation.patch`) 经 `ompgpt` 与 `cdx` 静态审查、`cc` 裁定不放行，不进入 `0.1.2` (V2)；`0.1.2` 预跑沿用现行 2 车道与既有准入白名单。本版 (`v2`) 逐条认领原五条阻断及二审、三审指出的并发准入与缓存 inode 隔离要求，锁定完整合同、资源声明 `--dry` 分析器与验证夹具：

(1) **阻断 1 (未接线、开发执行器未消费、默认未绑核却将 `DEFAULT_LANES` 从 4 升 6、缺 `taskset` 静默退化)**：
- 原补丁只定义了 `pd_parse_cpuset` 与 `fa_prerun_lease_set/clear`、`fa_dev_cpulist`，未在 `prerun-drive.cx` 主流程与 `flowact.cx` / `devsvc-core.cx` 派生入口实际调用，且在未绑核默认路径下把 `DEFAULT_LANES` 从 `4` 改为 `6`。
- **v2 修正**：
  - 未显式传 `--cpuset` 时，保持 `DEFAULT_LANES` 原有基线值 (`4`) 与原有行为不变，绝不静默提高并行度；
  - `prerun-drive.cx` 在 `--cpuset PRERUN_CPUS:DEV_CPUS` 入口显式调用 `pd_cpuset_parse_and_verify()` -> `fa_prerun_lease_acquire()`，在窗口循环期间由预跑主进程及全体窗口工作子进程树持有 `prerun-cpuset.lock` 的 `LOCK_EX` FD，主进程正常退出且确认全体后代已收割后调用 `fa_prerun_lease_release()`；
  - `flowact.cx` 的开发作业派生路径显式接入同一准入互斥协议（见 3.2 节），当预跑锁处于活跃持有态时把开发命令绑定到 `dev_cpus`，无预跑租约时让无绑核开发作业子进程持有 `prerun-cpuset.lock` 的 `LOCK_SH` FD 穿过其整个生命周期；
  - **绑核执行与读回强校验**：支持经环境变量 `UNISACC_TASKSET_BIN`（默认 `/usr/bin/taskset`）定位 `taskset`，并在子进程启动后读回 `/proc/<pid>/status` 的 `Cpus_allowed_list` 与目标 CPU 位图逐位比对；`taskset` 缺失、执行失败或亲和位图不匹配（含假 `taskset` 未实际改亲和性的情形）立即以 `rc=2` (`REFUSE LANE_CPUSET_UNAVAILABLE`) 拒绝，禁止静默无绑核运行。

(2) **阻断 2 (租约 `O_TRUNC` 无锁无原子发布、杀 wrapper 后代失锁、以及预跑获取/发布与开发派生之间的双向交错竞态)**：
- 原补丁直接 `O_WRONLY|O_CREAT|O_TRUNC` 写租约文件；二审进一步指出：若仅靠 wrapper PID/deadline 判失效，杀 wrapper 后后代仍裸奔；三审指出：(a) 预跑检查在途开发作业后到元数据 `prerun-cpuset.tsv` 发布前，开发进程仍可能并发派生或读到未发布/旧元数据；(b) 开发侧若拿 `LOCK_SH` 后立即释放再派生，新预跑的获取检查可与开发派生交错，导致无绑核开发进程与新租约重叠。
- **v2 修正（双稳定 inode 锁协议：`prerun-admission.lock` 串行准入互斥 + `prerun-cpuset.lock` 子树全生命周期持锁，失败关闭）**：
  - 在 `<lease_dir>/` 下固定两个创建后**永不 `unlink`** 的稳定 inode 锁文件：
    1. `<lease_dir>/prerun-admission.lock`：短临界区互斥锁 (`flock(adm_fd, LOCK_EX)`)，严格串行化“预跑获取锁 + 核验在途开发作业排空 + 原子发布 `prerun-cpuset.tsv`”与“开发作业准入判定 + 绑核/继承在途锁派生”；
    2. `<lease_dir>/prerun-cpuset.lock`：预跑长生命周期子树锁（FD **不设 `O_CLOEXEC`**，跨真实窗口执行器 `fork/exec` 继承，直至预跑主进程及**全部窗口工作后代**退出均持有 `flock(cpu_fd, LOCK_EX | LOCK_NB)`）；
    3. `<lease_dir>/dev-inflight.lock`：开发作业长生命周期在途子树锁（FD **不设 `O_CLOEXEC`**，**所有开发作业——无论无绑核还是在上一任预跑期间已绑核**——派生时一律继承持有 `flock(dev_fd, LOCK_SH | LOCK_NB)` 直至该开发子进程及其全部后代退出；父守护进程在 `fork` 后关闭父侧副本）。
  - **为什么该三锁协议完全消除双向竞态与活树重绑竞态（首版严格只排空、不在线重绑活树）**：
    - **预跑获取与发布（在 `prerun-admission.lock` 的 `LOCK_EX` 保护下一次性完成）**：
      1. 预跑先获取 `prerun-admission.lock` 的 `LOCK_EX`（此时任何新开发作业派生均在 `prerun-admission.lock` 外排队，不可能在检查后插队派生）；
      2. 在准入锁内，预跑对 `prerun-cpuset.lock` 尝试 `flock(cpu_fd, LOCK_EX | LOCK_NB)`：若返回 `EWOULDBLOCK`，说明上一轮预跑主进程或其工作后代仍在运行，立即释放准入锁并失败关闭 (`rc=2`, `REFUSE PRERUN_LEASE_HELD`)；
      3. **在途开发作业原子排空检查（不在线枚举重绑活树）**：由于活进程树在枚举 `/proc` 期间仍可 `fork` 或改线程亲和性，逐个在线重绑无法原子同步；因此预跑在 `prerun-admission.lock` 内对 `dev-inflight.lock` 尝试 `flock(dev_fd, LOCK_EX | LOCK_NB)`：
         - 若返回 `EWOULDBLOCK`：说明仍有之前启动的无绑核开发作业或上一任预跑期间启动的已绑核开发后代在途存活（持 `LOCK_SH`），预跑立即释放 `cpu_fd` 与 `adm_fd` 并失败关闭 (`rc=2`, `REFUSE PRERUN_DEV_INFLIGHT`)，待在途开发作业自然排空后再试；
         - 若 `flock(dev_fd, LOCK_EX | LOCK_NB)` 成功：内核原子保证**同租约目录且遵循本准入协议派生的在途开发进程及后代为 0**（未走 `devsvc` 准入协议的旁路命令不在该文件锁证明范围内）；预跑立即对 `dev_fd` 执行 `flock(LOCK_UN)` 并关闭 `dev_fd`（此时仍持有 `prerun-admission.lock` 的 `LOCK_EX` 与 `prerun-cpuset.lock` 的 `LOCK_EX`，遵循协议的新开发作业无法插队）；
      4. 预跑**仍在 `prerun-admission.lock` 内**写 `prerun-cpuset.tsv.tmp.<pid>` -> `fsync` -> 原子 `rename` 到 `prerun-cpuset.tsv`，读回校验 7 列完整后，才释放 `prerun-admission.lock`。
    - **开发作业派生（同样在 `prerun-admission.lock` 的 `LOCK_EX` 保护下完成）**：
      1. 开发执行器先获取 `prerun-admission.lock` 的 `LOCK_EX`；
      2. 在准入锁内，开发执行器先打开 `dev-inflight.lock`（不设 `O_CLOEXEC`）获取 `flock(dev_fd, LOCK_SH | LOCK_NB)`（使即将派生的开发子进程树全生命周期持有在途共享锁），随后打开 `prerun-cpuset.lock`（设 `O_CLOEXEC`）测试 `flock(cpu_fd, LOCK_SH | LOCK_NB)`：
         - **若 `cpu_fd` 的 `LOCK_SH` 成功**（当前无预跑持有 `LOCK_EX`）：立即释放并关闭 `cpu_fd`，在准入锁内直接 `fork/exec` 派生无绑核开发子进程（继承 `dev_fd` 的 `LOCK_SH`），父进程在 `fork` 返回后关闭父侧 `dev_fd` 副本，最后释放 `prerun-admission.lock`；
         - **若 `cpu_fd` 的 `LOCK_SH` 返回 `EWOULDBLOCK`**（预跑或其存活后代正持有 `LOCK_EX`）：关闭 `cpu_fd`，在准入锁内读取并校验 `prerun-cpuset.tsv`；**若 `prerun-cpuset.tsv` 缺失、为空、格式损坏或字段非法，立即关闭 `dev_fd`、释放准入锁并失败关闭 (`rc=2`, `REFUSE DEV_LEASE_METADATA_INVALID_WHILE_LOCKED`)，严禁按空记录或旧记录无绑核启动**；校验通过则在准入锁内派生 `taskset -c <dev_cpus>` 包裹的开发子进程（同样继承 `dev_fd` 的 `LOCK_SH`，以保护后续换租约边界）并读回 `/proc/<pid>/status` `Cpus_allowed_list` 确认绑核生效，父进程关闭父侧 `dev_fd` 副本后释放 `prerun-admission.lock`。

(3) **阻断 3 (CPU 列表解析漏洞、稀疏列表变连续区间、`lanes > cpus` 取模复用、未核两组不重叠与实际 affinity)**：
- **v2 修正**：
  - 使用 256 位严格位图 (`uint64_t mask[4]`) 表示 CPU 集合；
  - 语法机严格拒绝空串、首尾逗号 (`"0-3,"`)、连续逗号 (`"0,,1"`)、空白字符、非数字、逆序区间 (`"5-3"`)；逐位解析时在乘法累加前先核 `val > 255` (`REFUSE CPUSET_OUT_OF_RANGE`)；
  - 每次置位前检查 `(mask[c >> 6] >> (c & 63)) & 1`，若已置位立即拒绝 (`REFUSE CPUSET_DUPLICATE cpu=N`)；
  - 读取当前进程 `/proc/self/status` 的 `Cpus_allowed_list` 位图 `allowed_mask`，要求 `(prerun_mask & ~allowed_mask) == 0` 且 `(dev_mask & ~allowed_mask) == 0` (`REFUSE CPUSET_NOT_ALLOWED`)；
  - 强制要求 `prerun_mask != 0`、`dev_mask != 0` 且 `(prerun_mask & dev_mask) == 0`（交集非零即 `REFUSE CPUSET_OVERLAP`）；
  - 强制要求并行车道数 `lanes <= popcount(prerun_mask)`，禁止取模复用（不满足即 `REFUSE CPUSET_INSUFFICIENT_CPUS lanes=L cpus=C`）；
  - 从稀疏位图按实际置位重新格式化规范串（如 `0,2,5-7`），每个车道分配位图中不相交的置位 CPU，并用子进程读回 `/proc/self/status` 的 `Cpus_allowed_list` 验证实际亲和位图与分配位图逐位相等。

(4) **阻断 4 (原补丁仅在 `heavy` 查 `heavy_serial_only` 导致 `normal` 与 `long` 不查准入、`real-commit`/`list-cap*`/未知批未受保护、暖缓存硬链共享 inode 风险)**：
- **v2 修正**：
  - **保留既有准入白名单 (`normal_parallel_allow`)**，默认拒绝任何不在白名单内的批次进入多车道并行；`normal`、`long`、`heavy` 三档统一经过同一准入与冲突域检查；
  - 新增未知批次或拆片批次（如 `precommit-list-cap-a..d`）在未通过资源声明与实跑对跑证明前，一律强制为 `SERIAL_REQUIRED` (`lanes=1`)；
  - **核准运行库真实缓存环境变量并禁止硬链种子化 (`cx/runtime/modelcache.cx:221-331`)**：
    - 经核 `cx/runtime/modelcache.cx:221` (`mc_root_env`) 与 `src/runcache.cx:370`，运行库实际识别的缓存根目录变量为 `UNISACC_CACHE_DIR`（第一优先）与 `UNISACC_E3CACHE`（兼容，第二优先），未设置时回落到 `$XDG_CACHE_HOME/unisacc/e3cache` 与 `$HOME/.cache/unisacc/e3cache`，且要求目录权限为 `0700`；
    - 更关键的是：`cx/runtime/modelcache.cx:284` 在缓存目录下使用持久 inode `.budget.lock`，且 `cx/runtime/modelcache.cx:330` (`mc_acct_write`) 使用 `sy_write_file` **原位覆盖写** `.budget.acct`！若把暖缓存目录用硬链 (`link()`) 链入各车道，各车道会共享同一个 `.budget.lock` 与 `.budget.acct` inode，导致跨车道锁争用与账本原地覆写损坏；
    - 因此，**严禁使用硬链 (`link()`) 将暖缓存种子化到可写车道目录**：每车道设置 `UNISACC_CACHE_DIR=<prerun_out>/lanes/lane-<k>/e3cache`（同时设置 `UNISACC_E3CACHE=<prerun_out>/lanes/lane-<k>/e3cache`，权限 `0700`），从 `prepare` 暖缓存目录通过**独立文件复制或 `cp --reflink=auto` (CoW 克隆，不共享 inode)** 拷贝缓存条目（且跳过 `.budget.lock`，让每车道自建独立锁 inode）。

(5) **阻断 5 (回归夹具需覆盖真实窗口创建路径继承 FD、双向并发准入屏障、以及单车道缓存改写不影响源与另一道)**：
- **v2 修正**：在 `tools/check-lane-isolation.cx` 中补齐对应真实路径与并发屏障夹具（见第 4 节）。

---

## 2. 每批共享资源声明 `--dry` 分析器设计 (`analyze-isolation --dry`)

在放开任何新批次进入多车道之前，先实现只读分析模式：

```
sh ./unisacc.com tools/prerun-drive.cx -- analyze-isolation --dry [--batches tests/batches.tsv] [--out FILE]
```

### 2.1 可判定性原则与七个共享状态冲突域

分析器覆盖 `tests/batches.tsv` 中的全部入口（含 `tools/check-*.cx`、`tests/*.cx`）、`tests/manifests/<ID>.tsv` 声明材料及其传递派生的全部子进程：

- **可判定性铁律（无命中不等于隔离证明）**：
  - 静态文本扫描未发现敏感词**不能**直接推导为 `none`；
  - 一个批次在某冲突域判为 `none`，必须同时具备：(a) 批次清单或源码的显式资源隔离模式（如仅使用 `sr_scratch` / 车道私有目录，且只读访问仓根），以及 (b) 其全部子进程调用点的 `argv`、`cwd`、`env` 均可静态解析且落在隔离边界内；
  - 凡遇到动态拼接命令 (`sh -c` 动态串、非常量 `argv[0]`)、未受控 `chdir`/外部路径参数、未覆盖的外部脚本或缺失资源声明的批次，对应域一律标为 `unverified`（总裁决 `SERIAL_REQUIRED`）。

| 冲突域列 | 含义 | 判定为 `shared` | 判定为 `unverified` | 判定为 `none` (已证隔离) |
|---|---|---|---|---|
| `d1_worktree_git` | 工作树与 Git 引用/索引 (`HEAD`、`index.lock`、`refs/`、`build/freeze.tsv`) | 调用 `git commit`、`commit-own`、`push-replay`、`git update-ref`、`git worktree` 等写共享仓操作（如 `real-commit`、`precommit-list-cap*`） | 含动态 `git` 子命令或未证 `cwd` 的 `git` 调用 | 无 `git` 写操作，或已证 100% 落在 `sr_scratch`/`mkdtemp` 私有 `git init` 夹具仓内 |
| `d2_home_cache` | `$HOME` 与 `XDG_CACHE_HOME` (`~/.cache/unisacc`、`~/.unisacc-*`) | 显式读取真实宿主 `$HOME` 预存可变缓存或写宿主家目录固定路径 | 读取 `getenv("HOME")` 派生子进程但未证实是否兼容车道空 `HOME` | 不触 `$HOME`，或已证实可在车道私有 `HOME=<lane>/home`、`XDG_CACHE_HOME=<lane>/cache` 下独立通过 |
| `d3_run_cache` | 产品运行与模型缓存 (`UNISACC_CACHE_DIR` / `UNISACC_E3CACHE`) | 测试显式关闭车道私有缓存覆盖而直写共享缓存、或依赖跨批共享缓存状态 | 动态覆写 `UNISACC_CACHE_DIR` / `UNISACC_E3CACHE` 且目标非常量 | 继承车道独立拷贝/reflink 的 `UNISACC_CACHE_DIR=<lane>/e3cache` (`0700`)，不与源或其它车道共享 inode |
| `d4_registry` | 登记表与派生表 (`tests/batches.tsv`、`tests/coverage.tsv`、`tests/manifests/`、`build/*.tsv`) | 对检出根登记表或派生表执行 `--write` / `register.cx` | 调用通用表写工具且目标路径非常量 | 仅只读读取检出根表，或复制到 `sr_scratch` 私有目录后改写 |
| `d5_net_material` | 权重与网构造材料 (`out/`、`weights/`、`kernel/`、`netregen`) | 向共享 `out/` 或共享材料目录写入/重生成同名产物 | 含外部材料生成调用但输出目录不可静态解析 | 只读已准备好的 `PREPARED` 材料，或输出到每批专属 `sr_scratch` 目录 |
| `d6_fixed_tmp` | 固定 `/tmp` 路径 | 存在不含 PID / `mkdtemp` / `sr_scratch` 的固定 `/tmp/<name>` 读写 | 拼接 `/tmp/` 路径但模式无法静态证明唯一 | 全部临时文件均经 `sr_scratch`、`sy_mkdtemp` 或车道私有 `TMPDIR` 创建 |
| `d7_devsvc_daemon` | 常驻 `devsvc` 实例状态 (`locks/`、`jobs/`、`events.log`) | 不设私有 `UNISACC_DEVSVC_DIR` 而启停或请求共享仓 `devsvc` | 调用 `devsvc` 但环境未静态绑定私有实例目录 | 完全不调 `devsvc`，或显式绑定车道/夹具私有 `UNISACC_DEVSVC_DIR` 与 `DV_PIN_DIR` |

### 2.2 `--dry` 输出表格式

```
batch	tier	allowlist	d1_worktree_git	d2_home_cache	d3_run_cache	d4_registry	d5_net_material	d6_fixed_tmp	d7_devsvc_daemon	verdict	lane_policy
real-commit	long	no	shared	shared	none	none	none	none	shared	SERIAL_REQUIRED	serial-only
precommit-list-cap-a	normal	no	shared	none	none	none	none	none	shared	SERIAL_REQUIRED	serial-only
...
ISOLATION_DRY_SUMMARY	total=N	admitted_parallel=P	serial_required=S	unverified=U
```

---

## 3. 车道目录屏障与双锁准入状态机

### 3.1 每车道私有环境屏障 (`lane_env_barrier`)、独立缓存拷贝与后代收割硬门

当批次通过准入进入车道 `lane_k` (`k = 0 .. lanes-1`) 时，调度器为该车道维护专属根目录 `<prerun_out>/lanes/lane-<k>/` 并注入全部环境屏障：

1. `TMPDIR=<prerun_out>/lanes/lane-<k>/tmp`
2. `HOME=<prerun_out>/lanes/lane-<k>/home` 与 `XDG_CACHE_HOME=<prerun_out>/lanes/lane-<k>/cache`（落实 `d2` 屏障；若需只读基线二进制 `~/.unisacc-baselines`，仅在 `<lane>/home/.unisacc-baselines` 下建指向真实只读基线的符号链接）
3. `UNISACC_CACHE_DIR=<prerun_out>/lanes/lane-<k>/e3cache` 与 `UNISACC_E3CACHE=<prerun_out>/lanes/lane-<k>/e3cache`（落实 `d3` 屏障：对应 `cx/runtime/modelcache.cx:221` 的 `mc_root_env`；目录权限强制为 `0700`；初始化时从 `prepare` 暖缓存目录通过**独立文件拷贝或 `cp --reflink=auto`** 复制已完成缓存条目，**严禁 `link()` 硬链**，且不复制 `.budget.lock`，使每车道拥有独立 inode 的缓存文件、`.budget.lock` 与 `.budget.acct`）
4. `UNISACC_DEVSVC_DIR=<prerun_out>/lanes/lane-<k>/devsvc` 与 `DV_PIN_DIR=<prerun_out>/lanes/lane-<k>/dv-pin`（落实 `d7` 屏障）

**清理目录前的后代收割硬门**：
- 在窗口结束或下一窗复用 `lane-<k>` 清理 `<lane>/{tmp,home,cache,e3cache,devsvc}` 之前，调度器必须先终止并收割该车道进程组 (`-pgid`) 及会话内全部后代进程，并确认无任何存活进程仍打开或工作于 `<prerun_out>/lanes/lane-<k>/`；
- 若超时后仍有残留后代 (`residue > 0`)，**严禁在存活进程脚下删除或重置该车道目录**，立即将该车道标记为 `LANE_TAINTED` 并报 `RESIDUE` 失败。

### 3.2 三锁双向准入状态机 (`prerun-admission.lock` + `prerun-cpuset.lock` + `dev-inflight.lock`)

1. **预跑获取与原子发布 (`fa_prerun_lease_acquire`)**：
   - 打开 `<lease_dir>/prerun-admission.lock` (`O_CREAT | O_RDWR | O_CLOEXEC, 0600`) 并获取互斥锁 `flock(adm_fd, LOCK_EX)`；
   - 在 `adm_fd` 保护下，打开 `<lease_dir>/prerun-cpuset.lock` (`O_CREAT | O_RDWR, 0600`，**不设 `O_CLOEXEC`**，供真实窗口创建路径继承) 并尝试 `flock(cpu_fd, LOCK_EX | LOCK_NB)`：
     - 若返回 `EWOULDBLOCK`：说明仍有上一轮预跑子树持有 `LOCK_EX`，立即关闭 `cpu_fd`、释放 `adm_fd` 并失败关闭 (`rc=2`, `REFUSE PRERUN_LEASE_HELD`)；
   - `flock(cpu_fd, LOCK_EX | LOCK_NB)` 成功后，**仍在 `adm_fd` 保护下**打开 `<lease_dir>/dev-inflight.lock` (`O_CREAT | O_RDWR | O_CLOEXEC, 0600`) 并测试 `flock(dev_fd, LOCK_EX | LOCK_NB)`（**只排空、不在线重绑活树**）：
     - 若返回 `EWOULDBLOCK`：说明仍有在途开发子进程或其后代存活（无论是无绑核启动还是上一任预跑期间已绑核启动），立即关闭 `dev_fd`、释放并关闭 `cpu_fd`、释放并关闭 `adm_fd`，以 `rc=2` (`REFUSE PRERUN_DEV_INFLIGHT`) 拒绝，待在途开发作业自然排空后再试；
     - 若 `flock(dev_fd, LOCK_EX | LOCK_NB)` 成功：内核原子确认同租约目录下遵循准入协议的在途开发后代为 `0`（旁路命令不在锁证明范围），立即释放并关闭 `dev_fd`；
   - **仍在 `adm_fd` 保护下**写 `prerun-cpuset.tsv.tmp.<pid>` -> `fsync` -> 原子 `rename` 到 `prerun-cpuset.tsv`，并读回校验 7 列完整；
   - 释放并关闭 `adm_fd`，保留 `cpu_fd` (`LOCK_EX`, 无 `O_CLOEXEC`) 由真实窗口执行器派生的工作子进程树继承。
2. **开发作业准入与派生 (`fa_dev_spawn_admit`)**：
   - 打开 `<lease_dir>/prerun-admission.lock` 并获取 `flock(adm_fd, LOCK_EX)`；
   - 在 `adm_fd` 保护下，打开 `<lease_dir>/dev-inflight.lock` (`O_CREAT | O_RDWR, 0600`，**不设 `O_CLOEXEC`**) 获取 `flock(dev_fd, LOCK_SH | LOCK_NB)`，再打开 `<lease_dir>/prerun-cpuset.lock` (`O_CREAT | O_RDWR | O_CLOEXEC, 0600`) 测试 `flock(cpu_fd, LOCK_SH | LOCK_NB)`：
     - **分支 A (`cpu_fd` 的 `LOCK_SH` 成功，当前无活跃预跑租约)**：释放并关闭 `cpu_fd`，保持 `dev_fd` (`LOCK_SH`, 无 `O_CLOEXEC`) 打开并直接 `fork/exec` 派生无绑核开发子进程（使该开发子进程树全生命周期持有 `dev-inflight.lock` 的 `LOCK_SH`）；父进程在 `fork` 后关闭父侧 `dev_fd` 副本，释放并关闭 `adm_fd`；
     - **分支 B (`cpu_fd` 的 `LOCK_SH` 返回 `EWOULDBLOCK`，当前有预跑主进程或后代持有 `LOCK_EX`)**：关闭 `cpu_fd`，在 `adm_fd` 保护下读取并严格校验 `prerun-cpuset.tsv`；**若 `prerun-cpuset.tsv` 缺失、为空、格式损坏或不可读，立即关闭 `dev_fd`、释放 `adm_fd` 并失败关闭 (`rc=2`, `REFUSE DEV_LEASE_METADATA_INVALID_WHILE_LOCKED`)，严禁按空记录或旧记录启动开发子进程**；校验通过则在 `adm_fd` 保护下保持 `dev_fd` (`LOCK_SH`, 无 `O_CLOEXEC`) 打开，以 `taskset -c <dev_cpus>` 派生开发子进程并读回 `/proc/<pid>/status` `Cpus_allowed_list` 验证绑核生效，父进程关闭父侧 `dev_fd` 副本后释放并关闭 `adm_fd`。
3. **释放与到期回收 (`fa_prerun_lease_release` / `reclaim`)**：
   - 正常释放：预跑主进程确认全部窗口后代已收割后，获取 `prerun-admission.lock` (`LOCK_EX`)，核对 `prerun-cpuset.tsv` 中 `(owner_pid, owner_start_ticks, lease_token)` 一致后 `unlink("prerun-cpuset.tsv")`，再对 `cpu_fd` 执行 `flock(LOCK_UN)` 并关闭，最后释放 `adm_fd`；
   - 到期/孤儿强制回收：先终止并收割 `owner_pgid` 及持有 `prerun-cpuset.lock` 的全部后代，随后在 `prerun-admission.lock` (`LOCK_EX`) 内确认 `flock(cpu_fd, LOCK_EX | LOCK_NB)` 真正成功后，才 `unlink("prerun-cpuset.tsv")` 并释放双锁。

---

## 4. 验证夹具清单 (`tools/check-lane-isolation.cx`)

重写后的补丁随附 `tools/check-lane-isolation.cx`，包含以下 6 组确定性夹具（全部在私有临时目录下执行）：

1. **`cpuset-parser-strict` (对应阻断 3)**：
   - 负控：`"0-3,"`（尾逗号）、`"0,,1"`（空项）、`"0-3,2"`（重复核）、`"99999999999999999999"`（累加前溢出）、`"256"`（越界）、`"5-2"`（逆序）全部返回 `rc=2`；
   - 稀疏保持：输入 `"0,2,5:1,3"`，断言规范串为 `"0,2,5"` 与 `"1,3"`（不展开为 `"0-5"`）；
   - 交集与核数不足负控：`"0,2:2,3"` 返回 `REFUSE CPUSET_OVERLAP`；`prerun_cpus="0,2"` 配 `lanes=3` 返回 `REFUSE CPUSET_INSUFFICIENT_CPUS`。
2. **`cpuset-affinity-and-default` (对应阻断 1)**：
   - 默认不传 `--cpuset` 时，断言 `DEFAULT_LANES` 保持原基线 `4`（不升为 `6`），且不创建租约；
   - 注入 `UNISACC_TASKSET_BIN=/nonexistent/taskset` 以及注入不改亲和性直接 `exec` 的假脚本 `UNISACC_TASKSET_BIN=<tmp>/fake-noop-taskset.sh`，断言两种情况均被 `/proc/<pid>/status` `Cpus_allowed_list` 读回校验捕获并返回 `rc=2` (`REFUSE LANE_CPUSET_UNAVAILABLE`)；
   - 使用真实 `/usr/bin/taskset` 时，断言各车道子进程与开发子进程的 `Cpus_allowed_list` 与分配位图逐位一致。
3. **`lease-real-window-subtree-and-sigkill-wrapper` (对应阻断 2 与真实窗口创建路径继承 FD)**：
   - **通过真实窗口创建与执行器派生路径**启动预跑 wrapper 子进程 A 获取租约 (`prerun_cpus=0`, `dev_cpus=1`, `deadline=now+1s`)，由窗口执行器派生工作后代 A1（验证执行器未关闭或对 `cpu_fd` 设 `O_CLOEXEC`）；
   - 对 wrapper A 发送 `SIGKILL` 并 `waitpid` 确认 A 已死，同时等待超过 `deadline`，保持后代 A1 存活；
   - 在 A 已死且超期状态下断言：
     (a) 新预跑进程 B 调用 `fa_prerun_lease_acquire()` 因 `flock(LOCK_EX|LOCK_NB) == EWOULDBLOCK` 被拒绝 (`rc=2`, `REFUSE PRERUN_LEASE_HELD`)；
     (b) 开发侧调用 `fa_dev_spawn_admit()` 因 `flock(LOCK_SH|LOCK_NB) == EWOULDBLOCK` 仍强制按 `dev_cpus="1"` 绑核派生（绝不退化为无绑核，也不 `unlink` 租约）；
   - 终止并收割后代 A1 后，断言新预跑进程 B 可成功接管租约。
4. **`admission-bidirectional-barrier-race` (对应三审双向准入竞态、同锁目录真派生路径与两种拒绝码区分)**：
   - 全部子项均使用**同一租约目录与真实执行器派生路径**，并显式区分 `REFUSE PRERUN_LEASE_HELD`（`prerun-cpuset.lock` 仍被旧预跑或其后代占锁）与 `REFUSE PRERUN_DEV_INFLIGHT`（`prerun-cpuset.lock` 空闲但 `dev-inflight.lock` 仍被在途开发后代占锁）：
   - (a) **在途无绑核开发作业持 `dev-inflight.lock` `LOCK_SH` 阻断新预跑**：先经 `fa_dev_spawn_admit()` 真派生路径启动无绑核开发子进程 D1（持有 `dev-inflight.lock` 的 `LOCK_SH`），在 D1 存活时启动新预跑 P1，断言 P1 通过 `prerun-cpuset.lock` 后在 `flock(dev_fd, LOCK_EX|LOCK_NB)` 处收到 `EWOULDBLOCK` 并精确返回 `rc=2` (`REFUSE PRERUN_DEV_INFLIGHT`，区别于 `PRERUN_LEASE_HELD`)，直到 D1 退出排空后 P1 才可获取；
   - (b) **同步屏障并发推进**：用管道同步屏障同时推进“预跑获取+发布”与“开发派生准入”，断言在 `prerun-admission.lock` 串行化下，开发子进程要么在预跑拿锁前以 `LOCK_SH` 启动（此时预跑被阻断或重绑它），要么在预跑发布完成后读取完整 `prerun-cpuset.tsv` 以 `dev_cpus` 绑核启动，绝不出现“无绑核开发进程与已激活预跑租约并存”；
   - (c) **元数据未发布/损坏失败关闭负控**：模拟 `prerun-cpuset.lock` 被持 `LOCK_EX` 但 `prerun-cpuset.tsv` 缺失或损坏的故障态，断言 `fa_dev_spawn_admit()` 立即返回 `rc=2` (`REFUSE DEV_LEASE_METADATA_INVALID_WHILE_LOCKED`)，绝不按空记录或旧记录启动；
   - (d) **换租约改核集合与上一任已绑核开发后代排空负控 (`lease-rotation-pinned-dev-drain`)**：预跑 A 以 `prerun_cpus=0, dev_cpus=1` 获取租约；A 期间派生已绑核开发作业 D2 及其后代 D2a（绑在 CPU `1` 上，继承持有 `dev-inflight.lock` 的 `LOCK_SH`）；随后预跑 A 正常结束并释放 `prerun-cpuset.lock`，但 D2a 仍在 CPU `1` 上存活；此时新预跑 B 请求换核租约 `prerun_cpus=1, dev_cpus=0`：断言 B 在 `prerun-admission.lock` 内对 `dev-inflight.lock` 取 `LOCK_EX | LOCK_NB` 收到 `EWOULDBLOCK`，**不在线枚举重绑活树**，直接失败关闭返回 `rc=2` (`REFUSE PRERUN_DEV_INFLIGHT`) 且不发布新租约；待 D2a 正常退出排空后，B 再次调用立即获准并发布新租约。
5. **`schedule-allowlist-and-analyzer-soundness` (对应阻断 4)**：
   - 构造含 `real-commit` (`long`)、`precommit-list-cap-a` (`normal`)、含动态 `sh -c` 的 `dynamic-unverified-batch` (`normal`)、未知新批 `unknown-new-batch` (`normal`) 及已证白名单批的夹具；
   - 运行 `analyze-isolation --dry`，断言“文本扫描无敏感词但含动态 `argv`/`cwd` 或无声明”的批次被判为 `unverified` -> `SERIAL_REQUIRED`，`real-commit` (`long`) 与 `precommit-list-cap-a` (`normal`) 均被判为 `SERIAL_REQUIRED` (`lanes=1`)。
6. **`lane-cache-no-hardlink-and-reap-before-clean` (对应三审暖缓存禁止硬链与清理前收割硬门)**：
   - 准备一个含缓存条目与 `.budget.acct` 的暖缓存源目录，初始化 `lane-0` 与 `lane-1` 的 `UNISACC_CACHE_DIR` (`<lane>/e3cache`, mode `0700`)；
   - 断言 `lane-0/e3cache`、`lane-1/e3cache` 与源目录中各同名文件的 `st_ino` 互不相同（零硬链共享 inode）；
   - 在 `lane-0/e3cache` 中原地改写一个缓存条目及 `.budget.acct`，断言源目录与 `lane-1/e3cache` 对应文件的字节与 SHA256 完全不变；
   - 模拟某车道窗口超时后仍有存活后代进程时，断言调度器拒绝在该存活后代脚下清理车道目录并报 `LANE_TAINTED` / `RESIDUE`。

---

## 5. 实施顺序

1. 第一步：先实现只读分析器 `analyze-isolation --dry` 与 `tools/check-lane-isolation.cx` 夹具 1、5，对现行 `tests/batches.tsv` 生成完整 7 域资源声明表；
2. 第二步：实现 256 位严格 CPU 位图解析、`prerun-admission.lock` + `prerun-cpuset.lock` 双锁双向准入状态机、基于 `UNISACC_CACHE_DIR` 非硬链拷贝的 4 项车道目录屏障及夹具 2、3、4、6；
3. 第三步：交 `ompgpt` 与 `cdx` 复核全部 6 组夹具绿灯后，在下一次预跑前接入验证。

---

## 6. 阶段 2 实编实跑验收记录 (2026-11-25)

在私有目录 `/tmp/lane-v2-work` (基线 `7b49a9e473302e218e1f808d34856b855d44ad92`), 负载 `load1 = 2.18 ~ 5.09 < 8`, 以单进程 `CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" nice -n 19 taskset -c 10-15` 完成全部 3 项验收命令, 全部通过 (`rc=0`)。

### 6.1 阶段 2 实跑中发现并修复的 3 处问题

(1) `tools/prerun-drive.cx:1229`: `getpgid((int)pid)` 未声明 -> 改用 `sys.posix` 已导出的 `sy_getpgid((int)pid)`。
(2) `tools/prerun-drive.cx:3378`: `analyze-isolation --dry` 中 `pd_is_heavy_id(bid)` 抢先于 `tx_eqz(bid, "real-commit")` 导致 `real-commit` 的 `tier` 列输出为 `heavy` 而非 `long` -> 调整顺序为先判 `pd_is_isolated_long_id(bid) || tx_eqz(bid, "real-commit")`, 不改动 `plan` 调度的 `pd_is_heavy_id`。
(3) `tools/check-prerun-drive.cx:637`: 既有单测 `singleton-window-lanes-start-before-the-first-receipt` 使用虚拟批名 `alpha`/`beta` 测试 `window-lanes=2` 下多车道并发启动; 因 `lane-isolation-v2` 按安全合同把未知 `long` 批默认收紧为串行 (`SERIAL_REQUIRED`), 绝不为 `--mock-runner` 开后门, 将该测试块中的虚拟批名更新为白名单内合成 ID `netregen-alpha`/`netregen-beta` (保留全部断言与并发见证逻辑)。

### 6.2 验收命令逐字原行

(1) `sh ./unisacc.com tools/check-lane-isolation.cx --` (6/6 PASS, `RC=0`):

```text
PASS	check-lane-isolation.cpuset-parser-strict
PASS	check-lane-isolation.cpuset-affinity-and-default
PASS	check-lane-isolation.lease-real-window-subtree-and-sigkill-wrapper
PASS	check-lane-isolation.admission-bidirectional-barrier-race
PASS	check-lane-isolation.schedule-allowlist-and-analyzer-soundness
PASS	check-lane-isolation.lane-cache-no-hardlink-and-reap-before-clean
CHECK_LANE_ISOLATION	passed=6	failed=0
RC=0
```

(2) `sh ./unisacc.com tools/prerun-drive.cx -- analyze-isolation --dry` (12 列 TSV 共 2,040 行, `RC=0`):

```text
batch	tier	allowlist	d1_worktree_git	d2_home_cache	d3_run_cache	d4_registry	d5_net_material	d6_fixed_tmp	d7_devsvc_daemon	verdict	lane_policy
netregen-small-lnx-x86_64	heavy	yes	none	none	none	none	none	none	none	ADMITTED_PARALLEL	multi-lane
real-commit	long	no	shared	shared	none	none	none	none	shared	SERIAL_REQUIRED	serial-only
precommit-list-cap	normal	no	shared	none	none	none	none	none	shared	SERIAL_REQUIRED	serial-only
ISOLATION_DRY_SUMMARY	total=2038	admitted_parallel=69	serial_required=1969	unverified=1915
RC=0
```

(3) `sh ./unisacc.com tools/check-prerun-drive.cx --` (94/94 PASS, `RC=0`):

```text
PASS	singleton-window-lanes-start-before-the-first-receipt
PASS	slow-split-and-unit-frontcache-equiv-isolated-parallel-lanes: isolates slow-split and unit-frontcache-equiv0..4 into 1-batch long windows running on 4 parallel window lanes
CHECK_PRERUN_DRIVE	ok=94	failed=0	ms=17285
RC=0
```

### 6.3 导出补丁与核验摘要

- 补丁路径: `research/agy-flow/lane-isolation-v2.patch` 与 `~/.local/share/unisacc/research/agy-flow/lane-isolation-v2.patch` (2,129 行, 含 5 文件: `tools/cxlib/flowadmit.cx`, `tools/cxlib/flowexec.cx`, `tools/prerun-drive.cx`, `tools/check-lane-isolation.cx`, `tools/check-prerun-drive.cx`)
- `sha256`: `c164d6b488b52abf2d7703e719eb6663a395907ae019254578c8f1a4c1aac67f`
- `git apply --check`: `APPLY_CHECK_RC=0`
- `EVIDENCE_HOME_PATHS`: `0` (patch 与 md 均为 0)
