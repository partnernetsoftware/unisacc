# Tape 里程碑 1: osx 与 win 目标 `-S` 交叉失败全量取证与最小补法

- 取证时间: 2026-11-25
- 采样基线: `~/repos/unisacc/.claude/worktrees/v0.1.3` (`HEAD=8c984145b0db4a12870dc34eb78171914dd926b8`), 已装产品 `~/repos/unisacc/unisacc.com` (`0.1.2` `APE`, `CC=/usr/bin/cc PATH=/usr/bin:/bin`)
- 扫描覆盖 (总计 `658` 次跨目标实编译, 全部由私有 `.cx` 扫描器实跑, 无推测):
  (1) `5` 个代表 `.cx` 工具 (`tools/check-tapeshape.cx`, `tools/check-crypto-sha256.cx`, `tools/check-packagefooter.cx`, `tools/chain-tplsha.cx`, `tools/check-tsv-lint.cx`) x `6` 目标 (`lnx/x86_64`, `lnx/arm64`, `osx/x86_64`, `osx/arm64`, `win/x86_64`, `win/arm64`), `--target <T> -S <SRC> -o <OUT>.tape`, 共 `30` 格;
  (2) 全量 `34` 个 L0 `cx/sys/*.cx` 模块 + `10` 个 L1 `ext/sysx/*.cx` 模块 (合计 `44` 模块) x `5` 目标 (`lnx/x86_64`, `osx/x86_64`, `osx/arm64`, `win/x86_64`, `win/arm64`), 自动生成 `#pragma unisa use <mod>\nint main(void){return 0;}` 最小闭包容包跑 `--target <T> -S <WRAP> -o <OUT>.tape`, 共 `220` 格;
  (3) 全量 `102` 个 `g2` 产品/运行库单元 (`44` 个 `build/chain-facts.tsv` `root` 单元 + `58` 个 `g2/lnx-x86_64.closures.tsv` 闭包模块) x `4` 个非 Linux 目标 (`osx/x86_64`, `osx/arm64`, `win/x86_64`, `win/arm64`), 跑 `-funit -c -b <T> -DRT_EXTERN <SRC> -o <OUT>.o`, 共 `408` 格.

---

## 1. 总览结论

1. **102 个 `g2` 产品/运行库单元 (`-funit -c -b`) 跨 4 非 Linux 目标 100% 通过**:
   - `44 root` x `4` 目标: `176/176` (`fail=0`);
   - `58 closure` x `4` 目标: `232/232` (`fail=0`);
   - 合计 `408/408 PASS`. 原因: `-funit -c` 编译独立未链接对象单元 (`src/drive.cx:756` -> `rq_object`), 外部宿主调用保留为 `.extern` 供链接期/转发桩处理; 所有产品源自身的前端/宏/类型声明在 `osx` 与 `win` 上均无缺口.

2. **全闭包 `-S -o <OUT>.tape` 在 `osx`/`win` 上失败的两类根本机理**:
   - **机理 A (`osx` 与部分 `lnx` 模块的 `unisacc: error: undefined function '<sym>'`)**:
     在 `src/drive.cx:735-757`, 只有 `-run` (`mode2 == 3`) 或带宿主转发的镜像构建 (`image_fwd != 0`) 会走 `dv_run_lower_n` 生成 `uffi_dlsym` 宿主转发桩 (`cx/compiler/fwdstub.cx`). 而 `-S ... -o <OUT>.tape` (`mode2 == 2`, `image_fwd == 0`) 直接进入 `st_span` -> `e3`, **不挂 `\0cli/run-forward`**, 因此 E3 对所有「头文件中仅有原型声明、无 `static` 内联 syscall/API 包装体」的外部 C 函数一律报 `undefined function`.
     - 在 5 个工具和 44 个 `sys`/`sysx` 模块中, **`osx/x86_64` 与 `osx/arm64` 90% 的失败都来自两处高频 L0 模块**: `cx/sys/fs.cx:438` (`statfs`) 与 `cx/sys/time.cx:149` (`utimes`). 只要某工具间接 `use sys.fs` 或 `sys.time` (例如 `sys.poll`, `sys.posix`, `sys.proc`, `sys.runlaunch`, `sys.tty`, `sysx.*`), 两个 `osx` 目标立即被 `statfs`/`utimes` 拦下.
   - **机理 B (`win` 的 `error: call to undeclared function` 与 `incomplete struct`)**:
     `include/cx-flat.h:2454-2455` 注释已明确说明 Windows 隐式前导故意不包含 `<fcntl.h>`, `<poll.h>`, `readlink`, 且 `include/sys/_ffi_forward.h:133` 用 `#ifndef _WIN32` 排除了 `uffi_call`. 凡是模块**未通过 L0 `_os` 帮手隔离**而直接调用裸 `fcntl` / `struct pollfd` / `uffi_call` / `readlink`, 在 `win/x86_64` 与 `win/arm64` 上直接触发 C99 隐式函数声明拒绝 (`-Wc99-implicit-function`) 或不完整结构体拒绝 (`not covered: incomplete struct`).
     - 诊断位置说明: `cx-flat.h` 展开与模块拼接阶段使部分隐式函数报错的行号落到前导边界头 (`stdio.h:1:1` 对应 `ext/sysx/processcore.cx` 的 `fcntl`; `cx.h:1:1` 对应 `cx/sys/ffi.cx` 的 `uffi_call`; `string.h:1:1` 对应 `ext/sysx/trace.cx` 的 `readlink`), 下文表格均同时列出「编译器字面报错位置」与「真实源码 `file:line`」.

3. **扫描通过率汇总**:

| 集合 | 目标 | 总数 | 通过 | 失败 | 失败根因摘要 |
| --- | --- | ---: | ---: | ---: | --- |
| 5 代表工具 (`-S`) | `lnx/x86_64`, `lnx/arm64` | 10 | 10 | 0 | 全通 |
| 5 代表工具 (`-S`) | `osx/x86_64`, `osx/arm64` | 10 | 2 | 8 | 仅 `check-crypto-sha256` 通; 其余 4 工具均死于 `statfs` + `utimes` |
| 5 代表工具 (`-S`) | `win/x86_64`, `win/arm64` | 10 | 4 | 6 | `check-crypto-sha256`、`chain-tplsha` 通; 其余 3 工具死于 `processcore.cx` 裸 `fcntl` |
| 44 `sys`/`sysx` 模块 (`-S`) | `lnx/x86_64` | 44 | 38 | 6 | 固有外部/平台限制 (`dlcall`, `kernel`, `net`, `trace`, `process_group{,_osx}`) |
| 44 `sys`/`sysx` 模块 (`-S`) | `osx/x86_64`, `osx/arm64` | 44 x 2 | 24 x 2 | 20 x 2 | 6 固有项 + 13 项由 `sys.fs`/`sys.time` 级联触发 (`statfs`/`utimes`) + `sys.dl` (`dl*`) |
| 44 `sys`/`sysx` 模块 (`-S`) | `win/x86_64`, `win/arm64` | 44 x 2 | 36 x 2 | 8 x 2 | 3 固有项 (`kernel`, `process_group{,_osx}`) + 5 项 Windows 缺口 (`sys.ffi`, `sys.net`, `sysx.processcore`, `sysx.process`, `sysx.trace`) |
| 102 `g2` 产品单元 (`-funit -c`) | `osx/*`, `win/*` (4 目标) | 408 | 408 | 0 | 全通 |

---

## 2. 代表工具 6 目标 `-S` 实测明细

CLI 路由说明: 须用 `--target <T> -S <SRC> -o <OUT>.tape` (双横杠 `--target` 且输出后缀 `.tape` 命中 `src/driver/route.cx` `mode == 2` tape 路由; 若后缀 `.s`, 非 Linux 目标按合同报 `assembly text (-S -b ... -o FILE.s) is written for Linux targets`).

| 工具 | `lnx/x86_64` | `lnx/arm64` | `osx/x86_64` | `osx/arm64` | `win/x86_64` | `win/arm64` |
| --- | --- | --- | --- | --- | --- | --- |
| `tools/check-crypto-sha256.cx` | PASS `365,665 B` | PASS `365,665 B` | PASS `365,672 B` | PASS `365,672 B` | PASS `383,436 B` | PASS `383,436 B` |
| `tools/chain-tplsha.cx` | PASS `1,506,181 B` | PASS `1,501,582 B` | FAIL `statfs`, `utimes` | FAIL `statfs`, `utimes` | PASS `1,637,333 B` | PASS `1,637,333 B` |
| `tools/check-tapeshape.cx` | PASS `2,830,068 B` | PASS `2,837,616 B` | FAIL `utimes`, `statfs` | FAIL `utimes`, `statfs` | FAIL `fcntl` | FAIL `fcntl` |
| `tools/check-packagefooter.cx` | PASS `3,217,351 B` | PASS `3,215,604 B` | FAIL `statfs`, `utimes` | FAIL `statfs`, `utimes` | FAIL `fcntl` | FAIL `fcntl` |
| `tools/check-tsv-lint.cx` | PASS `3,056,228 B` | PASS `3,062,317 B` | FAIL `statfs`, `utimes` | FAIL `statfs`, `utimes` | FAIL `fcntl` | FAIL `fcntl` |

---

## 3. 完整缺失符号清单 (`target x symbol x file:line x module`)

### 3.1 高优先可修复 `osx`/`win` 交叉缺口 (修完即可打通 5 工具及绝大多数 `sys`/`sysx` 闭包)

| 序号 | 目标平台 | 缺失符号/类型 | 报错类别与编译器字面位置 | 真实源码位置 (`file:line`) | 所属模块 | 级联影响的模块/工具 |
| --- | --- | --- | --- | --- | --- | --- |
| G1 | `osx/x86_64`, `osx/arm64` | `statfs` | `undefined function 'statfs'` (E3) | `cx/sys/fs.cx:438` (前导外 `#include <sys/mount.h>` 于 `:435`) | `sys.fs` | `sys.fs`, `sys.posix`, `sys.proc`, `sys.runlaunch`, `sys.tty`, `sys.net`, `sysx.exec`, `sysx.linuxns`, `sysx.processcore`, `sysx.process`, `sysx.reaper`, `sysx.run` 及 4 个工具 |
| G2 | `osx/x86_64`, `osx/arm64` | `utimes` | `undefined function 'utimes'` (E3) | `cx/sys/time.cx:149` (声明在 `include/sys/time.h:10`) | `sys.time` | `sys.time`, `sys.poll`, `sys.proc`, `sys.runlaunch`, `sys.net`, `sysx.exec`, `sysx.processcore`, `sysx.process`, `sysx.reaper`, `sysx.run` 及 4 个工具 |
| G3 | `win/x86_64`, `win/arm64` | `fcntl` | `call to undeclared function 'fcntl'` (`stdio.h:1:1`) | `ext/sysx/processcore.cx:76,111,112` (4 处裸 `fcntl`) | `sysx.processcore` | `sysx.processcore`, `sysx.process`, `tools/check-tapeshape.cx`, `tools/check-packagefooter.cx`, `tools/check-tsv-lint.cx` |
| G4 | `win/x86_64`, `win/arm64` | `struct pollfd`, `poll`, `fcntl` | `not covered: incomplete struct` (`cx/sys/net.cx:528:19`) | `cx/sys/net.cx:528,530,531,536,543` | `sys.net` | `sys.net` (注: `net.cx` 后续行 `:530,:531,:543` 亦含裸 `fcntl` 与 `:536` `poll`) |
| G5 | `win/x86_64`, `win/arm64` | `uffi_call` | `call to undeclared function 'uffi_call'` (`cx.h:1:1`) | `cx/sys/ffi.cx:61` (`include/sys/_ffi_forward.h:133` `#ifndef _WIN32` 屏蔽) | `sys.ffi` | `sys.ffi` (注: `:51,:52,:53,:54,:58` 亦调用 `uffi_set_bridge_mode`/`uffi_last_error`, 均在 `include/unisacc_ffi.h:7` `#ifndef _WIN32` 之外) |
| G6 | `lnx/x86_64`, `osx/x86_64`, `osx/arm64` | `pread` | `undefined function 'pread'` (E3) | `ext/sysx/trace.cx:133` (`include/unistd.h:561` 仅声明无静态体) | `sysx.trace` | `sysx.trace` |
| G7 | `win/x86_64`, `win/arm64` | `readlink` | `call to undeclared function 'readlink'` (`string.h:1:1`) | `ext/sysx/trace.cx:172` (`include/unistd.h:416,576` Windows 缺席) | `sysx.trace` | `sysx.trace` |

### 3.2 固有宿主动态库转发 / 宿主运行期外链 / 架构守卫项 (非纯 osx/win 回归, 单列备查)

| 序号 | 目标平台 | 符号 / 诊断 | 真实源码位置 (`file:line`) | 所属模块 | 性质与说明 |
| --- | --- | --- | --- | --- | --- |
| N1 | `osx/x86_64`, `osx/arm64` | `dlopen`, `dlsym`, `dlclose`, `dlerror` | `cx/sys/dl.cx:234-237` | `sys.dl` | `sys.dl` 注释明确「Apple imports libSystem」取函数地址; Linux 走内联 ELF dynamic 解析, Windows 走 `_ux_sym`. 若需纯 `-S` 可在 Apple 分支改走 `__hostaddr0..3` / `uffi_dl*` 或交 M1X `hoststub` 槽 31-33. |
| N2 | `lnx/x86_64`, `osx/x86_64`, `osx/arm64` | `dlopen`, `dlsym`, `dlclose` | `cx/sys/dlcall.cx:18,28,39` | `sys.dlcall` | 设计注释 (`:5`) 明确标注为宿主动态导入 (`dlopen`/`dlsym`/`dlclose`), 与 Windows `_WIN32` 返回 `-ENOSYS` 形成对照; 属 `hoststub` 槽 31-33 边界. |
| N3 | `lnx/x86_64`, `osx/x86_64`, `osx/arm64` | `getaddrinfo`, `freeaddrinfo`, `gai_strerror` | `cx/sys/net.cx:150,189,192,209,238,474,481` | `sys.net` | DNS 解析走宿主 libc 转发 (Windows 已用 `_ux_sym` 绑定 `ws2_32.dll`, 故无此错); 纯 `-S` 若要脱宿主转发需独立 DNS 客户端或 `hoststub`. |
| N4 | 全 5 目标 (`lnx`/`osx`/`win`) | `core_host_panic`, `core_host_fetch` | `cx/sys/kernel.cx:30,41,90` | `sys.kernel` | 宿主运行期外链回调 (`src/host.cx` 链接提供), 单模块 `-S` 无法解析属设计预期. |
| N5 | 全 5 目标 (`lnx`/`osx`/`win`) | `exports setpgid under #if (conditional interface)` | `ext/sysx/process_group.cx:8`, `ext/sysx/process_group_osx.cx:8` | `sysx.process_group`, `sysx.process_group_osx` | 单平台 dock 模块, 顶层 `#if ... #error` 包裹导出函数触发 cx 模块接口检查; 正常路径应改用 `sys.posix` 的 `sy_setpgid`. |

---

## 4. 不引入上层 `#if` 的最小 L0 / 事实表补法

所有方案均严格遵守 cx 分层铁律: **平台 `#if` 与 `__syscall6` 仅留在 L0 `cx/sys/*` 的 `static` `_os` 帮手或事实表生成器中, 上层 (`ext/sysx/*`, `tools/*`, `src/*`) 零新增 `#if`**.

### 补法 1 (消除 G1 `osx` `statfs` + G2 `osx` `utimes` — 立即打通全部 4 工具及 13 个 `sys`/`sysx` 模块的 `osx` `-S`)

- **权威事实表**: `exec/facts/sys-syscalls.tsv` (由 `tools/target-gen.cx -- --write` 生成 `cx/sys/sysnums.cx`, `check` 批次 `target-gen`).
- **事实表新增 2 行** (列序: `name lnx_x86_64 lnx_arm64 osx_x86_64 osx_arm64 win_x86_64 win_arm64 fallback`):
  - `syp_utimes	0	0	138	138	0	0	0` (XNU `bsd/kern/syscalls.master` `SYS_utimes = 138`, 经 `_UNISA_SC(n)` 在 `osx/x86_64` 自动加 `0x2000000L`, `osx/arm64` 保持 `138`)
  - `syp_statfs64	0	0	345	345	0	0	0` (XNU `SYS_statfs64 = 345`, `osx/x86_64` 与 `osx/arm64` 共用完全相同的 64-bit `struct statfs64` 内存布局, 与 `include/sys/mount.h:16-24` 一致)
- **改动 A (`cx/sys/time.cx:123-151`, `syt_utimes_os`, 约 +10/-3 行)**:
  - 引入 `#pragma unisa use sys.sysnums` (或直接使用 `_UNISA_SC(sn_osx_..._syp_utimes)`).
  - 在 `syt_utimes_os` 的 `#else` (Apple) 分支中, 用 `__syscall6(_UNISA_SC(nr), (long)path, (long)tv, 0, 0, 0)` 替换第 `149` 行的 `return utimes(path, tv);`, 负值按与 Linux 分支相同的合同设 `errno = (int)-r; return -1;`.
  - 导出函数 `sy_set_mtime` 零 `#if`.
- **改动 B (`cx/sys/fs.cx:429-444`, `sy_statfs_os`, 约 +16/-6 行)**:
  - 在 `#elif defined(__APPLE__)` 分支移除 `#include <sys/mount.h>` (`:435`) 与宿主 `statfs(path, &s)` (`:438`), 改用与 `include/sys/mount.h:16-24` 字段一致的局部 `struct { unsigned int f_bsize; int f_iosize; unsigned long f_blocks; unsigned long f_bfree; unsigned long f_bavail; unsigned long f_files; unsigned long f_ffree; int f_fsid[2]; unsigned int f_owner; unsigned int f_type; unsigned int f_flags; unsigned int f_fssubtype; char f_fstypename[16]; char f_mntonname[1024]; char f_mntfromname[1024]; unsigned int f_flags_ext; unsigned int f_reserved[7]; } s;` (紧邻同函数第 `446-459` 行的 Linux 局部结构体写法, 风格 100% 统一) 并调用 `__syscall6(_UNISA_SC(nr), (long)path, (long)&s, 0, 0, 0)` (`nr = 345`).
  - 收益: 同时消除 `statfs` 宿主转发依赖和 `cx/sys/fs.cx:435` 的内嵌 `#include <sys/mount.h>` 例外.
- **派生同步**: 运行 `sh ./unisacc.com tools/target-gen.cx -- --write` 更新 `cx/sys/sysnums.cx` 并跑 `--check` 验证.

### 补法 2 (消除 G3 `win` `fcntl` — 立即打通 `check-tapeshape`, `check-packagefooter`, `check-tsv-lint` 及 `sysx.process{,core}` 的 `win` `-S`)

- **改动 (`ext/sysx/processcore.cx`, 约 +5/-3 行, 零 `#if`)**:
  - 在头部增加 `#pragma unisa use sys.io` (L0 `cx/sys/io.cx:92-100` 已导出跨平台 `sy_fd_cloexec(int fd, int on)` 与 `sy_fd_nonblock(int fd, int on)`, 内部 `sy_fd_flag_os` 已按 `#ifdef _WIN32 return -ENOSYS;` 隔离).
  - 将 4 处裸 `fcntl` 直接替换为 L0 调用:
    - `:76`: `fcntl(pf[1], F_SETFD, FD_CLOEXEC);` -> `(void)sy_fd_cloexec(pf[1], 1);`
    - `:111`: `fcntl(pf[0], F_SETFL, O_NONBLOCK);` -> `(void)sy_fd_nonblock(pf[0], 1);`
    - `:112`: `fcntl(po[0], F_SETFL, O_NONBLOCK); fcntl(pe[0], F_SETFL, O_NONBLOCK);` -> `(void)sy_fd_nonblock(po[0], 1); (void)sy_fd_nonblock(pe[0], 1);`
  - **注意**: 同步确认 `ext/sysx/processcore.cx` 其它符号 (`pipe`/`fork`/`waitpid`/`dup2`/`execvp`/`usleep`) 在 Windows 前导 `include/cx-flat.h` 已声明/内联, 无次级缺口 (本轮实测 `sys.proc` 在 `win/x86_64` 与 `win/arm64` 均已 `RC=0`).

### 补法 3 (消除 G6 `pread` 与 G7 `readlink` — 打通 `ext/sysx/trace.cx` 全 5 目标 `-S`)

- **改动 (`ext/sysx/trace.cx`, 约 +4/-2 行, 零 `#if`)**:
  - 头部增加 `#pragma unisa use sys.posix` 与 `#pragma unisa use sys.fs` (L0 `cx/sys/posix.cx:437` 已通过 `exec/facts/sys-syscalls.tsv:59` 的 `syp_pread` 实现全平台 `sy_pread`; `cx/sys/fs.cx:592` 已通过 `sy_readlink_os` 实现全平台 `sy_readlink`).
  - `:133`: `pread(fd, out + at, (size_t)(cap - 1 - at), off + (long)at)` -> `sy_pread(fd, out + at, (long)(cap - 1 - at), (int64_t)(off + (long)at))` (注意 `sy_pread` 失败返回 `-errno`, 保持 `z <= 0` 语义不变).
  - `:172`: `readlink(link, out, (size_t)(cap - 1))` -> `sy_readlink(link, out, (long)(cap - 1))`.

### 补法 4 (消除 G4 `win` `sys.net` `struct pollfd`/`fcntl` 与 G5 `win` `sys.ffi` `uffi_call`)

- **`cx/sys/net.cx:525-545` (`syn_connect_ms`, 约 +14/-4 行)**:
  - 方案 A (优先, 复用现有 L0): 在 `cx/sys/net.cx` 引入 `#pragma unisa use sys.io` 和 `#pragma unisa use sys.poll`, 用 `sy_fd_nonblock(s, 1/0)` 替换 `:530,:531,:543` 的裸 `fcntl`, 用 `sy_pollfd_t` + `SY_POLLOUT` + `sy_poll(&pf, 1, ms)` 替换 `:528,:535-537` 的 `struct pollfd` + `poll`, 全程零新增 `#if`. (注: `:543` 恢复原阻塞态可由 `sy_fd_nonblock(s, 0)` 实现, 若需保留原非阻塞态可在 `sys.io` 补 `sy_fd_get_nonblock` 或在 `syn_connect_ms_os` 局部隔离 `#ifdef _WIN32`).
- **`cx/sys/ffi.cx:45-62` (`sy_ffi_set_bridge_mode`/`sy_ffi_last_error`/`sy_ffi_call`, 约 +18/-6 行)**:
  - 在 L0 `cx/sys/ffi.cx` 内部抽 3 个 `static` `_os` 帮手 (`sy_ffi_bridge_os`, `sy_ffi_lasterr_os`, `sy_ffi_call_os`): `#ifdef _WIN32` 分支分别返回 `-ENOSYS` / `""` / `-ENOSYS`, `#else` 分支保持调用 `uffi_set_bridge_mode` / `uffi_last_error` / `uffi_call`; 3 个导出函数本身无 `#if`.
  - 不改 `include/sys/_ffi_forward.h` 与 `include/cx-flat.h`, 变更面最小且不触及头扁平派生门.

### 改动量总计估算

| 补法 | 涉及文件 | 估算改动行数 | 清除缺口 |
| --- | --- | --- | --- |
| 补法 1 | `exec/facts/sys-syscalls.tsv`, `cx/sys/sysnums.cx` (生成), `cx/sys/time.cx`, `cx/sys/fs.cx` | 手改约 `+28/-9` 行 | G1 (`statfs`), G2 (`utimes`) -> 解锁全部 4 工具 + 13 模块 `osx` `-S` |
| 补法 2 | `ext/sysx/processcore.cx` | `+5/-3` 行 | G3 (`fcntl`) -> 解锁全部 3 工具 + 2 模块 `win` `-S` |
| 补法 3 | `ext/sysx/trace.cx` | `+4/-2` 行 | G6 (`pread`), G7 (`readlink`) -> 解锁 `sysx.trace` 全 5 目标 `-S` |
| 补法 4 | `cx/sys/net.cx`, `cx/sys/ffi.cx` | `+32/-10` 行 | G4 (`pollfd`/`fcntl`), G5 (`uffi_call`) -> 解锁 `sys.net`/`sys.ffi` `win` `-S` |
| **合计** | `7` 个文件 (含 1 生成物) | 手改约 `+69/-24` 行 | 7 类全部 `osx`/`win` 非固有缺口清零, 零上层 `#if` |

---

## 5. 验收命令模板 (合入补法后复验)

```sh
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin"
# 1. 派生门与棘轮
sh ./unisacc.com tools/target-gen.cx -- --check
sh ./unisacc.com tools/ratchets.cx

# 2. 五工具六目标 -S 全绿验证 (30/30 RC=0)
for src in tools/check-tapeshape.cx tools/check-crypto-sha256.cx tools/check-packagefooter.cx tools/chain-tplsha.cx tools/check-tsv-lint.cx; do
  for tgt in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do
    sh ./unisacc.com --target "$tgt" -S "$src" -o /tmp/verify.tape || echo "FAIL $src $tgt"
  done
done
```
