---
name: unisacc-cx
description: 写脚本、自动化、批处理、文本与文件处理、TSV 表、子进程、测试夹具、构建胶水、小型本机服务工具时，优先用 `sh ./unisacc.com FILE.cx -- ARGS`（unisacc 自带的 .cx 方言，直接运行、不落盘），而不是 python / node / bun。覆盖：三分钟上手、python/node 惯用法到 cx 模块的迁移对照表（每条实跑）、架构规则（不直调 libc、有界 API、≤60 秒）、模块化与内存安全现状、常用工具箱、常见坑、逐步迁移路线。不适用：需要大型三方生态（numpy/pandas、浏览器、ORM、HTTP 框架）、JSON/正则/哈希表这些 cx 还没有的模块——先查 `cx/`、`ext/`、`tools/cxlib/` 有没有，没有就回落到原语言并把缺口登记到 `agentwork/items.tsv`，不要假装有。
---

# unisacc + .cx：把脚本从 python/node/bun 换过来

每条断言来自仓库文件或 2026-11-06 在已装产品 `unisacc 0.0.44`（dev-lnx，`sh ./unisacc.com --version`）上的实跑。
标 ```cx-run 的块由 `tests/check-skill-cx.cx` 实跑，输出必须与紧随的 ```expect 块逐字节一致；文档过期就红。
示例源码在 `skills/unisacc-cx/examples/`，每个 ≤40 行。

## 0. 什么时候选它

| 场景 | 选 |
|---|---|
| 一次性文本/TSV 处理、文件改写、测试夹具生成、构建胶水、子进程编排、哈希/校验、小 CLI | **cx**（本文） |
| 要 numpy/pandas/浏览器/ORM/HTTP 框架、或要 JSON/正则/哈希表且今天就要 | 先查 `cx/ ext/ tools/cxlib/`；没有 → 回落原语言，并在 `agentwork/items.tsv` 立 DRAFT 缺口条目 |
| 另一个车道的文件（`apps/csih/`、`ujs/`） | 不动，发信封给其维护者 |

为什么是 cx：不用装运行时（一个文件 `unisacc.com` 就是编译器和解释器）；输出可写成六目标可执行文件；模块边界、带界 API 和运行期所有权检查由产品和棘轮一起守着；工具沉淀到 `tools/` 后别的代理能复用。

## 1. 三分钟上手

- 运行：`sh ./unisacc.com FILE.cx -- ARGS`。默认就是 `-run`，在内存里编译并运行，不落 `a.out`，不要先编译再运行。`--` 之后的词全部给程序；源文件之后 `--` 之前的 `-o/-b` 仍是编译器选项。
- 从仓库根运行。`#pragma unisa use NAME` 先在源文件旁边找 `NAME.cx`，找不到按最近祖先的 `cxroot.tsv` 映射（`sys`→`cx/sys` 等 12 条前缀）。仓外的 `/tmp/x.cx` 没有 `cxroot.tsv`，`use sys.io` 报 `module sys.io: no file /tmp/sys/io.cx`（实跑）。
- 不手写 `#include`。前导 `cx.h` 隐式带 libc 子集与 `cx_read/cx_write/cx_runv_timeout/cx_walk/cx_path_join/cx_rmtree`（CX-A-4 认定为 L0）。其余经 `use`。
- 耗时（本机实跑，`date +%s%N` 计）：热运行 hello.cx 21–22 ms；改一行后第一次运行 1847 ms（重编该单元并缓存）；带纯 cx sha256 的 hash.cx 热运行 35 ms；对照 `python3 -c 'print(1)'` 18 ms。
- 看模块依赖计划：`sh ./unisacc.com -cx-plan FILE.cx`（每行：序号、文件、前导、CXIF 临时头）。

stdin、argv、退出码一次看完（`examples/hello.cx`）：

```cx-run
printf 'abc' | sh ./unisacc.com skills/unisacc-cx/examples/hello.cx -- Ada Bob; echo rc=$?
sh ./unisacc.com skills/unisacc-cx/examples/hello.cx </dev/null 2>/dev/null; echo rc=$?
```

```expect
hello, Ada
hello, Bob
stdin bytes=3
rc=0
stdin bytes=0
rc=3
```

```cx-run
sh ./unisacc.com -cx-plan skills/unisacc-cx/examples/use_kv.cx | cut -f1,2
```

```expect
0	skills/unisacc-cx/examples/use_kv.cx
1	cx/sys/io.cx
2	skills/unisacc-cx/examples/kv.cx
3	cx/mem/bytes.cx
4	cx/text/text.cx
```

## 2. 迁移对照表（python / node → cx）

模块名写在 `#pragma unisa use` 后；函数在 `cx/sys/README.md`、`cx/mem/README.md`、`cx/text/README.md` 逐个列出。返回约定：L0 返回 0 或 `-errno`，不靠全局 errno。

| # | python / node 惯用法 | cx 模块 → API | 示例 |
|---|---|---|---|
| 1 | `print()` / `console.log` / `sys.stderr.write` | `sys.io` → `sy_printf`、`sy_puts`、`sy_eprintf` | hello.cx |
| 2 | `sys.argv` / `process.argv`、`sys.exit(n)` | `main(argc, argv)`，`return n`；`sys.sig` → `sy_exit(n)` | hello.cx |
| 3 | `sys.stdin.read()` / `process.stdin` | `sys.fs` → `sy_read(0, buf, n)` 循环 | hello.cx |
| 4 | `open().read()` / `fs.readFileSync` | 前导 `cx_read(path, &n)`（malloc 整文件，NUL 结尾）；流式 `sys.fs` → `sy_open_read`+`sy_read_full` | files.cx |
| 5 | `open('w').write` / `fs.writeFileSync` | 前导 `cx_write(path, buf, n)`；追加/分块 `sy_open_new`+`sy_write_all` | files.cx |
| 6 | `os.stat` / `os.path.exists` / `fs.statSync` | `sys.fs` → `sy_stat(path,&st)`（`st.size/kind('f','d','l')/mtime/mode`）、`sy_exists` | files.cx |
| 7 | `os.remove` / `os.rename` / `os.mkdir` / `os.rmdir` | `sys.fs` → `sy_unlink`、`sy_rename`、`sy_mkdir(path,0700)`、`sy_rmdir` | files.cx、tmpdir.cx |
| 8 | `os.path.join` / `path.join`、`realpath` | 前导 `cx_path_join(dst,cap,dir,name)`（溢出返回非 0）；`sys.fs` → `sy_realpath` | files.cx |
| 9 | `os.walk` / `glob` | 前导 `cx_walk(dir, fn, ctx)`：按名序深度优先，回调只收常规文件 | — |
| 10 | `shutil.rmtree` | 前导 `cx_rmtree(path)`；本文规则：清理只删确切路径，示例用 `sy_unlink`+`sy_rmdir` | tmpdir.cx |
| 11 | `tempfile.mkdtemp` | **缺**（见 §2.1）；替代：`sy_mkdir` 原子创建、`-EEXIST` 换名重试 | tmpdir.cx |
| 12 | `os.environ.get(k, d)` / `process.env` / `os.getcwd` | `sys.env` → `sy_env`、`sy_env_or`、`sy_setenv`、`sy_cwd` | env.cx |
| 13 | `subprocess.run(argv, capture_output=True)` / `execFileSync` | `sys.proc` → `sy_run(argv,out,cap,&cut)` 返回退出码或 `-errno`；带超时 前导 `cx_runv_timeout(argv,secs,&out,&n)`（124=超时，整进程组被杀） | subprocess.cx |
| 14 | `subprocess.Popen(stdout=PIPE)` / `spawn` | `sys.proc` → `sy_spawn_pipe(argv,cwd,flags,&ofd,&efd)` + `sy_wait_status`；超时收整棵树 `sys.process` → `np_run`、`sy_kill_tree` | tools/commit-own.cx |
| 15 | `hashlib.sha256().hexdigest()` / `crypto.createHash` | `sys.sha` → `sy_sha256_hex`（有 AF_ALG 用内核）；纯 cx `crypto.sha256` → `cs_sha256_hex`、`cs_hmac_sha256` | hash.cx |
| 16 | `str.split('\t')` / `line.split` | `text.text` → `tx_fields(s,n,c)` 计数、`tx_field(s,n,c,i,&off,&len)` 借用第 i 段（不复制） | fields.cx |
| 17 | `str.strip` / `startswith` / `in` / `find` / `==` | `text.text` → `tx_trim`、`tx_prefix`、`tx_suffix`、`tx_contains`、`tx_find`、`tx_eqz`、`tx_copy(dst,cap,src,n)` | fields.cx |
| 18 | `int(s)` / `float(s)` / `parseInt` | `text.num` → `tx_strtol_r(s,&end,base,&v)`（返回 0 才是数）、`tx_strtod_r` | fields.cx |
| 19 | `f"..."` / 模板字符串 | `text.fmt` → `tx_snprintf(dst,cap,fmt,...)`；保留返回值（cxtrunc 棘轮） | fields.cx |
| 20 | `bytearray` / `Buffer.alloc` 增长 | `mem.grow` → `mg_init/mg_append/mg_push/mg_pop/mg_move/mg_take/mg_free`（Vec<u8>） | vec.cx |
| 21 | `memoryview` / 切片 `b[6:11]` | `mem.slice` → `ms_of(&buf)`、`ms_at(&s,off,n,&sub)` 越界拒绝、`ms_find`、`ms_starts_with`（&[u8]） | vec.cx |
| 22 | `''.join(parts)` / `Array.join` / `+=` 拼串 | `text.build` → `sb_new/sb_join/sb_puts/sb_putc/sb_truncate/sb_cstr/sb_free`（String） | strbuild.cx |
| 23 | `malloc`/`memcpy` 式手工内存 | `mem.heap` → `mm_alloc/mm_zalloc/mm_grow/mm_free`；`mem.bytes` → `mm_copy(dst,dcap,src,n)` 越界拒绝、`mm_same`、`mm_fill` | check-skill-cx.cx |
| 24 | `time.time()` / `Date.now()` / `strftime` / `sleep` | `sys.time` → `sy_now_ms`（单调）、`sy_wall_s`、`sy_local_fmt(out,cap,fmt,sec)`、`sy_sleep_ms` | clock.cx |
| 25 | `csv` 模块 | 没有通用 CSV 模块；现成管道 `tools/csv2tsv.cx`（`tx_fields`+`sb_*`），TSV 用第 16 条 | — |
| 26 | `zlib.compress` | `text.deflate`（deflate/gzip，调用方给输出缓冲） | — |
| 27 | `socket` / `net.createServer` | `sys.net` → `sy_tcp_listen/sy_tcp_accept(fd,ms)/sy_tcp_dial(host,port)/sy_tcp_connect`；TLS 1.3 纯 cx `crypto.tls13` | — |
| 28 | `base64` | `crypto.b64` | — |

### 2.1 缺口清单（今天没有的，不要假装有）

| 缺 | 现状 / 替代 |
|---|---|
| JSON 解析与生成模块 | `cx/`、`ext/`、`tools/cxlib/` 都没有 `json` 模块（只有 `apps/csih/json.cx`、`release/ci/json.cx` 是别的车道/发布脚本的私有实现）。结构化数据用 TSV（第 16 条）；必须读 JSON 时回落并登记 |
| 正则 | 没有。用 `tx_find/tx_prefix/tx_fields` 组合 |
| 哈希表 / 通用容器 | 没有。小表用定长数组加线性查找（kv.cx）；大表登记缺口 |
| `mkdtemp` 的 `sy_*` 封装 | 只有前导 libc `mkdtemp`（上层新文件调它会计入 cxlibc 棘轮）；用 `sy_mkdir` 重试（tmpdir.cx） |
| HTTP 客户端/服务端 | 只有 TCP（`sys.net`）与 TLS 1.3 记录层（`crypto.tls13/record`），没有 HTTP 解析 |
| 同一单元同时 `use mem.grow` 与 `use text.build` | 已装 0.0.44 报 `mm_alloc exported by both heap (cx/mem/heap.cx:15) and mem.heap (cx/text/mem/heap.cx:15)`（实跑，`tools/practice-vec.cx` 同样 rc=1）；根因是软链农场的两种路径拼法，包根解析 CX-U-7 要产品重装后才生效。规避：拆成两个单元（vec.cx / strbuild.cx） |
| 仓外脚本 | 没有 `cxroot.tsv` 就不能 `use`；仓外目录要自带一份 `cxroot.tsv` 指回模块目录 |
| 编译期所有权/借用检查 | 未实现（CX-W-4），见 §4 |

### 2.2 对照表示例实跑

```cx-run
sh ./unisacc.com skills/unisacc-cx/examples/files.cx -- /tmp
SKILL_DEMO=1 sh ./unisacc.com skills/unisacc-cx/examples/env.cx
sh ./unisacc.com skills/unisacc-cx/examples/subprocess.cx
```

```expect
exists=1 size=23 kind=f
line0=first line
unlink=0 exists=0
SKILL_DEMO=1
MISSING_VAR is null: 1
cwd is absolute: 1
echo rc=0 cut=0 out=[a	b
]
fail rc=7
slow rc=124 (124 = timeout)
```

```cx-run
sh ./unisacc.com skills/unisacc-cx/examples/hash.cx -- abc
sh ./unisacc.com skills/unisacc-cx/examples/fields.cx
```

```expect
sys.sha    ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
crypto     ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
same=1
fields=3
[0] str alpha
[1] int 42
[2] str beta
prefix=1 contains=1
```

```cx-run
sh ./unisacc.com skills/unisacc-cx/examples/vec.cx
sh ./unisacc.com skills/unisacc-cx/examples/strbuild.cx
TZ=UTC sh ./unisacc.com skills/unisacc-cx/examples/clock.cx
sh ./unisacc.com skills/unisacc-cx/examples/tmpdir.cx
```

```expect
len=12 cap>=len=1
at(6,5)=0 sub=world
at(6,99)=-1 (refused, sub untouched)
pop=! len=11
after move a.len=0 b.len=11 a.data_null=1
after free b.len=0
x, y, z -> abc (14 bytes)
truncated: x, y, z
cleared len=0
2023-11-14 22:13:20
slept>=20ms: 1
wall_s>1.7e9: 1
made dir=1 file=1
cleaned dir=0
```

## 3. 架构规则（CLAUDE.md、cx/spec.md §8–§11）

- **上层不直调 libc/POSIX**：`printf/malloc/strcmp/open/fork/getenv` 改用 `sy_*`（sys）、`mm_*`（mem）、`tx_*`/`sb_*`（text）。棘轮 `tests/cxlibc.cx`、`tests/cxraw.cx` 对**新文件要求 0**。
- **产品代码与脚本的区别**：`src/`、`cx/core|runtime|compiler|driver` 严格按上条（为跨六目标）；`tools/`、`tests/` 在 `tests/cxraw-layers.tsv` 里是免计层，允许 `printf/fprintf` 给人看。`skills/` 不在免计层，所以本文示例全部用 `sy_*`。
- **不写平台 `#if`**（`__linux__`、`_WIN32`、`__x86_64__`…），**不写 `__syscallN`**：差异只放 L0 的 `sy_*_os` 静态助手或表（棘轮 `tests/cxplatform.cx`）。
- **有界 API**：不用 `strcpy/strcat/sprintf/gets`（`tests/cxapis.cx`）；不丢弃 `snprintf/strncpy/memcpy` 的返回值去做可随输入变长的截断写入（`tests/cxtrunc.cx`）；复制用 `mm_copy(dst,dcap,src,n)`、`tx_copy(dst,cap,src,n)`。
- **临时目录**：mkdtemp 语义（原子、0700、不可预测名）；不用 pid 命名；清理只删确切路径，禁用通配符 `rm -rf $DIR/*`。
- **每条命令及其子进程 ≤60 秒**：子进程用 `cx_runv_timeout`/`np_run` 给 deadline；超时清理整棵树（`sys.process` → `sy_kill_tree`）。不用 `sleep` 轮询等；等待要有总 deadline 和次数上限。
- **管道**：`cmd | head` 吞掉 `cmd` 的退出码；判断上游失败用 `set -o pipefail`，或让程序自己读完再截断。
- **编译行为差异进表**：事实表 → 生成器（`--check`）→ 网；不在 `drive.cx`/`host.cx` 写特判。`kernel/` 是生成物，禁止手改。
- **十三个棘轮一次跑完**：`sh ./unisacc.com tools/ratchets.cx`（2026-11-06 克隆实跑 `RATCHETS ok=13 fail=0 elapsed_ms=24306`）。新 `tools/*.cx` 增加行数会触 `toolsloc`（基线 `tests/toolsloc.tsv` TOTAL 71961，只降不升，新增工具须先改基线并经 cc 批准）；`tests/`、`skills/` 下的文件不计入 toolsloc。

## 4. 模块化与内存安全：已有的，和方向

**已有（运行期，可实跑）**

- 模块：`#pragma unisa module NAME` 声明；未 `static` 的函数即公开，`static` 的名字与数据不进 CXIF（CX-M-4）。消费者 `#pragma unisa use NAME`、`use NAME::sym [as alias]`；**间接可见不给原型**（CX-U-5）：用到哪个模块就直接 `use` 它。模块循环硬拒绝。
- 所有权与借用（运行期合同 CX-O-1…O-7）：`mg_buf` 调用方拥有、`mg_move` 是 Rust 赋值（先释放 dst、src 清零）、`mg_take` 交出、`mg_free` 后再 free 是空操作；`ms_slice` 借用不拥有、子切片越界拒绝；`cx/core/memory.cx` 的 `cm_*` 句柄带世代、共享/独占借用、越界拒绝。
- 源码所有权检查：`sh ./unisacc.com tools/cx-own.cx -- FILE...`（函数内局部 owner 的 DOUBLE-FREE / USE-AFTER-FREE / LEAK，保守）。
- 层向检查：`tools/check-cx-layers.cx`（L0–L3 向上越层即红）；core 导出面棘轮 `tests/coreapi.cx`。

**方向（项目所有者定；未实现，不要说成已有）**

- 编译期所有权/借用检查（CX-W-4，0.0.45 路线 C）。今天的句柄检查是**过期句柄隔离**：句柄是可复制整数，猜中就能访问；不是线性类型，不是机密性。
- `export`（`pub use`，CX-W-3）、层感知前导（CX-W-5）、`-c -funit` 每模块一只对象（CX-W-1）。
- 产品所有权多在结构体字段与包装分配器里，函数内文本分析覆盖不到（2026-11-03 全仓 9043 函数，产品目录仅 60 个局部 owner）。

**写新模块的推荐形态**（`examples/kv.cx` + `use_kv.cx`，实跑）：状态 `static` 在模块内；对外只给整数句柄或 `(ptr,len)`，不给内部结构体指针；句柄带世代，过期句柄拒绝而非解引用；满了返回 `-1` 而不是静默覆盖；每个公开函数先验参再碰数组。

```cx-run
sh ./unisacc.com skills/unisacc-cx/examples/use_kv.cx
```

```expect
live=2 get=0 v=42
drop=0 get_after_drop=-1 live=1
drop_twice=-1
reuse_slot_new_handle_differs=1
other_still=1
```

**反例**：导出 `kv_slot *kv_table(void)` 让调用方拿到内部数组；用 `char*` 无界参数并在里面 `strcpy`；模块内 `#include <stdio.h>`；在上层写 `#ifdef __linux__`；用 `pid` 当临时目录名；用 `exit()`/`fprintf` 代替返回 `-errno`（产品代码）。

## 5. 工具箱（tools/，各一句话 + 一条实跑命令）

| 工具 | 何时用 | 命令（2026-11-06 克隆实跑） |
|---|---|---|
| `refactor.cx sites` | 查调用点（跳过注释/字面量） | `sh ./unisacc.com tools/refactor.cx -- sites sy_mkdir skills/unisacc-cx/examples/tmpdir.cx tests/check-skill-cx.cx` → 两行 `FILE:LINE<TAB>call` |
| `refactor.cx find/apply/sweep` | 批量机械改写，每文件改前改后验证，不过即还原；多轮用 `sweep` 重复同一命令直到 rc 0 | `sh ./unisacc.com tools/refactor.cx -- sweep RULES.tsv 4 --tracked -- 'sh ./unisacc.com {file} -- --check'` |
| `refactor.cx rename` | 标识符改名 | `sh ./unisacc.com tools/refactor.cx -- rename OLD NEW --tracked > cand.tsv` |
| `edit-span.cx` | 按行段替换，两端断言 | `sh ./unisacc.com tools/edit-span.cx -- FILE 10 20 'static int' '}' new.txt` |
| `affected.cx` | 改了哪些文件要重跑哪些批次 | `sh ./unisacc.com tools/affected.cx -- --files tests/check-skill-cx.cx` → 登记前 `batches=0`，登记后 `AFFECTED skill-cx via=source` |
| `flow.cx` | 一条命令看爆炸半径（符号图 + 批次） | `sh ./unisacc.com tools/flow.cx -- check FILE --run` |
| `ratchets.cx` | 13 个棘轮一次跑完 | `sh ./unisacc.com tools/ratchets.cx` → `RATCHETS ok=13 fail=0` |
| `register.cx` | 把一个检查登记成批次（追加两行、验证、实跑一次，失败回滚） | `sh ./unisacc.com tools/register.cx -- skill-cx tests/check-skill-cx.cx CHECK_SKILL_CX "..." "skills/unisacc-cx/** + tests/check-skill-cx.cx"` |
| `commit-own.cx` | 多人同改的文件只提交自己的行 | `sh ./unisacc.com tools/commit-own.cx -- -m MSG --keyed tests/batches.tsv skill-cx` |
| `own-diff.cx` | 提交前看哪些改动不是自己的 | `sh ./unisacc.com tools/own-diff.cx -- tests/batches.tsv skill-cx` → `OWN_DIFF own=0 foreign=0` |
| `decide-gen.cx` | 有限决策表编成网行（`--check` 逐字节比） | `sh ./unisacc.com tools/decide-gen.cx -- --check TABLE PREFIX STATE OUT.tsv` |
| `doc-links.cx` | Markdown 断链 | `sh ./unisacc.com tools/doc-links.cx -- CLAUDE.md` → `DOC_LINKS files=1 links=0 broken=0` |
| `symindex.cx` | 符号定义/调用/use 闭包（冷建 ~2.4 s，查询 ~250 ms） | `sh ./unisacc.com tools/symindex.cx -- def sy_mkdir` |
| `census.cx` | 普查红批次 | `sh ./unisacc.com tools/census.cx -- $PWD out/census` |
| `csv2tsv.cx` | CSV→TSV 文本管道（文本模块用法范本） | `sh ./unisacc.com tools/csv2tsv.cx -- in.csv out.tsv` |

**造工具规则**：同一类查找或改写要做第二遍、或涉及十处以上，没有合适的工具就当场用 `.cx` 造一个放进 `tools/`，带 `check-*.cx` 检查并用 `register.cx` 登记批次，在 `prd.md` 能力速查补一行；不开成百次 grep、不手工逐个编辑。

## 6. 常见坑（2026-11-05/06 实际踩过）

1. **材料名在不同批次被当文件/目录**：all-waves 会话把 `chain-gen1` 绑成目录、`kernels/contracts.cx` 绑成父目录，造出假 FAIL；单跑 HEAD 才是事实（ledger RC1-MATERIAL-BIND）。写 check 时参数形态（文件还是目录）要在用法注释里写明并在入口判 `st.kind`。
2. **测试原地改输入**：check 不得改自己的输入；产物进 mkdtemp 临时目录或 `out/`，`outdep` 棘轮禁止批次读未声明的 `out/` 残留。`check-skill-cx.cx` 的 `--self-test` 把夹具写到临时目录再删。
3. **写死计数的断言**：`sourcepackage.cx` 六处写死 34/107 与 recipes 表不同源，报告 `models=42` 被 `34` 卡住、`job_seen[107]` 越界静默写坏（ledger INSTALL44）。计数来自表或 `#define` 单一来源；本文的 expect 块只断言示例自身输出，不断言 `scanned=400` 这类全局计数。
4. **pid 命名临时目录**：pid 复用、跨会话冲突；用 mkdtemp 语义（`sy_mkdir` 原子重试，tmpdir.cx）。
5. **`-c` 多对象链接的 stdio 告警是已知误报**：两个及以上 `.cx` 对象链接恒打印一次 `warning: 2 objects each carry a private stdio table`（判据只数 `.bss g_<probe>` 行），rc 与产物不变；依赖「链接时 stderr 为空」的脚本要调整（CHANGELOG 0.0.44-rc1，WARN-PROBE-USED）。
6. **改了 `tools/`、`tests/` 的检查后要跑 affected**：`sh ./unisacc.com tools/affected.cx -- --files F --run`，或用已装产品实编一遍；候选版本放宽的限制在安装前不算数。
7. **`use` 旁边优先**：源文件旁边若有同名 `.cx`，它会盖过 `cxroot.tsv` 的映射；`mem.grow`+`text.build` 同单元冲突见 §2.1。
8. **`| head` 吞退出码**；手工拼 `stream` 管道要 `set -o pipefail`。

## 7. 逐步迁移路线

**先换**（收益高、风险低）：① 一次性文本/TSV 处理与报表（`tx_fields` + `sb_*`）；② 测试夹具生成与 golden 比对（`cx_read` + `mm_same`，本文的 check 就是）；③ 构建胶水与子进程编排（`sy_run`/`cx_runv_timeout`，天然带 60 秒 deadline）。

**验收 = 行为等价对照**：同一输入分别喂旧脚本与新 `.cx`，stdout、stderr、退出码、产物逐字节比较（`cmp`/`sha256sum`），对照至少覆盖正例、空输入、坏输入三类；对照通过后旧脚本移到 `archive/` 并在 `agentwork/ledger.tsv` 记一行，不删历史。

**不该换**：要 JSON/正则/HTTP 框架/数值栈且 cx 没有对应模块；别的车道的文件；一次性用完即弃且不进仓的命令。遇到这些：先查模块，没有就在 `agentwork/items.tsv` 立缺口条目，再回落。

**登记**：新 check 用 `tools/register.cx` 登记（`tests/batches.tsv` 一行 + `tests/coverage.tsv` 一行），提交用 `commit-own.cx --keyed`。本文自身的检查：`sh ./unisacc.com tests/check-skill-cx.cx`（批次 `skill-cx`，标记 `CHECK_SKILL_CX`）。
