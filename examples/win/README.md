# examples/win — Windows 实测（unisacc 0.0.22）

这个目录不是示例，是**实测记录**：每条结论都在这台 Windows 11 x64 上真跑过。
测的是 unisacc 能不能替代 explorer.exe，以及更具体的——**能不能给 agent 提供
computer-use / KVM 能力**。

结论：**今天不能**。卡点不是"找不到 user32"，是三件更具体的事。

## 文件一览

按"能跑"和"按设计失败"分两堆。`refused/` 与 `gui/` 里的失败是**数据**，不要修好它们。

| 文件 | 状态 | 测的是什么 |
|---|---|---|
| `tick.c` | 通过 | 最短的 kernel32 转发：0/1 参数、void 返回、指针出参 |
| `clockfmt.c` | 通过 | SYSTEMTIME 与时区换算，任务栏时钟那套算术 |
| `envdir.c` | 通过 | 环境变量与路径；1–4 参数转发；截断语义 |
| `fileio.c` | 通过 | stdio + 元数据 API；顺带说明为何没有句柄 IO |
| `vmem.c` | 通过 | VirtualAlloc/Protect/Free，以及 SYSTEM_INFO |
| `structs.c` | 通过 | 把内核写下的字节 dump 出来按显式偏移解码 |
| `procsnap.c` | 通过 | Toolhelp32 进程表（本机 447 个进程） |
| `argceil.c` | 通过 | 参数天花板逐档测量：4 参 OK，5/6 参"返回但丢参" |
| `outparam.c` | 通过 | **判决性实验**：第 5、6 参数到底送没送达 |
| `mailbox.c` | 通过 | 文件邮箱：agent 与 unisacc 程序今天就能用的 rendezvous |
| `posix.c` | 通过 | route 2：POSIX 名字在 Windows 上跑通 |
| `gui/dll.c` | 通过 | **决定性**：user32/gdi32 能解析、不能按平台 ABI 调用 |
| `gui/input.c` | 编译过、运行期 127 | 输入注入：缺失的宿主函数在**动作被尝试时**才暴露 |
| `gui/window.c` | 编译期拒绝 | 建窗口：12 参数（`CreateWindowExA`） |
| `gui/capture.c` | 编译期拒绝 | 截图：9 参数（`BitBlt`） |
| `refused/createfile7.c` | 编译期拒绝 | 7 参数墙（`CreateFileA`） |
| `refused/shm.c` | 崩溃 | 6+5 参数 → 假句柄 → 访问违例 |

一次跑完（参考路线，git-bash）：

```sh
mkdir -p dist/tmp
for f in examples/win/*.c; do
  n=$(basename "$f" .c)
  out/ua-ref-win.exe -b win/x86_64 -o "dist/tmp/$n.exe" "$f" \
    && { printf '%-12s ' "$n"; "dist/tmp/$n.exe" | head -3; }
done
```

## 怎么跑

产品二进制在这台机器上本身是好的：

```sh
dist/unisacc.com --version
dist/unisacc.com -run examples/hello.c
dist/unisacc.com -b win/x86_64 -o hello.exe examples/hello.c
dist/unisacc.com -b win/arm64 -o ctour.exe examples/apps/ctour.c
```

Win32 探针要走参考路线，且**不需要宿主 cc**——用产品把经典参考的完整导出编成
Windows PE 即可（`tests/export_ref.sh` 展开 `unisacc.c`，33,888 行）：

```sh
bash tests/export_ref.sh out/unisacc-flat.c
dist/unisacc.com -O2 -b win/x86_64 -o out/ua-ref-win.exe out/unisacc-flat.c
out/ua-ref-win.exe -b win/x86_64 -o tick.exe examples/win/tick.c && ./tick.exe
```

`-S -b win/...` 被明确拒绝（"assembly text is written for Linux targets"），
PE 只能整体生成，看不到中间汇编。

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
| `argceil.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | 4 参 OK；5/6 参"返回但丢参" |
| `outparam.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | **4 参出参对；5 参缓冲区全零；6 参静默默认** |
| `mailbox.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | 文件邮箱可用（4 参以内 + stdio） |
| `posix.c` | 编译+运行 OK | 交叉 OK | 编译期拒绝 | route 2 全通：open/stat/dirent/time/setjmp |
| `gui/window.c` | **编译期拒绝** | 拒绝 | 拒绝 | `CreateWindowExA` 12 参数 |
| `gui/capture.c` | **编译期拒绝** | 拒绝 | 拒绝 | `BitBlt` 9 参数 |
| `gui/input.c` | 编译 OK | 交叉 OK | 拒绝 | 运行期 `no host function`、exit 127 |
| `gui/dll.c` | 编译+运行 OK | 交叉 OK | 拒绝 | **user32/gdi32 全部解析成功** |
| `refused/createfile7.c` | **编译期拒绝** | 拒绝 | 拒绝 | `CreateFileA` 7 参数 |
| `refused/shm.c` | 崩溃 | 编译 OK（未实跑） | 拒绝 | 6+5 参数 → 假句柄 → 访问违例 |

"编译期拒绝"指 `unisacc: error: undefined function 'X'`——**每一个** Win32 原型
都这样，因为 0.0.22 的产品路线在 win 目标上不生成转发桩（`plans/v0.0.23.md` 的
A1 条目就是这件事）。参考路线（`out/ua-ref-win.exe`）能生成桩，所以下面所有
测量都来自参考路线。
## 三个卡点，按严重程度

### 1. 转发只送 4 个参数，第 5 个起静默丢弃（最严重）

`outparam.c` 是判决性实验：

| 调用 | 参数个数 | 结果 |
|--- |--- |--- |
| `GetDiskFreeSpaceExA(path,&a,&b,&c)` | 4 | ✅ 出参全部正确（free 29 GB / total 268 GB） |
| `SearchPathA(0,"notepad.exe",0,buf,520)` | 5 | ❌ 返回**正确的长度 32**，但 buf 全零 |
| `GetPrivateProfileStringA(sec,key,def,out,128,file)` | 6 | ❌ 文件确实存在，仍静默返回 DEFAULT |

MS x64 只有 4 个整数寄存器槽（rcx/rdx/r8/r9），第 5、6 个参数走栈。unisacc 生成的
hostcall 序列**没有把它们放到栈上**，被调用方读到垃圾：不崩溃、不报错、返回值看着
合理。这是最贵的一种错——程序照跑，数据不来。

`argceil.c` 显示 7 个参数时前端干脆不生成桩（`undefined function`），而
`src/fwdstub.c:38` 写的是 `intonly = np <= 6`。**前端承诺 6，Windows 后端只给 4。**
两个数字必须对齐：要么把前端上限降到 4 并按名拒绝 5 以上的调用（安全、立即可用），
要么把 `src/back_encode.c:88` 那段 167 字节 win64 hostcall 模板
（`BK_HOST_WIN_X86`）扩成会填 32 字节 shadow space 与 `rsp+0x20`/`rsp+0x28` 栈槽。

这条不修，**任何 ≥5 参数的 Win32 函数都不能用**。过 4 的全断：CreateFileA(7)、
BitBlt(9)、CreateWindowEx(12)、MapViewOfFile(5)、CreateFileMapping(6)……
还在 4 以内的恰好是消息循环与输入注入那一批：GetMessageW(4)、PostMessage(4)、
RegisterHotKey(3)、Shell_NotifyIcon(3)、EnumWindows(2)、GetWindowTextW(3)、
SetCursorPos(2)、GetAsyncKeyState(2)、GetDC(1)、CreateCompatibleDC(1)。

### 2. user32 / gdi32 只能"找到"，不能"调用"

`gui/dll.c` 把两件常被混为一谈的事分开了：

```
user32.dll 0x7ff8cb9c0000 first bytes 4d 5a 90 00 03 00 00 00   <- MZ，是真的 DLL
EnumWindows 0x7ff8c8103100 first bytes 48 8b 05 99 fe 02 00 49
SendInput   0x7ff8cb9f50d0 first bytes ff 25 02 27 06 00 cc cc   <- jmp [IAT]
gdi32.dll   0x7ff8cb8d0000 ... BitBlt 0x7ff8cb8d3d30 48 89 5c 24 08
```

- **解析成功**：`LoadLibraryA` + `GetProcAddress` 都是 kernel32 转发，user32 与
  gdi32 都打开了，`EnumWindows`/`SendInput`/`BitBlt`/`CreateCompatibleDC`
  全部拿到真实地址（能看到 x86-64 函数序言）。
- **调用不可用**：拿这个指针当函数调用走的是 unisacc 私有约定（参数压栈、`r9` 作
  帧指针、调用者弹栈），而 user32 期待 MS x64 约定。地址拿到了，调用方式不对。

所以缺的不是"能到达 user32"，而是"能按平台 ABI 调用它"——这正是 0.0.23 的
A1（Windows 转发：条件导入、IAT、Win64 hostcall）与 A2（cc 互调）要解决的事。
并且即使 A1 做完，≥5 参数这条仍要单独修。

### 3. 名字只能从 4 个 DLL 里解析

转发桩用 `uffi_dlsym` 找名字，按顺序试 `ucrtbase.dll`、`kernel32.dll`、
`ws2_32.dll`、`msvcrt.dll`（`include/unisacc_ffi.h:82-83`）。user32 与 gdi32
不在其中。后果都在**运行期**才暴露：

```
unisacc: no host function GetCursorPos     <- gui/input.c 编译干净，运行 exit 127
```

对 agent harness 来说这点最要命：缺失的宿主函数是在**动作被尝试时**才发现，不是
构建时。按名拒绝一个不存在的名字（`Process32FirstA`）也是同一条路径。
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

`structs.c` 就是为此存在的：把内核写下的字节 dump 出来，再按显式偏移解码。本目录
其他探针的 Win32 结构都按它的输出写，不靠信任手抄的结构体。
（好消息：`procsnap.c` 的 `sizeof(struct entry)` = 304，与内核期望一致，说明编译器
的自然填充是对的——错的只有 Win32 侧的宽度假设。）

## 对 agent computer-use / KVM 的结论

按 0.0.22 的能力，unisacc 编译的 Windows 程序**做不到**：

| computer-use 需要的 | 落在哪个 DLL | 参数个数 | 现在能不能 |
|--- |--- |--- |--- |
| 建窗口 / 收消息 | user32 | 12 / 4 | 12 参数编译期被拒 |
| 枚举窗口 / 取标题 | user32 | 2 / 3 | 编译通过、运行期 127 |
| 截图（BitBlt / DIB） | gdi32 | 9 / 3 | 9 参数编译期被拒 |
| 注入输入（SendInput） | user32 | 4 | 编译通过、运行期 127 |
| 截屏另路（WGC / DXGI） | user32 / dxgi | 复杂 | 同上 |

**但这不是"路断了"**：缺口是三个明确且都不大的编译器改动，不是架构问题。按代价排序：

1. **`src/fwdstub.c:38` 的上限 6 → 4**，并对 ≥5 参数的 win 转发按名拒绝。半小时，
   立刻消灭"静默丢参"这一类最贵的错，同时把 Win32 表面缩到"4 个参数以内"——
   上表里那两行"运行期 127"正好落在这一档，**做完窗口枚举和输入注入立刻可用**。
2. **按 MS x64 调用已解析地址**：`gui/dll.c` 已经证明解析不是问题，所以第二件是
   把调用约定补对（扩 `BK_HOST_WIN_X86` 模板，让外部 callee 走 rcx/rdx/r8/r9 +
   shadow space + 栈槽）。这与 A1 是同一件事，做完 `BitBlt`/`CreateWindowEx` 才有
   意义。
3. **DLL 白名单**（`include/unisacc_ffi.h:82-83` 那 4 个）加上 user32/gdi32/dxgi。

三件都做完，tinywm 那套需求（ORB + taskbar + tray + hotkey + 运行框）在 unisacc
上可行：`shell/shell_cosmo.c` 那 8000 行 C 能缩到"消息泵 + 结构声明 + 转发桩"，
业务逻辑仍留在 JS/ujs 层——这与 unisacc 现在的分层主张一致。

在那之前的**务实分工**：agent 侧（Claude / CodeBuddy / Python）自己持有
computer-use（它已有截图与输入工具），unisacc 编译的程序通过 `mailbox.c` 那种
文件协议或 ≤4 参 kernel32 转发做**被驱动的计算侧**。本目录 10 个通过的探针就是
这条分工能覆盖的范围：时钟、进程表、内存、文件、环境、模块基址——shell 的"后台"
可以搬，"前台"（GDI 绘制与输入注入）暂时不行。

## 门禁影响（给仓库维护者）

`tests/*.sh` 与 `tests/gate.sh` 收集 examples 的 glob 全是 `examples/*.c`（顶层），
没有任何 `examples/**/*.c`；`tests/comdemo.py`、`tests/libneed.sh`、
`tests/appsrealcheck.py` 是显式点名。所以这个目录对现有门禁**完全惰性**：不会被跑，
也不会让任何 gate 变红（已核对 `script-inventory`、`closure`、`stages`、`ccrun`、
`fat`、`difftest`、`docs`、`consts`、`subtract-safety`、`c99-ledger`、`gateaudit`）。

要让它们进门禁，两条路（都不是我该替你决定的）：

- 放进 `tests/forward/`（那里已有 `win.c` 这个 kernel32 冒烟）并在
  `tests/forward.sh` 显式登记——但 `forward` 门禁目前是 macOS-only 且没有
  `--com` 变体，A1 之前登记会一直红。
- 或新建 `tests/winforward.sh`，只跑 ≤4 参数那批（tick / clockfmt / envdir /
  fileio / vmem / structs / procsnap / argceil / outparam / mailbox），
  `gui/` 与 `refused/` 里的失败样本留作文档，不进门禁。

`refused/` 里的样本**故意是失败的**，价值在失败方式本身（编译期
`undefined function` vs 运行期 `no host function` + exit 127 vs 假句柄后访问
违例），所以不要"修好"它们。## 两条路线（用户 2026-10-03 补充的要求）与它们各自的实测边界

Windows 上要同时支持两条路线：**原生 Win32 libc/DLL**，和**包装出来的 Win POSIX**
（类似 cosmopolitan，但更轻）。实测下来这两条路线不是二选一，而是应该分工——
因为仓库里其实有**三条**互不相同的 Windows 互操作通道，各自的能力上限不同：

| 通道 | 名字来源 | 参数个数 | 实测上限 |
|--- |--- |--- |--- |
| PE 导入表 | 固定 14 个 kernel32 名 | **不限** | stdio 就走这条；`open()` 的 CreateFileA(7) 也只能走这条 |
| hostcall 转发 | ucrtbase/kernel32/ws2_32/msvcrt | **4** | 5 参起静默丢参（outparam.c） |
| libffi 桥 | dlsym + libffi | 不限 | **仅 macOS**（`unisacc_ffi.h:125`） |

分工 therefore 是清楚的：**宽调用（参数多）归库，薄包装归转发**。
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

顺带一个跨平台写法的坑：**`#include <...>` 必须带 `.h`**，`<unistd.h>` 行，
`<unistd>` 报 `no such file for #include`。移植别的项目时这会先炸一批。

route 2 现在最要命的缺口是**不能起子进程**（`fork`/`execvp`/`waitpid` 全无）。
对 agent harness 来说这直接限制了"由 unisacc 程序去启动别的程序"。
补法很清楚：`CreateProcessW` 是 10 个参数，走转发一定失败，但可以走**导入表**——
把 `CreateProcessW` 加进 `unisa/image/pe.py:21` 的 `IMPORTS` 列表即可。
注意那个列表被 `tests/consts_check.py:84` 与 C 侧 `BK_NIMP` 钉在一起，
所以加名字要同时改 Python、C 和那条门禁。

### 路线分工对两个目标的映射

| 需求 | 需要的路线 | 现状 |
|--- |--- |--- |
| 消息循环 GetMessageW/PostMessage/DispatchMessage（4 参） | route 1 | 修好参数上限即可 |
| 热键 RegisterHotKey(3) | route 1 | 同上 |
| 托盘 Shell_NotifyIcon(3) | route 1 | 同上 |
| 窗口枚举 EnumWindows(2) GetWindowTextW(3) | route 1 | 同上（还要 DLL 白名单） |
| 输入注入 SetCursorPos(2) keybd_event(4) | route 1 | 同上 |
| 建窗口 CreateWindowEx(12) | route 1 | 需要 MS x64 调用约定（A1） |
| 截图 BitBlt(9) | route 1 | 同上，且需要 gdi32 进白名单 |
| 文件/目录/时间/环境 | route 2 | **已经可用** |
| 启动子进程、socket | route 2 | 需给导入表加名字 |
| 与 agent 之间的 IPC | route 2（文件邮箱） | **已经可用**（mailbox.c） |

所以顺序上有个重要结论：**route 1 的"薄"部分（消息循环、热键、托盘、窗口枚举、
输入注入）参数都在 4 以内，只要把转发上限与 DLL 白名单修好，shell 的骨架就能跑起来**；
真正需要 A1（MS x64 宽调用）的只有建窗口和截图。而 route 2 现在就能承担
文件/目录/时间/IPC 这一整块，不必等任何修复。
