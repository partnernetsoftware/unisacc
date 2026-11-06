# C44-TOOLS-01 工具链事件化 + 事件式并行工作流：设计（2026-11-01，只读调研）

目标（prd/v0.0.44）：改一个文件 → 自动重算模块关系图、自动浮现受影响下游、自动并行派发只读检查，agent 拿「谁 use 谁 / 谁调用谁 / 下游哪批受影响」的图，不用自己 grep。三层：① 事件层（缺，新造）；② 图 + 增量层（已有 symindex，接上）；③ 并行工作流层（缺，设计）。原则：**读并行、写串行、按冲突域分**——废止 AGENTS.md「单机同时只做一组重测试」这句。

本文只读调研，不改 src/cx/tests 任何源码。所有结论给「实测数字 + 可复现命令」。

---

## 0. 现状核对（实测，2026-11-01，已装 83343ec1）

### 0.1 symindex.cx 已经能做什么（图 + 增量层的底子）

`tools/symindex.cx`（1109 行，注册批次 `symindex-tool`）是一个**内容寻址的增量词法事实索引**，每个文件一份 TSV 分片 `out/symindex/v1/<sha256>.tsv`，事实行 M/U/D/P/T/X/C（模块头/use/定义/原型/typedef/宏/调用）。关键机制：

- **增量**：`update` 逐个 `lstat`，(size, mtime, ino, dev) 全同就复用清单里的 sha256，不读文件；否则重 hash；分片缺失才重 parse。只重算变化的文件。
- **身份**：manifest 记 `path size mtime_s ino dev sha256`；`--verify` 全量重 hash 报 STALE（同大小同 mtime 的改写只有它能抓）。
- **闭包**：`uses FILE` 正向、`users FILE` 反向 `#pragma unisa use` 传递闭包（按 realpath）；`def NAME`、`calls NAME[,NAME...]`（输出与 `refactor.cx -- sites` 逐字节同）；`undeclared`、`external`、`dump`。

实测（可复现）：

```sh
# 冷建（清 out/symindex 后）
sh ./unisacc.com tools/symindex.cx -- update
#   实测 SYMINDEX files=1000 reparsed=994 hashed=1000 elapsed_ms=2421（plan 记录）

# 热查询（无改动）：reparsed=0 hashed=0，~250ms（读 ~9.6MB 分片）

# users 反向闭包：cx/sys/fs.cx 被 189 个文件 use
sh ./unisacc.com tools/symindex.cx -- users cx/sys/fs.cx
#   → USERS file=cx/sys/fs.cx count=189

# 调用点（格式与 refactor sites 相同）
sh ./unisacc.com tools/symindex.cx -- calls sy_open,sy_read,sy_poll
```

仓库规模：`git ls-files '*.cx'` = **1961** 个跟踪文件；当前 `out/symindex/v1/` 已有 **2647** 个分片。

**缺什么（相对本任务目标）**：
1. 产出是**命令行文本**，不是图结构。agent/上层要自己解析 `USE\tpath`、`USER\tpath` 行，没有「节点 + 边」的一等产物。
2. 是**手动调**，没有事件触发，也不生成「改了一组文件 → 受影响下游集合」的差分。
3. `uses/users` 只覆盖 `#pragma unisa use` 闭包，不把 `calls` 的调用边、`def` 的定义边并进同一张图。

### 0.2 affected.cx 已经能做什么（「改文件 → 受影响批次」的底子）

`tools/affected.cx`（532 行，注册批次 `affected-tool`）读 `tests/batches.tsv` + `tests/coverage.tsv`（经 `cxlib.testplan`），对每个 IMPLEMENTED+SOURCE 批次算其 root 的 use 闭包，按 via=source/use/literal/literal-use/arg/arg-use 七类判定「哪些批次被这些文件命中」。已带 `--run -j N`（N 1..8）并行派发，和 45s/54s 双 deadline + 进程组 kill。

实测（可复现）：

```sh
sh ./unisacc.com tools/affected.cx -- --files cx/sys/fs.cx
#   → AFFECTED_SUMMARY changed=1 batches=78 scanned=336
#   （78 个受影响批次，336 个 IMPLEMENTED+SOURCE 批次被扫）
```

**缺什么**：
1. `--run` 的并行度 `-j N` 是「无差别并行」：**没有冲突域概念**，会把「写同一份共享可变状态」的任务也并行（它现在只跑 SOURCE 批次、不碰材料/候选，所以碰巧安全；一旦扩展到会写工作树/候选的任务就有竞态）。
2. 事件触发、图产物、限并发策略都没有。

### 0.3 事件层：cx/sys 现在**没有**文件监听能力

在 `cx/` 全树 grep `inotify|FSEvents|ReadDirectoryChanges|kqueue|fanotify|watch`（大小写不敏感）——**0 个真正的文件监听原语**，命中的只有 `cx/workspace/driver.cx` 里的 `cxw_watchdog`（那是对 `tools/bound` 做内容 sha256 的看门狗，不是 fs watch）。

`cx/sys/` 现有 16 个具名模块 + 4 个非 `sys.` L0 文件（见 `cx/sys/README.md`），**没有一个**做目录/文件变更监听：

| 相关模块 | 提供 | 与监听的关系 |
|---|---|---|
| `sys.poll` | `sy_poll(fds,n,ms)`，SY_POLL* 位 | **事件层的等待半成品**：inotify/kqueue 的 fd 是普通 fd，`sy_poll` 现成可等 |
| `sys.posix` | `syp()` 薄 syscall 封装（`__syscall6`），已有 `syp_ioctl`/`syp_openat`/`syp_dents` 等按 arch 分号表 | **事件层的 syscall 通路模板**：inotify/kqueue 数字照它的 per-arch `static const long` 表加 |
| `sys.fd` | `sy_dup`/`sy_dup2` | 把 watch fd 复制/关闭 |
| `sys.time` | `sy_now_ms`/`sy_sleep_ms`/`sy_readable` | 超时与空闲节流 |
| `sys.proc` | `sy_pipe`/`sy_spawn_pipe`/`sy_fork_session` | 工作流层派发子任务用（Windows 侧用 pipe 传事件也行） |

主机头文件核对：本机 Linux 有 `/usr/include/linux/inotify.h` 和 `/usr/include/sys/inotify.h`（`struct inotify_event`、`IN_CLOSE_WRITE=0x8`、`IN_MOVED_TO=0x80`、`IN_ALL_EVENTS`、`inotify_init1` 等常量齐全）。macOS FSEvents/kqueue、Windows ReadDirectoryChangesW 的头只在对应宿主，本机不可见。

**结论：事件源要新造，落在 `cx/sys` 新增一个 `watch` 模块（L0，`use sys.watch`）。** 等待用现成 `sys.poll`，syscall 走现成 `sys.posix` 的 `syp` 通路，不另起炉灶。

---

## 1. 三层架构总览

```
┌─────────────────────────────────────────────────────────────────────┐
│ ③ 并行工作流层  tools/flow.cx（新，L3 工具）                          │
│    事件/一次性请求 → 增量重算 → 图差分 → 按冲突域并行派发只读检查      │
├─────────────────────────────────────────────────────────────────────┤
│ ② 图 + 增量层   tools/symindex.cx（改） + out/toolgraph/v1/（新产物）  │
│    update --graph：增量索引 → 节点+边 TSV（模块=节点，use/调用=边）    │
├─────────────────────────────────────────────────────────────────────┤
│ ① 事件层        cx/sys/watch.cx（新，L0，归 omp-go）                  │
│    sy_watch_*：inotify / kqueue(FSEvents 备选) / ReadDirectoryChangesW │
└─────────────────────────────────────────────────────────────────────┘
```

- ①②之间的接口：`sy_watch` 给一个可 `sy_poll` 的 fd + 事件队列（path, mask, cookie）。
- ②③之间的接口：`symindex --graph [--for FILES]` 吐 TSV 图；`affected --files F` 吐受影响批次（已有）。
- ③对外：`flow.cx -- watch`（常驻，事件驱动）/ `-- check FILES`（一次性，可脚本化）/ `-- graph FILES`。

---

## 2. 第 ① 层：事件源（cx/sys，omp-go 那一半）

### 2.1 落点

新增 **`cx/sys/watch.cx`**：`#pragma unisa module watch`，上层 `use sys.watch`。属于 L0（cx/sys 在 cxraw 白名单，可直接 libc/syscall）。平台差异收在 `static …_os` 里，公开接口无 `#if`——与 `fs.cx`/`time.cx`/`posix.cx` 的既有范式一致。

### 2.2 公开接口（跨平台统一的形状）

```c
/* 事件掩码（Linux inotify 语义的最小公分母，三平台都映射到它） */
enum {
    SY_W_MODIFY  = 0x02,   /* 文件被写 */
    SY_W_ATTRIB  = 0x04,   /* 元数据变（chmod/utimes） */
    SY_W_CLOSE_W = 0x08,   /* 可写 fd 关闭（编辑器保存的可靠信号） */
    SY_W_CREATE  = 0x100,  /* 目录里新建 */
    SY_W_DELETE  = 0x200,  /* 目录里删除 */
    SY_W_MOVED_TO = 0x080, /* 改名/移入（rename 的目标） */
    SY_W_SELF    = 0x400,  /* 被 watch 的条目本身被删/移走 */
};

typedef struct {
    char path[4096];   /* 全路径（sy_watch 内部按 wd→dir 拼好，上层不用再映射） */
    int  mask;         /* 命中掩码 */
    int  isdir;        /* 事件对象是目录 */
    long cookie;       /* 改名配对 cookie（inotify/kqueue 有；Windows 0） */
} sy_watch_event_t;

/* 初始化监听器，返回一个可 sy_poll 的 fd；失败 -errno。 */
int  sy_watch_init(void);
/* 登记 PATH（目录则目录本身，recursive=1 时还要递归登记子目录——上层亦可自行逐目录调） */
int  sy_watch_add(const char *path, int recursive);
/* 读一批事件到 EV 数组；返回条数、0、或 -errno（-EAGAIN 无事件）。fd 就绪了再调。 */
long sy_watch_read(sy_watch_event_t *ev, int max);
/* 关闭并回收。 */
int  sy_watch_close(void);
```

等待：监听 fd 是普通 fd（Linux inotify / macOS kqueue 都是），上层 `sy_poll` 现成等；Windows 例外见 2.4。

### 2.3 跨平台怎么分（三档，首刀只做 Linux）

| 平台 | 机制 | 实现载体 | 备注 |
|---|---|---|---|
| Linux x86_64/arm64 | **inotify**（`inotify_init1`+`inotify_add_watch`） | `__syscall6`，走 `posix.cx` 的 `syp` 同款 per-arch 数字表 | x86_64: init1=294, add_watch=254, rm_watch=255；aarch64 数字在该宿主 `asm/unistd.h` 填（init1=27/add=28/rm=29，编译该目标时核对）。recursive 由上层逐目录 `sy_watch_add`，不靠内核 |
| macOS x86_64/arm64 | **kqueue EVFILT_VNODE**（首选，因为返回 fd、能 `sy_poll`）；FSEvents 仅当要整树递归时作备选 | kqueue 是 syscall 378（macOS 通用）；FSEvents 是 CoreServices 宿主 API，不进 L0，只在需要时由上层经 `sys.dlcall` 调 | 与 inotify 一样：逐目录 watch，`NOTE_WRITE/NOTE_DELETE/NOTE_RENAME/NOTE_ATTRIB` 映射到掩码 |
| Windows x86_64/arm64 | **ReadDirectoryChangesW**（异步 + 完成端口/事件句柄） | `_ux_sym("ReadDirectoryChangesW")` 等，照 `time.cx`/`fs.cx` 里已有的 WinAPI 宿主调用样式 | **Windows 没有 fd 语义**：watch 句柄不能进 `sy_poll`（`poll.cx` 在 Windows 返回 -ENOSYS）。做法见 2.4 |

### 2.4 Windows 的 fd 缺口（明确的边界）

`sys.poll` 在 Windows 是 `-ENOSYS`，`sy_watch_init` 返回的句柄等不了。两条路，本设计选 ②：

- ① 给 `sys.poll` 加 Windows 的 `WaitForMultipleObjects` 实现——动 `poll.cx`，牵动面大；
- ② **`sy_watch` 在 Windows 内部自起一个线程**（用 `sy_fork_session` 不适用，得用 WinAPI CreateThread），把 ReadDirectoryChangesW 的事件通过一条**具名管道**喂给读端 fd，上层继续只靠 `sy_poll` 等那个读端 fd。事件从「句柄」转成「字节流 + 普通 fd」，`sy_poll` 在 Windows 仍等不了——所以 Windows 上 `flow.cx -- watch` 的常驻等待退化为 `sy_readable` 轮询或短超时循环。

**首刀明确：Windows 半做。** Linux 全功能；macOS kqueue 按 2.3 的 per-arch 表写好、留「编译验证」标记（照 `posix.cx`/`sig.cx` 里「written from the contract, UNMEASURED on Mac」的既有标注法）；Windows 只落接口骨架 + `-ENOSYS`，ReadDirectoryChangesW 的真实实现和句柄→fd 桥**不在首刀**，等本机 Linux 事件链跑通后单独一刀。

### 2.5 omp-go 交接边界（写死，双方各持一半）

| 归 omp-go（cx/sys，L0） | 归本任务（tools，L2/L3） |
|---|---|
| `cx/sys/watch.cx` 整个模块：`sy_watch_init/add/read/close`、事件掩码、三平台 `*_os`、Windows 句柄→字节流桥 | 不碰 cx/sys；只用 `use sys.watch` 消费 |
| 保证：返回可 `sy_poll` 的 fd；事件是「全路径 + 掩码 + cookie」；不吞错误（0/-errno，同 cx/sys 契约） | `flow.cx` 里做**去抖/合并**（同一文件 50–200ms 内的 MODIFY+CLOSE_WRITE 合成一次）、把事件映射到「脏文件集」、触发重算 |
| 保证：`recursive` 语义只到「本目录 + 已登记的子目录」，不自动跟新目录（上层补 watch） | 决定 watch 哪些目录（仓库根下的 cx/tools/tests/src/…，忽略 out/、.git/），事件里新目录出现时补 `sy_watch_add` |

**交接物**：① 一个可运行的 Linux 冒烟夹具（`touch`/改字节 → 事件读得到），② 接口签名与掩码表（即 2.2）。图/工作流半不依赖 watch 落地也能先用「一次性」模式跑（见 4.2 首刀），所以两条线并行不互相卡。

---

## 3. 第 ② 层：图 + 增量层（symindex 从手动改事件触发，产出图结构）

### 3.1 目标图结构（agent 读的产物）

产出 **`out/toolgraph/v1/graph.tsv`**，一个文件、三行种、TSV、`\t\n\\` 转义（与 shard 一致）：

```
V   toolgraph-v1
N   <path>          <module>        <sha256>        <nlines>
E   <from-path>     <to-path>       <kind>          <site-line>
```

- **节点 = 文件**（按 realpath 去重，`N` 行的 path 是 repo 相对拼写，module 是 `#pragma unisa module` 名或 `-`）。
- **边 kind**：`use`（A `#pragma unisa use` B，从 U 行）、`call`（A 调 B 顶层定义的符号，从 C 行对 D 行按名 join）、`def`（B 定义某符号，属节点属性也可不单独成边）、`include`（预留，本层先不解析 `#include`，和现状 symindex 一致——L0 include 归 cxinclude 棘轮管）。
- 节点的 sha256 就是分片寻址键，**图和分片天然同源**：图可从 `manifest.tsv` + 各 shard 的 U/C 行完全重建，不引入第二套身份。

### 3.2 symindex 怎么改（增量 + 事件化，最小改动）

现有代码里，`resolve_uses`（U 行→entry 边）、`collect_defs`（D 行→名字）、`calls_of`（C 行→名字）已经把三件事都算过一次了，只是**算完就丢、只按命令行临时用**。所以图能力是把这些临时中间结果**固化成一次遍历**：

1. 新增子命令 **`symindex.cx -- graph [--for FILE...]`**：
   - 先做一次现成的增量 `update_all()`（复用 stat 预筛，只重算变化文件）。
   - 对每个 listed entry：`load_shard` 后，把 U 行解析成 `use` 边（复用 `resolve_uses` 的路径解析逻辑）；把 C 行按名 join 到 `def_head` 的定义表（复用 `collect_defs`），产出 `call` 边。
   - 输出 `out/toolgraph/v1/graph.tsv` + stdout 摘要行 `GRAPH files=N nodes=M edges=K elapsed_ms=T`。
   - `--for FILE...`：只输出「这组文件的（反向）闭包子图」——正向 use 闭包 + 反向 users 闭包 + 定义/调用邻接，**这就是 agent 要的「改一个文件后的局部图」**。
2. 新增子命令 **`symindex.cx -- delta [--since REV | --files F...]`**：等价于 `affected` 的「文件集」输入，但输出**图差分**——哪些节点 sha 变了、哪些边增删、受影响下游（users 闭包）集合，一行一条 `DELTA path old_sha new_sha`。可复现命令与 `affected --files` 对齐。
3. **事件触发**：加一个「watch 模式」入口 `symindex.cx -- watch`（或交给 flow.cx，见第 4 层）——`sy_watch_read` 拿到脏文件集 → `--delta --files 脏集` → 打印差分。为避免和 ③ 重复，本设计把**事件循环放在 flow.cx**，symindex 保持「无状态、一次调用算一件事」，只提供 `--graph/--delta` 两个纯函数式子命令；事件循环是 flow 层的事。这样 symindex 改动最小、可独立测试（check-symindex 直接断言 graph.tsv 的节点/边）。

### 3.3 已有 vs 新造（本层）

| 已有可复用 | 缺、要新造 |
|---|---|
| 增量 stat 预筛 + 内容寻址分片（`update_all`/`index_one`/`load_shard`） | `--graph` 的「一次性图固化」子命令 |
| U 行解析（`resolve_uses`）、D 行集合（`collect_defs`）、C 行遍历（`calls_of`） | C 行按名 join 到 D 行的「call 边」装配（现在是分开算的，没 join） |
| realpath 去重、`hreal` 表 | `--delta` 的「旧图 vs 新图」差分输出 |
| shard/manifest 的 TSV + 转义 + 原子写（`write_atomic`） | `out/toolgraph/v1/` 目录与 graph.tsv 的原子写（可复用 write_atomic） |
| `uses/users` 命令 | 事件循环（不放在 symindex，放 flow，见下） |

---

## 4. 第 ③ 层：并行工作流层（事件 → 并行派发，读并行写串行）

### 4.1 新工具 `tools/flow.cx`（L3，注册新批次 + check）

三个入口：

```sh
# 一次性：改了几个文件 → 局部图 + 受影响批次 + 并行跑只读检查
sh ./unisacc.com tools/flow.cx -- check FILE... [--jobs N] [--run]
# 常驻：事件驱动，保存即重算并派发
sh ./unisacc.com tools/flow.cx -- watch [--jobs N] [--once]
# 只出图
sh ./unisacc.com tools/flow.cx -- graph FILE...
```

`--check FILE...` 的管线（每一环都是现成工具或上一节的新子命令，全部可脚本化）：

```
flow check F...
  → symindex --delta --files F            # 哪些节点 sha 变、边增删
  → symindex --graph --for F              # 局部图（users 闭包 + use/调用邻接）
  → affected --files F                    # 受影响批次（复用，输出 AFFECTED 行）
  → [--run] affected --run -j N 的调度（见 4.2，替换为冲突域版）
  → 打印一段聚合报告：GRAPH n 行 + AFFECTED m 行 + 各批次状态
```

`--watch` 循环：`sy_watch_init` → 对仓库根（含 cx/tools/tests/src/include，忽略 out/、.git/）`sy_watch_add` → `sy_poll` 等 fd → `sy_watch_read` → **去抖合并**（同一 path 在 150ms 窗口内的 MODIFY+CLOSE_WRITE 合成一次）→ 脏集送 `check` 的同一管线 → 新目录出现时补 watch。

### 4.2 冲突域调度（读并行、写串行的落法）

废止「单机同时只做一组重测试」的**执行语义**是：真正要串行的只有**三类共享可变写入**，不是所有测试。把它们定为三个命名冲突域：

| 冲突域 | 代表资源 | 谁写 |
|---|---|---|
| `worktree` | 工作树（源文件、`out/` 产物、git 索引/HEAD） | 编辑/refactor apply、git add/commit、candidate 构建、census/affected 的 out 目录 |
| `candidate` | 候选材料身份（`unisacc.com`、材料、批次 digest 前/后对比的冻结输入） | candidate-install/promote、材料重建（asm-material 等） |
| `harness` | 测试骨架（`tests/batches.tsv`、`coverage.tsv`、`run-batch.cx`、`run.cx`、`tools/bound`、`unisacc.com` 本体） | 改注册表/run 脚本/换产品 |

调度规则（`flow.cx` 内的一个小调度器，每任务声明 `reads:{}` 和 `writes:{}` 域集合）：

1. **写集不相交 → 并行**；写集相交 → 串行。读不占锁。
2. 只读任务（`writes=∅`，即现有 SOURCE 批次的只读检查、`symindex update`、`affected --list-only`）**全部并行**，上限 `--jobs`（默认 = 在线核数，封顶 16，与 refactor MAXJOBS 一致）。
3. 三个域是写锁的粒度，**不是**「重测试 vs 轻测试」的粒度——一次只写 `worktree` 的 refactor 和一次只读测试可以并行；两个都写 `candidate` 的材料重建才串行。

这是对 `affected.cx --run -j N` 的**冲突域化**：现在 `-j` 是裸并行（碰巧安全，因为只跑不写材料的 SOURCE 批次），flow 版把「要写 out/ 或候选/材料的批次」按上面三条排，其余照旧并行。

首刀只把**只读任务的全并行**落地（不引入写任务调度，写任务仍由人/agent 显式串行），写锁机制作为接口预留——因为首刀范围内「事件触发的自动派发」只派发只读检查，不自动改工作树/换候选。

### 4.3 已有 vs 新造（本层）

| 已有可复用 | 缺、要新造 |
|---|---|
| `affected.cx` 的 use 闭包 + 批次匹配 + `--run` 派发/回收/deadline/kill 树（`sy_spawn_pipe` + `sy_poll` 事件循环） | `flow.cx` 本体（聚合 + 图输出 + 冲突域调度） |
| `run-batch.cx`（每批次独立、自看门狗、digest 候选前/后） | 冲突域声明表（先写死三类，格式预留 `reads/writes` 字段） |
| `symindex` 的增量更新（`flow` 每轮调用它，不重写） | 事件循环 + 去抖 + 新目录补 watch |
| `cx/workspace/driver.cx` 的「声明输入 + 内容身份 + 有界子进程」范式 | 把它的 `sequential` 调度换成冲突域并行（本层借鉴，不改 workspace） |

### 4.4 验收路径（对应 prd 的验收）

改 `cx/sys/fs.cx` 后，一条命令：

```sh
sh ./unisacc.com tools/flow.cx -- check cx/sys/fs.cx
# 预期输出（本机实测基数）：users 闭包 189 个文件 + affected 78 个批次 + 局部图，
# 全部来自 symindex/affected 的既有机制，无需 grep。
```

加 `--run -j N` 则并行跑这 78 个只读批次，读并行（写冲突域为空）。

---

## 5. 首刀实现路径（谁先做什么，一步一验）

**结论：首刀不碰 cx/sys，不依赖 watch，先把「一次性」整条链用现有 symindex + affected 打通，拿到 prd 的验收；watch 是第二步接进去的开关，不是前置。**

1. **（本任务，先做）`symindex.cx -- graph [--for FILES]`**：最小改动，复用 `resolve_uses`/`collect_defs`/`calls_of`，产出 `out/toolgraph/v1/graph.tsv`。验：`tools/check-symindex.cx` 加一组断言（节点数、`cx/sys/fs.cx` 的 use 边、某 `call` 边），`symindex-tool` 批次保持绿。
2. **（本任务）`symindex.cx -- delta --files F`**：读 manifest 前后差，输出 `DELTA`。验：改一个夹具文件，delta 只报它。
3. **（本任务）`tools/flow.cx -- check/--graph`（一次性模式）**：串起 1+2+`affected`，打印聚合报告。验：`check cx/sys/fs.cx` 输出 users=189 / affected=78 与 0.1/0.2 实测一致；新增 `flow-tool` 批次 + `tools/check-flow.cx`。
4. **（本任务）`flow.cx -- run` 的只读并行派发**：把 `affected --run -j` 接进 flow，写冲突域为空的全并行。验：读并行，无共享写。
5. **（omp-go，并行）`cx/sys/watch.cx`**：先 Linux inotify 全功能 + 冒烟夹具，macOS kqueue 写好留编译验证，Windows 接口骨架 `-ENOSYS`。验：`touch`/改字节事件可读。
6. **（合流）`flow.cx -- watch`**：把 5 的 `sy_watch` 接进 3 的管线，加去抖 + 新目录补 watch。验：保存即触发 `check`，输出与 3 一致。

每刀之间无硬依赖：1–4 与 5 完全并行；6 是唯一等 5 的一刀。这样即使 watch 落地慢，验收（改文件 → 一条命令拿闭包 + 批次）已经先在 1–3 达成。

---

## 6. 明确不改 / 不碰

- **不改 `cx/sys` 任何文件**（watch 归 omp-go）；本任务只用 `use sys.watch` 消费。
- 不改 `affected.cx` 的匹配语义（flow 只是调用它），不改 `run-batch.cx`/`tests/batches.tsv`。
- 不解析 `#include` 进图（L0 include 归 cxinclude 棘轮，symindex 现状一致）。
- 不缓存任何测试结论，只缓存词法事实和结构图（延续 AGENTS.md 的「缓存命中仍核内容」，`--verify` 仍是内容核验入口）。
- 不把写任务（改工作树/换候选/重建材料）放进首刀的事件自动派发——首刀只自动派发只读检查；写任务仍显式串行，写锁机制作为 flow 调度器的预留接口。
