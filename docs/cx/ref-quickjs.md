# 参照调研: QuickJS 与 quickjs-ng 的核心面与版本策略

用途: 为 cx 核心层(cx.sys/os/path/mem/text/stream)与扩展(ext.*)的一次性定面、定稳定性策略提供参照。
待判假设(主人): 按 QuickJS 的风格把 sys + os + net 设计完整, 就差不多够了。

取证方式与可信度标注:

- [源码实查] 2026-11-12 取 master 分支原文件并逐表核对:
  - https://raw.githubusercontent.com/bellard/quickjs/master/quickjs-libc.c (4506 行), quickjs.h (1289 行), VERSION (内容 2026-06-04)
  - https://raw.githubusercontent.com/quickjs-ng/quickjs/master/quickjs-libc.c (5085 行), quickjs.h (1514 行), quickjs-libc.h, CMakeLists.txt, docs/docs/diff.md
- [文档实查] 同日抓取:
  - https://bellard.org/quickjs/quickjs.html (页面标注 version 2026-06-04)
  - https://bellard.org/quickjs/Changelog
  - https://quickjs-ng.github.io/quickjs/stdlib
  - https://github.com/quickjs-ng/quickjs/releases (仅第一页, v0.12.1 到 v0.18.0)
- [回忆] 未在本轮核实, 来自既有知识, 使用前需复核。

先给出与派工前提不符的一条事实: **两个仓的 quickjs-libc.c 都没有套接字 API**。
bellard 版文件头仍留着 `/* TODO: - add socket calls */`(第 84 到 86 行), 全文件 socket/getaddrinfo 字样只此一处;
quickjs-ng master 的 os 函数表(第 4420 到 4515 行)同样没有 socket/connect/listen/accept。
"quickjs-ng 已有 sockets" 这一说法本轮查不到证据, 按不成立处理。唯一的网络入口是 `std.urlGet`, 它是调外部 curl 命令行实现的。

## 1 分层与模块结构

QuickJS 是三层, 每层一个到几个 C 文件:

| 层 | 文件 | 内容 | 是否可裁掉 |
|---|---|---|---|
| 引擎 | quickjs.c, quickjs.h, libregexp, libunicode, cutils, dtoa | 语言本体, 值/对象/GC/模块/字节码; 不含任何 I/O | 不可 |
| 系统库 | quickjs-libc.c, quickjs-libc.h | std, os(ng 另有 bjson)三个原生模块, 事件循环, Worker, 模块加载器 | 可, 嵌入者常整文件替换 |
| 程序 | qjs.c(解释器/REPL), qjsc.c(字节码编译成 C) | 命令行外壳 | 可 |

要点:

- (1) 引擎层没有 I/O。连 `print`、`console.log`、`scriptArgs` 都由 quickjs-libc 的 `js_std_add_helpers` 注入。[源码实查]
- (2) 系统库只切成两个模块, 切分标准是抽象高度而不是功能域:
  - `std` = libc stdio 高度(FILE 缓冲流, printf, 环境变量, 进程退出, 脚本求值)。
  - `os` = POSIX 系统调用高度(fd, 文件系统元数据, 进程, 信号, 定时器, 事件循环, tty, Worker)。
- (3) 事件循环不是独立模块, 藏在 os 里: `os.setReadHandler/setWriteHandler/signal/setTimeout` 向每运行时的线程状态登记回调,
  由 C 侧 `js_std_loop` 用一次 `poll()`(Windows 上是另一套实现)驱动。[源码实查, ng 第 2841 到 2998 行]
- (4) 原生模块接口(两仓一致)[源码实查 quickjs.h]:
  - `JSModuleDef *JS_NewCModule(ctx, name, init_func)` 创建模块;
  - `JS_AddModuleExport / JS_AddModuleExportList` 在创建期声明导出名;
  - `JS_SetModuleExport / JS_SetModuleExportList` 在 init 回调里填值;
  - 表驱动: 导出用 `JSCFunctionListEntry` 静态数组(`JS_CFUNC_DEF`、`JS_PROP_INT32_DEF`、`JS_OBJECT_DEF` 等宏)描述, 一张表就是一个模块的全部面;
  - 动态模块: `.so` 导出 `js_init_module`, 由 `js_module_loader` 以 dlopen 装入; ng 加了 `JS_MODULE_EXTERN` 可见性宏。
  - 加载策略外置: `JS_SetModuleLoaderFunc/Func2` 由嵌入者提供解析与装载函数, 引擎不认识文件系统。
- (5) ng 把模块名改成带前缀的 `qjs:std`、`qjs:os`、`qjs:bjson`。[文档实查]

对照 cx: "一张静态表 = 一个模块的面" 与 cx 的 `build/layer-api/*.tsv` 事实表是同一思路; QuickJS 没有生成器与 `--check`, 表就是源码。

## 2 核心面清单

以下逐表列全。未加标注的两仓都有; `[ng]` 仅 quickjs-ng; `[b]` 仅 bellard。全部为 [源码实查] 加 [文档实查] 交叉。

### 2.1 std 模块

进程与求值:

- `exit(n)`
- `gc()`
- `evalScript(str, options)`; options: `backtrace_barrier`, `async`
- `loadScript(filename)`
- `parseExtJSON(str)` [b](2025-09-13 起向 JSON5 靠拢; ng master 表内无此项)
- `__printObject`, `__sprintObject` [b](内部用)

环境:

- `getenv(name)`, `setenv(name, value)`, `unsetenv(name)`, `getenviron()`

整文件与网络便捷:

- `loadFile(filename)`; ng 增 options `binary`
- `writeFile(filename, data)` [ng]
- `urlGet(url, options)`; options: `binary`, `full`; 实现为调 curl 命令行

FILE 构造:

- `open(filename, flags, errorObj)`, `popen(command, flags, errorObj)`, `fdopen(fd, flags, errorObj)`, `tmpfile(errorObj)`
- 预开对象: `in`, `out`, `err`

格式化:

- `puts(str)`, `printf(fmt, ...)`, `sprintf(fmt, ...)`

错误:

- `strerror(errno)`
- `Error` 常量对象: `EINVAL EIO EACCES EEXIST ENOSPC ENOSYS EBUSY ENOENT EPERM EPIPE`, ng 多一个 `EBADF`。只有 10 到 11 个, 不是 errno 全集。
- `SEEK_SET`, `SEEK_CUR`, `SEEK_END`

FILE 原型方法:

- `close()`, `flush()`
- `puts(str)`, `printf(fmt, ...)`
- `seek(offset, whence)`, `tell()`, `tello()`, `eof()`, `fileno()`, `error()`, `clearerr()`
- `read(buffer, position, length)`, `write(buffer, position, length)`
- `getline()`, `readAsString(max_size)`, `readAsArrayBuffer(max_size)` [ng], `getByte()`, `putByte(c)`

### 2.2 os 模块

fd 级 I/O:

- `open(filename, flags, mode = 0o666)`; 标志 `O_RDONLY O_WRONLY O_RDWR O_APPEND O_CREAT O_EXCL O_TRUNC`, Windows 另有 `O_TEXT`(ng 加 `O_BINARY`)
- `close(fd)`, `seek(fd, offset, whence)`, `read(fd, buffer, offset, length)`, `write(fd, buffer, offset, length)`
- `dup(fd)`, `dup2(oldfd, newfd)`, `pipe()`

tty:

- `isatty(fd)`, `ttyGetWinSize(fd)`, `ttySetRaw(fd)`

文件系统:

- `remove(filename)`, `rename(old, new)`, `realpath(path)`, `getcwd()`, `chdir(path)`, `mkdir(path, mode = 0o777)`
- `stat(path)`, `lstat(path)`; 返回 `[obj, err]`; 模式常量 `S_IFMT S_IFIFO S_IFCHR S_IFDIR S_IFBLK S_IFREG S_IFSOCK S_IFLNK S_ISGID S_ISUID`
- `utimes(path, atime, mtime)`, `symlink(target, linkpath)`, `readlink(path)`, `readdir(path)`
- `mkdtemp(pattern)`, `mkstemp(pattern)` [ng, v0.13.0 加入; Windows 与 WASI 不可用]
- `exePath()` [ng; 不支持时返回 undefined]

进程与信号:

- `exec(args, options)`; options: `block`, `usePath`, `file`, `cwd`, `stdin`, `stdout`, `stderr`, `env`, `uid`, `gid`, ng 另有 `groups`
- `getpid()`, `waitpid(pid, options)`, 常量 `WNOHANG`, `kill(pid, sig)`
- `signal(signal, func)`; 常量 `SIGINT SIGABRT SIGFPE SIGILL SIGSEGV SIGTERM`;
  ng 在非 Windows 非 WASI 上另有 `SIGQUIT SIGPIPE SIGALRM SIGUSR1 SIGUSR2 SIGCHLD SIGCONT SIGSTOP SIGTSTP SIGTTIN SIGTTOU`

时间与事件循环:

- `now()`(2024-01-13 加入), `cputime()` [ng]
- `sleep(delay_ms)`, `sleepAsync(delay_ms)`(2024-01-13 加入)
- `setTimeout(func, delay)`, `clearTimeout(handle)`; ng 另有 `setInterval`, `clearInterval`
- `setReadHandler(fd, func)`, `setWriteHandler(fd, func)`; 每 fd 每方向只能登记一个回调, func 传 null 即注销

其它:

- `platform` 字符串: bellard 为 `linux darwin win32 js`; ng 另有 `cygwin openbsd netbsd freebsd wasi hurd unknown`
- `Worker(module_filename)`, 静态 `Worker.parent`, 实例 `postMessage(msg)`, `onmessage`; 不支持嵌套 Worker

### 2.3 bjson 模块 [ng]

- `bjson.write(obj, flags)`, `bjson.read(buf, pos, len, flags)`; 标志 `WRITE_OBJ_BYTECODE/REFERENCE/SAB/STRIP_DEBUG/STRIP_SOURCE`, `READ_OBJ_BYTECODE/REFERENCE`
- bellard 文档明示该二进制格式"可能不经通知改变, 不应用于持久存储"。

### 2.4 quickjs-libc 的 C 侧面(嵌入者用) [源码实查 ng quickjs-libc.h]

- `js_init_module_std`, `js_init_module_os`, `js_init_module_bjson`
- `js_std_add_helpers`, `js_std_init_handlers`, `js_std_free_handlers`
- `js_std_loop`, `js_std_loop_once`, `js_std_poll_io`, `js_std_await`
- `js_std_dump_error`, `js_std_promise_rejection_tracker`, `js_std_eval_binary`
- `js_load_file`, `js_module_loader`, `js_module_load`, `js_module_set_import_meta`, `js_module_check_attributes`
- `js_std_set_worker_new_runtime_func`, `js_std_set_worker_new_context_func`

### 2.5 规模

- std 约 21 个函数加 17 到 18 个 FILE 方法; os 约 36 个(bellard)到 43 个(ng)函数; 合计不到 110 个入口, 常量约 40 个。
- 这就是一个"能写命令行工具与 REPL"的最小系统面, 而不是系统编程核心的全集。

## 3 错误与资源模型

错误有三种并存的约定, 没有统一:[文档实查]

- (1) std 的 FILE 构造函数: 失败返回 `null`, 若传了 `errorObj` 则把 `errorObj.errno` 置为错误码, 成功置 0; 不传时不抛异常。
- (2) os 的大多数函数: 返回整数, 负值即 `-errno`(`os.open/close/read/write/remove/mkdir/kill` 等)。
- (3) os 中需要返回值的函数: 返回二元组 `[值, err]`(`stat/lstat/readdir/realpath/getcwd/readlink`)。
- (4) 参数类型错与内存不足走 JS 异常; `std.loadFile` 失败返回 null, `std.loadScript/evalScript` 抛异常。

评价: 调用方必须逐函数查手册才知道怎么判错; `std.Error` 只导出 10 到 11 个常量, 其余错误码只能靠 `strerror` 转字符串。这是该设计最明显的债。

资源模型:

- fd 是裸整数, 无所有权: 不 close 就泄漏, close 之后可被复用而被误用, 没有句柄代次检查。
- FILE 是带类 ID 的对象, GC 终结器会 fclose(popen 的走 pclose); `std.in/out/err` 标记为不可关闭。这是唯一带生命周期的资源。
- 定时器、读写回调、信号回调挂在每运行时的 `JSThreadState` 链表上, `js_std_free_handlers` 统一释放。
- 没有截止时间与取消: `read/write/waitpid` 都是阻塞调用; 超时只能靠 `setTimeout` 加回调自己拼; `exec` 没有 timeout 选项。
- 引擎侧的资源界限在 C API 上而不在 std/os 上: `JS_SetMemoryLimit`, `JS_SetMaxStackSize`, `JS_SetInterruptHandler`。
- 引用计数加环回收; C API 要求嵌入者手工 `JS_DupValue/JS_FreeValue`, 配对错误是嵌入者最常见的缺陷来源。[回忆]

## 4 可移植性处理

做法只有一种: 在 quickjs-libc.c 里用预处理条件整段开关函数表项, 不存在的函数直接不出现在模块里。[源码实查 ng 第 4416 到 4516 行]

| 条件 | 被关掉的面 |
|---|---|
| `_WIN32` | 扩展信号常量, `mkdtemp/mkstemp`, `S_IFSOCK S_IFLNK S_ISGID S_ISUID`, `lstat symlink readlink exec getpid waitpid pipe kill dup dup2` |
| `__wasi__` | 以上全部, 另加 `ttyGetWinSize ttySetRaw realpath`, popen/tmpfile 等, Worker |
| Apple tvOS/watchOS | 进程类(exec 一组) |
| 仅 `_WIN32` | 多出 `O_TEXT`(ng 加 `O_BINARY`) |

特点与代价:

- (1) 平台差异对脚本可见的方式是"属性是否存在"加 `os.platform` 字符串, 没有能力查询表, 也没有"该函数在本平台返回 ENOSYS"的统一退路。
- (2) Windows 长期是二等: bellard 版只支持 MinGW 交叉构建; ng 把 Windows 与 MSVC 提为一等并改用 CMake, 每个 PR 在 50 多种配置上测。[文档实查 diff.md]
- (3) 没有路径抽象: 分隔符、盘符、大小写全由脚本自己处理。
- (4) ng v0.13.0 删除了大端支持, v0.14.0 又修了原生大端构建, 说明目标集合本身也在变。[文档实查 releases]
- (5) 2024-01-13 起 bellard 版支持 cosmopolitan 构建; ng v0.18.0 也加了。与 unisacc.com 的单文件多目标形态最接近的先例在这里。

与 cx 规则对照: QuickJS 把平台 `#if` 集中在 libc 一个文件, 上层(JS)完全不写平台分支, 这点与 "平台差异只放 L0" 一致;
不同的是它让面本身随平台伸缩, cx 若要六目标同面, 就得走"同名同签名, 不支持时返回明确错误"的路线。

## 5 稳定性与版本策略(实际做法与缺口)

### 5.1 bellard/quickjs

实际做法:[文档实查 Changelog 与 VERSION]

- 版本号就是日期, VERSION 文件一行。全部发布: 2019-07-09 起到 2021-03-27 共 16 个, 之后空 32 个月, 再是 2023-12-09, 2024-01-13, 2025-04-26, 2025-09-13, 2026-06-04。
- Changelog 每版十来行短句, 不分"新增/变更/破坏"类别, 不标兼容性。
- 没有书面稳定性承诺。文档里与稳定性有关的只有两句: 字节码格式绑定具体版本且不做安全校验; bjson 格式可不经通知改变。
- 没有 API 版本宏, 没有 SOVERSION, 只发源码包, 预期嵌入者把源码拷进自己仓。

破坏性变更的实例:

- 2025-04-26 一次性删除 bignum 扩展(BigFloat, BigDecimal, 运算符重载)、qjscalc、"use strip", 无弃用期。
- C API 只见增补记录(`JS_UpdateStackTop`, `JS_GetAnyOpaque`, `JS_PrintValue`, `JS_FreePropertyEnum`, `JS_AtomToCStringLen`), 签名变化不单列。
- std/os 面变化极慢: 近六版里只有 2024-01-13 加了 `os.sleepAsync/getpid/now` 三个函数, 2025-09-13 调了 `parseExtJSON`。

结论: 稳定性来自"面很小而且作者几乎不动它", 不来自任何机制。面一旦要长(例如加套接字), 这种稳定性就没有保障。

### 5.2 quickjs-ng

实际做法:[源码实查 quickjs.h 与 CMakeLists.txt, 文档实查 releases 与 diff.md]

- 语义化形式的标签 v0.x.y; `quickjs.h` 内有 `QJS_VERSION_MAJOR/MINOR/PATCH/SUFFIX` 宏(master 为 0.18.0)与 `JS_GetVersion()`。
- 共享库 `VERSION = 主.次.补`, `SOVERSION = 主版本号`。主版本一直是 0, 所以 SOVERSION 恒为 0, 无法区分互不兼容的次版本。
- 发布节奏约每两个月一版; 发布说明是 PR 标题罗列, 没有"破坏性变更"分区。
- 定位声明: "面向已使用 QuickJS 者的直接替换, 可能需要少量 API 改动"。这是意图, 不是承诺。
- v0.16.0 起共享构建只导出带 `JS_EXTERN` 的 API 符号, 这是唯一一项机制性的面收口。
- v0.15.0 加 SECURITY.md, 写明 `JS_READ_OBJ_BYTECODE` 等的信任前提。

0.x 期间实际发生的破坏(仅据第一页发布说明的标题推断, 标题本身未标注破坏):

- v0.18.0: `JSInterruptHandler` 回调增加 JSContext 参数(回调签名变); OOM 与 InternalError 改为 JS 代码不可捕获(行为变)。
- v0.17.0: `JSClass` 移出 quickjs.h。
- v0.16.2: `JS_GetClassName()` 返回值改为原子; `os.chdir`、`std.setenv` 的参数个数修正。
- v0.13.0: bjson 去掉 SAB 支持并加校验字段(序列化格式变); 删除大端支持。

### 5.3 嵌入者如何依赖 C API

- [回忆] 主流嵌入者(txiki.js, 各语言绑定如 rquickjs、quickjs-emscripten、QuickJS 的 Go/Zig/Python 包装)几乎都是把源码按提交或标签整份收进自己仓,
  升级时自己跑测试、自己改适配层; 没有人依赖系统级共享库的二进制兼容。
- [回忆] 较大的运行时(txiki.js 用 libuv, 其它基于 QuickJS 的服务端运行时各有自己的 I/O 层)都不使用 quickjs-libc 的 std/os, 而是整层替换。
  也就是说 std/os 在生态里的地位是"参考实现与 qjs 自用", 不是被广泛依赖的稳定面。
- [源码实查] 真正被当作稳定面使用的是 quickjs.h 里的值/对象/类/模块那一组, 它之所以稳是因为十年没大改, 而不是因为有承诺。

### 5.4 缺口清单

- (1) 没有成文的稳定性等级(稳定/试验/内部)。`__printObject` 这类内部项与公开项混在同一张表里。
- (2) 没有弃用流程: 删除即删除(bignum), 没有"先标记, 隔 N 版再删"。
- (3) 没有面的机器可查清单与差异检查: 是否破坏靠审阅者读 PR, 发布说明不强制标注。
- (4) 0.x 语义化版本对兼容性不携带信息: 次版本既装新增也装破坏。
- (5) 模块面没有独立版本: std/os 跟随引擎版本, 脚本无法询问"os 是否有 mkdtemp", 只能探测属性是否存在。
- (6) 两个仓的面已经分叉(writeFile, exePath, cputime, setInterval, mkdtemp, parseExtJSON, 模块名前缀), 同一脚本不能保证两边都跑。
- (7) 序列化与字节码格式明示不稳定, 但没有版本字段之外的兼容策略(ng 加了校验字段, 属防误读而非兼容)。

cx 已有而 QuickJS 没有的: `build/layer-api/*.tsv` 面事实表, `layer-api --bump` 按变更类别升级, `build/tlib-policy.tsv` 预稳定封顶, `check-tlib-frozen` 冻结断言。
在版本机制上 cx 已经走在两者前面, 这一节对 cx 的价值主要是反例。

## 6 对 cx 的启示

对假设的判断在前: **"sys + os + net 按 QuickJS 风格做完整就差不多够了" 只成立一半**。
成立的一半: 系统调用高度的面确实可以很小, 约 100 个入口就能支撑命令行工具、构建脚本、REPL, QuickJS 六年只加过个位数函数即是证据。
不成立的一半有三点:
(a) QuickJS 自己并没有 net, 该假设里的 net 没有可抄的原型, 得另找参照(libuv, Go net, Rust std::net);
(b) QuickJS 之所以不需要 path/text/buffer/stream, 是因为宿主语言 JS 自带 String、ArrayBuffer、TypedArray、Promise、JSON、RegExp, cx 没有这些内建, 必须自己做成核心层;
(c) 它在截止时间、取消、进程派生选项、错误码全集、句柄所有权上是空白或欠账, 这些正是 cx 设计要求里明写的项。

QuickJS 未覆盖、而系统核心需要的区域(证据见第 2、3 节):

| 区域 | QuickJS 现状 | 只做 sys+os+net 会缺什么 |
|---|---|---|
| net | 无套接字, 仅 curl 外壳 | 无原型可循; 地址解析、非阻塞连接、监听/接受、关闭半连接都要自定 |
| path | 仅 realpath/getcwd | join、规范化、拆分、相对化、分隔符与盘符 |
| text/buffer | 靠 JS 内建; 库里只有 printf/sprintf/getline | 字节串与文本的区分、编码、扫描、格式化、构造器 |
| stream | FILE 是唯一抽象, 不可组合 | 读写接口、缓冲层、管道/文件/套接字/内存的统一、背压 |
| 截止时间与取消 | 无 | 每个阻塞调用的 deadline 参数与统一取消语义 |
| 进程派生 | exec 选项 10 到 11 个 | 无超时、无进程组、无资源上限、无"输出采集"便捷层、Windows 整组缺席 |
| 错误 | 三种约定并存, 常量 10 个 | 统一返回约定与全集错误码表 |
| 资源所有权 | fd 裸整数 | 句柄代次、关闭后使用检测、作用域释放 |
| 文件系统余项 | 无 chmod/chown/link/fsync/truncate/flock/mmap, 无递归建删, 无目录流式遍历 | 构建工具日常要用的那几项 |
| 时间 | now/cputime/sleep | 单调钟与墙钟的区分, 日历时间, 时区 |
| 并发 | 仅 Worker 加消息 | 互斥、原子、通道(QuickJS 明确不做线程) |

决策(至多 12 条):

- (1) 采纳"两个高度"的切分: cx.sys = 系统调用高度(fd、进程、信号、时钟, 一对一薄封装), cx.os = 便捷高度(整文件读写、环境、临时文件、派生加采集)。不按功能域再细切核心。
- (2) 假设修正为 **sys + os + net 再加 path、text、mem、stream 四个语言补位层**。后四层在 QuickJS 里由 JS 语言承担, 在 cx 里必须是核心而不是 ext。
- (3) net 不以 QuickJS 为参照。另开一份参照调研(建议 libuv 与 Go net 各一), 在那之前 net 只定最小面: 解析、连接、监听、接受、收发、关闭, 全部带 deadline。
- (4) 错误约定只许一种, 全核心统一。不采用 QuickJS 的三种并存; 错误码表走事实表生成, 覆盖全集而不是挑 10 个。
- (5) 每个可能阻塞的入口在签名里带 deadline(或显式的无限期常量), 首版就定, 不留到以后补。QuickJS 至今没有, 补不进去了。
- (6) 句柄不用裸整数: 沿用 cx 已有的运行期句柄检查(代次加类型), 并在文档里写明这不等于编译期所有权。
- (7) 平台差异采用"同面同签名, 不支持即返回明确的不支持错误", 不采用 QuickJS 的"函数表项随平台消失"。另提供可查询的能力表, 取代 `os.platform` 字符串比较。
- (8) 派生进程的选项结构一次定全: cwd、env、三个标准流、超时、进程组、是否继承 fd, 六目标同构; 权限类(uid/gid/groups)放 ext。QuickJS 的 exec 在 Windows 上整组缺席, 是反例。
- (9) 事件循环留在 sys 里做成"登记 fd 回调加定时器加一次 poll"的最小形态(QuickJS 约 150 行即够), 不单立模块; 更重的调度放 ext。
- (10) 明确划在核心之外: HTTP、TLS、压缩、线程池、数据库、正则引擎。QuickJS 十年不收这些而生态照常运转, 证明核心可以不含; 它们进 ext.*, 版本独立。
- (11) 稳定性策略以 QuickJS 为反例, 继续走 cx 现有机制并补三项: 每个入口在面事实表里带等级列(稳定/试验/内部); 删除必须先经"弃用"等级至少一个版本; 发布说明的破坏性变更一节由面差异工具生成而不是手写。
- (12) 预稳定期的版本号不要重蹈 ng 的覆辙: 现行 `tlib-policy` 把预稳定的 y/x 封顶为 z 是合理的, 但必须同时在发布说明里打印"按稳定口径本应是 y 或 x", 现有 `pre-stable class=z (would be y|x)` 输出应保留并进入发布说明, 让消费者看得到真实的破坏等级。

未核实与风险:

- 第 5.3 节关于嵌入者做法为回忆, 未逐个项目核对。
- quickjs-ng 发布页只读了第一页; "没有成文稳定性政策" 是基于已读页面(diff.md、releases、stdlib 文档)未见, 不排除仓内别处另有说明。
- ng 的破坏性变更清单是从 PR 标题推断, 未逐个读 PR。
- 套接字结论基于 2026-11-12 两仓 master 的 quickjs-libc.c; 未检查未合并的 PR 与其它分支。
