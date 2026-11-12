# 参照: CPython 核心与标准库的分层、核心面与稳定性策略

用途: 为 cx 核心层(cx.sys/os/path/mem/text/stream)与扩展层(ext.*)的核心面和版本策略提供一份成熟运行时的对照。
调研日期 2026-11-12。本文只是参照材料,不是 cx 的规范;第 6 节是给 cx 的决策建议。

证据标记:

- `[核]` 本次调研已抓取原文核对(来源 URL 见文末对应编号)。
- `[忆]` 凭记忆写出,未在本次逐条核对;用于决策前应再查原文。

## 1 分层与模块结构

### 1.1 四层结构

| 层 | 内容 | 例子 | 稳定性承诺 |
|---|---|---|---|
| (1) 解释器内核 | 对象模型、求值循环、内存分配、导入机制 | `Objects/`、`Python/` | 不对外,随时可改 |
| (2) 内建模块(C,编入解释器) | 启动就必须在的模块,不依赖文件系统导入 | `sys`、`builtins`、`_io`、`posix` 或 `nt`、`_thread`、`errno`、`time`、`_signal`、`_codecs` | 带下划线的是私有;不带的按公共 API 对待 |
| (3) 标准库(Python 为主,带 C 加速模块) | 对内建模块的可移植封装与高层接口 | `os`、`io`、`pathlib`、`subprocess`、`socket`、`selectors`、`shutil`、`tempfile`、`threading` | PEP 387 全面覆盖 |
| (4) 第三方(PyPI) | 不随解释器发布 | `requests`、`numpy` | 各自为政 |

要点:

- 「内建」与「标准库」的界线是实现位置,不是稳定性等级。`sys.builtin_module_names` 列出编入解释器的模块 `[忆]`。稳定性只看「是否公共 API」(第 5 节),不看是 C 还是 Python 写的。
- 常见模式是「私有 C 模块 + 公共 Python 门面」:`_io` 对 `io`、`_socket` 对 `socket`、`_thread` 对 `threading`、`_signal` 对 `signal`、`_posixsubprocess` 对 `subprocess`、`_struct` 对 `struct`、`_codecs` 对 `codecs` `[忆]`。门面是承诺面,私有模块可以随版本重排。
- 加入一个顶层模块要走 PEP,子模块由核心团队自行裁量 `[核 S6]`。

### 1.2 posix/nt 与 os 的统一方式

- `posix`(Unix)与 `nt`(Windows)是两个平台专属的内建 C 模块,导出同名函数(`stat`、`open`、`read`、`listdir` 等)。`os.py` 在导入时按 `sys.builtin_module_names` 里有哪一个来决定 `from posix import *` 还是 `from nt import *`,并把 `os.path` 绑定到 `posixpath` 或 `ntpath`,把 `os.name` 设为 `'posix'` 或 `'nt'` `[忆]`。
- 文档明示的设计原则 `[核 S12]`:「只要功能相同,就用同一个接口」,例如 `os.stat(path)` 在各平台返回同一格式(格式源自 POSIX)。平台特有的扩展也经由 `os` 暴露,但「使用它们威胁可移植性」。
- 结论: `os` 不是「最小公分母」,而是「公共交集同名同义 + 平台专属项原样暴露并标注可用性」。用户永远只 import `os`,不直接 import `posix`/`nt`。

### 1.3 各区域的层次关系

| 区域 | 低层(贴系统调用) | 中层(可移植语义) | 高层(便利) |
|---|---|---|---|
| 文件与目录 | `os.open/read/write/stat/scandir` | `io.open`、`os.path` | `pathlib.Path`、`shutil`、`tempfile` |
| 进程 | `os.fork/execve/waitpid/posix_spawn`、`_winapi.CreateProcess` | `subprocess.Popen` | `subprocess.run` |
| 网络 | `_socket`(BSD socket 直通) | `socket.socket` | `socket.create_connection`、`socketserver`、`asyncio` |
| 多路复用 | `select.select/poll/epoll/kqueue` | `selectors.DefaultSelector` | `asyncio` 事件循环 |
| 时间 | `time.time_ns/monotonic_ns/sleep` | `time.struct_time` | `datetime`、`zoneinfo` |
| 文本与编码 | `_codecs`、`bytes`/`str` 内建 | `codecs`、`io.TextIOWrapper` | `locale`、`unicodedata` |
| 并发 | `_thread` | `threading` | `concurrent.futures`、`queue` |

规律: 每个区域都保留低层直通入口,高层只是组合,不隐藏低层。高层晚于低层出现(`pathlib` 3.4、`selectors` 3.4、`subprocess.run` 3.5 `[忆]`),低层接口基本没有因此被废弃。

### 1.4 C API 的头文件三层

`[核 S10]`

| 目录 | 层级 | 谁能用 | 规则 |
|---|---|---|---|
| `Include/`(不含子目录) | Limited API | 所有扩展;定义 `Py_LIMITED_API` 后只见这一层 | 进 Stable ABI,「不允许破坏 Stable ABI 的变更」 |
| `Include/cpython/` | 一般公共 API 与 Unstable API(`PyUnstable_` 前缀) | 为特定小版本编译的扩展 | 公共部分走 PEP 387;Unstable 部分小版本间可变 |
| `Include/internal/` | 内部 API | 只能用于构建 CPython 自身 | 随时可改;外部用例应去讨论区提出,找一条受支持的路 |

前导下划线的名字一律视为内部 `[核 S10]`。

## 2 核心面清单

以下均为 `[忆]`(函数名取自长期稳定的文档面,数量与细节未逐条核对)。

### 2.1 sys(解释器自身状态)

- 参数与流: `argv`、`stdin/stdout/stderr`、`exit`。
- 平台识别: `platform`、`byteorder`、`maxsize`、`version_info`、`implementation`、`builtin_module_names`。
- 路径与导入: `path`、`modules`、`executable`、`prefix`。
- 编码: `getfilesystemencoding`、`getdefaultencoding`。
- 诊断: `exc_info`、`getrefcount`、`getsizeof`、`settrace`、`audit`。

### 2.2 os(系统调用门面)

- 文件描述符: `open/close/read/write/lseek/dup/dup2/pipe/fsync/ftruncate`、`set_inheritable`、`set_blocking`。
- 文件系统: `stat/lstat/fstat`、`listdir/scandir`、`mkdir/makedirs/rmdir/remove/unlink/rename/replace`、`link/symlink/readlink`、`chmod/chown/utime`、`walk`、`access`。
- 进程: `getpid/getppid`、`fork/execve/_exit/waitpid/kill`、`posix_spawn`、`system`、`environ/getenv/putenv`、`getcwd/chdir`、`umask`。
- 常量: `sep/altsep/pathsep/linesep/devnull/curdir/pardir`、`O_*`、`name`。
- 类型: `stat_result`、`DirEntry`、`PathLike`、`terminal_size`。
- 杂项: `urandom`、`cpu_count`、`strerror`、`fspath/fsencode/fsdecode`。

### 2.3 os.path 与 pathlib(路径)

- `os.path` 是纯字符串函数: `join/split/splitext/basename/dirname/normpath/abspath/realpath/relpath/isabs/commonpath/expanduser`,外加少量触盘查询 `exists/isfile/isdir/islink/getsize/getmtime`。
- `pathlib` 把「纯词法」和「触盘」分成两个类族: `PurePath`(`PurePosixPath`、`PureWindowsPath`,任何平台都能实例化,只做词法运算)与 `Path`(`PosixPath`、`WindowsPath`,带 `open/read_text/iterdir/glob/stat/mkdir/unlink/resolve`)。
- 两者经 `os.PathLike`/`__fspath__` 协议互通: 任何接收路径的函数都对入参调用 `os.fspath()` `[核 S12]`。

### 2.4 io(流)

- 三层类族: `RawIOBase`(`FileIO`,无缓冲,一次调用一次系统调用)-> `BufferedIOBase`(`BufferedReader/Writer/Random`、`BytesIO`)-> `TextIOBase`(`TextIOWrapper`、`StringIO`,负责编码与换行)。
- 统一协议: `read/readinto/write/seek/tell/flush/close`、`readable/writable/seekable`、`fileno`、迭代取行、上下文管理。
- `open()` 按 mode 与 buffering 参数组装这三层;内存流(`BytesIO/StringIO`)与文件流同接口。

### 2.5 subprocess、signal、threading

- `subprocess`: `Popen`(`stdin/stdout/stderr`、`communicate/wait/poll/kill/terminate`、`returncode`)、`run` 返回 `CompletedProcess`、常量 `PIPE/DEVNULL/STDOUT`、异常 `CalledProcessError/TimeoutExpired`。
- `signal`: `signal/getsignal/raise_signal/pthread_kill/set_wakeup_fd`、`SIG*` 枚举;处理函数只在主线程的字节码间隙执行。
- `threading`: `Thread`、`Lock/RLock/Condition/Event/Semaphore/Barrier`、`local`、`current_thread`。

### 2.6 socket、select、selectors

- `socket`: `socket` 对象(`bind/listen/accept/connect/send/sendall/recv/recv_into/shutdown/close/setsockopt/settimeout/setblocking/fileno`)、`getaddrinfo`、`create_connection`、`socketpair`、常量 `AF_*/SOCK_*/SOL_*`。
- `select`: 平台有什么导出什么(`select` 全平台;`poll`、`epoll`、`kqueue` 按平台存在)。
- `selectors`: `DefaultSelector` 自动选最优实现,接口只有 `register/unregister/modify/select/close`。这是「低层按平台裸露 + 中层统一抽象」的典型。

### 2.7 time、errno、struct、codecs、shutil、tempfile

- `time`: `time/time_ns`、`monotonic/monotonic_ns`、`perf_counter`、`process_time`、`sleep`、`gmtime/localtime/strftime`、`get_clock_info`。
- `errno`: `E*` 整数常量与 `errorcode` 反查表;只有平台存在的常量才定义。
- `struct`: `pack/unpack/pack_into/unpack_from/calcsize/iter_unpack`、`Struct` 预编译对象;格式串首字符显式选字节序与对齐(`@ = < > !`)。
- `codecs`: `encode/decode/lookup/register`、`CodecInfo`、增量编解码器、错误处理器注册(`strict/ignore/replace/surrogateescape`)。
- `shutil`: `copyfile/copy/copy2/copytree/rmtree/move/which/disk_usage/make_archive`。
- `tempfile`: `TemporaryFile/NamedTemporaryFile/TemporaryDirectory/mkstemp/mkdtemp/gettempdir`。

## 3 错误与资源模型

### 3.1 OSError 与 errno 子类

- PEP 3151(Python 3.3)把 `IOError`、`EnvironmentError`、`WindowsError`、`socket.error`、`select.error`、`mmap.error` 全部并入 `OSError`,旧名留作别名 `[核 S7]`。动机: 同一种失败在不同模块抛不同异常(`os.remove` 抛 `OSError`,`open` 抛 `IOError`),而用户关心的是「出了什么错」,不是「哪个模块报的」。
- 同时按 errno 细分子类 `[核 S7]`:

| 子类 | errno |
|---|---|
| `FileNotFoundError` | ENOENT |
| `FileExistsError` | EEXIST |
| `IsADirectoryError` / `NotADirectoryError` | EISDIR / ENOTDIR |
| `PermissionError` | EACCES、EPERM |
| `BlockingIOError` | EAGAIN、EALREADY、EWOULDBLOCK、EINPROGRESS |
| `BrokenPipeError` | EPIPE、ESHUTDOWN |
| `InterruptedError` | EINTR |
| `ConnectionError` 下的 Aborted / Refused / Reset | ECONNABORTED / ECONNREFUSED / ECONNRESET |
| `TimeoutError` | ETIMEDOUT |
| `ChildProcessError` / `ProcessLookupError` | ECHILD / ESRCH |

- `OSError` 实例带 `errno`、`strerror`、`filename`、`filename2`,Windows 上另带 `winerror`,并把 Windows 错误码映射成近似的 errno `[忆]`。
- `os` 文档的总则 `[核 S12]`: 文件名或路径无效、不可访问,或参数类型对但系统不接受时,模块内所有函数都抛 `OSError` 或其子类。
- 设计取舍: 子类只覆盖「调用者常常想单独处理」的十几种;其余几十上百个 errno 仍靠 `e.errno` 判别。分类是按处理方式分,不是把 errno 一一映射。

### 3.2 EINTR 与默认安全

- PEP 475(3.5): 系统调用被信号打断返回 EINTR 时,标准库自动重试(信号处理函数抛异常则不重试),调用者不再需要自己写重试循环 `[忆]`。
- PEP 446(3.4): 新建的文件描述符默认不可继承(close-on-exec),要继承必须显式 `os.set_inheritable` 或 `subprocess` 的 `pass_fds` `[忆]`。
- 两者都是「把常见的低层陷阱收在核心层,默认值选安全的一侧」。

### 3.3 资源处理

- 上下文管理协议(PEP 343): `with` 语句调用 `__enter__`/`__exit__`,保证离开块时释放 `[忆]`。文件、socket、锁、`Popen`、`TemporaryDirectory`、`scandir` 迭代器、selector 都实现它。
- 兜底: 未显式关闭的资源在析构时发 `ResourceWarning`(默认不显示,测试模式下显示)`[忆]`。析构回收只是兜底,不是合同。
- `close()` 幂等;关闭后再操作抛 `ValueError`,而不是把已关闭的描述符号传给系统 `[忆]`。
- `contextlib.ExitStack` 用来管理数量在运行时才确定的一组资源 `[忆]`。
- 没有编译期所有权: 正确性靠约定(`with`)加运行期检查(已关闭标志)加警告。

## 4 可移植性处理

- **按平台选实现,按名字统一接口**: `posix`/`nt` 同名导出,`os` 统一;`posixpath`/`ntpath` 同接口,`os.path` 选一。`pathlib` 的 `PurePosixPath`/`PureWindowsPath` 可以在任一平台上对另一平台的路径做词法运算。
- **平台专属项不隐藏,但标注**: 文档里几乎每个函数带 Availability 标记(如「Unix, not WASI」「Unix, Windows」)`[核 S12]`。平台没有的函数在模块里直接不存在,检测方式是 `hasattr(os, 'fork')` 而不是调了再失败 `[忆]`。在 WebAssembly、Android、iOS 上 `os` 的大部分不可用或行为不同 `[核 S12]`。
- **识别粒度分两级**: `os.name`(`posix`/`nt`/`java`)粗,`sys.platform` 细 `[核 S12]`。
- **路径与字节**: 接收路径的函数同时接受 `str` 与 `bytes`,返回同类型 `[核 S12]`。Unix 上不可解码的文件名用 `surrogateescape` 无损往返(PEP 383);Windows 上文件系统编码统一为 UTF-8(PEP 529)`[忆]`。
- **时钟**: PEP 418 增加 `monotonic`/`perf_counter`/`process_time` 并提供 `get_clock_info` 报告底层实现与分辨率;PEP 564 增加 `*_ns` 整数纳秒版本,避免浮点丢精度 `[忆]`。
- **平台支持分级(PEP 11)** `[核 S8]`:

| 级 | 要求 | 失败后果 | 例子 |
|---|---|---|---|
| Tier 1 | CI 必过,破坏 main 的改动不得合并;全体核心开发者共同负责 | 阻塞发布 | `x86_64-unknown-linux-gnu`、`x86_64-pc-windows-msvc`、`aarch64-apple-darwin` |
| Tier 2 | 可靠 buildbot,至少两名核心开发者认领;破坏须 24 小时内修复或回退 | 阻塞发布 | `aarch64-pc-windows-msvc`、`x86_64-apple-darwin` |
| Tier 3 | 可靠 buildbot,至少一名核心开发者认领;无响应时限 | 不阻塞发布 | `aarch64-linux-android`、`arm64-apple-ios`、`wasm32-unknown-emscripten`、`x86_64-unknown-freebsd` |

  晋级需要指导委员会支持。降级流程: 到新特性版 b1 仍不满足任何一级要求,就在 b1 公告里预警;到 rc1 仍未恢复,列为不支持,并在 PEP 里记下「首个不支持的版本」与「首个删除历史代码的版本」`[核 S8]`。

- **ABI 的「平台」定义**: Stable ABI 的稳定性还取决于编译器、底层库和编译选项,这些合起来定义「平台」;由各发行方负责保证同一平台上各版本的构建方式不破坏 ABI `[核 S9]`。

## 5 稳定性与版本策略

### 5.1 什么算公共 API(PEP 387)

`[核 S1]`

算公共、受保护的:

- 语言参考里定义的语法与行为;
- C API;
- 函数、类、模块、属性、方法的名字与类型;
- 给定参数下的返回值、副作用与抛出的异常(合理的缺陷修复不受此限);
- 参数的位置与期望类型;
- 类对子类的行为(包括被覆盖的方法何时被调用);
- 文档写明的异常及其触发语义。

不算公共、「可以在任何时候以任何方式改变或删除」的:

- 以 `_` 开头的名字(特殊名除外),C API 同理;
- 文档明确写为私有的内容。注意反过来不成立: 没写进文档不等于私有;
- 被 import 进来的模块(除非文档写明是公共的);
- 内部类的继承关系;
- 测试套件(`Lib/test`);
- 按 PEP 411 标为 Provisional 的 API。

### 5.2 不兼容变更的判据与流程(PEP 387)

`[核 S1]`

判据:

- 不兼容变更必须「收益相对破坏面足够大」,且受影响的代码要容易改。
- 任意两个相邻版本之间,API 行为不得不兼容地改变,除非走完弃用流程;功能不得不经通知就删除。

流程:

- (1) 讨论(论坛、问题跟踪器或工作组);有共识后可写 PEP。
- (2) 在主干加警告。行为要变时,新行为可以先挂在新函数或新方法上。警告信息应写明变更生效的版本,并链接到收集反馈的 issue。
- (3) 等待: 警告至少出现在同一主版本的两个小版本里。优选等 5 年再删。
- (4) 收集看到警告的用户的反馈,必要时重新考虑。
- (5) 把变更设为默认或永久,删除旧代码与警告。
- (6) 无法给出警告的变更,必须先咨询指导委员会。

细则:

- 最短期限按年度节奏折算约两年(两个小版本);PEP 602 沿用「弃用到删除至少隔两个版本」`[核 S3]`。早期的 PEP 5 只要求一年过渡期,已被 PEP 387 取代 `[核 S5]`。
- 警告类别: 通常用 `DeprecationWarning`;新旧并存很多个版本的特殊情况用 `PendingDeprecationWarning`;C API 可以用 `Py_DEPRECATED` 宏产生编译器警告。
- 保留成本低的旧接口(例如已在新接口之上重新实现)可以无限期保留。被替代的功能一般等到「最后一个没有新功能的版本」停止支持后才删。
- **软弃用**: 不应再用于新代码、但留在现有代码里是安全的。只在文档里说明(最好写明原因与替代),不发运行期警告,不排删除日期,仍有文档有测试,只是不再发展。软弃用转硬弃用要重新走完整流程。
- **例外**: 只有指导委员会能批准例外或缩短弃用期,且「仅限极端情况,例如危险的缺陷或不安全的功能」;或者没人能合理依赖的东西(如对过时平台的支持)。

### 5.3 角色

- **指导委员会** `[核 S11]`: 5 人,每个特性版发布后改选一次,无任期限制;同一雇主至多 2 人。有权接受或否决 PEP,可以转授权力。表决要求非弃权成员的严格多数;章程要求先求共识、尽量少动用权力。
- **核心团队** `[核 S11]`: 新成员须核心团队投票三分之二以上赞成;修改治理文件本身也需要三分之二多数。
- **发布经理** `[核 S3]`: 一人通常负责两个特性版;在 beta 与 rc 阶段裁决什么能进。
- **模块维护者**: 标准库里的代码由核心团队维护,每次加入都是一笔「永久成本」`[核 S6]`。
- **C API 工作组**: 负责 C API 的演进评审(PEP 731)`[忆]`。

### 5.4 发布节奏与支持期(PEP 602)

`[核 S3]`

- 每 12 个月一个特性版,10 月发布。一个特性版的开发周期约 17 个月: 前 5 个月与上一版的 beta/rc 重叠;随后 7 个月出 alpha(可加功能);随后 3 个月 4 个 beta(只修缺陷);最后 2 个月 rc。
- **特性冻结点是 beta 1**,此后不再加新功能。
- 支持期共 5 年: 完整支持(缺陷修复加二进制安装包,约每两个月一个补丁版),之后只做安全修复(只发源码)直到满 5 年。3.13 起完整支持 2 年;3.9 到 3.12 约 1.5 年。
- 补丁版(3.x.y)只修缺陷,不加功能,不改 ABI。
- 版本号仍是 3.x 递增;改为日历版本号的 PEP 2026 已被否决 `[核 S14]`。

### 5.5 C API 的分级稳定(PEP 384、PEP 689)

| 层 | 标识 | 变更规则 |
|---|---|---|
| Stable ABI | 定义 `Py_LIMITED_API` 为所支持的最低版本(如 `0x030a0000`);产物带 `abi3` 标签 | 「为 Stable ABI 3.x 编译的扩展,与 3.x 及以上版本 ABI 兼容」`[核 S9]`。ABI 条目一经加入不得破坏 `[核 S10]` |
| 一般公共 C API | `Py` 前缀,无下划线 | 走 PEP 387;同一小版本内二进制兼容(3.10.0 编出的可用于 3.10.8,但 3.9 与 3.11 要另编)`[核 S9]` |
| Unstable API | `PyUnstable_` 前缀 | 小版本间可以不经弃用就改,补丁版内不变;不兼容改动应当让旧调用方编译失败,不得悄悄改变参数含义 `[核 S10][核 S13]` |
| 内部 | `_Py` 或前导下划线,`Include/internal/` | 随时可改,补丁版里也可以改 `[核 S9]` |

关键做法:

- **清单文件驱动**: Stable ABI 的全部条目登记在 `Misc/stable_abi.toml`,生成物用 `make regen-limited-abi` 产生,`make check-limited-abi` 校验。加入条件: 所有参数、返回值、结构成员的类型本身必须已在 Limited API 内;不收 `FILE*` 相关函数;尽量不用宏;扩大 Limited API 之前先公开讨论 `[核 S10]`。
- **稳定换性能**: Limited API 的目标是「完整 C API 能做的都能做,但可能有性能代价」,例如提供函数 `PyList_GetItem()` 而不提供宏 `PyList_GET_ITEM()`,因为内联会把结构布局烧进调用方 `[核 S9]`。
- **ABI 稳定不等于行为稳定**: Stable ABI 只防 ABI 问题;行为仍可能随版本变化,官方建议对所支持的每个小版本都测试 `[核 S9]`。
- **API 与 ABI 可以分离**: 用 `Py_LIMITED_API` 3.8 编译的扩展应能在 3.12 加载,但同一份源码未必能以 3.12 为目标编译;Limited API 的一部分可以被弃用和删除,只要 Stable ABI 保持不变 `[核 S9]`。
- **把不稳定写进名字**: PEP 689 的动机是下划线原来兼表「小版本间可能变」和「私有」两种含义;分出 `PyUnstable_` 后,下划线只表示私有,而不看文档的人也能从名字看出不稳定 `[核 S13]`。
- **新 ABI 变体**: 3.15 增加面向自由线程构建的 `abi3t`,限制比 `abi3` 更严;同时增加装载期的 ABI 自检(`PyABIInfo_Check`)`[核 S9]`。

### 5.6 Provisional 状态(PEP 411)

`[核 S2]`

- 定义: 被有意排除在标准库兼容性承诺之外的包。允许按反馈改 API,极端情况下可以不经弃用期直接移除。
- 标记义务: 文档页必须写明「以 provisional 方式纳入……可能发生不兼容变更(直至移除该包)」;docstring 开头也要写。毕业就是删掉这些标记。
- 期限: 原定一个特性版;之后毕业、继续 provisional 或移除,由核心开发者逐案决定。
- 历史用例: `pathlib`(3.4 到 3.5)、`asyncio`(3.4 到 3.5)、`typing`(3.5 到 3.6)`[忆]`。
- **现状: 该 PEP 状态为 Superseded**。PEP 页面上的后记写道: 十年经验表明这是很少被用到的机制,而且它没能阻止用户重度依赖 provisional 模块,因此真要改动时仍然造成显著破坏。教训是: 「声明不稳定」挡不住依赖;东西一旦随主发行版发出,实际承诺就已形成。

### 5.7 模块的进入与退出

进入 `[核 S6][核 S15]`:

- 顶层模块要走 PEP。开发者指南列的条件: 解决的是难以做对或被反复重造的问题,且对社区的大部分人有用;「至少已在社区流通一年」;公认同类最佳;开发迁入 Python 的基础设施;有人承诺维护两年;满足代码与测试要求。
- 指南直言「新模块进入标准库非常困难」,理由是进入后 API 即冻结,所以要等它成熟、度过成长期之后再进。
- 给已有模块加函数门槛低得多: 先探社区意向,再开 issue 带实现,由一名核心成员决定。

退出 `[核 S4][核 S16]`:

- 删除顶层模块要走 PEP(PEP 4),弃用流程按 PEP 387。
- PEP 594「移除死电池」: 3.11 起发 `DeprecationWarning`,3.12 是最后一个带这些模块的版本,3.13 移除。删掉的有 `aifc`、`audioop`、`cgi`、`cgitb`、`chunk`、`crypt`、`imghdr`、`mailcap`、`msilib`、`nis`、`nntplib`、`ossaudiodev`、`pipes`、`sndhdr`、`spwd`、`sunau`、`telnetlib`、`uu`、`xdrlib`;`asyncore`、`asynchat`、`smtpd` 在 3.12 移除。
- 入选判据: 多为历史数据格式、早被取代的 API 或操作系统接口、有安全问题且已有更好替代的模块;优先删「争议最小或收益最大」的。
- 理由三条: 维护占用核心开发者时间;用户默认信任标准库,留着劣质模块反而妨碍更好的社区替代品被采用;更小的标准库有利于存储受限的平台(微控制器、WebAssembly、移动端)。
- 留下的也有判据: `getopt`、`optparse`(用得广、成熟、维护成本很低)、`wave`(格式仍通用)、`socketserver`(`http.server` 的底座)。即「长期软弃用但不删」是合法终态。

### 5.8 「自带电池」与「最小核心」的张力

- 「自带电池」的收益: 装上就能用,不必先选第三方;接口跨版本稳定。
- 代价: 标准库跟着解释器每年一版,修缺陷要等发行版,功能迭代比 PyPI 慢得多;API 一进即冻;维护负担落在核心团队身上。
- 近十年的走向 `[忆]`: 核心收敛(PEP 594 删模块;`distutils` 在 3.12 移除),新功能优先放 PyPI,只有「生态的公共地基」才进标准库(`tomllib` 3.11、`zoneinfo` 3.9 之类)。几乎每个进入标准库的模块都先在 PyPI 上有一段独立生命。
- 现行的事实判据: 进核心的应当是「别人要在它上面搭东西、且需求已经不怎么变」的接口;变化快或受众窄的留在外面。

## 6 对 cx 的启示

每条写决策(采纳/改造/不采纳)与原因。cx 现状依据: `build/tlib-policy.tsv`(预稳定,`stable no`)、`build/tlib-packages.tsv`(X.Y.Z 加 api_sha)、`build/layer-api/*.tsv`。

- (1) **采纳: 公共面由清单文件定义并由工具校验**。对应 `Misc/stable_abi.toml` 加 `check-limited-abi`。cx 已有 `build/layer-api/PKG.tsv` 加 api_sha,方向一致;应把它明确为「公共面的唯一定义」: 不在表里的符号就不是承诺,表的差异决定 bump 类别。原因: PEP 387 的「没写进文档不等于私有」造成长期歧义,cx 用表可以从一开始就避开。
- (2) **采纳: 稳定性等级写进名字或包名,不靠文档**。对应 `PyUnstable_` 与前导下划线的分工(PEP 689)。cx 建议三级: 核心稳定面(`cx.*` 中标 stable 的包)、不稳定面(名字或包名带显式标记)、私有(模块私有状态,不出接口)。原因: PEP 689 的经验是使用者不读文档,只看名字。
- (3) **改造: provisional 不作为随核心发出的状态,改为「放在 ext.* 里孵化,成熟后提升进 cx.*」**。原因: PEP 411 已被取代,官方后记承认声明 provisional 挡不住依赖,变更照样伤人。cx 的 ext 层天然就是孵化地;提升条件仿开发者指南: 在 ext 里至少被若干真实消费者用过一个周期、接口在该周期内没有破坏性变更。
- (4) **采纳: 「低层贴系统调用 + 中层可移植 + 高层便利」三层,且高层不隐藏低层**。对应 `os` -> `io` -> `pathlib/shutil`。cx 对应 `cx.sys`(L0 `sy_*`)-> `cx.stream`/`cx.path`/`cx.text` -> `ext.*`。原因: CPython 的高层(pathlib、selectors、subprocess.run)都是后加的,低层接口没有因此作废;分层正确时加高层不是破坏性变更。
- (5) **采纳: 平台差异收在最底一层,上层同名同义**。对应 `posix`/`nt` 同名导出、`os` 统一。这与 cx 现行规则(平台差异只放 L0 的 `_os` 静态帮手或表里,上层不写平台 `#if`)一致,CPython 是佐证。补充一条: 平台专属能力允许经由核心层暴露,但必须在接口表里带「可用目标」列(对应文档的 Availability 标记),且缺失时在编译期或装载期拒绝,不是调了再失败。
- (6) **改造: 错误模型取 PEP 3151 的分类思路,不取异常机制**。cx 没有异常,应在 L0 定一个统一错误码类型,并提供按「处理方式」归并的谓词或类别(不存在、已存在、权限、会阻塞、被中断、对端断开、超时等十余类),其余保留原始码。原因: PEP 3151 的核心教训有两条,一是同一种失败不应因模块不同而有不同错误类型,二是分类应按调用者如何处理来分,不是 errno 的一一映射。同时沿用 cx 现行规则: 只把 ENOENT 当不存在,ENOTDIR/EACCES 必须报告。
- (7) **采纳: 常见低层陷阱在核心层用安全默认值收掉**。对应 PEP 475(EINTR 自动重试)与 PEP 446(描述符默认不继承)。cx 的 `cx.sys` 应在接口合同里写明这两条的取舍,不留给每个调用者各写一遍。
- (8) **改造: 资源模型用 cx 的所有权与运行期句柄检查,不照搬「约定加警告」**。CPython 靠 `with` 约定、已关闭标志和 `ResourceWarning`,没有编译期保证。可借鉴的只有三点: `close` 幂等;关闭后使用是确定的错误而不是把旧句柄号传给系统;泄漏在测试模式下可见。cx 已把所有权和生命周期列为设计要求,应做得更强,并如实标明哪些性质只是运行期检查。
- (9) **采纳: 破坏性变更的固定流程与唯一例外通道**。取 PEP 387 的骨架: 先提供替代 -> 标弃用(编译期警告,对应 `Py_DEPRECATED`)-> 至少隔 N 个版本 -> 删除;例外只在「危险或不安全」时由单一角色(cx 里是主人或其授权的集成者)批准并在 `agentwork/ledger.tsv` 留痕。N 的取值: 预稳定期(现状 `stable no`)不设弃用期,这与现行 tlib 策略一致;某个包翻到 `stable yes` 之后,X 级变更必须先经过至少一个带弃用标记的 Y 版本。
- (10) **采纳: 软弃用作为合法终态**。对应 PEP 387 的软弃用与 PEP 594 保留 `getopt`/`optparse` 的理由。cx 里维护成本低、已在新接口之上重写的旧入口可以长期保留,只在接口表里标 soft-deprecated,不发警告,不排删除日期。原因: 删除本身有成本,只有维护负担或安全问题足够大时才值得。
- (11) **不采纳: 年度固定节奏与 5 年支持期**。原因: 那是为庞大的下游生态设计的;cx 目前消费者就是仓内自身,且有「从冻结到可发布超过 2 小时即视为设计需要改」的迭代目标。可取的只有两个概念: 特性冻结点(对应 beta 1,此后只修不加)与「补丁版不改接口」(对应 cx 的 Z 级只是实现发布点)。
- (12) **采纳: 核心面取「最小核心」,判据写成可检查的条目**。取 PEP 594 之后的事实判据: 进 `cx.*` 的必须是别的包要在其上搭建、且需求基本不再变化的接口;变化快或受众窄的留在 `ext.*`。同时预留退出路径(核心包降回 ext 或删除,走第 (9) 条流程)。原因: CPython 花了约二十年才承认标准库里的每个模块都是永久成本,并为此专门做了一轮清理;cx 应当从第一天起就让进入比退出难。

## 来源

本次已抓取核对的原文:

- S1 PEP 387 Backwards Compatibility Policy: https://peps.python.org/pep-0387/
- S2 PEP 411 Provisional packages in the Python standard library(状态 Superseded): https://peps.python.org/pep-0411/
- S3 PEP 602 Annual Release Cycle for Python: https://peps.python.org/pep-0602/
- S4 PEP 594 Removing dead batteries from the standard library: https://peps.python.org/pep-0594/
- S5 PEP 5 Guidelines for Language Evolution(状态 Superseded,由 PEP 387 取代): https://peps.python.org/pep-0005/
- S6 PEP 2 Procedure for Adding New Modules: https://peps.python.org/pep-0002/
- S7 PEP 3151 Reworking the OS and IO exception hierarchy: https://peps.python.org/pep-3151/
- S8 PEP 11 CPython platform support: https://peps.python.org/pep-0011/
- S9 C API and ABI Stability: https://docs.python.org/3/c-api/stable.html
- S10 开发者指南 Changing Python's C API: https://devguide.python.org/developer-workflow/c-api/
- S11 PEP 13 Python Language Governance: https://peps.python.org/pep-0013/
- S12 os 模块文档(只核对了页首总则、`os.name`、`fspath`/`PathLike`): https://docs.python.org/3/library/os.html
- S13 PEP 689 Unstable C API tier: https://peps.python.org/pep-0689/
- S14 PEP 2026 Calendar versioning for Python(状态 Rejected): https://peps.python.org/pep-2026/
- S15 开发者指南 Adding to the stdlib: https://devguide.python.org/developer-workflow/stdlib/
- S16 PEP 4 Deprecation of Standard Modules: https://peps.python.org/pep-0004/

未核对、仅凭记忆引用的(用前应查原文): PEP 384(Stable ABI 的原始定义;本文关于它的陈述均取自 S9 与 S10)、PEP 343、PEP 383、PEP 418、PEP 446、PEP 475、PEP 519、PEP 529、PEP 564、PEP 652、PEP 731;第 2 节的全部函数清单;`os.py` 选择 `posix`/`nt` 的具体实现;provisional 历史用例的版本号;5.8 节「近十年的走向」。

已知缺口:

- os 文档只读了前 10 万字符,`os.sep` 等常量条目和 `posix`/`nt` 关系的原文未核对。
- C API 稳定性文档(S9)末尾约 2.7 万字符未读。
- PEP 602 正文内部对完整支持期有 18 个月与 24 个月两种说法,本文取其实施节的「3.13 起 2 年」。
- PEP 8 与 PEP 7 是代码风格规范,与本题无关,未调研。
