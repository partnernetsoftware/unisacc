# prd_cx_spec.md -- cx 使用规范(给 agent 与开发者)

本文件是「用 unisacc + cx 写工具和脚本」的权威入口:读完即可动手,不必再翻散落文档。结构、规模与路线仍在 [prd_cx.md](prd_cx.md);条款原文在 [cx/spec.md](cx/spec.md);逐函数表在 `build/std-surface.tsv` 与 [docs/cx/api/](docs/cx/api/README.md)。

取证口径:2026-11-14,仓根 `unisacc.com`,lnx/x86_64 本机。凡写「实测」的命令都在 `$CLAUDE_JOB_DIR/tmp/cxspec` 下跑过,末行输出照录;没跑的标 UNVERIFIED。osx/win 行为本文一律未测,以表内单元格为准。

## 1 定位

- cx 是 C99 加模块指令的方言:源文件后缀 `.cx`,函数体就是 C99,多出的只有 `#pragma unisa module / use / dock` 三种指令行(cx/spec.md CX-G-1;未知指令被拒绝,实测 `unknown unisa pragma frob`,rc=1)。
- 不需要构建系统:`unisacc.com` 读 `.cx`,沿 `use` 找到所有模块,编译并直接运行。写法像脚本,产物是原生代码。
- 所有权与边界目前是运行期检查(句柄、带界接口),不是编译期借用检查,不得宣称内存安全(prd_cx.md 0.1 非目标)。
- 什么时候用 cx:仓内一切工具、检查、一次性分析脚本、子代理取证脚本、构造与调度逻辑。入库与否都用 cx。
- 什么时候才用别的:`git`、`gh`、`mux`、`df` 等只读或版本控制命令直接用;shell 只作薄入口(一行调用),不承载算法;不新增 Python、bun、node 脚本。cx 缺能力时按第 5.2 节登记缺口,不退回 Python。

## 2 怎么运行

以下命令均实测(`T=$CLAUDE_JOB_DIR/tmp/cxspec`,从仓根执行):

| 用途 | 命令 | 输出末行 |
|---|---|---|
| 直接运行(默认即 -run) | `sh ./unisacc.com $T/ex1.cx -- $T/in.tsv n` | `rows=3 sum=12` |
| 落盘成可执行文件 | `sh ./unisacc.com $T/ex1.cx -o $T/ex1.bin` 然后 `$T/ex1.bin $T/in.tsv n` | `rows=3 sum=12` |
| 只编单元对象 | `sh ./unisacc.com -c -funit $T/ex1.cx -o $T/ex1.o` | rc=0,产物 266400 字节 |
| 看模块依赖计划 | `./unisacc.com -cx-plan $T/ex3.cx` | 逐行 `序号 路径 cx.h 接口头...` |
| 测试清单 | `sh ./unisacc.com tests/run.cx -- --list` | `PLAN metadata_valid=1 batches=1085 ...` |
| 改动影响哪些批次 | `sh ./unisacc.com tools/affected.cx -- --files ext/tsvx/read.cx --list-only` | `AFFECTED_SUMMARY changed=1 batches=1 scanned=1067` |
| 查调用点 | `sh ./unisacc.com tools/refactor.cx -- sites tv_col tools/check-tsvx-read.cx` | `tools/check-tsvx-read.cx:54 tv_col(0, "id", 2)` |
| 归档引用检查 | `sh ./unisacc.com tools/check-archive-refs.cx` | `ARCHIVE_REFS rows=157 live_references=3 failed=0` |

- `--` 之后是传给脚本 `main` 的参数。日常不要先落盘再运行;落盘只用于格式、链接、平台、自举验证。
- 受影响批次实跑:`tools/affected.cx -- --files F... --run [-j N]`(头注;本文只实测了 `--list-only`)。
- 每条命令及其子进程上限 60 秒;超时用 `timeout 60` 包住。

## 3 语言要点

### 3.1 不写 #include

新 `.cx` 不手写 `#include`。每个单元自动带隐式前导(`-cx-plan` 输出里的 `cx.h`),所以 `printf`、`strlen`、`int64_t` 等无需包含头文件即可用(三个例子均未写 include 而编过)。能否调用它们是另一回事,见 3.4。

### 3.2 module / use / dock

```
#pragma unisa module NAME            库模块声明;NAME 必须等于文件 stem(CX-M-1);库模块不得定义 main
#pragma unisa use NAME               导入 NAME 的全部公开符号(CX-U-1)
#pragma unisa use NAME::sym          只导入 sym(CX-U-2)
#pragma unisa use NAME::sym as alias 导入并改名(CX-U-3)
#pragma unisa dock                   本模块对接宿主,允许 #include 与裸 libc;只给 L0 用
```

- 使用方单元(带 `main` 的脚本)可以没有 `module` 行。
- `NAME` 的点号是子目录:`sys.fs` 即 `<某根>/sys/fs.cx`。先找使用方文件旁边;没有再向上找最近的 `cxroot.tsv`(包根),按其中 `R<TAB>前缀<TAB>目录` 行映射(CX-U-7)。仓根 `cxroot.tsv` 现有前缀:`sys mem text core runtime driver compiler workspace cx0 stream path os sysx crypto cxlib tsvx`。
- `net` 不在仓根 `cxroot.tsv` 里:`tools/` 下靠旁边的目录链接 `tools/net -> ../ext/net` 解析;别处使用要在自己的包根加一行(见 3.3)。
- 不传递:A `use` mid、mid `use` text.fmt,A 不能直接调 `tx_snprintf`;用到哪个模块就直接 `use` 它(CX-U-5)。
- 模块图必须无环,否则拒绝 `module cycle`(CX-M-3)。
- 符号级导入实测可用:`#pragma unisa use text.text::tx_len` 后调用 `tx_len("hello", 64)`,输出 `len=5`。
- 已知缺口(实测):`use text.text::tx_len as slen` 能通过规划,但默认运行与 `-o` 产物都在调用时报 `unisacc: no host function slen`(rc=127)。别名在当前产品上不可用,冲突请改用符号级导入避开。
- 再导出未实现:[plans/cx-use-alias-reexport.md](plans/cx-use-alias-reexport.md) 第 3 部分的 `#pragma unisa export` 实测被拒(`unknown unisa pragma export`)。被多处需要的底层模块,各使用方自己 `use`。
- `dock` 的完整设计见 [plans/cx-dock-pragma.md](plans/cx-dock-pragma.md);带参数写法 `dock libc|include` 本文未实测(UNVERIFIED)。

### 3.3 仓外脚本的包根(实测)

脚本放在仓外(如 `$CLAUDE_JOB_DIR/tmp`)时找不到仓根 `cxroot.tsv`,实测报 `module sys.fs: no file .../sys/fs.cx`。做法:在脚本目录放一个链接和一份自己的 `cxroot.tsv`。

```
ln -sfn /home/devali/repos/unisacc r        在脚本目录内
cxroot.tsv 内容:首行 CXROOT1,其后每行 R<TAB>sys<TAB>r/cx/sys 这样把仓根各行的目录加前缀 r/
需要网络时追加 R<TAB>net<TAB>r/ext/net
```

目录字段不得以 `/` 开头,也不得含 `..`(CX-U-7),所以必须经链接。

### 3.4 层与层向

| 层 | 目录 | 能否碰 libc/POSIX 与 #include |
|---|---|---|
| cx-L0 宿主对接 | `cx/sys`、`cx/mem`、`cx/text` | 能,且只有这里能(模块头写 `dock`) |
| cx-L1 机制 | `cx/core`、`cx/runtime` | 不能 |
| cx-L2 驱动与编译器 | `cx/driver`、`cx/compiler`、`src/` | 不能 |
| cx-L3 工具、测试、应用 | `tools`、`tests`、`apps` | 允许 printf/fprintf 给人看(本机跑,不跨目标);其余仍应走 L0 |
| EXT 包层 | `ext/<包>/`(sysx、crypto、net、tsvx、memx、textx) | 各包自述;产品层不得 `use` 到 ext |

- 层向只能向下:编号层向上越层即红,产品层用到 `ext/` 即红,检查是 `tools/check-cx-layers.cx`(本文未重跑,UNVERIFIED 其当前结果)。
- 产品代码(`src/`、`cx/core`、`cx/runtime`、`cx/compiler`、`cx/driver`)严格只用 `sy_*`、`mm_*`、`tx_*`,不写平台 `#if`(`__linux__`、`_WIN32`、`__APPLE__`、`__x86_64__`)和 `__syscallN`(CX-Y-2、CX-Y-3)。平台差异只放 L0 的 `_os` 静态帮手或表。
- 错误码只走返回值:L0 失败返回 `-errno`,不靠全局 `errno` 跨模块传递(CX-Y-4)。
- 包名与目录的对应(表里的 `pkg` 列是目标布局):`cx.sys`/`cx.os` 的函数现都在 `cx/sys`、`cx/os`,`use` 时按文件写,如 `sys.proc`、`os.utc`。`ext.memx` 的 arena 经链接出现在 `cx/mem/arena.cx`,写 `use mem.arena`;`ext.textx` 的 build 写 `use text.build`。

### 3.5 计划中语法(未实现,不可用)

本节只是规格草案与拒绝合同,设计全文在 [prd_cx.md](prd_cx.md) 第 10 节;export 的事实来源是 10.7(D1 设计)。下列写法在当前产品上**全部未实现、不可用**;写了会在规划期被具名拒绝(rc=1),不会被静默接受。下表"计划中的拒绝"列的文案同为设计(未实现,以 10.7 的 X1 至 X13 为准),不是现产品输出。计划版本 0.0.64,以届时的候选为准。

```
#pragma unisa export NAME[,NAME...]   计划(D1a,首版):裸名 export 只表示本模块自有、非 static 函数的公开名单;不是重导出
#pragma unisa export use MODULE       计划(D1b,排在 D1a 之后):门面重导出。直接 use 本模块的单元视同直接 use 了 MODULE;符号身份仍属 MODULE
#pragma unisa export use MODULE::sym  计划(D1b):只重导出 sym
#pragma unisa use PFX.*          计划:纯展开为 PFX 目录下每个直接子模块的 use;不递归;冲突仍拒;只许 tools/tests/apps
```

没有 export 的模块行为完全不变;有 export 的模块以源码内名单为准,迁移期与 `cxpack.tsv` 互校(10.7 X9)。

| 写法 | 现行为(2026-11-14 已装产品实测) | 计划行为 | 计划中的拒绝 |
|---|---|---|---|
| `export NAME`(裸名,D1a) | 拒绝 `unknown unisa pragma export` | 公开名单,见上 | 见 10.7 X1 至 X11(名字无定义、static、类型、重复、与 cxpack 不等、调用未导出函数等) |
| `export use MODULE[::sym]`(D1b) | 拒绝 `unknown unisa pragma export` | 门面重导出,见上 | 非模块单元写:`export outside module`;导出自己:`exports itself`;成环:`module cycle` |
| `export use M::sym as alias` | 拒绝 `unknown unisa pragma export` | 首版仍拒绝 | `export alias unsupported` |
| `use PFX.*` | 拒绝 `bad module name "PFX.*"` | 见上 | 空展开:`wildcard PFX.* matches no module`;产品层(`src/`、`cx/`、`ext/`)使用:棘轮拒绝;展开后超 48 条 use:沿用现上限拒绝 |
| `use *`、`use PFX.*::sym`、`use PFX.*.x` | 拒绝 `bad module name` | 永久拒绝 | 同左 |
| `use NAME::*` | 旧产品:被静默接受(规划 rc=0);源码已修(2026-11-14,提交 57c7427f6f):拒绝 `bad symbol name "*"` | 永久拒绝 | 同左 |

现行缺口(实测,与计划语法相邻,修复属产品输入):

CX-U-2 三条具名拒绝(源码已修,提交 57c7427f6f 与 68039f11f1;事实来源 `tests/cx-namespace.cx` 的 `symbol-star-refused`、`symbol-not-exported-refused`、`unimported-symbol-call-refused` 与 `tools/check-cxmod-tri.cx` 同名三例):

- (1) `use NAME::*` -> rc 1,文案 `bad symbol name "*"`。
- (2) `use NAME::nosuch`(模块未导出的符号)-> rc 1,文案 `module NAME does not export nosuch`。
- (3) `use aa::other` 之后调用未导入的 `clash()` -> rc 1,文案 `call to undeclared function clash`。符号级导入从此可以当作封装手段,但仅限新产品。
- 状态:三条在 tests/cx-namespace.cx 是真断言(rc 1 加具名文案)。装在本机的旧产品内嵌旧 cxmod,遇到接受时该批次打印 `UNVERIFIED needs-new-product`,不记 PASS;新产品仍接受则记 FAIL。三条的新产品验证待下一轮整链,现为 UNVERIFIED。
- 产品层禁止 `use X::*`:棘轮规则 `cx-wildcard-product`(`tests/cxinclude.cx`,提交 4966abbf9c),按行数 `#pragma unisa use` 目标以 `::*` 结尾者;产品层须逐名导入。
- 实现其余计划语法时,同批次的 `planned-*-refused-by-name` 断言要换成对应语义断言,不是删除。

cxmod 三源归一现状(事实来源:`tools/check-cxmod-tri.cx` 头注释与提交 68039f11f1、3761234e57):

- 权威源:`cx/cx0/plan.cx`(`CX0_COMPAT_BODY`,函数 `cx0_plan`)。`src/cxmod.c` 由 `cx0-extract` 从 plan.cx 逆投影生成,`--check` 判 EXACT(最近记录 bytes=73271),不手改。`cx/compiler/cxmod.cx`(`cxm_plan`)手工对齐。
- `sh ./unisacc.com tools/check-cxmod-tri.cx`:同一批夹具经 plan.cx 与产品 cxmod.cx 真跑,比 rc 与拒绝文案;C 源随 plan.cx 计,不另编译;已知分叉输出具名 GAP,不记 PASS 也不记 FAIL。
- `sh ./unisacc.com tools/cx0-diff.cx [-- --check|--pair A B|--out|--verify]`:三对源的 hunk 差异按六类归 (a)(b)(c)(d)(e)(f)(t);(e)(f) 为语义分叉(cxm_checkcalls 一族、上限 CXM_MAXU),其余为宿主差异;`--check` 断言无「其他」块;差异表 `build/cx0-three-source-diff.tsv`。设计背景见 prd_cx.md 10.5。

cx.stream 句柄与所有权合同(事实来源 `prd_cx.md` 10.6、`tools/check-cx-streamown.cx`,批次 `cx-streamown`):

- reader、writer、pipeline 句柄 close 后恒被拒;同槽重开 4096 次,gen 逐次加 1(gen 回绕未测,UNMEASURED:需 4194304 次重开)。
- `xs_reader_fd`/`xs_writer_fd` 借用 fd(own 0),关句柄不关 fd;`xs_pipe_pair`(own 1)关两个句柄即关两个 fd。
- stage 在自己所在的运行中管线上调 `xs_run`/`xs_add`/`xs_close` 返回 4。
- 已知风险 KNOWN-RISK-DOUBLE-FREE-ON-ERROR:`xs_item_set` 参数非法返回 4 时仍释放调用方的 data;若调用方把 4 当「所有权未转移」再释放,即双重释放。检查中按现行为固定,未修。
- 10.6 的整体重构设计未实现,不得记 PASS。

封网新增接口索引(只引用,签名与文案见生成页,不手改):`sy_snm_sendto`、`sy_snm_recvfrom`(netmsg)、`sy_snb_get`、`sy_snb_set_nonblock`(netnb)、`sy_sig_watch`、`sy_sig_taken`(sigwatch),见 [docs/cx/api/cx.sys.md](docs/cx/api/cx.sys.md);`sy_resolve`、`sy_tcp_connect_to`、`sy_tcp_accept_from`、`sy_tcp_dial`、`sy_udp_*`、`sy_unix_listen`/`sy_unix_connect`、`sy_recv_until`、`sy_send_all_until`(带 `sy_deadline_t` 截止时间),见 [docs/cx/api/ext.net.md](docs/cx/api/ext.net.md)。osx/win 行为未验证,看各行的 lnx/osx/win 列。

## 4 标准库速查

函数名全部取自 `build/layer-api/*.tsv`,第 9 节列出核对结果。「use」列是要写的模块名;签名与错误合同看「页」列对应的 `docs/cx/api/<页>.md`(生成物,勿手改)。

### 4.1 文件与路径

| 函数 | use | 一行用法 | 页 |
|---|---|---|---|
| `sy_read_file` | sys.fs | `sy_read_file(path, &buf, &n, max)` 整读,成功返回 0,用完 `mm_free(buf)` | cx.os |
| `sy_write_file`、`sy_write_atomic` | sys.fs | `sy_write_atomic(path, buf, n, 0644)` 临时文件加改名发布 | cx.os |
| `sy_exists`、`sy_stat`、`sy_file_size` | sys.fs、os.osx | 存在性只把 ENOENT 当不存在,其余错误要报告 | cx.os |
| `sy_mkdir_all`、`sy_rename`、`sy_unlink` | sys.fs | 返回 0 或 `-errno` | cx.os |
| `sy_dir_open`、`sy_dir_next`、`sy_dir_close` | sys.posix | `sy_dir_t d; sy_dir_open(&d, path); while (sy_dir_next(&d) == 1) 用 d.name` | cx.os |
| `sy_open_read`、`sy_read`、`sy_write_all`、`sy_close` | sys.fs | 需要 fd 级读写时用 | cx.os |
| `sy_mkstemp`、`sy_abspath`、`sy_realpath`、`sy_cwd` | os.osx、sys.fs、sys.env | `sy_mkstemp(dir, prefix, path, cap)` 返回 fd | cx.os |
| `pt_join`、`pt_dirname`、`pt_basename`、`pt_ext`、`pt_normalize` | path.path | `pt_join(dst, cap, a, b)`;取段的函数给出 `off`、`len`,不分配 | cx.path |
| `xs_reader_fd`、`xs_read_line` | stream.rw | 大文件逐行读,不整读入内存 | cx.stream |

### 4.2 进程与管道

| 函数 | use | 一行用法 | 页 |
|---|---|---|---|
| `sy_spawn_init`、`sy_proc_run` | sys.proc | 填 `sy_spawn_t`(argv、cwd、`deadline_ms`、`pgroup`),同步跑完得 `sy_exit_t` | cx.os |
| `sy_proc_spawn`、`sy_proc_wait`、`sy_proc_try_wait`、`sy_proc_kill` | sys.proc | 异步起子进程,轮询或等待 | cx.os |
| `sy_exit_code` | sys.proc | 归一退出码:超时 124,被信号杀 128 加信号号 | cx.os |
| `sy_run`、`sy_spawn_pipe`、`sy_pipe` | sys.proc | `sy_run(argv, out, cap, &cut)` 取子进程输出到缓冲 | cx.os |
| `np_run`、`np_free` | sysx.process | 带秒级上限并分别收 stdout、stderr 到 `np_result` | ext.procx |
| `sy_kill_tree`、`sy_ncpu` | sysx.process | 按真实 PID 清理进程树;取核数定并行度 | ext.procx |

进程只收 argv 向量,不经 shell 拼命令行(表头 `out popen-with-shell`);要 shell 语义就显式写 `{"sh", "-c", ...}`。

### 4.3 文本与 TSV 扫描

| 函数 | use | 一行用法 | 页 |
|---|---|---|---|
| `tx_len`、`tx_eq`、`tx_eqz`、`tx_same` | text.text | 带界长度与比较;`tx_len(s, cap)` | cx.text |
| `tx_find`、`tx_trim` | text.text | `tx_find(s, n, p, pn)` 返回偏移,未中 -1 | cx.text |
| `tx_iter_init`、`tx_line_next`、`tx_split_next` | text.iter | 逐行、按分隔符迭代,字段以 `it.off`、`it.len` 指回原串 | cx.text |
| `tx_replace`、`tx_lower`、`tx_ieq` | text.edit | `tx_replace(dst, cap, s, n, from, fn, to, tn)` 返回新长度,装不下 `-ERANGE` | cx.text |
| `tx_to_i64`、`tx_from_i64` | text.conv | `tx_to_i64(s, n, 10, &v)` 成功返回 0 | cx.text |
| `tx_snprintf` | text.fmt | 产品代码里代替 snprintf | cx.text |
| `tv_row_next`、`tv_field`、`tv_col`、`tv_expect_cols` | tsvx.read | 逐行读 TSV(跳空行与 `#` 行),按列名取列号,整表列数 lint | ext.tsvx |
| `sb_new`、`sb_puts`、`sb_putn`、`sb_cstr`、`sb_free` | text.build | 可增长字符串拼装 | ext.textx |

### 4.4 哈希与编码

| 函数 | use | 一行用法 | 页 |
|---|---|---|---|
| `sy_sha256_hex` | sys.sha | `char hex[65]; sy_sha256_hex(data, n, hex)`(表内是 `cs_sha256_hex` 的别名) | ext.crypto |
| `cs_sha256_init`、`cs_sha256_update`、`cs_sha256_final`、`cs_sha256_hex`、`cs_hmac_sha256_hex` | crypto.sha256 | 流式摘要与 HMAC | ext.crypto |
| `tx_hex_encode`、`tx_b64_encode` | text.codec | 十六进制与 base64 | cx.text |
| `dz_crc32` | text.deflate | crc32;另有 deflate、gzip 压缩 | ext.textx |

### 4.5 内存与 arena

| 函数 | use | 一行用法 | 页 |
|---|---|---|---|
| `mm_alloc`、`mm_zalloc`、`mm_grow`、`mm_free`、`mm_strdup` | mem.heap | 代替 malloc 族 | cx.mem |
| `mm_copy` | mem.bytes | `mm_copy(dst, dcap, src, n)` 带目标容量 | cx.mem |
| `mg_init`、`mg_append`、`mg_free` | mem.grow | 可增长字节缓冲 `mg_buf` | cx.mem |
| `mm_map_new`、`mm_map_set`、`mm_map_get` | mem.map | 字符串键 map,句柄是 int | cx.mem |
| `ar_new`、`ar_alloc`、`ar_dup`、`ar_drop` | mem.arena | `int a = ar_new(limit)`,一次 `ar_drop(a)` 全部释放 | ext.memx |

### 4.6 时间

| 函数 | use | 一行用法 | 页 |
|---|---|---|---|
| `sy_now_ms`、`sy_mono_ns`、`sy_sleep_ms` | sys.time | 计时用单调钟;不要忙等 | cx.os |
| `sy_deadline_in`、`sy_deadline_left`、`sy_deadline_passed` | sys.time | `sy_deadline_t d = sy_deadline_in(ms)`,循环同时带次数上限 | cx.os |
| `sy_utc_fmt`、`sy_utc_parse` | os.utc | UTC 文本与秒互转;不做时区 | cx.os |

### 4.7 网络套接字

| 函数 | use | 一行用法 | 页 |
|---|---|---|---|
| `sy_tcp_listen`、`sy_tcp_port`、`sy_tcp_connect` | net.tcp | `sy_tcp_listen(0, backlog)` 取临时端口;`sy_tcp_connect(port)` 连回环 | ext.net |
| `sy_send_all`、`sy_recv` | net.tcp | 收发 | ext.net |
| `sy_tcp_accept`、`sy_wait_port`、`sy_shutdown` | net.sock | `sy_tcp_accept(fd, ms)` 带超时 | ext.net |
| `sy_addr_parse` | net.addr | 文本地址到 `sy_addr_t` | ext.net |

实测:`use net.tcp` 的回环 listen、port、connect 输出 `listen_ok=1 port_ok=1 connect_ok=1`。收发、accept 超时本文未实测(UNVERIFIED),以 `tools/check-ext-net-tcp.cx`、`tools/check-ext-net-sock.cx` 为准。

### 4.8 并发、超时与环境

| 函数 | use | 一行用法 | 页 |
|---|---|---|---|
| `sy_poll` | sysx.poll | 多 fd 等待,带毫秒超时 | cx.sys |
| `sy_readable` | sys.time | 单 fd 可读等待 | cx.os |
| `sy_sock_poll` | sys.netpoll | 单套接字等待;在 layer-api 里,`std-surface.tsv` 尚无行,无 API 页 | 无 |
| `sy_env`、`sy_env_or`、`sy_setenv` | sys.env | 环境变量 | cx.os |
| `sy_printf`、`sy_eprintf`、`sy_strerror`、`sy_exit`、`sy_err_kind` | sys.io、sys.sig、sys.errkind | 产品代码的输出与错误分类 | cx.sys |

没有线程、互斥、事件循环:并发靠多进程加 `sy_poll`,定时只有 deadline(表头 `out threads`、`out async-loop`)。

## 5 替代 python / bun / node / shell

### 5.1 对照表

| 脚本任务 | cx 做法 | 现成工具 |
|---|---|---|
| 读写文件 | `sy_read_file` 整读;`sy_write_atomic` 发布 | `tools/edit-span.cx`(按行段或按唯一文本替换,先断言再写) |
| TSV 处理 | `tx_iter_init` 加 `tv_row_next`、`tv_col`、`tv_field` | `tools/tables-gen.cx`、`tools/register.cx`、`tools/commit-own.cx --keyed` |
| grep 式扫描 | `tx_line_next` 逐行加 `tx_find` | `tools/refactor.cx sites`(调用点)、`tools/symindex.cx -- def NAME`(定义) |
| 批量机械改写 | 例二的模式:找全、算容量、`tx_replace`、原子发布 | `tools/refactor.cx find/apply/sweep`(每文件改前改后验证,不过就还原)、`rename` |
| 起子进程并带超时 | `sy_spawn_t.deadline_ms` 加 `sy_proc_run`;收输出用 `np_run` | `tools/affected.cx --run`(并行重跑受影响批次) |
| sha256 | `sy_sha256_hex`;大输入用 `cs_sha256_update` 流式 | 无需外部 `sha256sum` |
| JSON 或文本拼装 | `sb_puts`、`sb_putn` 拼装;工具脚本可直接 `printf` | 无 |
| HTTP 或套接字 | TCP 用 `net.tcp`、`net.sock` | 无 |
| 一次跑完棘轮 | 不用 shell 循环 | `tools/ratchets.cx` |

### 5.2 已知限制(如实)

- 没有 JSON 解析与序列化库:只能手工拼装与手工扫描。
- 没有 HTTP 客户端与服务端、没有 TLS 高层接口、没有 URL 解析(表头 `out TLS/HTTP/URL`);`ext.crypto` 只有 TLS 1.3 记录层等底层件。
- 没有正则:只有子串查找、分隔迭代与修剪。
- 没有线程与事件循环;没有泛型容器,只有 `mm_vec_*` 与字符串键 `mm_map_*`。
- 没有时区、locale、浮点格式化选项;inflate 解压不在面内(只有 deflate、gzip 压缩与 crc32)。
- `use ... as alias` 与 `export` 在当前产品上不可用(3.2)。
- 仓外脚本需自备包根(3.3)。
- 遇到缺口:保留最小默认运行复现,写进 [prd_mempalace.md](prd_mempalace.md) 的「阻塞与未验证」一节。`agentwork/pkg-gaps.tsv` 的记录方式已退役,不再追加。不因缺口退回 Python 或 shell 算法。

## 6 三个从零写的例子(均实测)

源码在本机 `$CLAUDE_JOB_DIR/tmp/cxspec/ex1.cx`、`ex2.cx`、`ex3.cx`(临时目录,不入库);下面是可照抄的全文或骨架。

### 6.1 统计 TSV 列

```c
#pragma unisa use sys.fs
#pragma unisa use text.iter
#pragma unisa use text.conv
#pragma unisa use tsvx.read
int main(int argc, char **argv) {
    unsigned char *b = 0; int64_t n = 0, v, sum = 0; tx_iter_t it; int cols, cn, rows = 0; long o, l;
    if (argc != 3) { fprintf(stderr, "usage: ex1 FILE COLNAME\n"); return 2; }
    if (sy_read_file(argv[1], &b, &n, 1 << 20)) { fprintf(stderr, "cannot read %s\n", argv[1]); return 2; }
    tx_iter_init(&it, (const char *)b, (long)n);
    if (tv_row_next(&it, &cols) != 1) return 2;                 /* 表头行 */
    cn = tv_col(&it, argv[2], (long)strlen(argv[2]));
    if (cn < 0) { fprintf(stderr, "no column %s\n", argv[2]); return 2; }
    while (tv_row_next(&it, &cols) == 1) {                      /* 自动跳过空行与 # 行 */
        if (tv_field(&it, cn, &o, &l)) return 2;
        if (tx_to_i64((const char *)b + o, l, 10, &v)) return 2;
        sum += v; rows++;
    }
    printf("rows=%d sum=%lld\n", rows, (long long)sum);
    return 0;
}
```

命令:`sh ./unisacc.com $T/ex1.cx -- $T/in.tsv n`(输入 3 数据行加 1 注释行,列 n 为 3、4、5)。输出末行:`rows=3 sum=12`。

### 6.2 批量替换断言文本

```c
#pragma unisa use sys.fs
#pragma unisa use text.text
#pragma unisa use text.edit
#pragma unisa use mem.heap
int main(int argc, char **argv) {
    int i, changed = 0; long fn, tn;
    if (argc < 4) { fprintf(stderr, "usage: ex2 FROM TO FILE...\n"); return 2; }
    fn = tx_len(argv[1], 4096); tn = tx_len(argv[2], 4096);
    if (fn <= 0 || tn < 0) return 2;
    for (i = 3; i < argc; i++) {
        unsigned char *b = 0; int64_t n = 0; char *out; long cap, m, at = 0, hits = 0, p;
        if (sy_read_file(argv[i], &b, &n, 1 << 24)) { fprintf(stderr, "cannot read %s\n", argv[i]); return 2; }
        while ((p = tx_find((const char *)b + at, (long)n - at, argv[1], fn)) >= 0) { hits++; at += p + fn; }
        if (!hits) { printf("SAME\t%s\n", argv[i]); mm_free(b); continue; }
        cap = (long)n + hits * (tn > fn ? tn - fn : 0) + 1;    /* 先算容量再改写 */
        out = mm_alloc(cap); if (!out) return 2;
        m = tx_replace(out, cap, (const char *)b, (long)n, argv[1], fn, argv[2], tn);
        if (m < 0 || sy_write_atomic(argv[i], out, m, 0644)) { fprintf(stderr, "cannot rewrite %s\n", argv[i]); return 1; }
        printf("REPLACED\t%s\t%ld\n", argv[i], hits); changed++;
        mm_free(out); mm_free(b);
    }
    printf("files=%d changed=%d\n", argc - 3, changed);
    return 0;
}
```

命令:`sh ./unisacc.com $T/ex2.cx -- old-name new-name $T/w/a.txt $T/w/b.txt $T/w/c.txt`。输出末行:`files=3 changed=2`(a 命中 1 处、b 命中 2 处、c 报 SAME;改后 b 内容已核为 `new-name`)。仓内真实改写优先用 `tools/refactor.cx` 与 `tools/edit-span.cx`,它们带改前断言与验证还原,本例只演示写法。

### 6.3 带超时的子进程与 sha256

```c
#pragma unisa use sys.proc
#pragma unisa use sys.time
#pragma unisa use sys.sha
int main(int argc, char **argv) {
    sy_spawn_t o; sy_exit_t st; char hex[65]; long t0, ms; int r;
    if (argc < 3) { fprintf(stderr, "usage: ex3 MS CMD [ARG...]\n"); return 2; }
    ms = atol(argv[1]);
    sy_spawn_init(&o); o.argv = argv + 2; o.pgroup = -1; o.deadline_ms = ms;   /* 到点杀整个进程组 */
    t0 = sy_now_ms();
    r = sy_proc_run(&o, &st);
    if (r) { fprintf(stderr, "spawn failed rc=%d\n", r); return 2; }
    if (sy_sha256_hex("abc", 3, hex)) return 2;
    printf("timed_out=%d code=%d elapsed_lt_5s=%d sha256(abc)=%.16s\n", st.timed_out, sy_exit_code(&st), sy_now_ms() - t0 < 5000, hex);
    return 0;
}
```

- 命令:`sh ./unisacc.com $T/ex3.cx -- 300 sh -c 'sleep 30'`。输出末行:`timed_out=1 code=124 elapsed_lt_5s=1 sha256(abc)=ba7816bf8f01cfea`。
- 命令:`sh ./unisacc.com $T/ex3.cx -- 3000 sh -c 'exit 7'`。输出末行:`timed_out=0 code=7 elapsed_lt_5s=1 sha256(abc)=ba7816bf8f01cfea`。

## 7 规则与禁区

- (1) 产品代码不直接调 libc/POSIX,不写平台 `#if`,不写 `__syscallN`;工具与测试脚本只放宽到 printf/fprintf 输出。
- (2) 改仓内任何文件(源码、表、文档、账本)只用 Edit/Write 或 `.cx` 工具(`tools/edit-span.cx`、`tools/refactor.cx`);不用 sed、awk、perl、临时 Python 或 TypeScript。
- (3) 分析脚本也用 `.cx`,入库与否都一样;不新增 Python 算法或 shell 算法,shell 只作薄入口。
- (4) 模块私有状态:模块内部状态写成 `static`,对外只给函数;跨模块传的是 int 句柄(如 `ar_new`、`mm_map_new`、`xs_open` 的返回)或调用方提供的缓冲。
- (5) 句柄生命周期:谁创建谁释放,配对调用(`ar_new`/`ar_drop`、`sb_new`/`sb_free`、`sy_dir_open`/`sy_dir_close`、`sy_proc_spawn`/`sy_proc_wait`);失败路径不得留下半开 fd(CX-Y-4)。释放责任看表内 `own` 列。运行期句柄检查不等于编译期所有权。
- (6) 带界接口:写入类函数都带目标容量,装不下返回负码而不是截断;调用方必须查返回值。
- (7) 新工具入库:放 `tools/`,带 `check-*.cx`,用 `tools/register.cx` 登记批次,并在 prd.md 能力速查补一行;建名前先 `git log --all --oneline -- tools/NAME` 查历史。
- (8) 改过的每个 `.cx` 提交前用已装产品实编一遍,或跑 `tools/affected.cx --files ... --run`。
- (9) 别的车道的文件(`apps/csih/`、`ujs/`)不直接改。
- (10) 不通过删断言、缩覆盖、放宽时限制造绿;未实跑不记 PASS。

## 8 稳定性与进化指针

- 策略全文:[docs/cx/stability.md](docs/cx/stability.md)。何时稳定由 `tools/stable-eval.cx` 求值,不由人裁量;现全局策略为 `stable no`,即所有包都无兼容承诺。
- `build/std-surface.tsv` 的 `status` 列:`exists` 已导出且签名与 layer-api 逐字相同;`new` 表里定了但尚无导出,不能调用;`alias-of:NAME` 是同一合同的别名。现分布(2026-11-14 统计):exists 509、new 16、alias-of 3。
- `stab` 列:`incubating`(`ext.*`,无承诺)190、`pre`(`cx.*` 预稳定,无承诺)301、`legacy` 25、`internal` 12;另有 `stable`、`deprecated:CXDEPnnnn` 两种取值,现无行。
- `lnx/osx/win` 列:`impl` 已实现;`refuse:原因` 调用返回该错误,不模拟;`unverified` 未实测。跨目标前先看这三列。
- 改面流程:加或改 `build/std-surface.tsv` 的行 -> `tools/layer-api.cx --bump` 判类升版 -> `tools/surface-doc-gen.cx` 重生 `docs/cx/api`;检查 `tools/check-std-surface.cx`(本文未跑这三条,UNVERIFIED)。
- 设计依据与参照调研:[docs/cx/design-core.md](docs/cx/design-core.md)、`docs/cx/ref-rust.md`、`ref-node.md`、`ref-quickjs.md`、`ref-cpython.md`;模块与层的展开讨论:[plans/cx-modules.md](plans/cx-modules.md)、[plans/cx-layers.md](plans/cx-layers.md)。

## 9 核对记录

- 速查函数名共 119 个,逐个执行 `cat build/layer-api/*.tsv | grep -c "^NAME<TAB>"`:ok=119、bad=0。
- 同一批名字在 `build/std-surface.tsv` 第 2 列精确匹配:117 个有行;`sy_sock_poll` 无行(已在 4.8 注明);`tx_fmt` 无行,故未列入速查。
- 「页」列用 `grep -l "^| NAME |" docs/cx/api/*.md` 抽查 17 个名字,归属与表一致;`sy_sock_poll` 无页。
- UNVERIFIED 汇总:osx/win 行为;`dock` 带参数写法;`check-cx-layers` 当前结果;`affected --run`;网络收发与 accept 超时;改面三条命令;gcc_cx、clang_cx 宿主。
