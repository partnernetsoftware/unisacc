# csih.com：一个文件，六个目标

csih.com 是一个 APE（Actually Portable Executable）。它和 unisacc.com、gcc_cx.com 的打法相同：五个按目标编出的切片，由 `ujs/product/ape-pack.cx` 拼成一个文件，开头的 shell 文本按 `uname` 选出该跑的那一片。

## 怎么构造

从仓库根目录，一条命令做一步。每个 Unix 切片要把 csih 编一遍，约 45–50 秒：

```
sh ./unisacc.com tools/csih-ape.cx -- slice lnx/x86_64
sh ./unisacc.com tools/csih-ape.cx -- slice lnx/arm64
sh ./unisacc.com tools/csih-ape.cx -- slice osx/x86_64
sh ./unisacc.com tools/csih-ape.cx -- slice osx/arm64
sh ./unisacc.com tools/csih-ape.cx -- slice win/x86_64
sh ./unisacc.com tools/csih-ape.cx -- pack
sh ./unisacc.com tools/csih-ape.cx -- verify
sh ./unisacc.com tools/csih-ape.cx -- smoke
```

- **smoke**（约 5 秒）：在本机能查的都实际查一遍，查不了的标 UNMEASURED，不记 PASS。
  - 五个切片的头部：win 是 PE、机器号 0x8664；lnx 是 ELF64，机器号 62 / 183；osx 是 Mach-O 64，cputype 0x1000007 / 0x100000c。
  - csih.com 确实带着每一片：在每片的 1/4、1/2、3/4 处各取 64 字节，到 csih.com 里找。
  - 本机经 csih.com 跑 `selftest`。
  - 在 qemu-aarch64-static 加 `/usr/aarch64-linux-gnu` sysroot 下，跑 arm64 切片的 `selftest`；找不到 qemu 或 sysroot 就记 UNMEASURED。
  - macOS 两片、Windows 一片只查头部，运行记 UNMEASURED。
  - 末行是 `CSIH_APE SMOKE_OK ran=2 fail=0 unmeasured=3`。

- 产物都在 `out/csih-ape/`，git 不跟踪：五个切片、`csih.com`、`manifest.tsv`。
- 每个切片旁边有一个 `.src` 文件，记着它是用哪一版 csih 源码编的（就是 `apps/csih/csih.sh digest` 给出的产品源摘要）。
- 不一致时会拒绝：
  - **pack**：各切片记录的源码摘要不一致，或者和当前源码对不上，就拒绝打包。
  - **verify**：清单里记的各切片哈希、csih.com 哈希、编译器 unisacc.com 的哈希、源码摘要，只要有一项和现在对不上，就拒绝。
- 构造期间要占用单机的重构造窗口，按 CLAUDE.md 一次只做一组。别人在跑重测试时，就等它跑完。

## 用

```
sh out/csih-ape/csih.com            # 默认 agent
sh out/csih-ape/csih.com selftest
sh out/csih-ape/csih.com keyprobe
```

csih.com 是自包含的二进制，不需要 `apps/csih` 源码树，也不读 `csih.sh` 的产物戳。在 macOS 上和 unisacc.com 一样，要用 `sh csih.com` 启动（直接 `execve` 一个 APE 会得到 ENOEXEC）。

## 六目标矩阵（2026-10-30，unisacc 0.0.43，本机 Linux x86_64）

| 目标 | 切片 | 编出 | 实跑 | 说明 |
| --- | --- | --- | --- | --- |
| lnx/x86_64 | csih_lnx64-x86.bin（真 csih） | 是 | 本机经 csih.com：selftest、keyprobe，以及一轮真实 DeepSeek 对话（exec `uname -m; ls -1 \| wc -l`，答复正确） | 主力平台 |
| lnx/arm64 | csih_lnx64-arm.bin（真 csih） | 是，`file` 认作 ELF aarch64 | qemu-aarch64-static 下 selftest 通过 | 真机 UNMEASURED；qemu-user 下 `uname` 报的是宿主架构，所以测的是切片本身，不是 csih.com 选片的那一步 |
| osx/x86_64 | csih_osx64-x86.bin（真 csih） | 是，Mach-O x86_64 | UNMEASURED | 本机跑不了 macOS |
| osx/arm64 | csih_osx64-arm.bin（真 csih） | 是，Mach-O arm64 | UNMEASURED | 同上 |
| win/x86_64 | csih_win64-x86.exe（占位） | 是，PE32+ | 同一份源码在 Linux 上 `-run`：打印说明，退出码 2；PE 本身 UNMEASURED | 见下文"Windows" |
| win/arm64 | 用 win/x86_64 那个 PE | 不单独编 | UNMEASURED | APE 的约定（与 gcc_cx.com 相同） |

## 已知在别的平台上可能出问题的地方

- **HTTPS**：`net.cx` 用 dlopen 加载 libcurl。Linux 上找 `libcurl.so.4`，macOS 上找 `/usr/lib/libcurl.4.dylib`。没有的话联网会失败；本机以外没有实测过。
- **沙箱**：read/work 档的只读挂载、机密遮蔽、私有 PID、断网，都靠 Linux 的 `unshare`。macOS 上没有，会退回到只靠关键字拦截危险命令（PRD 里已写明），隔离明显变弱。
- **tmux**：窗口列表、`/copy` 和多会话协作要用 tmux；没装 tmux 时，这些功能如实报不可用。
- **信号**：unisacc 的 `signal()` 不向内核注册（BLOCKED）。被 kill 时，csih.sh 会帮忙恢复终端，直接跑 csih.com 没有这层兜底。终端如果乱了，执行一次 `reset`。

## Windows

csih 的 TUI 是 POSIX 写法：termios 原始模式、`O_DIRECTORY`、`fcntl(F_GETFL)`、进程组、tmux。按 win/x86_64 编译时会报 `unknown identifier` 和 `incomplete struct termios`。要有真正的 Windows 版，得先把这几处移植到 Console API。

2026-10-30 在仓外副本里做了一次试探：把第一层的 3 个错误用占位定义压掉，再编一次，又冒出第二层 6 个错误，涉及 `O_NOFOLLOW`、`O_NONBLOCK`、`tcsetattr`/`TCSANOW`/`TCSAFLUSH`、`struct winsize`。可见错误是一层一层露出来的，不是少数几处。

更大的缺口是运行期：exec 工具要靠 `sh`，沙箱要靠 `unshare`，还有 tmux 见证和进程组。这些在 Windows 上都没有对应。本机也没有 Windows 或 wine，移植出来也只能记 UNMEASURED。所以 Windows 版不在快车道里做。真要做时，从三处入手：`term.cx` 改用 Console API，`shell.cx` 改用 CreateProcess 加 cmd/PowerShell，沙箱档在 Windows 上一律降成拒绝。

在那之前，win 切片是 `apps/csih/tools/win-unsupported.cx`：只打印"Windows 尚未支持"，退出码 2。这样不会出现一个半能用、或者干脆没有的切片。
