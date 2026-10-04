# examples/win — Windows 上的实测与修复记录

这个目录不是示例，是**实测记录**：每条结论都在这台 Windows 11 x64 上真跑过。
测的是 unisacc 能不能替代 explorer.exe，以及更具体的——**能不能给 agent 提供
computer-use / KVM 能力**。

结论（0.0.23 三处修复之后）：**能做一半**。窗口枚举、热键、托盘、输入注入
已经可用；建窗口与截图仍然被拒，因为它们需要按平台 ABI 做宽调用。

## 本次修复（0.0.23，均已实测）

| 修复 | 位置 | 效果（实测） |
|--- |--- |--- |
| Windows 转发上限 6 → 4，超出按名报错 | `src/fwdstub.c`（`fwd_maxargs`）、`src/front_parse.c`、`src/main.c` | 5/6 参从"静默丢参"变成编译期诊断；≤4 参无回归 |
| 转发解析补 user32/gdi32/shell32/advapi32/dxgi | `include/unisacc_ffi.h`（4 → 9 个 DLL） | `GetCursorPos`/`SetCursorPos`/`keybd_event`/`GetForegroundWindow` 从 exit 127 变成可用 |
| PE 导入表补 8 个宽调用名字 | `unisa/image/pe.py`、`src/back_encode.c`、`src/back_lower.c`、`exec/enc/pecheck.py` | 每个 Windows 镜像多 8 个 kernel32 槽位（22 → 26），POSIX 层将来能拿到 `CreateProcessW` |

第三项只**开通道**，不实现 `fork`/`exec`：`CreateProcessW` 有 10 个参数，转发发不了，
只有导入表能装下，而 libc 里的 POSIX 主体还没写。

## 能力矩阵

| 探针 | 参考 x86_64 | 参考 arm64 | 产品 | 运行时结论 |
|--- |--- |--- |--- |--- |
| `tick.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | 0/1 参数转发正常 |
| `clockfmt.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | SYSTEMTIME、时区换算正确（bias -480） |
| `envdir.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | 1–4 参数转发正常 |
| `fileio.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | stdio + 元数据 API 正常，无句柄 IO |
| `vmem.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | VirtualAlloc/Protect/Free 正常 |
| `structs.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | 内核结构按显式偏移读，正常 |
| `procsnap.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | 447 个进程，字段偏移正确 |
| `posix.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | route 2 全通：open/stat/dirent/time/setjmp |
| `mailbox.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | 文件邮箱可用（4 参以内 + stdio） |
| `outparam.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | 4 参的三个出参全部到位（对照实验） |
| `gui/dll.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | user32/gdi32 全部解析成功（自建 DLL 句柄） |
| `gui/input.c` | **编译+运行 OK** | 交叉 OK | 编译期拒绝 | **输入注入可用**：读/移鼠标、读前台窗口与 PID、注入按键、读修饰键 |
| `refused/wideargs.c` | **编译期拒绝** | 拒绝 | 拒绝 | 5/6 参转发 → 新的按名诊断 |
| `refused/argceil.c` | **编译期拒绝** | 拒绝 | 拒绝 | 参数天花板逐档：0/1/4 通过，5/6 被拒，7 无桩 |
| `refused/shm.c` | **编译期拒绝** | 拒绝 | 拒绝 | 6+5 参数的共享内存 → 被拒（修复前是访问违例） |
| `refused/createfile7.c` | **编译期拒绝** | 拒绝 | 拒绝 | `CreateFileA` 7 参 → 无桩，报告 undefined function |
| `gui/window.c` | **编译期拒绝** | 拒绝 | 拒绝 | `CreateWindowExA` 12 参，需 MS x64 宽调用 |
| `gui/capture.c` | **编译期拒绝** | 拒绝 | 拒绝 | `BitBlt` 9 参，需 MS x64 宽调用 |

"编译期拒绝"指 `unisacc: error: ...`。对**产品** `unisacc.com` 而言每一个 Win32 原型
都是这样（0.0.22 的产品路线在 win 目标上不生成转发桩，见 `archive/plans/v0.0.23.md` 的 A1）；
参考路线（`out/ua-ref-win.exe`）能生成桩，所以下面所有测量都来自参考路线。

## 文件一览

`refused/` 与 `gui/` 里的失败是**数据**，不要修好它们。

| 文件 | 状态 | 测的是什么 |
|--- |--- |--- |
| `tick.c` | 通过 | 最短的 kernel32 转发：0/1 参数、void 返回、指针出参 |
| `clockfmt.c` | 通过 | SYSTEMTIME 与时区换算，任务栏时钟那套算术 |
| `envdir.c` | 通过 | 环境变量与路径；1–4 参数转发；截断语义 |
| `fileio.c` | 通过 | stdio + 元数据 API；顺带说明为何没有句柄 IO |
| `vmem.c` | 通过 | VirtualAlloc/Protect/Free 与 SYSTEM_INFO |
| `structs.c` | 通过 | 把内核写下的字节 dump 出来按显式偏移解码 |
| `procsnap.c` | 通过 | Toolhelp32 进程表（本机 447 个进程） |
| `posix.c` | 通过 | route 2：POSIX 名字在 Windows 上跑通 |
| `mailbox.c` | 通过 | 文件邮箱：agent 与 unisacc 程序今天就能用的 rendezvous |
| `outparam.c` | 通过 | 4 参出参真的到位（`refused/wideargs.c` 的对照组） |
| `gui/dll.c` | 通过 | user32/gdi32 能解析、按平台 ABI 调用仍然不成立 |
| `gui/input.c` | 通过 | 输入注入：修复前 exit 127，现在真的能注入 |
| `refused/wideargs.c` | 编译期拒绝 | 5/6 参转发的按名诊断（修复的核心回归测试） |
| `refused/argceil.c` | 编译期拒绝 | 参数天花板逐档测量 |
| `refused/shm.c` | 编译期拒绝 | 共享内存需要 6 参 |
| `refused/createfile7.c` | 编译期拒绝 | 7 参墙（`CreateFileA`） |
| `gui/window.c` | 编译期拒绝 | 建窗口 12 参 |
| `gui/capture.c` | 编译期拒绝 | 截图 9 参 |
## 怎么跑

产品二进制在这台机器上本身是好的：

||sh
dist/unisacc.com --version
dist/unisacc.com -run examples/hello.c
dist/unisacc.com -b win/x86_64 -o hello.exe examples/hello.c
dist/unisacc.com -b win/arm64 -o ctour.exe examples/apps/ctour.c
||

Win32 探针要走参考路线，且**不需要宿主 cc**——用产品把经典参考的完整导出编成
Windows PE 即可（`tests/export_ref.sh` 展开 `unisacc.c`，33,931 行）：

||sh
bash tests/export_ref.sh out/unisacc-flat.c
dist/unisacc.com -O2 -b win/x86_64 -o out/ua-ref-win.exe out/unisacc-flat.c
out/ua-ref-win.exe -b win/x86_64 -o dist/tick.exe examples/win/tick.c
||

**源文件请用仓库相对路径**（见下面「已知缺陷」：某些路径形态会产出一个丢掉
`bk_dyn` 导入的镜像，症状是转发全部失效）。`-S -b win/...` 被明确拒绝
（"assembly text is written for Linux targets"），PE 只能整体生成。

## 卡点 1：转发只送 4 个参数（**已修**）

这是修复前最严重的一条，因为它不崩溃、不报错、返回值看着合理：

| 调用 | 参数 | 修复前 | 修复后 |
|--- |--- |--- |--- |
| `GetDiskFreeSpaceExA(path,&a,&b,&c)` | 4 | 出参全对 | 不变 |
| `SearchPathA(0,"notepad.exe",0,buf,520)` | 5 | 返回正确长度 32，buf 全零 | **编译期按名拒绝** |
| `GetPrivateProfileStringA(sec,key,def,out,128,f)` | 6 | 静默退回 DEFAULT | **编译期按名拒绝** |
| `CreateFileA(...)` | 7 | 无桩，undefined function | 不变 |

机制：MS x64 只有 4 个整数寄存器槽（rcx/rdx/r8/r9），第 5、6 个参数走栈，而生成的
hostcall 序列（`src/back_encode.c` 的 `BK_HOST_WIN_X86` 模板）不建 shadow space 与栈槽。
修复不是把模板改对（那是 A1 的后端工作），而是**让编译器按目标能送达的数量拒绝**：
`src/fwdstub.c` 新增 `fwd_maxargs`，`src/main.c` 按 `-b` 目标设定（win 为 4），
`src/front_parse.c` 在 `fwd_stub` 里按名报错并让调用方计错。

||sh
unisacc: error: host function 'SearchPathA' takes 5 arguments, and this target's
host call delivers 4. Put the wide call in a bundled libc body, where it goes
through the import table instead.
||

## 卡点 2：user32 / gdi32 能解析、按平台 ABI 调用仍不成立（**部分修**）

`gui/dll.c` 是决定性实验，它把两件常被混为一谈的事分开：

|||
user32.dll 0x7ff8cb9c0000 first bytes 4d 5a 90 00 03 00 00 00   <- MZ，是真的 DLL
EnumWindows 0x7ff8c8103100 first bytes 48 8b 05 99 fe 02 00 49
SendInput   0x7ff8cb9f50d0 first bytes ff 25 02 27 06 00 cc cc   <- jmp [IAT]
|||

- **解析成功**（修复后由编译器自己完成，不需要程序写 `LoadLibraryA`）：
  转发桩用 `uffi_dlsym` 找名字，现在按 ucrtbase → kernel32 → ws2_32 → msvcrt →
  **user32 → gdi32 → shell32 → advapi32 → dxgi** 的顺序试。前四个是原来的顺序，
  所以原有名字的解析结果不变，只是多了后面五个。
- **调用仍不成立**：拿解析到的指针当函数调用走的是 unisacc 私有约定（参数压栈、
  `r9` 作帧指针、调用者弹栈），而 user32 期待 MS x64 约定。这与 A1 是同一件事。

所以：**4 参以内的 user32/gdi32 调用现在直接可用**（`gui/input.c`：
`GetCursorPos`/`SetCursorPos`/`mouse_event`/`keybd_event`/`GetAsyncKeyState`/
`GetForegroundWindow`/`GetWindowThreadProcessId` 全部跑通），而 `CreateWindowEx`(12)、
`BitBlt`(9)、`SendInput` 的 `INPUT` 结构体这类宽调用仍然被拒。

## 卡点 3：名字只能从固定几个 DLL 里解析（**已修**）

修复前转发只试 4 个 DLL，user32 与 gdi32 不在其中，后果**在运行期才暴露**：

||sh
unisacc: no host function GetCursorPos     <- 编译干净，运行 exit 127
||

对 agent harness 来说这点最要命：缺失的宿主函数是在**动作被尝试时**才发现。按名
写错一个不存在的名字（`Process32FirstA`）也是同一条路径。

## 类型映射（win64，踩过两次）

| Win32 | C | 备注 |
|--- |--- |--- |
| DWORD / BOOL / LONG / UINT | `unsigned int` / `int` | **4 字节**，写成 `unsigned long` 会宽一倍 |
| HANDLE / LPVOID / ULONG_PTR / LPARAM | `unsigned long` 或指针 | 8 字节 |
| WORD / WCHAR | `unsigned short` | |
| QWORD / LONGLONG | 无对应类型 | 只能拆成两个 DWORD 或用数组 |

两个实测踩到的坑：

1. `TIME_ZONE_INFORMATION.Bias` 是 `LONG`（4 字节）。声明成 `long`（win64 上 8 字节）
   会读进后面 StandardName 的第一个 wchar，printf 出 `6268252215845060128` 这样的
   指针值——看起来像数据，其实是布局错位。
2. **`SYSTEM_INFO` 在 win64 上没有 `dwOemId`**，那是 32 位布局的成员。照 x86 头文件
   抄会让后面每个字段错位 4 字节：`wProcessorArchitecture` 读出 4096（其实是
   dwPageSize），`dwNumberOfProcessors` 读出 `0x5507000600010000`。

`structs.c` 就是为此存在的：把内核写下的字节 dump 出来，再按显式偏移解码。
（好消息：`procsnap.c` 的 `sizeof(struct entry)` = 304，与内核期望一致，说明编译器
的自然填充是对的——错的只有 Win32 侧的宽度假设。）
## 两条路线（用户 2026-10-03 补充的要求）与它们各自的实测边界

Windows 上要同时支持两条路线：**原生 Win32 libc/DLL**，和**包装出来的 Win POSIX**
（类似 cosmopolitan，但更轻）。实测下来这两条路线不是二选一，而是应该分工——
因为仓库里其实有**三条**互不相同的 Windows 互操作通道，各自的能力上限不同：

| 通道 | 名字来源 | 参数个数 | 实测上限 |
|--- |--- |--- |--- |
| PE 导入表 | 22 个固定 kernel32 名 | **不限** | stdio 与 POSIX 的 `open()` 走这条；CreateFileA(7) 也只能走这条 |
| hostcall 转发 | 9 个 DLL 白名单 | **4（win）/ 6（其他）** | 超出按名拒绝（0.0.23） |
| libffi 桥 | dlsym + libffi | 不限 | **仅 macOS**（`unisacc_ffi.h:125`） |

分工因此是清楚的：**宽调用（参数多）归库，薄包装归转发**。
`include/sys/_win.h` 里 `_ux_call` 只有 4 个槽——POSIX 层是在这个限制**之内**
写成的，它需要宽调用时走的是导入表而不是转发。

### route 2（Win POSIX）今天在 Windows 上的实测覆盖面

`posix.c` 是可运行的那一半（open/read/write/lseek/close、stat、opendir/readdir、
clock_gettime、nanosleep、getpid/chdir、setjmp/longjmp 全部 OK）。逐函数测的结果：

| 可用 | 不可用（编译期按名拒绝或缺成员） |
|--- |--- |
| open read write lseek close | fileno |
| unlink rename stat | fork execvp wait waitpid |
| opendir readdir closedir | socket |
| getcwd chdir getpid isatty | strdup |
| clock_gettime nanosleep sleep time | poll（`struct pollfd` 缺成员） |
| localtime strftime difftime | ioctl / `struct winsize`（termios 未提供） |
| setjmp longjmp | `pwd.h` `regex.h` `strings.h` `dlfcn.h`（整体缺失） |
| malloc qsort bsearch | |
| snprintf vsnprintf | |
| getaddrinfo（无 DNS，只解析得出本地名） | |

头文件层面**全部齐全**（`unistd.h` `fcntl.h` `dirent.h` `poll.h` `termios.h`
`sys/stat.h` `sys/ioctl.h` `sys/wait.h` `sys/socket.h` `netinet/in.h`
`arpa/inet.h` `netdb.h` `sys/un.h` `setjmp.h` 在 win/x86_64 上都能编），
只有 `pwd.h` `regex.h` `strings.h` `dlfcn.h` 四个不存在——两个目标都一样。

**补上第三项修复之后**，起子进程的通道有了：`CreateProcessW` 现在在导入表里
（实测产物 PE 的导入表 = 22 个常驻名 + 4 个转发用 loader 名，`CreateProcessW`
在其中），所以 `fork`/`execvp`/`waitpid` 只差 `include/unistd.h` 里的 Windows 主体。

顺带一个跨平台写法的坑：**`#include <...>` 必须带 `.h`**，`<unistd.h>` 行，
`<unistd>` 报 `no such file for #include`。移植别的项目时这会先炸一批。

### 路线分工对两个目标的映射

| 需求 | 需要的路线 | 现状 |
|--- |--- |--- |
| 消息循环 GetMessageW/PostMessage/DispatchMessage（4 参） | route 1 | **可用**（≤4 参） |
| 热键 RegisterHotKey(3) | route 1 | **可用** |
| 托盘 Shell_NotifyIcon(3) | route 1 | **可用**（shell32 已进白名单） |
| 窗口枚举 EnumWindows(2) GetWindowTextW(3) | route 1 | **可用** |
| 输入注入 SetCursorPos(2) keybd_event(4) | route 1 | **可用，已实测** |
| 建窗口 CreateWindowEx(12) | route 1 | 仍拒：需要 MS x64 调用约定（A1） |
| 截图 BitBlt(9) | route 1 | 仍拒：同上，且需要 gdi32 进白名单（已进） |
| 文件/目录/时间/环境 | route 2 | **已可用** |
| 启动子进程 | route 2 | 导入表已开通道，缺 libc 主体 |
| socket | route 2 | 仍按名拒绝（0.0.23 计划里的第三批） |
| 与 agent 之间的 IPC | route 2（文件邮箱） | **已可用**（mailbox.c） |

**剩下唯一的大项是 A1**：把 `BK_HOST_WIN_X86` 那段模板扩成会填 32 字节 shadow space
与 `rsp+0x20`/`rsp+0x28` 栈槽，并让外部 callee 走 rcx/rdx/r8/r9。做完之后
`CreateWindowEx` 与 `BitBlt` 才有可能，那之后一个替代 explorer.exe 的 shell
在 unisacc 上就完整了。

## 已知缺陷（本次修复之外的既有 bug，未修）

**同一份源码、同一个编译器，产出的镜像会随调用形态变化，而坏的那个会丢掉
`bk_dyn` 带来的 4 个 loader 导入，于是所有转发在运行期报 "no host function"。**

实测（`examples/win/gui/input.c`，0.0.23 修复后的参考编译器）：

||sh
# 好：仓库相对路径 + 输出到 dist/
out/ua-ref-win.exe -b win/x86_64 -o dist/v/input.exe examples/win/gui/input.c   # rc=0

# 坏：把源文件写成绝对路径（或在 examples/win 里以 cwd 相对方式编译）
#     -> 镜像 sha 变成另一个，GetCursorPos 全部 "no host function"，exit 127
||

同一台机器、同一份源码，得到两个不同的 sha（`3174834e…` 与 `3c81c8a9…`），
差别只在于 `bk_dyn` 是否为真；`src/back_lower.c:201` 每个 pass 都会把 `bk_dyn` 归零，
而 `fe_units_fwd` 会编译两遍（第二遍带上生成的转发桩），所以**最终镜像可能是
在没看到任何 `.hostcall` 的那一遍写出的**。症状有误导性：编译干净、链接干净、
程序能跑，只是所有宿主调用返回 0。

这条与 0.0.23 的三处修复无关（它们不碰路径、临时桩文件与导入槽计算），但它会
让任何用绝对路径调用编译器的 Windows 流程静默拿到一个坏镜像。**建议的修法**是让
`bk_dyn` 一旦被看到就保持（或者在写镜像前按整程序的 hostcall 数量重算导入数），
但这要动核心 lowering 状态机，需要在有全套门禁的机器上做。

## 门禁影响（给仓库维护者）

`tests/*.sh` 与 `tests/gate.sh` 收集 examples 的 glob 全是 `examples/*.c`（顶层），
没有任何 `examples/**/*.c`；`tests/comdemo.py`、`tests/libneed.sh`、
`tests/appsrealcheck.py` 是显式点名。所以这个目录对现有门禁**完全惰性**：不会被跑，
也不会让任何 gate 变红（已核对 `script-inventory`、`closure`、`stages`、`ccrun`、
`fat`、`difftest`、`docs`、`consts`、`subtract-safety`、`c99-ledger`、`gateaudit`）。

要让它们进门禁，建议新建 `tests/winforward.sh`，只跑 ≤4 参数那批（tick / clockfmt /
envdir / fileio / vmem / structs / procsnap / posix / mailbox / outparam / gui/dll /
gui/input），`refused/` 与 `gui/window.c`、`gui/capture.c` 留作文档。
注意现有 `forward` 门禁是 macOS-only 且没有 `--com` 变体，而 `gui/input.c` 会在
真实桌面上注入一次按键（游标已复位）——进门禁前先想清楚要不要它在 CI 里跑。