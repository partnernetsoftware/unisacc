# 参考调研: Node.js 核心面与稳定性策略

用途: 为 cx 核心层(cx.sys/os/path/mem/text/stream)与扩展(ext.*)的表面设计、稳定性与版本策略提供成熟运行时参照。
调研日期: 2026-11-12。

证据标注:

- `[核]` 本次会话实际抓取了所列 URL 并据其内容撰写。
- `[忆]` 凭既有知识写出,本次未抓取对应页面;用作设计参照可以,引用为事实前须再核。

本次抓取的页面(全部 `[核]` 结论均出自这些页面):

- https://nodejs.org/api/documentation.html (稳定性指数、稳定性总览)
- https://nodejs.org/api/errors.html (错误分类、SystemError、常见系统错误)
- https://nodejs.org/api/deprecations.html (弃用种类与编号)
- https://nodejs.org/api/modules.html (内置模块与 node: 前缀)
- https://nodejs.org/api/path.html (path 全部导出)
- https://nodejs.org/api/n-api.html (Node-API,只读到前 10 万字符)
- https://github.com/nodejs/node/blob/main/doc/contributing/collaborator-guide.md (落地、破坏性变更、弃用、新模块)
- https://github.com/nodejs/node/blob/main/doc/contributing/backporting-to-release-lines.md (回移)
- https://github.com/nodejs/Release/blob/main/README.md (发布线与日程)
- https://github.com/nodejs/node/blob/main/BUILDING.md (平台分级)
- https://docs.libuv.org/en/v1.x/design.html (libuv 设计)

## 1 分层与模块结构

### 1.1 分层

自下而上四层 `[忆]`,其中 libuv 部分 `[核]`:

| 层 | 内容 | 稳定性承诺 |
| --- | --- | --- |
| 平台层 libuv | 事件循环、句柄/请求、线程池、跨平台系统调用封装 | libuv 自身按 semver;对 Node 插件不承诺跨大版本 ABI |
| 引擎与依赖 | V8、OpenSSL、zlib、c-ares、llhttp、ada、simdutf 等,置于 `deps/` | 随 Node 大版本升级,无跨大版本 ABI |
| C++ 绑定层 | `src/` 下的内部绑定,只对 `lib/` 开放 | 内部,无承诺 |
| JS 核心库 | `lib/` 下的内置模块;`lib/internal/` 为私有 | 公开模块按稳定性指数与 semver |
| 原生扩展接口 | Node-API(C)与 node-addon-api(C++ 头文件封装) | 唯一承诺跨大版本 ABI 稳定的原生接口 |

libuv 设计要点 `[核]`:

- 两类抽象:句柄(handle,长寿对象,如 TCP 服务端,用 `uv_close()` 关闭)与请求(request,短寿操作,完成或取消后自动清理)。
- 轮询后端按平台分: Linux 用 epoll,macOS 与 BSD 用 kqueue,SunOS 用 event ports,Windows 用 IOCP;文档强调在 Unix 与 Windows 之间保持同一执行模型。
- 网络 I/O 用非阻塞套接字,总在循环所在的单线程上执行。
- 文件 I/O 没有可依赖的平台级异步原语,因此阻塞式文件操作放到全局线程池;线程池同时承担 DNS(getaddrinfo/getnameinfo)与 `uv_queue_work()` 用户任务。
- 一轮循环的阶段顺序:更新时间 -> 到期定时器 -> 挂起回调 -> idle -> prepare -> 计算轮询超时 -> 阻塞轮询 I/O -> check -> close 回调 -> 再次更新时间与定时器。
- 线程池默认 4 线程、由 `UV_THREADPOOL_SIZE` 调整 `[忆]`(设计页只说池子"相当有限",未给数字)。

### 1.2 模块命名与 node: 前缀

`[核]` 出自 modules.html 与 collaborator-guide:

- 内置模块编译进二进制,源在 `lib/`;`require('http')` 总是优先得到内置模块,即使存在同名文件。
- `node:` 前缀明确指向内置模块,并绕过 `require.cache`;前缀自 v16.0.0 与 v14.18.0 起可用。
- 部分新模块只能带前缀加载,当前名单: `node:ffi`、`node:sea`、`node:sqlite`、`node:test`、`node:test/reporters`。文档给出的理由:防止新内置模块与用户态已占用的包名冲突。
- 规则:新模块只允许以 `node:` 前缀加入,此时算 semver-minor;若要不带前缀可用,则须放在运行期开关之后或作为 semver-major。
- 名称未被占用时尽早在 npm 注册占位包并在 README 链接引入它的 PR,防混淆与抢注;名称已占用且使用不广时联系持有者协商转让。
- `module.builtinModules` 给出全部内置模块名单,必须带前缀的那些以带前缀形式列出。
- 子路径模块存在先例: `node:path/posix`、`node:path/win32`(v15.3.0 起)、`node:fs/promises`、`node:test/reporters`。

## 2 核心面清单

稳定性等级一列 `[核]`(出自 documentation.html 的稳定性总览);代表性函数与类型除 path 外均为 `[忆]`。

| 区域 | 等级 | 代表性函数/类型 |
| --- | --- | --- |
| fs | 2 稳定 | 三套并存: 回调式 `fs.readFile`、同步式 `fs.readFileSync`、Promise 式 `fs/promises`;`open/read/write/close/fsync`、`stat/lstat/fstat`、`readdir/opendir`(Dir/Dirent)、`mkdir({recursive})`、`rm({recursive,force})`、`rename`、`copyFile`、`cp`、`link/symlink/readlink/realpath`、`chmod/chown/utimes`、`truncate`、`mkdtemp`、`watch/watchFile`、`createReadStream/createWriteStream`、`FileHandle`、`Stats`、`fs.constants`(O_*、S_IF*、COPYFILE_*) |
| path | 2 稳定 | `[核]` 全部导出: `basename`、`dirname`、`extname`、`format`、`parse`、`isAbsolute`、`join`、`normalize`、`relative`、`resolve`、`matchesGlob`、`toNamespacedPath`、`sep`、`delimiter`、`posix`、`win32` |
| os | 2 稳定 | `platform()`、`arch()`、`type()`、`release()`、`version()`、`machine()`、`hostname()`、`homedir()`、`tmpdir()`、`cpus()`、`availableParallelism()`、`totalmem()/freemem()`、`uptime()`、`loadavg()`、`networkInterfaces()`、`userInfo()`、`endianness()`、`EOL`、`devNull`、`constants.signals/errno` |
| process | 2 稳定(全局对象) | `argv`、`execPath`、`execArgv`、`env`、`cwd()/chdir()`、`exit()/exitCode`、`pid/ppid`、`platform/arch`、`version/versions/release`、`stdin/stdout/stderr`、`hrtime.bigint()`、`memoryUsage()`、`cpuUsage()`、`resourceUsage()`、`kill()`、`umask()`、`nextTick()`、`emitWarning()`、事件 `exit`/`beforeExit`/`uncaughtException`/`unhandledRejection`/`warning`/信号 |
| child_process | 2 稳定 | `spawn`、`exec`、`execFile`、`fork` 与同步版 `spawnSync/execSync/execFileSync`;`ChildProcess`(`stdin/stdout/stderr`、`pid`、`kill()`、事件 `exit`/`close`/`error`);选项 `cwd`、`env`、`stdio`、`detached`、`shell`、`timeout`、`signal`、`windowsHide` |
| net | 2 稳定 | `createServer`、`connect/createConnection`、`Server`(`listen/close/address`)、`Socket`(双工流,`setTimeout/setNoDelay/setKeepAlive`)、`isIP/isIPv4/isIPv6`、`BlockList`、`SocketAddress`;同一接口覆盖 TCP 与 IPC(Unix 域套接字或 Windows 命名管道) |
| dgram | 2 稳定 | `createSocket('udp4'|'udp6')`、`bind`、`send`、`connect`、`close`、组播 `addMembership`、事件 `message` |
| stream | 2 稳定 | `Readable`、`Writable`、`Duplex`、`Transform`、`PassThrough`;`pipeline`、`finished`、`compose`、`Readable.from`、`stream/promises`;背压靠 `highWaterMark`、`write()` 返回值与 `drain` 事件;另有 Web Streams(稳定)。Iterable Streams 为实验(需 `--experimental-stream-iter`)`[核]` |
| buffer | 2 稳定 | `Buffer.alloc`、`allocUnsafe`、`from`、`concat`、`byteLength`、`compare`;`readUInt32LE/writeInt16BE` 等定宽读写、`subarray`、`toString(encoding)`;`Blob`、`File`;`constants.MAX_LENGTH`。`new Buffer()` 已弃用(DEP0005)`[核]` |
| events | 2 稳定 | `EventEmitter`(`on/once/off/emit`、`setMaxListeners`)、`once()`、`on()`(异步迭代)、`EventTarget`、`errorMonitor`;`error` 事件无人监听即抛出 |
| timers | 2 稳定 | `setTimeout/setInterval/setImmediate` 及 clear 版,`Timeout.ref()/unref()/refresh()`,`timers/promises` |
| url | 2 稳定 | WHATWG `URL`、`URLSearchParams`、`fileURLToPath`、`pathToFileURL`、`domainToASCII`;旧 `url.parse()` 属遗留/弃用路径 |
| zlib | 2 稳定 | `gzip/gunzip/deflate/inflate/brotliCompress` 及同步版,流类 `createGzip` 等,`constants`;较新版本加入 zstd |
| crypto | 2 稳定 | `createHash`、`createHmac`、`randomBytes`、`randomUUID`、`createCipheriv/createDecipheriv`、`pbkdf2/scrypt/hkdf`、`sign/verify`、`generateKeyPair`、`KeyObject`、`timingSafeEqual`、`getHashes()`;Web Crypto 稳定 `[核]` |
| worker_threads | 2 稳定 | `Worker`、`isMainThread`、`parentPort`、`workerData`、`MessageChannel/MessagePort`、`threadId`、`resourceLimits`;共享靠 `SharedArrayBuffer` 与可转移对象 |
| Node-API | 2 稳定 | 见 3.4 |

稳定性总览中非稳定项举例 `[核]`: Domain 为 0 弃用,Punycode 为 0 弃用;Async hooks、FFI、Trace events、WASI、Virtual File System 为 1 实验;Benchmark runner 为 1.0;单可执行应用与 DTLS 为 1.1;SQLite 为 1.2。

观察到的表面设计规律 `[忆]`:

- (1) 同一能力常有多种调用形态(回调、同步、Promise),后加的 Promise 形态放在子路径 `fs/promises`、`timers/promises`、`stream/promises`,不改旧形态。
- (2) 可选行为走尾部 options 对象,新增选项不破坏旧调用。
- (3) 常量集中放在模块的 `constants` 对象里,不散落为顶层名字。
- (4) 平台专有行为保留同名接口,在文档中声明各平台差异,而不是分叉出不同名字。

## 3 错误与资源模型

### 3.1 错误分类 `[核]`

- 标准 JS 错误(TypeError、RangeError 等)、DOMException、系统错误(由操作系统约束触发)、AssertionError、用户自定义错误。
- 四种传播形态:同步 API 用 throw;返回 Promise 的 API 用拒绝;回调式 API 以错误优先回调(第一个参数为 Error 或 null);EventEmitter 对象上的异步操作走 `error` 事件,无处理者时作为未捕获异常使进程崩溃。

### 3.2 错误码是合同,消息不是 `[核]`

- 原文: `error.code` 是识别错误最稳定的方式,只在大版本之间才会变化;`error.message` 可在任何版本变化。
- Node 自有错误码形如 `ERR_INVALID_ARG_TYPE`、`ERR_MODULE_NOT_FOUND`、`ERR_STREAM_PREMATURE_CLOSE`;错误码自身也可带稳定性标记(例如 `ERR_REQUIRE_ESM` 标为 0 弃用,`ERR_QUIC_*` 标为 1 实验)。
- 例外: `ABORT_ERR` 不用 `ERR_*` 约定,为与 Web 平台的 AbortError 兼容。

### 3.3 SystemError `[核]`

| 属性 | 含义 |
| --- | --- |
| `code` | 字符串错误码,如 `ENOENT` |
| `errno` | 负数,对应 libuv 错误码;Windows 上的系统错误号由 libuv 归一化;用 `util.getSystemErrorName(errno)` 取名字 |
| `syscall` | 失败的系统调用名 |
| `path` / `dest` | 相关路径 / 文件系统操作的目标路径 |
| `address` / `port` | 网络连接失败的地址 / 端口 |
| `info` | 附加细节对象 |
| `message` | 系统给出的可读描述 |

文档列出的常见系统错误: EACCES、EADDRINUSE、ECONNREFUSED、ECONNRESET、EEXIST、EISDIR、EMFILE、ENOENT、ENOTDIR、ENOTEMPTY、ENOTFOUND、EPERM、EPIPE、ETIMEDOUT。其中 ENOTFOUND 是 Node 自造(DNS 的 EAI_NODATA/EAI_NONAME),文档明说它不是标准 POSIX 错误。

### 3.4 Node-API 的错误与 ABI 模型 `[核]`

- 每个调用返回 `napi_status`,结果经出参返回;`napi_ok` 表示成功且无挂起异常,`napi_pending_exception` 表示有 JS 异常挂起。
- `napi_get_last_error_info` 给出扩展信息,但明文规定其内容与格式不受 semver 约束,只供日志使用。
- 要求插件对枚举出参处理未知值(switch 带 default),为将来新增枚举值留余地。
- ABI 承诺原文要点:该 API 跨 Node 版本 ABI 稳定,为一个大版本编译的模块无需重编即可在之后的大版本运行。此承诺只覆盖 Node-API;`node.h`、`uv.h`、`v8.h` 没有跨大版本 ABI 保证。
- 版本机制:到版本 8 为止严格累加;自版本 9 起版本号独立于 Node 版本,升级 Node-API 版本可能需要改代码,但 ABI 稳定仍保持。`#define NAPI_VERSION n` 把可见表面限制到该版本及之前,未设置时默认 8;矩阵中最高为版本 10(v22.14.0+、23.6.0+)。
- 实验接口须 `#define NAPI_EXPERIMENTAL` 才可见;会影响稳定接口行为的实验特性另给逐特性的退出宏 `NODE_API_EXPERIMENTAL_<FEATURE>_OPT_OUT`。
- 头文件分两半: `js_native_api.h` 只含与引擎相关、与 Node 无关的部分,`node_api.h` 在其上加 Node 专有部分,目的是允许 Node 之外的实现。
- node-addon-api 是纯头文件的 C++ 封装,只依赖导出的 C 符号,因此继承同样的 ABI 稳定性。

### 3.5 资源模型 `[忆]`

- 文件描述符是整数,显式 `close`;`FileHandle` 是对象化句柄,未关闭即被回收时发警告,新版本支持显式资源管理(`using`/`Symbol.asyncDispose`)。
- 句柄的 `ref()/unref()` 决定它是否让事件循环保持存活;与 libuv 的"循环是否存活取决于活动且被引用的句柄"一致(后半句 `[核]`)。
- 流的销毁统一走 `destroy(err)`,结束状态用 `finished()` 观察,`pipeline()` 负责在任一环节出错时销毁全部环节。
- 取消统一用 `AbortSignal`(fs、child_process、timers/promises、stream 等的 `signal` 选项)。
- 内存: `Buffer.allocUnsafe` 不清零并可能来自内部池,`Buffer.alloc` 清零;上限为 `buffer.constants.MAX_LENGTH`。

## 4 可移植性处理

- 平台分级 `[核]`(BUILDING.md):
  - Tier 1: 代表多数用户的平台,测试失败阻塞发布。
  - Tier 2: 较小用户群,测试失败同样阻塞发布,基础设施问题可能推迟二进制发布。
  - Experimental: 可能无法编译或测试不过,核心团队不为其出发布,测试失败不阻塞。
  - 当前表: Linux x64(glibc) 与 arm64、Windows x64、macOS arm64 为 Tier 1;Linux musl x64、ppc64le、s390x、Windows arm64、SmartOS、AIX 为 Tier 2;macOS x64、Linux x86/armv7/loong64/riscv64、FreeBSD、OpenHarmony 为 Experimental。每行带最低系统版本(例如 Linux Tier 1 要求内核 5.14+、glibc 2.34+);厂商已停止支持的系统版本不再支持。
- 系统调用文档约定 `[核]`:封装系统调用的函数会在文档中注明并链接 man 页;多数 Unix 系统调用在 Windows 上有对应物,但行为差异可能不可避免。
- 错误号归一 `[核]`: Windows 系统错误号由 libuv 归一到同一套负数码与 E* 名字。
- path 的双实现 `[核]`:默认按宿主系统语义;`path.posix` 与 `path.win32` 在任何宿主上都可用,用来得到与宿主无关的确定结果;`isAbsolute` 只判断字面,文档明说不能用来防路径穿越;`resolve`/`relative` 依赖当前工作目录;Windows 有按盘符的工作目录概念;`toNamespacedPath` 在 POSIX 上是空操作。
- 其余平台差异的典型处理 `[忆]`:
  - 同名接口、文档声明差异:信号在 Windows 上只部分支持;`fs.chmod` 在 Windows 上只影响只读位;`fs.watch` 的 recursive 支持与事件语义依平台而异;`process.getuid()` 等只在 POSIX 存在。
  - IPC 统一在 `net` 下,路径形态不同(Unix 域套接字路径与 `\\.\pipe\` 命名管道)。
  - 常量按平台裁剪,放在 `os.constants`、`fs.constants`。
  - 平台探测只有 `process.platform`、`process.arch`、`os.*` 这几个入口。

## 5 稳定性与版本策略

### 5.1 稳定性指数 `[核]`

| 等级 | 名称 | 承诺 |
| --- | --- | --- |
| 0 | Deprecated | 可能发警告,不保证向后兼容 |
| 1 | Experimental | 不受 semver 约束,任何版本都可能不兼容变更或移除,不建议生产使用 |
| 1.0 | Early development | 未完成,会有重大变化 |
| 1.1 | Active development | 接近最小可用 |
| 1.2 | Release candidate | 预期可转稳定,原则上不再破坏,但仍可能因用户反馈或底层规范演进而变 |
| 2 | Stable | 与 npm 生态的兼容性是高优先级 |
| 3 | Legacy | 不太会移除,仍受 semver 保护,但不再积极维护,另有替代;其缺陷多半不修 |

- 标为遗留而非弃用的条件:使用它无害,且在 npm 生态中被广泛依赖。
- 实验特性的出路:通常是转为稳定,或者不经弃用周期直接移除。
- 等级标到模块,也可标到单个函数、单个选项、单个错误码。

### 5.2 落地流程与角色 `[核]`

- 每个 PR 至少两名协作者批准(开放超过七天时一名即可),批准者不得是作者。
- 落地前留 48 小时给其他协作者提意见;聚焦的文档或测试改动、回归修复可申请快速通道,快速通道申请本身需两名协作者同意。
- CI 必须通过: GitHub Actions 全绿;影响 `node` 二进制的改动还要 Jenkins 绿或黄。
- 向后不兼容的改动落到 main,至少两名 TSC 投票成员批准。
- 拿不准时从严,标 semver-major。
- 角色: 协作者(评审与落地)、TSC(破坏性变更与新模块签字、争议裁决)、发布组与回移组(只有 `@nodejs/backporters` 成员可向 LTS staging 分支落提交)。

### 5.3 什么算破坏性变更 `[核]`

- 示例:移除或重新定义 API 参数、改变返回值、改变事件的时机。
- 判据是生态后果而非改动形态:一个改动会造成生态破坏,它就是 semver-major。
- 运行期弃用与终止期移除都是 semver-major。
- 无条件新增到全局作用域的 API 须标 semver-major,可经 TSC 共识豁免;推荐做法是先放在 `--experimental-<特性名>` 开关之后,功能完整后默认开启并提供 `--no-experimental-<特性名>` 退出。
- 错误消息不属于合同(见 3.2),错误码属于。

### 5.4 弃用周期 `[核]`

弃用理由(deprecations.html):使用不安全;已有更好的替代;预计未来大版本会有破坏性变更。

| 阶段 | 行为 | 版本类别 |
| --- | --- | --- |
| Documentation-only | 只在文档标注,默认无运行期警告;可在 `--pending-deprecation` 或环境变量 `NODE_PENDING_DEPRECATION` 下警告 | 可进小版本,非 semver-major |
| Application | 只对非 `node_modules` 的代码在首次使用时向 stderr 发进程警告;加 `--pending-deprecation` 后对 `node_modules` 内代码也警告 | deprecations.html 列为四种之一 |
| Runtime | 所有代码首次使用即警告;`--throw-deprecation` 下改为抛错 | semver-major |
| End-of-Life | 不再受 semver 约束,可移除 | semver-major |

规则:

- (1) 一个弃用周期等于一个大版本:文档级弃用要到下一个大版本才能升为运行期弃用。
- (2) 不经过一轮运行期弃用,不得进入终止期。因此从"文档标注"到"移除"最少跨两个大版本边界。
- (3) 文档级与运行期弃用都可以无限期停留,不强制推进。
- (4) 别名或空实现桩就够用时,不要上运行期弃用。
- (5) 每条弃用有唯一且不可变的标识(形如 DEP0005,页面目录中最高可见 DEP0211);即使 API 已移除,标识仍留在文档里;弃用被撤销时更新文档说明,但标识不改。
- (6) 引入文档级弃用或改变弃用等级的 PR 加 `notable-change` 标签。
- 实例: DEP0005 `Buffer()` 构造器, v6.0.0 文档级 -> v10.0.0 运行期,类型为 Application;DEP0040 `node:punycode`, v7.0.0 文档级 -> v16.6.0 支持 pending -> v21.0.0 运行期。从文档标注到运行期警告历时数年是常态。
- `--no-deprecation`、`--trace-deprecation` 两个开关 `[忆]`。

### 5.5 从实验到稳定 `[核]`

- 新核心模块以实验等级落地,评审期至少一周,需至少两名 TSC 投票成员签字。
- 模块必须保持实验等级直到一个 semver-major 版本。
- 引入实验特性即承诺:要么及时转稳定,要么及时移除;作者应协助评估和修补其安全问题。
- 事实稳定条款:实验特性若已被主流采用,以致破坏性变更会造成生态破坏,就应视为稳定,此后要么正式转稳定,要么走弃用周期后移除。也就是说"实验"标签不能无限期充当免责。
- 未找到一份统一的"转稳定检查表"文档 `[核:未见]`;实际判据散落在各特性的讨论中,通常包含文档齐全、测试覆盖、无未决设计问题、跨平台通过、经过若干版本的使用反馈 `[忆]`。

### 5.6 发布线与 LTS `[核]`

- 到 Node.js 26 为止:每六个月一个大版本,偶数版 4 月、奇数版 10 月;只有偶数线升为 LTS;LTS 线先 12 个月 Active LTS 再 18 个月 Maintenance。
- 自 Node.js 27 起改制:每年 10 月从 main 切一条发布线,每条线依次经历约 6 个月 Alpha、6 个月 Current、30 个月 LTS,所有发布线都升 LTS;从首个 Current 版到终止共 36 个月。
- 各阶段允许的改动: Alpha 接纳包括 semver-major 在内的大部分改动(面向库作者与 CI);Current 接纳大部分非破坏性改动;Active LTS 接纳经发布组审核的新特性、缺陷修复与更新;Maintenance 只收关键缺陷修复与安全更新。
- 当前日程表: 22.x 维护期至 2027-04-30;24.x 维护期至 2028-04-30;26.x 于 2026-10-28 进入 Active LTS,至 2029-04-30;27.x 于 2026-10-28 进入 Alpha,2027-04-22 转 Current,2027-10-20 转 LTS,至 2030-04-30。
- 回移规则:改动先落 main,再拣选到 `vN.x-staging`;进 LTS 之前须在 Current 发布中成熟至少两周(标签 `baking-for-lts`、`lts-watch-vN.x`、`dont-land-on-vN.x`)。

### 5.7 生态回归 CITGM `[核]`

- CITGM(Canary in the Goldmine)对一批常用 npm 模块执行 `npm install && npm test`,用来发现生态破坏;Jenkins 作业名 `citgm-smoker`。
- 协作者指南的用法:对可能破坏生态的内部改动用它测试;出现意外破坏后,评估把相关包加入 CITGM 是否能提前发现。指南把它作为推荐工具,未列为每个 PR 的强制门。
- 发布组在切大版本和发布前例行跑 CITGM `[忆]`。

## 6 对 cx 的启示

每条为决策型:采纳、改造或不采纳,附原因。

- (1) 采纳: 稳定性等级逐符号标注。Node 把 0/1/2/3 标到模块、函数、选项、错误码各级。cx 在 `build/layer-api/*.tsv` 每行增加稳定性列,取值沿用 0 弃用、1 实验、2 稳定、3 遗留,由生成器带 `--check` 核对,不靠文档自述。
- (2) 改造: 实验子阶段 1.0/1.1/1.2 只取 1.2 的含义。cx 当前全局预稳定(`build/tlib-policy.tsv` stable no),三级细分没有消费者;保留"候选稳定"一档即可,其判据写成可执行门:接口冻结核对通过、六目标同候选实跑通过、至少一个仓内调用者。
- (3) 采纳: 事实稳定条款。实验接口一旦被 `tools/`、`tests/`、`apps/` 广泛调用,就按稳定对待。cx 用 `refactor.cx sites` 的调用点计数作判据,超过阈值的实验符号不得再无周期地破坏。
- (4) 采纳: 错误码是合同、消息不是。cx 的 `sy_*` 返回值与错误码进入接口表并受版本规则约束;消息文本可随时改,测试不得断言消息全文。
- (5) 采纳: 系统错误在 L0 归一。Node 由 libuv 把 Windows 错误号归一成同一套 E* 名字和负数码。cx 在 L0 的 `_os` 表里完成同样的归一,上层只见一套码;同时保留失败的系统调用名与路径两个字段,对应 `syscall`、`path`。
- (6) 采纳: path 双实现。`cx.path` 提供与宿主无关的 posix 与 win32 两套纯字面实现,默认名绑定到目标平台语义;六目标下可在任一宿主上对两套语义做字节等价测试。并在接口说明中写明字面判断不构成路径穿越防护。
- (7) 改造: 弃用周期保留三段与不可变编号,去掉按日历大版本推进。cx 没有半年一个大版本的节奏;改为"文档级 -> 构造期警告 -> 移除",每段至少跨一个已发布版本,编号(如 CXDEP0001)登记进表且永不复用,撤销弃用时编号保留。
- (8) 采纳: 能用别名或空桩解决就不发运行期警告。对应 cx 的改名迁移:旧名保留为别名并登记弃用编号,不以删除旧功能完成改名,与现行工具改名规则一致。
- (9) 采纳: 新核心模块只以带命名空间的名字加入。Node 用 `node:` 前缀避免与用户态包撞名;cx 的 `cx.*` 与 `ext.*` 已是命名空间,补一条规则:核心新增名字只能落在 `cx.` 之下,不得占用无前缀短名,`ext.*` 升入核心时换名并留别名。
- (10) 改造: ABI 稳定面单独划出并独立编号。Node 只对 Node-API 承诺跨大版本 ABI,其余内部头文件不承诺,且 ABI 版本号与产品版本号脱钩、用宏限定可见表面。cx 把对宿主互操作与库/ABI 层的稳定面单列一张表,独立版本号,其余 L0 内部帮手明确标为无承诺;枚举类返回值要求调用方处理未知值。
- (11) 不采纳: 回调、同步、Promise 三套并存的表面。那是 JS 事件循环的历史产物,使 fs 表面膨胀约三倍。cx 每个能力只给一种调用形态,流式能力统一走 `cx.stream`。
- (12) 改造: 平台分级与生态回归。采纳"分级决定是否阻塞发布"并把每个目标的最低系统版本写进事实表;CITGM 改造为仓内等价物:以 `tools/`、`tests/`、`apps/` 的现有 `.cx` 作为金丝雀集合,接口变更后用 `tools/affected.cx` 选批实跑,未实跑不记 PASS。Node 的 48 小时等待与双人批准不采纳,原因是 cx 为单集成者写 main,以可执行门替代人工等待。

未覆盖范围: 本文未核对 fs、os、process、child_process、net、dgram、stream、buffer、events、timers、url、zlib、crypto、worker_threads 各模块页面的逐函数细节(上表标 `[忆]`);n-api.html 只读了前约三分之一;未查阅 TSC 章程与 GOVERNANCE.md 原文。
