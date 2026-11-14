# 参考调研: Rust 标准库的核心面与稳定性策略

用途: 为 cx 核心层(cx.sys/os/path/mem/text/stream)与扩展(ext.*)的核心面、稳定性和版本策略提供一份成熟运行库的对照。本文只调研 Rust,不改任何 cx 表或代码。调研日期 2026-11-12。

证据标记:
- `[核]` 本次用 WebFetch 打开官方页面核对过,给出 URL。
- `[忆]` 来自记忆,本次未逐条打开原文核对;用于设计参考可以,写进合同前须再核。

本次核对过的页面:
- U1 std 首页 https://doc.rust-lang.org/std/index.html
- U2 io::ErrorKind https://doc.rust-lang.org/std/io/enum.ErrorKind.html
- U3 std-dev-guide 功能生命周期 https://std-dev-guide.rust-lang.org/development/feature-lifecycle.html
- U4 std-dev-guide 稳定化 https://std-dev-guide.rust-lang.org/development/stabilization.html
- U5 std-dev-guide 破坏性变更 https://std-dev-guide.rust-lang.org/breaking-changes/summary.html
- U6 std-dev-guide 目标相关代码 https://std-dev-guide.rust-lang.org/policy/target-code.html
- U7 rustc-dev-guide 稳定性属性 https://rustc-dev-guide.rust-lang.org/stability.html
- U8 RFC 1105 API 演进 https://rust-lang.github.io/rfcs/1105-api-evolution.html
- U9 RFC 1122 语言 semver https://rust-lang.github.io/rfcs/1122-language-semver.html
- U10 RFC 507 发布通道 https://rust-lang.github.io/rfcs/0507-release-channels.html
- U11 The Book 附录 G https://doc.rust-lang.org/book/appendix-07-nightly-rust.html
- U12 Edition Guide https://doc.rust-lang.org/edition-guide/editions/index.html
- U13 目标分级策略 https://doc.rust-lang.org/rustc/target-tier-policy.html
- U14 rfcbot https://github.com/rust-lang/rfcbot-rs

## 1 分层与模块结构

### 1.1 三层库

| 层 | 依赖 | 内容 | 对应 cx 的位置(类比) |
|---|---|---|---|
| core | 无操作系统、无堆分配 | 基本类型方法、Option/Result、迭代器、切片与 str、fmt、mem、ptr、cell、atomic、time::Duration | 不依赖 L0 系统调用的纯计算部分(cx.mem/cx.text 的无分配子集) |
| alloc | 需要一个全局分配器 | Box、Vec、String、Rc/Arc、BTreeMap、VecDeque、format! | cx.mem 之上需要堆的容器与文本 |
| std | 需要操作系统 | 重新导出 core 与 alloc,再加 fs/io/net/process/env/thread/sync/time/path/collections::HashMap/os | cx.sys/os/path/stream |

- std 默认对所有 crate 可用;`#![no_std]` 只是不自动链接 std。`[核 U1]`
- std 文档自述其主要工作是抹平平台差异,尤其是 Windows 与 Unix 系。`[核 U1]`
- core 与 alloc 在受限环境(main 之前之后、无 OS)最可能可用,但 panic、OOM、分配器等可挂钩行为取决于钩子本身。`[核 U1]`
- 三层是同一棵源码树(library/core、library/alloc、library/std),同一版本号,随编译器一起发布;用户不能单独选择 std 的版本。std-dev-guide 直接称标准库是 unversioned 的,所以准入门槛高。`[核 U3]`
- 上层对下层是「重新导出」关系: `std::vec::Vec` 与 `alloc::vec::Vec` 是同一个类型,稳定性标注跟着定义处走。`[忆]`

### 1.2 std 顶层模块(稳定部分)`[核 U1]`

alloc any arch array ascii backtrace borrow boxed cell char clone cmp collections convert default env error f32 f64 ffi fmt fs future hash hint io iter marker mem net num ops option os panic path pin prelude primitive process ptr range rc result slice str string sync task thread time vec。

另有一组实验模块(async_iter、bstr、random、simd 等)只在 nightly 可见,以及一组已弃用但仍保留的整数常量模块(i8 ... usize)。后者是「弃用不删除」的活例子。`[核 U1]`

### 1.3 结构特点

- (1) 模块按「领域」切,不按「平台」切;平台特有的东西集中到 `std::os::{unix,windows,linux,...}`。
- (2) prelude 很小(Option、Result、Vec、String、Box、常用 trait),其余都要显式 use。往 prelude 加东西被当成破坏性变更的专门风险类别。`[核 U5]`
- (3) 内部实现模块 `std::sys` 不公开,是平台抽象层;公开模块只调用 sys 的统一接口。`[忆]`

## 2 核心面清单

以下各区的函数与类型名来自记忆 `[忆]`,用于给 cx 对照覆盖面;ErrorKind 清单与顶层模块名已核。

### 2.1 fs

- 类型: File、OpenOptions、Metadata、Permissions、FileType、DirEntry、ReadDir、DirBuilder、FileTimes。
- 整文件便利函数: read、read_to_string、write、copy、rename、remove_file、exists。
- 目录: create_dir、create_dir_all、remove_dir、remove_dir_all、read_dir。
- 元数据与链接: metadata、symlink_metadata、canonicalize、hard_link、read_link、set_permissions。
- File 方法: open、create、create_new、sync_all、sync_data、set_len、metadata、try_clone、lock/unlock 系列。
- OpenOptions 是构造器: read/write/append/truncate/create/create_new 逐项设定再 open。

### 2.2 path

- Path(借用,不可变)与 PathBuf(拥有,可增长),关系等同 str 与 String。
- 方法: join、push、pop、parent、file_name、file_stem、extension、with_extension、components、ancestors、is_absolute、starts_with、strip_prefix、to_str(可能失败)、to_string_lossy、display。
- 常量: MAIN_SEPARATOR;Component 枚举区分 Prefix/RootDir/CurDir/ParentDir/Normal。
- 路径是纯词法操作,不碰文件系统;碰文件系统的(exists、is_file、canonicalize)另列。
- 底层是 OsStr/OsString(在 std::ffi): 不保证是 UTF-8,平台各有内部表示。

### 2.3 process

- Command(构造器): new、arg、args、env、env_remove、env_clear、current_dir、stdin/stdout/stderr、spawn、status、output。
- Child: wait、try_wait、kill、id、wait_with_output;字段 stdin/stdout/stderr 是可取走的管道端。
- Stdio: inherit、piped、null。ExitStatus: success、code。ExitCode、Termination(main 的返回值协议)。
- 进程自身: exit、abort、id。

### 2.4 env

- args/args_os、var/var_os、vars/vars_os、set_var/remove_var(2024 edition 起为 unsafe)、current_dir、set_current_dir、current_exe、temp_dir、split_paths/join_paths。
- 常量模块 env::consts: OS、ARCH、FAMILY、EXE_SUFFIX、DLL_EXTENSION。
- 每个取值函数都有成对的 `_os` 版本,把「非 Unicode」的情况留给调用者处理,而不是悄悄替换。

### 2.5 net

- TcpListener(bind、accept、incoming)、TcpStream(connect、connect_timeout、shutdown、set_nodelay、set_read_timeout、peer_addr)、UdpSocket(bind、send_to、recv_from、connect)。
- 地址: IpAddr/Ipv4Addr/Ipv6Addr、SocketAddr、ToSocketAddrs(含阻塞式名字解析)。
- 只有阻塞式 TCP/UDP 与地址类型;没有 HTTP、TLS、异步运行时。这是有意的小面。

### 2.6 time

- Duration(在 core): from_secs/from_millis/from_nanos、as_secs、checked_add、saturating_sub。
- Instant: 单调钟,只能相减得 Duration,不能转成日历时间。now、elapsed、duration_since、checked_add。
- SystemTime: 墙上钟,可能回拨;duration_since 返回 Result;UNIX_EPOCH。
- 没有日历、时区、格式化;交给生态库。

### 2.7 io

- trait: Read、Write、Seek、BufRead;适配器 BufReader、BufWriter、LineWriter、Cursor、Take、Chain。
- 函数: stdin、stdout、stderr、copy、empty、sink、read_to_string。
- 类型: Error、ErrorKind、Result<T>(即 Result<T, io::Error>)、SeekFrom、IoSlice/IoSliceMut(向量读写)。
- 约定: read 返回 Ok(0) 表示流结束;Interrupted 应重试;write 可以短写,write_all 负责写完。

### 2.8 thread 与 sync

- thread: spawn、scope(作用域线程,借用栈上数据)、JoinHandle::join、sleep、park/unpark、current、available_parallelism、Builder(名字与栈大小)、thread_local!。
- sync: Mutex、RwLock、Condvar、Once、OnceLock、LazyLock、Barrier、Arc、Weak、mpsc::channel/sync_channel、atomic::{AtomicBool, AtomicUsize, Ordering}。
- 锁通过守卫对象(MutexGuard)访问数据,守卫析构即解锁;持锁线程 panic 后锁进入 poisoned 状态。

### 2.9 collections

- Vec、VecDeque、LinkedList、HashMap、HashSet、BTreeMap、BTreeSet、BinaryHeap。
- HashMap 默认哈希带随机种子(防碰撞攻击);可替换 Hasher。
- Entry API(entry().or_insert_with())是「查一次、改一次」的统一入口。
- 除 HashMap/HashSet(需要随机源,在 std)外,其余都在 alloc。

### 2.10 观察

- 核心面的边界是「需要语言或编译器配合,或是对已有 std 类型的扩展」;预期会持续演进的复杂 API(例如 GUI 框架)明确判为不适合进 std。`[核 U3]`
- 每个区域都是「少量拥有型类型 + 构造器 + 少量整操作便利函数」,便利函数(fs::read)直接建立在类型(File)之上,不是第二套实现。

## 3 错误与资源模型

### 3.1 错误

- 可恢复错误一律走返回值 Result<T, E>;不可恢复的程序错误走 panic。标准库不用异常也不用全局 errno 作为公开接口。`[忆]`
- I/O 类错误统一为一个类型 io::Error,内部三种来源: 操作系统错误码、仅有 ErrorKind 的简单错误、带任意载荷的自定义错误。访问口: kind()、raw_os_error()、last_os_error()、from_raw_os_error()。`[忆]`
- ErrorKind 是可移植分类。当前稳定 39 个变体(NotFound、PermissionDenied、AlreadyExists、WouldBlock、NotADirectory、IsADirectory、DirectoryNotEmpty、InvalidInput、InvalidData、TimedOut、Interrupted、Unsupported、UnexpectedEof、OutOfMemory、Other 等),另有 4 个仍不稳定(FilesystemLoop、InProgress、TooManyOpenFiles、InputOutputError)。`[核 U2]`
- ErrorKind 标了 non_exhaustive,文档原话是清单会随时间增长,不建议穷尽匹配;应用代码只匹配预期的种类,其余走通配。`[核 U2]`
- 标准库自己产生的、尚未归类的错误不能被具名匹配,只能落到通配分支,将来可能为它们新增 ErrorKind;Other 留给用户自造错误,标准库自己不用。`[核 U2]` 这条的意义: 新增一个分类不会让旧代码的具名分支改变含义。
- `?` 运算符加 From 转换做错误传播;std::error::Error trait 提供 source() 链。`[忆]`
- 可失败的「取值」成对出现: 一个返回 Result 或 Option 的严格版本,一个 lossy 版本(to_str 与 to_string_lossy),不做静默替换。`[忆]`

### 3.2 资源与所有权

- 资源由拥有型值代表(File、TcpStream、Child、MutexGuard、Box、Vec),离开作用域时 Drop 自动释放;没有公开的「裸句柄 + 手动 close」主路径。`[忆]`
- Drop 里的错误被吞掉: File 析构时 close 失败不报。需要确认落盘的调用者必须显式 sync_all 并检查返回值。这是 RAII 的已知代价,文档明写。`[忆]`
- 不是所有资源都在 Drop 时做「最强清理」: Child 析构不杀也不等子进程;JoinHandle 析构是分离线程;BufWriter 析构会尝试刷出但忽略错误。每个类型在文档里单独写明析构合同。`[忆]`
- 句柄层也有所有权: OwnedFd/BorrowedFd/AsFd(Unix)与 OwnedHandle/BorrowedHandle(Windows),把「谁负责关」写进类型(RFC 3128 I/O safety)。裸 RawFd 的转换是 unsafe 的 from_raw_fd。`[忆]`
- 借用规则在编译期阻止释放后使用与数据竞争;运行期没有句柄有效性检查这一层。cx 的运行期句柄检查与此不是同一性质,不能互相冒充。
- 作用域式 API(thread::scope)用于「资源生命周期必须被括住」的场合,替代泄漏式的后台资源。`[忆]`

## 4 可移植性处理

- (1) 条件编译用 cfg 谓词: target_os、target_arch、target_family、unix、windows、target_pointer_width、target_env。用户代码与 std 内部用同一机制。`[忆]`
- (2) std 内部把平台实现收进 sys 模块(library/std/src/sys,下分 pal/unix、pal/windows、pal/wasi、pal/unsupported 等),公开模块只写一份、调 sys 的同名接口。平台分支不散落在 fs.rs、net.rs 里。`[忆]`
- (3) 不支持的平台走 unsupported 实现: 函数存在、可编译,运行时返回 ErrorKind::Unsupported,而不是让符号消失。`[忆]` Unsupported 是稳定 ErrorKind。`[核 U2]`
- (4) 平台特有能力通过 `std::os::<平台>` 下的扩展 trait 暴露(unix 的 PermissionsExt、FileExt、CommandExt、OsStrExt;windows 的 OpenOptionsExt)。使用者必须显式 use 平台模块,代码里一眼可见不可移植点。`[忆]`
- (5) 字符串边界: OsString/OsStr 承载平台原生文本(Unix 是任意字节,Windows 是可能不合法的 UTF-16),CString/CStr 承载以零结尾的 C 文本;与 str 的转换显式且可失败。`[忆]`
- (6) 目标分三级: 一级保证能构建且通过测试,二级保证能构建但测试不保证,三级无任何保证。`[核 U13]` 维护者人数门槛分别为至少 3 人、至少 2 人、至少 1 人在册;二级不得把 core 或 std 的重要部分留空或打桩,除非该目标根本无法支持。`[核 U13]` 晋级需先在低一级停留若干个稳定版;一级降级要走完整 RFC。`[核 U13]`
- (7) 评审力度按级分: 一级目标的平台代码按「不是平台专属代码」全面评审;二三级只核三件事(确实只影响该目标、不引入许可问题、由目标维护者提出或背书),不核正确性与质量,理由是评审者缺少带宽与专门知识。`[核 U6]`
- (8) 行为差异写进文档的 Platform-specific behavior 段,并声明「将来可能改变」,把底层系统调用的选择排除在稳定承诺之外。`[忆]`

## 5 稳定性与版本策略

### 5.1 通道与节奏

- 三个通道 nightly、beta、stable。每六周从主干切一次 beta,上一个 beta 变 stable;回归在主干修再回移到 beta。`[核 U11]`
- 只支持最新的稳定版,每个版本的支持期就是六周。`[核 U11]`
- 开发中的功能进主干但藏在 feature flag 后面;只有 nightly 能开 flag,beta 与 stable 一律不能。原文: 这是让新功能在宣布永久稳定之前先得到实际使用的关键。`[核 U11]`
- 机制上是一个 lint(unstable_features)在 nightly 默认 allow,在 beta 与 stable 设为 forbid。`[核 U10]`
- 口号是 stability without stagnation: 升级稳定版不应让人害怕,同时每次升级带来新功能。`[核 U11]`

### 5.2 标注机制

- 每个公开项带稳定性属性。`#[unstable(feature = "名字", issue = "跟踪单号")]`: 跨 crate 使用需要 `#![feature(名字)]`;issue 必填,所有不稳定功能都要有跟踪单。`[核 U7]`
- `#[stable(feature = "名字", since = "版本")]`: since 记录稳定的版本,文档自动显示。提交时不写具体版本号,写字面占位符 CURRENT_RUSTC_VERSION,由发布流程替换,避免合并延期造成版本写错。`[核 U4]`
- unstable 会向下传染到子项,子项可再单独标 stable,工作方式类似 pub。`[核 U7]`
- 稳定函数的函数体内可以使用不稳定内部项;但稳定的宏展开到调用点,所以需要 allow_internal_unstable 显式放行。`[核 U7]`
- const 能力是独立的第二条稳定性轴(rustc_const_unstable / rustc_const_stable),且是递归的: const 不稳定的函数不能被稳定代码间接调用。`[核 U7]`
- 弃用用 `#[deprecated(since, note, suggestion)]`,必须与 stable 或 unstable 配对;std 里的 since 会与当前编译器版本比对,写未来版本则只触发 deprecated_in_future,可以提前预告。`[核 U7]`
- 历史错误的处理: 误稳定的路径用 rustc_allowed_through_unstable_modules 保住兼容,并注明不得再对新项使用。`[核 U7]` 即: 误放出去的东西也按已承诺对待。
- 使用这套属性的 crate 需开启 staged_api;普通第三方 crate 不用这套机制,靠 semver。`[核 U7]`

### 5.3 库功能从提出到稳定的流程 `[核 U3 U4]`

- (1) 从真实问题出发,不从理论上的 API 缺口出发。
- (2) 判断是否适合进 std: 因为不单独定版,门槛高。
- (3) ACP(API Change Proposal): 在 libs-team 仓开单,由库 API 组评审;通过的标准是「乐观认为它最终能通过 FCP」。ACP 不是硬性要求,但不走它直接提 PR 有白做的风险;ACP 通过也不等于最终稳定。
- (4) 设计讨论在社区进行,库组不直接主持。
- (5) 实现 PR: 必须附已考虑的替代方案摘要,否则可能被关闭并要求先探索替代方案。
- (6) 合并前必须有跟踪单,一直跟到稳定。例外: 改已有不稳定 API 可复用原单;「即刻稳定」的变更(例如给稳定类型加 trait 实现)没有不稳定期,因此要求额外审查。
- (7) 稳定化: 任何人都可以推动。先在跟踪单写稳定化报告,再请 libs 组成员发起 FCP,等 FCP 以 disposition-merge 结束,然后提稳定化 PR。

大的或跨领域的设计走完整 RFC(rust-lang/rfcs 仓,由对应组达成共识后接受),接受后才开跟踪单并在 feature gate 后落地。`[核 U11]`

### 5.4 稳定化的具体判据

- 稳定化报告三段(官方称推荐而非强制): `[核 U4]`
  - 实现历史: 最初的讨论、之后每次改动、提出过的所有问题及其如何解决。
  - API 摘要: 被稳定的 API 的精确描述。
  - 使用经验: 实际试用者的具体用例,最好带项目链接;导出相同 API 的 crate 的使用情况也算。
- 稳定化 PR 的机械清单: unstable 改 stable;从测试、文档测试、编译器与工具里去掉对应的 `#![feature]`;涉及 const 的另改 const 稳定性并通知常量求值工作组;若暴露了 intrinsic,语言组也要进 FCP;unsafe 需写明 const 安全论证。`[核 U4]`
- 可以只稳定一个子集(部分稳定化),不另开跟踪单。`[核 U4]`
- FCP 的运作: 组内成员用 rfcbot 发起 `fcp merge`;成员勾选自己名字表示同意,或登记具名 concern 阻塞;concern 只能由提出者本人标记已解决;评审请求满足且 concern 清零后进入最终意见期。`[核 U14]`
- 进入 FCP 的勾选门槛(除至多两人外全部勾选且过半数)与最终意见期长度(10 个日历日)是记忆值。`[忆]` rfcbot 页面本次只核到「开始一周后机器人会发一条提示」,未核到门槛与 10 日原文。
- 没有「不稳定期至少多久」的硬性时长;判据是设计已定、未决问题清零、有使用经验。`[忆]`

### 5.5 角色

| 角色 | 职责 |
|---|---|
| 库 API 组(libs-api) | 决定公开 API 的形状: 评审 ACP、勾选 FCP、对稳定化表决 `[核 U3 U4]` |
| 库组(libs) | 实现质量、内部结构、评审 PR `[忆]` |
| 语言组、编译器组 | 语言特性与编译器行为的同类决定;intrinsic 暴露时联合 FCP `[核 U4]` |
| 目标维护者 | 对本目标的平台代码负责,二三级目标代码须由其提出或背书 `[核 U6 U13]` |
| 任何贡献者 | 可写 RFC、可提实现、可推动稳定化 `[核 U4 U11]` |

要点: 提议与实现对所有人开放,但「承诺」只有一个小组能做,并且必须留下书面表决记录。

### 5.6 破坏性变更的规则

- 总原则: 破坏性变更默认算 major;但有一类「可能让某些代码编译失败、却能通过本地消歧修好,而且这种写法本可以提前写上」的变更算 minor。`[核 U8]`
- 算 minor 的: 新增公开项(可能撞 glob 导入);新增带默认实现的 trait 项;为已有类型实现非 fundamental 的已有 trait;新增固有方法(可能遮住同名 trait 方法);放宽泛型约束;把参数泛化为泛型且原用法仍成立;引起的类型推断失败。`[核 U8]`
- 算 major 的: 新增无默认实现的 trait 项;改变 trait 项签名;给枚举加变体或给变体加字段;给全公开字段的结构体加私有字段;增删函数参数;收紧约束。`[核 U8]`
- 由此得出的设计纪律: 想保留将来加字段或加变体的权利,就得一开始留私有字段或标 non_exhaustive(ErrorKind 即如此)。`[核 U2 U8]`
- std 的实操: 破坏性变更尽量避免;是否可接受取决于实际影响,用 crater(对整个公开生态重新编译)估计;影响不大时联系受影响 crate 的维护者并替他们提修复。专门列出的风险类别: 新 trait 实现、prelude 变更、文档变更。`[核 U5]`
- 语言与编译器层面: 小版本只允许修编译器缺陷或类型系统问题的破坏性变更,分三类(健全性漏洞、其他类型系统不一致、尚未充分规定的语义)。流程: 先用 crater 量影响;影响小则直接落地,提交说明带 `[breaking-change]` 并写修法;影响大则要缓解(提前联系重要 crate、清楚的报错、自动修复工具、临时的作用域内豁免标注、先警告后报错)。`[核 U9]`
- 弃用规则: 稳定的 std 项弃用后不删除,只给编译警告与替代建议;已弃用的整数常量模块至今仍在文档里。`[核 U1]` 「稳定项永不删除」作为成文规则本次未找到原文。`[忆]` RFC 507 只规定弃用如何标注(在 stable 之外再加一条带 since 的标记),没有规定何时删除。`[核 U10]`
- 不稳定项没有任何兼容承诺: 可以随时改签名、改名或整个删掉。`[忆]`

### 5.7 edition

- edition 是容纳不兼容语言变更的机制(例如把 async 变成关键字),按 crate 逐个选择,默认不变。`[核 U12]`
- 硬约束: 不同 edition 的 crate 必须无缝互操作;某个 crate 迁不迁是它的私事,不影响生态里的其他 crate。`[核 U12]`
- 因此 edition 里的变更只能是「表皮层」的: 所有 edition 最终编译到同一内部表示。`[核 U12]`
- 提供自动迁移工具(cargo fix),但声明不完美,且避免做影响正确性或性能的语义改动。`[核 U12]`
- 已有 2015、2018、2021、2024 四个 edition,约三年一个。`[忆]`
- std 只有一份,所有 edition 共用;edition 能改的库侧内容基本只有 prelude 的内容与个别宏的行为。`[忆]`

### 5.8 小结: 这套策略靠什么成立

- (1) 稳定与否是逐项的机器可读标注,不是整库的一个版本号。
- (2) 不稳定功能在稳定通道上物理上用不了,所以「试用」不会变成「事实依赖」。
- (3) 稳定是单向门,所以门前的判据重(书面报告、小组表决、公开异议期),门后的规则简单(不破坏)。
- (4) 破坏的定义是成文的逐类清单,并且用全生态重编译来量,而不是凭感觉。
- (5) 留有两个泄压口: 健全性修复可以破坏;语法层不兼容走 edition。

## 6 对 cx 的启示

每条是决策型结论: 采纳、改造或不采纳,加原因。涉及的 cx 现状(build/layer-api、build/tlib-policy.tsv 的预稳定封顶、build/std-surface.tsv)以仓内表为准,本文不改它们。

- (1) 采纳: 稳定性按「项」标注,进事实表。原因: Rust 的整套策略建立在逐项机器可读标注上(5.2)。cx 已有 build/layer-api 的逐层 API 表,给每行加 stability(unstable/stable/deprecated)、since、tracking 三列即可,由生成器带 --check 维护,不写成文档约定。
- (2) 采纳: 不稳定项在发布产品上默认用不了,需显式开门。原因: 5.1 的核心论点是试用不能变成事实依赖。cx 可用 `#pragma unisa` 的显式特性开关承担 feature gate 的角色,并由棘轮禁止 src/ 与已发布 ext 使用未稳定项。
- (3) 改造: 稳定化判据采用「报告三段 + 机械清单」,但把 FCP 换成可执行 gate。原因: Rust 的表决与异议期是为多人小组设计的;cx 是单集成者加多代理,等价物是 ledger 里的一行决定加一条可重跑的检查命令(批次全绿、六目标同候选证据、至少一个真实消费者)。「使用经验」这一条保留为硬判据: 无消费者不稳定。
- (4) 采纳: 预稳定期不给兼容承诺,稳定后是单向门。原因: 与现有 tlib-policy 的 stable no 默认与 y/x 封顶为 z 一致;Rust 的经验是门前重、门后简单(5.8 第 3 条)。稳定一项须在账本写明回滚不可行,所以要先过(3)。
- (5) 采纳: 把「什么算破坏」写成逐类清单并入表。原因: RFC 1105 证明这件事可以成文(5.6)。cx 没有 trait 与泛型,清单更短: 删项或改名、改参数个数或类型、改返回与错误合同、改所有权或释放责任、收紧前置条件算破坏;新增函数、新增错误种类(在调用者被要求留通配的前提下)算兼容。layer-api --bump 按此清单判 x/y/z。
- (6) 采纳: 错误模型分两层,可移植分类加原始系统码。原因: io::Error 的 kind() 与 raw_os_error() 让上层只看分类、诊断时仍能拿到原码(3.1)。cx 的 sy_* 返回值应统一成同一组分类码,原始 errno 另口取得;分类清单声明为可增长,调用者必须有兜底分支。
- (7) 采纳: 不支持的能力返回 Unsupported,符号不消失。原因: 4 的第 3 条使六目标的接口面完全一致,差异只在运行期结果,正好满足「上层不写平台 #if」。每个目标对每个核心项的状态(实现、Unsupported、未实跑)进事实表,未实跑不记 PASS。
- (8) 采纳: 平台特有能力单独成包,不混进核心面。原因: std::os::unix 的扩展 trait 让不可移植点在源码里显式可见(4 的第 4 条)。cx 对应做法是 cx.os 只放六目标都有语义的项,平台专有的进 ext.os.<目标> 一类扩展包,使用即显式 use。
- (9) 改造: 资源合同采用「每类型写明释放责任与析构错误去向」,但不照搬 RAII。原因: cx 没有编译期所有权与自动 Drop,运行期句柄检查不是同一性质(3.2)。可采纳的是其文档纪律: 每个返回句柄的函数在表里标明谁关、关失败如何报、重复关是否被拒;以及作用域式入口(打开、回调、必关)作为首选形状。
- (10) 不采纳: 六周火车与三通道。原因: 该节奏服务于大量外部贡献者与 crater 规模的生态回归;cx 已有快慢两条车道与每周一次慢车道,再加通道只增加候选身份的数量。保留其实质即可: 不稳定项只在开发候选可用。
- (11) 不采纳(现阶段): edition。原因: edition 解决的是海量存量代码面对语法不兼容的问题,且要求永久维护多套前端行为(5.7);cx 处于预稳定期,存量代码都在本仓,可用 refactor.cx 一次改完。条件: 当出现仓外消费者且语法需要不兼容变更时再议。
- (12) 采纳: 核心面保持小,准入先问「是否必须在核心」。原因: std 的准入线是需要语言或编译器配合、或是扩展已有核心类型,会持续演进的复杂 API 留在外面(2.10);net 只到阻塞 TCP/UDP,time 没有日历。cx 核心六层照此收口,其余进 ext.*;ext 可以独立定版,核心随编译器定版,两者的版本策略因此可以不同。

## 附: 本次未核实、落合同前须再查的点

- FCP 勾选门槛与 10 日时长(5.4)。
- 「稳定的 std 项永不删除」的成文出处(5.6)。
- sys/pal 的当前目录布局与 unsupported 实现的确切行为(4)。
- 第 2 节各函数名与 3.2 的各析构合同均为记忆,个别项的稳定状态可能已变。
- edition 的年份清单与节奏(5.7)。
