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

- **从提交构造（推荐）**：多个会话同时改 cx 底层时，直接在工作区编，五个切片中途源码摘要就可能变掉，pack 会拒绝。改用快照：
  ```
  sh ./unisacc.com tools/csih-ape.cx -- snapshot [REV]       # 默认 HEAD，git archive 导出到 out/csih-ape/tree
  cd out/csih-ape/tree
  sh ./unisacc.com tools/csih-ape.cx -- slice lnx/x86_64     # ……五片、pack、verify、smoke 都在快照里跑
  cd ../../..
  sh ./unisacc.com tools/csih-ape.cx -- install              # 装到 apps/csih/csih.com，清单加 commit 行
  sh ./unisacc.com tools/csih-ape.cx -- verify               # 对 commit 清单：核对快照里的摘要、版本和切片
  ```
  - 快照里只有已提交的内容，别人没提交的改动混不进来，也打断不了构造；同一个提交可以重复构造。
  - 编译器 unisacc.com 没进 git，会复制进快照；它的字节算在源码摘要里。
  - 快照路径是新的，而 unisacc 的 E3 编译缓存（`/tmp/unisacc-e3cache-<uid>`）按路径命中，所以快照里全是冷编译：一片要 59 秒以上（user 时间约 2 分 11 秒），在工作区里约 51 秒（user 约 1 分钟）。负载一高，冷编译就超过 60 秒的命令上限。
  - 在快照里，切片要直接用 unisacc 编，再用 `mark TARGET` 记下摘要；套一层 csih-ape 工具的话，工具自己的 -run 冷启动还要再加几秒。
- **平时的做法：在工作区编，记下提交号**。pack 时，如果 csih.cx 编译闭包里的文件（以及 csih.sh）跟 HEAD 相比没有已暂存或未暂存的改动，清单就写 `commit` 行，记下 HEAD。
  - 这时 csih.com 就是那个提交的 csih，verify 照常核对工作区的源码摘要。
  - 闭包里有未提交的改动（例如别的会话正在改 cx/sys）时，不写 commit 行；要构造对应提交的版本，就等它们提交以后再编。
- **smoke**（约 5 秒）：在本机能查的都实际查一遍，查不了的标 UNMEASURED，不记 PASS。
  - 五个切片的头部：win 是 PE、机器号 0x8664；lnx 是 ELF64，机器号 62 / 183；osx 是 Mach-O 64，cputype 0x1000007 / 0x100000c。
  - csih.com 确实带着每一片：
    - 四个 Unix 片：按 shell 头里对应 uname 那一行的 `o=` 和 `n=` 截出 gzip 段，用系统的 `gzip -dc` 解开（目标机上的启动器跑的就是这条命令），再和切片逐字节比对。
    - PE 片：按原样放在文件里，在它的 1/4、1/2、3/4 处各取 64 字节到 csih.com 里找。
  - `--version` 和 `-v` 都打印 `csih <CSIH_VERSION>`。
  - 本机经 csih.com 跑 `selftest`。
  - 在 qemu-aarch64-static 加 `/usr/aarch64-linux-gnu` sysroot 下，跑 arm64 切片的 `selftest`；找不到 qemu 或 sysroot 就记 UNMEASURED。
  - macOS 两片、Windows 一片只查头部，运行记 UNMEASURED。
  - 末行是 `CSIH_APE SMOKE_OK ran=2 fail=0 unmeasured=3`。

- 成品是 `apps/csih/csih.com`，和 `csih.cx` 放在同一目录，git 不跟踪（见 `apps/csih/.gitignore`）。
- 中间产物在 `out/csih-ape/`：五个切片和 `manifest.tsv`。
- **体积**：pack 调 ape-pack 时带 `--deflate`，四个 Unix 片用 `cx/text/deflate.cx` 压缩，结果是 gzip 格式。
  - csih.com 从 8.9MB 降到 1.34MB（2026-10-30；d8eafebcee 时为 1.12MB，839a435f36 时为 1,129,680 字节，7a8c033ec9 时为 1,130,032 字节，0.1.1 / 4e73f38a8b 时为 1,130,992 字节）。
  - 每片约 1.9–2.5MB，压到 0.32–0.35MB，比 `gzip -6` 还略小一点。
  - 目标机第一次运行时，用 `gzip -dc` 解到 `~/.cache/csih/csih-<key>`，以后直接用缓存。
  - 压缩器单独检查：`sh ./unisacc.com tools/check-cx-deflate.cx`。
  - ape-pack 不带 `--deflate` 时，输出和原来逐字节相同。
- **版本**：只有一个来源，就是 `apps/csih/tui.cx` 里的 `CSIH_VERSION`，现在是 0.1.1（0.1.0 → 0.1.1 的变化见 PRD 第六十七批）。
  - pack 从源码读出它，写进 PE 的 VERSIONINFO 和清单的 `version` 行。
  - verify 会拿清单里的版本和源码对照。
  - `csih -v`、`--version`、`version` 打印同一行。
- 每个切片旁边有一个 `.src` 文件，记着它是用哪一版 csih 源码编的（就是 `apps/csih/csih.sh digest` 给出的产品源摘要）。
- 不一致时会拒绝：
  - **pack**：各切片记录的源码摘要不一致，或者和当前源码对不上，就拒绝打包。
  - **verify**：清单里记的各切片哈希、csih.com 哈希、编译器 unisacc.com 的哈希、源码摘要，只要有一项和现在对不上，就拒绝。
- 构造期间要占用单机的重构造窗口，按 CLAUDE.md 一次只做一组。别人在跑重测试时，就等它跑完。

## 用

```
sh apps/csih/csih.com            # 不带参数即 agent（与 csih.sh 相同；d8eafebcee 之前直接跑会只打印用法）
sh apps/csih/csih.com -v         # csih 0.1.1
sh apps/csih/csih.com selftest
sh apps/csih/csih.com keyprobe
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

## 启动器的两个已知问题（ape-pack 生成，未修）

csih.com 开头那段 shell 由 `ujs/product/ape-pack.cx` 生成，unisacc.com、gcc_cx.com 用的也是这一段。ujs/ 是独立子项目，本车道不改，问题记在这里，交主人裁定。

- **退到 /tmp 时会执行别人放的文件**：HOME 为空，或 `mkdir -p` 失败时，缓存目录退到 `${TMPDIR:-/tmp}`。脚本只要发现 `$d/csih-<k>` 存在且可执行，就直接 exec，不检查属主，也不校验内容。共享的 /tmp 里，别的用户可以先放一个同名文件。
  - 平时 HOME 有值，走不到这条路；没有 HOME 的 cron、systemd 服务会走到。
  - 建议的改法：退路改成 `mkdir -m 700` 建一个按 uid 区分的私有目录，并检查它属于自己（`[ -O ]`）、不是软链；或者 exec 之前核对缓存文件的摘要。
- **缓存目录存在但不可写时直接失败**：`~/.cache` 存在但不可写时，`mkdir -p` 照样成功，接着解压失败，`exit 1`，不会退到别的目录。建议同时检查 `[ -w "$d" ]`。

## 已知在别的平台上可能出问题的地方

- **HTTPS**：`net.cx` 用 dlopen 加载 libcurl。Linux 上找 `libcurl.so.4`，macOS 上找 `/usr/lib/libcurl.4.dylib`。没有的话联网会失败；本机以外没有实测过。
- **沙箱**：read/work 档的只读挂载、机密遮蔽、私有 PID、断网，都靠 Linux 的 `unshare`。macOS 上没有，会退回到只靠关键字拦截危险命令（PRD 里已写明），隔离明显变弱。
- **tmux**：窗口列表、`/copy` 和多会话协作要用 tmux；没装 tmux 时，这些功能如实报不可用。
- **信号**：
  - 自 839a435f36 起，终端恢复走 `sys.sig`（rt_sigaction 真注册）。在 Linux x86_64 的 pty 里对 csih.com 发 SIGTERM，终端模式恢复成启动前的样子，退出码 143。旧版前导 `signal()` 不进内核，被杀后终端停在原始模式。
  - macOS 上 `sy_sig_handle` 返回 -ENOSYS，被杀时不恢复终端，乱了就执行一次 `reset`。

## Windows

csih 的 TUI 是 POSIX 写法：termios 原始模式、`O_DIRECTORY`、`fcntl(F_GETFL)`、进程组、tmux。按 win/x86_64 编译时会报 `unknown identifier` 和 `incomplete struct termios`。要有真正的 Windows 版，得先把这几处移植到 Console API。

2026-10-30 在仓外副本里做了一次试探：把第一层的 3 个错误用占位定义压掉，再编一次，又冒出第二层 6 个错误，涉及 `O_NOFOLLOW`、`O_NONBLOCK`、`tcsetattr`/`TCSANOW`/`TCSAFLUSH`、`struct winsize`。可见错误是一层一层露出来的，不是少数几处。

更大的缺口是运行期：exec 工具要靠 `sh`，沙箱要靠 `unshare`，还有 tmux 见证和进程组。这些在 Windows 上都没有对应。本机也没有 Windows 或 wine，移植出来也只能记 UNMEASURED。所以 Windows 版不在快车道里做。真要做时，从三处入手：`term.cx` 改用 Console API，`shell.cx` 改用 CreateProcess 加 cmd/PowerShell，沙箱档在 Windows 上一律降成拒绝。

在那之前，win 切片是 `apps/csih/tools/win-unsupported.cx`：只打印"Windows 尚未支持"，退出码 2。这样不会出现一个半能用、或者干脆没有的切片。
