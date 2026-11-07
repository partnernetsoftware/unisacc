# core 层接口清单（固化契约，已实现）

日期 2026-11-02，作者 omp-ds。第 1–5 节由 cc 裁定后按裁定落地，第六节给出复现命令；落地内容不改任何 core 模块代码。

## 一、这份清单是什么

- 集合来源：`tests/cx-module-freeze.tsv` 的 26 个 core 模块（`cx/core`、`cx/cx0`、`cx/mem`、`cx/text` 与 P0 的 `cx/sys`，含 `dl`、`dlcall`、`ffi`）。
- 接口来源：`tools/symindex.cx -- dump FILE` 的 `D` 行里非 `static` 的顶层定义（这就是 `use` 能看见的全部）；规模 26 模块 / 191 个导出符号（`core_run` 的两条链接合并为一行，见第五节 a）。
- 意义：冻结表管「谁算 core」，层棘轮管「谁能用谁」，本文管「core 对外说什么」。三者合起来才是固化：接口**只降不升**——删实现+删行合法，新增或改名必须动本文，而本文是评审点。

## 二、契约规则（已裁定并生效）

1. 导出面 = 非 `static` 顶层函数。`static` 帮手（`syd_*`、`sym_*`、`kn_*`、`fail`、`grow` 等）随时可改，不受本契约约束。
2. 只降不升：允许删行，禁止新增行而不评审。新增/改名/改签名需要 cc 批复，并同时改代码与本文。
3. 语义也是契约：错误表示（`-errno`、`0/-1`、越界一律拒绝而非读取、短读是错误、空栈是拒绝）写在用途列里，改语义等于改契约，要单列提案。
4. 类型也是契约，但字段级暂不进机械检查：下表类型列的字段集合与含义冻结（`sy_stat_t`、`sy_tty_t`、`sy_ts_t`、`cm_*`、`Cx*`、`cxb_*`、`cx0_result`、`CoreModel`/`CoreResult` 的字段顺序），第一阶段棘轮只冻函数名与签名，字段级见第五节 b。
5. 跨目标一致：导出名与签名在六个目标上必须相同；平台差异只允许在实现内部（`_os` 静态帮手或表）。`dlcall` 在 Windows 上以 `0`/`-ENOSYS` 表达不可用，而不是不导出。

## 三、棘轮（已实现）

- 基线 `tests/core-interface.tsv`：191 行数据，每行 `path<TAB>symbol<TAB>签名（空白归一）<TAB>标记`；标记为 `-`，或 `DUAL-IMPL linked=PATH`、`DEPRECATED-ALIAS sites=N`。
- 检查 `tests/coreapi.cx`（批次 `cx-core-interface`，`-- --check`，末行 `CORE_INTERFACE`）：对 26 个模块取非静态 `D` 集合，四类红：
  - `CORE-MISSING`：基线有、代码里没有（删了代码却没删行）；
  - `CORE-NEW`：代码里有、基线没有（偷偷扩了接口）；
  - `CORE-SIG`：同名但归一后的签名串不同（改参/改返回）；同名多实现最多记一条，用 `DUAL-IMPL` 标记声明第二链接；
  - 别名调用方：`DEPRECATED-ALIAS sites=N` 行的实际调用方文件数超过 `N` 即红，用来把兼容入口的调用面继续压实而不是当场删。
- 签名归一：先反解 shard 的 `\t`/`\n`/`\\` 转义，再把所有空白折叠为单空格，最后删掉 `,` 与 `*` 两侧空格；因此跨行声明与单行声明可比。
- `--write` 是唯一合法下调通道：保留存活符号的标记，新符号写 `-` 并打印 `coreapi: added row …`，消失符号删行，签名变动打印 `coreapi: signature moved …` 并把标记归 `-`。工具自身不给自动追加。
- `--fixtures` 八个用例自证四类红与两条绿（`green-baseline`、`new-symbol-red`、`new-symbol-green-after-row`、`signature-text-red`、`missing-symbol-red`、`missing-symbol-green-after-drop`、`alias-caller-red`、`alias-caller-green-at-sites`）。
- 与现有分工：`check-cx-layers` 判方向（层、包）、`cx-module-freeze.tsv` 判成员、本棘轮判成员说的话、`cx-core-run-diff` 判 `core_run` 两条链接的行为一致。四者都不看语义正确性（那归各自批次）。
- 成本：26 次 `symindex dump`，热索引实测 11.9s，批次预算 55s。
- 接进 `tools/ratchets.cx` 作第八项 `coreapi`：七项棘轮一次跑完时一并判红。
- `core_run` 的两条链接不是靠基线行判定的，基线只记「有一行、两个实现同签名」；行为一致由批次 `cx-core-run-diff` 单独判：
  - `tools/core_run_spec.cx` 是唯一的模型定义（64 位指针/32 位 int 的 ABI 结构、动作表、三组固定输入）；两个驱动各自只声明 `core_run` 原型并读回结果，因此两边共用同一份模型而不共享符号。
  - `tools/core-run-diff-cx.cx` 链接 `core.execute` 及其配套 core 模块（transition/alu/sparse/bytes/stack/output/memory）并自备 `core_host_panic`；`tools/core-run-diff-kernel.cx` 链接 `sys.kernel`，`core_host_fetch` 返回 0 让内核回落到 `UNISA_KERNEL`。
  - 两侧都打印同一行 `CORE_RUN case=… st=… outn=… errn=… outsha=… errsha=…`；`tools/check-core-run-diff.cx ROOT KERNELS` 跑三组固定输入（空、`ab`、0/1/2/127/128/255），要求两侧 rc 都为 0 且整行逐字节相同，末行 `CORE_RUN_DIFF<TAB>cases=3<TAB>failed=N`。
  - 负向自证：把任一侧打印的 `outn` 加 1，三例全 DIFF；实测 16.6s（批预算 55s），产品批次路径 `BATCH_RUN id=cx-core-run-diff status=PASS`。

## 四、清单（按模块）

### `cx/core/alu.cx`（2 个导出）

网络动作的无状态整数算术；非法动作码走宿主 panic。

- 类型：无（只出 int64_t）

| 导出 | 签名 | 用途 |
|---|---|---|
| `core_alu32` | `int64_t (int op,int64_t a64,int64_t b64)` | 网络动作的 32 位无状态算术入口（op 为动作码，非法码走宿主 panic） |
| `core_alu64` | `int64_t (int op,int64_t a,int64_t b,int *z)` | 同一算术的 64 位形式，附带零标志出参 |

### `cx/core/bytes.cx`（3 个导出）

blob 表、字符串驻留、资源表；所有表归调用方所有。

- 类型：CxBlob、CxBlobs、CxIntern、CxInternEntry、CxResourceEntry、CxResources

| 导出 | 签名 | 用途 |
|---|---|---|
| `core_blob_add` | `int (CxBlobs *t,const unsigned char *b,int n)` | 把一个字节块登记进调用方的 blob 表并返回下标 |
| `core_string_intern` | `int64_t (CxIntern *t,const unsigned char *b,int n)` | 字节串驻留：命中返回既有 id，未命中追加 |
| `core_resource_find` | `int (CxResources *t,CxBlobs *bs,const unsigned char *p,int n)` | 按字节内容在资源表里查条目，返回 absent/borrowed/owned |

### `cx/core/coregate.cx`（12 个导出）

动作与效果之间的闸门：比较、跨度、切片、输入压栈、停机与元数。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `cr_cmp` | `int (long long u, long long v)` | 有符号比较，返回 -1/0/1 |
| `cr_cmpu` | `int (unsigned long long u, unsigned long long v)` | 无符号比较，返回 -1/0/1 |
| `cr_rld` | `int (long long v)` | RLD 动作：按重复计数把值压回缓冲 |
| `cr_span` | `int (int op, long long s0, long long s1, long long cursor, long long end, long long *lo, long long *hi)` | 跨度动作（SPAN/SPANT/SPAN2）的起止裁剪 |
| `cr_olast` | `int (const unsigned char *b, int n, int *out)` | 输出缓冲最后一个字节，供 OLAST |
| `cr_ocut` | `int (long long s0, long long n, long long *cut)` | 输出切分点计算，供 OCUT |
| `cr_xattr` | `int (const long long *at, long long i, long long end, long long *v)` | XATTR：读属性寄存器，越界可判定 |
| `cr_slice` | `int (long long s0, long long s1, long long end, long long *lo, long long *hi)` | 切片动作的下标区间校验与返回 |
| `cr_inpush` | `int (int op, long long blob_n, long long i, long long lim, long long xn, long long *out_i, long long *out_end)` | INPUSH 家族：把输入块的一段压入输入栈 |
| `cr_byte` | `int (const unsigned char *b, long long i, long long end, int *out)` | 按索引读输入字节（越界返回拒绝而非读取） |
| `cr_halt` | `int (int op)` | HALT 动作的终止判定 |
| `cr_arity` | `int (int op)` | 动作码到元数的表查询 |

### `cx/core/execute.cx`（1 个导出）

CX 执行循环的总入口：模型+输入→结果，不含编译、文件格式或 OS 语义；一次只允许一个执行在跑。

- 类型：CoreModel、CoreResult、I、动作码 enum（56 个动作）

| 导出 | 签名 | 用途 |
|---|---|---|
| `core_run` | `int (const CoreModel *m,unsigned char *input,\n int inputn,const char *src,I maxsteps,CoreResult *result)` | 执行器总入口/汇编内核入口：模型+输入→结果，无编译与 OS 语义。两实现同一签名：`cx/core/execute.cx` 的 CX 版与 `cx/sys/kernel.cx` 的汇编版，产品 entry 择一链接；基线只占一行标 `DUAL-IMPL linked=cx/sys/kernel.cx`，两实现的行为由 `cx-core-run-diff` 固定输入逐字节比对 |

### `cx/core/memory.cx`（17 个导出）

句柄化内存：所有权、借用、租约，运行期检查句柄与边界。

- 类型：cm_instance、cm_owner、cm_lease、cm_result

| 导出 | 签名 | 用途 |
|---|---|---|
| `cm_create` | `cm_result (size_t byte_limit,int buffer_limit,int lease_limit)` | 建立受管内存实例并设定字节/缓冲/租约上限 |
| `cm_destroy` | `cm_result (cm_instance instance)` | 销毁实例，清掉其全部所有权账目 |
| `cm_new` | `cm_result (cm_instance instance,size_t length)` | 在实例里新建一个受管块 |
| `cm_length` | `cm_result (cm_instance instance,cm_owner owner)` | 查某个 owner 的长度 |
| `cm_read_byte` | `cm_result (cm_instance instance,cm_owner owner,size_t offset)` | 按 owner+偏移读一个字节（句柄与边界都在运行期检查） |
| `cm_write_byte` | `cm_result (cm_instance instance,cm_owner owner,size_t offset,unsigned byte)` | 按 owner+偏移写一个字节 |
| `cm_copy_range` | `cm_result (cm_instance instance,cm_owner destination,size_t to,cm_owner source,size_t from,size_t count)` | 两个受管块之间的区间复制 |
| `cm_unsafe_import` | `cm_result (cm_instance instance,cm_owner destination,size_t to,const void *data,size_t count)` | 不安全边界：把宿主指针的一段字节导入受管块 |
| `cm_move` | `cm_result (cm_instance instance,cm_owner owner)` | 转移所有权并让源句柄失效 |
| `cm_drop` | `cm_result (cm_instance instance,cm_owner owner)` | 释放 owner 的所有权 |
| `cm_borrow_read` | `cm_result (cm_instance instance,cm_owner owner,size_t offset,size_t length)` | 建立只读借用，所有权仍归原 owner |
| `cm_borrow_write` | `cm_result (cm_instance instance,cm_owner owner,size_t offset,size_t length)` | 建立可写借用 |
| `cm_lease_length` | `cm_result (cm_instance instance,cm_lease lease)` | 查租约长度 |
| `cm_lease_read_byte` | `cm_result (cm_instance instance,cm_lease lease,size_t offset)` | 经租约读一个字节（租约失效即失败） |
| `cm_lease_write_byte` | `cm_result (cm_instance instance,cm_lease lease,size_t offset,unsigned byte)` | 经租约写一个字节 |
| `cm_lease_copy_range` | `cm_result (cm_instance instance,cm_lease destination,size_t to,cm_lease source,size_t from,size_t count)` | 两个租约之间的区间复制 |
| `cm_release` | `cm_result (cm_instance instance,cm_lease lease)` | 归还租约 |

### `cx/core/output.cx`（3 个导出）

输出缓冲、十进制格式化、字段填充；缓冲归调用方。

- 类型：CxCoreBuffer

| 导出 | 签名 | 用途 |
|---|---|---|
| `core_buffer_put` | `void (CxCoreBuffer *o,int c,int64_t at)` | 把字节连同源位置压进输出缓冲 |
| `core_decimal` | `int (char *s,int64_t v)` | 把整数写成十进制到缓冲，返回长度 |
| `core_field_fill` | `int (CxCoreBuffer *o,int64_t at,int64_t width,int64_t value)` | OFILL：按宽度填充字段 |

### `cx/core/sparse.cx`（2 个导出）

稀疏内存（键→值）：占用槽即使值为 0 也保持占用，写前先查容量。

- 类型：CxSparse

| 导出 | 签名 | 用途 |
|---|---|---|
| `core_memory_get` | `int64_t (const CxSparse *m,int64_t k)` | 稀疏内存读；未占用槽返回 0 |
| `core_memory_set` | `void (CxSparse *m,int64_t k,int64_t v)` | 稀疏内存写；容量先检查再覆盖 |

### `cx/core/stack.cx`（4 个导出）

整数栈与帧栈；帧借用输入与属性跨度。

- 类型：CxCoreFrame、CxCoreFrames、CxCoreStack

| 导出 | 签名 | 用途 |
|---|---|---|
| `core_stack_push` | `void (CxCoreStack *s,int value)` | 整数栈压栈 |
| `core_stack_pop` | `void (CxCoreStack *s)` | 整数栈出栈（空栈是拒绝而不是越界） |
| `core_frame_push` | `void (CxCoreFrames *s,const CxCoreFrame *value)` | 帧栈压入一帧（借用输入与属性跨度） |
| `core_frame_pop` | `void (CxCoreFrames *s)` | 帧栈弹出一帧 |

### `cx/core/transition.cx`（6 个导出）

状态转移与停机：给定状态与键求下一状态、预算与终止原因。

- 类型：CxTransitionModel

| 导出 | 签名 | 用途 |
|---|---|---|
| `core_transition` | `const char * (const CxTransitionModel *m,int q,int key,int *nx,int *sq)` | 状态转移入口：给定状态与键，返回下一状态与序号 |
| `cxtn_transition` | `const char * (const CxTransitionModel *m,int q,int key,int *nx,int *sq)` | 转移的兼容入口（同一实现，供旧调用方） |
| `core_run_budget` | `int (long long steps, long long maxsteps, long long *next, const char **why)` | 步数预算推进与超限判定 |
| `core_run_key` | `int (int mode, int stack_n, int stack_top, long long i, long long end, int byte, long long r)` | 按模式/栈/字节算出转移键 |
| `core_run_stop` | `int (const char *reason, int nx, const char **out)` | 停机原因归类：接受、拒绝或错误串 |
| `core_run_take` | `int (int status)` | 把终态判定折算为运行结果码 |

### `cx/cx0/bundle.cx`（4 个导出）

CX0 不可变编译单元包：严格读取（先拷贝再校验）与封装。

- 类型：cxb_unit、cxb_resource、cxb_input、cxb_options、cxb_bundle、cxb_result

| 导出 | 签名 | 用途 |
|---|---|---|
| `cxb_clear` | `void (cxb_bundle *b)` | 清空一个 bundle（不释放调用方内存） |
| `cxb_release` | `void (cxb_result *r)` | 释放 cxb_read/cxb_seal 的结果 |
| `cxb_read` | `int (const void *input,size_t n,cxb_bundle *out,char *why,size_t cap)` | 严格读取并校验交织序列化的编译单元包 |
| `cxb_seal` | `cxb_result (char **sources,int count,const char *directory,const cxb_options *options,const cxb_input *extra,int nextra)` | 把源码与资源封成不可变编译单元包 |

### `cx/cx0/plan.cx`（3 个导出）

CX0 计划：源码列表→单元计划；CXIF v0 与版本化旧 TSV 计划，不是完整 bundle。

- 类型：cx0_result

| 导出 | 签名 | 用途 |
|---|---|---|
| `cx0_release` | `void (cx0_result *r)` | 释放 cx0_plan 的结果 |
| `cx0_plan` | `cx0_result (int interface_only,char **sources,int count,char **forced,int nforced,const char *directory)` | CX0 计划：源码列表→单元计划（forced 可覆盖解析） |
| `cx0_capture_plan` | `cx0_result (char **sources,int count,const char *directory)` | 同一计划，但记录实际读取观察供 bundle 复用 |

### `cx/mem/heap.cx`（5 个导出）

L0 单块堆：语义与 malloc/calloc/realloc/free/strdup 一致，迁移是机械改名。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `mm_alloc` | `void * (long n)` | 单块堆分配（语义同 malloc） |
| `mm_zalloc` | `void * (long n)` | 零填充单块分配 |
| `mm_grow` | `void * (void *p, long n)` | 重分配：成功返回新块，失败返回 0 且原指针仍有效 |
| `mm_free` | `void (void *p)` | 释放（空指针无操作） |
| `mm_strdup` | `char * (const char *s)` | 堆上的 z-string 副本 |

### `cx/mem/bytes.cx`（5 个导出）

L0 字节复制/移动/填充/比较：每次调用都带目标容量，装不下先拒绝。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `mm_copy` | `int (void *dst, long dcap, const void *src, long n)` | 带容量检查的字节复制，重叠须改用 mm_move |
| `mm_move` | `int (void *dst, long dcap, const void *src, long n)` | 带容量检查的字节移动（允许重叠） |
| `mm_fill` | `int (void *dst, long dcap, int byte, long n)` | 带容量检查的字节填充 |
| `mm_same` | `int (const void *a, const void *b, long n)` | n 字节相等判定 |
| `mm_cmp` | `int (const void *a, const void *b, long n)` | 按无符号字节比较 |

### `cx/text/text.cx`（19 个导出）

L0 定长字节串：所有读取不超过给定长度；z-string 参数同时给容量，缺 NUL 返回 -1。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `tx_len` | `long (const char *s, long cap)` | z-string 在 cap 内的长度，无 NUL 返回 -1 |
| `tx_same` | `int (const char *a, long an, const char *b, long bn)` | 两段定长字节相等 |
| `tx_eq` | `int (const char *a, const char *b, long cap)` | z-string 与另一个 z-string 在 cap 内相等 |
| `tx_eqz` | `int (const char *a, const char *b)` | z-string 与字面量相等 |
| `tx_prefixz` | `int (const char *a, const char *lit, long n)` | z-string 是否以字面量开头 |
| `tx_prefix` | `int (const char *s, long n, const char *p, long pn)` | 定长字节是否以给定前缀开头 |
| `tx_contains` | `int (const char *s, long n, const char *p, long pn)` | 定长字节是否含子串 |
| `tx_count` | `long (const char *s, long n, const char *p, long pn)` | 子串出现次数 |
| `tx_suffix` | `int (const char *s, long n, const char *p, long pn)` | 定长字节是否以给定后缀结尾 |
| `tx_chr` | `long (const char *s, long n, int c)` | 首次出现字节的位置，无则 -1 |
| `tx_rchr` | `long (const char *s, long n, int c)` | 末次出现字节的位置 |
| `tx_cmp` | `int (const char *a, long an, const char *b, long bn)` | 按无符号字节比较两段定长字节 |
| `tx_find` | `long (const char *s, long n, const char *p, long pn)` | 首次匹配子串的位置 |
| `tx_rfind` | `long (const char *s, long n, const char *p, long pn)` | 末次匹配子串的位置 |
| `tx_copy` | `int (char *dst, long cap, const char *src, long n)` | 带容量的字节复制（不足返回 -ERANGE） |
| `tx_fields` | `long (const char *s, long n, int c)` | 按分隔字节计字段数 |
| `tx_field` | `int (const char *s, long n, int c, long idx, long *off, long *len)` | 取第 idx 个字段的偏移与长度 |
| `tx_trimc` | `int (const char *s, long n, int c, long *off, long *len)` | 去掉首尾分隔字节，返回偏移长度 |
| `tx_trim` | `int (const char *s, long n, long *off, long *len)` | 去掉首尾空格类字节，返回偏移长度 |

### `cx/sys/dl.cx`（2 个导出）

已登记宿主符号的取址与运行期标志；不带 dlopen 导入，静态链接产品可用。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_rtld_now` | `int (void)` | 运行期映像已加载的标志（供 loader 判定路径） |
| `sy_dl_addr` | `int (int slot, unsigned long long *out)` | 按槽号取已登记宿主符号地址（不引入 dlopen 导入） |

### `cx/sys/dlcall.cx`（3 个导出）

宿主 dlopen/dlsym/dlclose 的薄层，与 sys.dl 分开，避免静态产品引入动态导入。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_dlopen` | `void * (const char *path, int mode)` | 宿主 dlopen；静态链接产品不可用，仅动态/-run 映像 |
| `sy_dlsym` | `void * (void *handle, const char *name)` | 宿主 dlsym |
| `sy_dlclose` | `int (void *handle)` | 宿主 dlclose；Windows 返回 -ENOSYS |

### `cx/sys/env.cx`（8 个导出）

环境变量、uid、工作目录；cx/sys 里唯一调 getenv 的文件。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_env` | `const char * (const char *name)` | 环境变量值，未设返回 0（字符串归环境所有） |
| `sy_env_vec` | `char ** (void)` | 当前环境向量 |
| `sy_env_or` | `const char * (const char *name, const char *fallback)` | 环境变量值或给定默认值 |
| `sy_setenv` | `int (const char *name, const char *val, int overwrite)` | 设置环境变量（本运行时是进程内表） |
| `sy_unsetenv` | `int (const char *name)` | 删除环境变量 |
| `sy_uid` | `int (void)` | 真实 uid |
| `sy_euid` | `long (void)` | 有效 uid |
| `sy_cwd` | `int (char *dst, int cap)` | 当前工作目录写入调用方缓冲 |

### `cx/sys/ffi.cx`（7 个导出）

FFI 对外窗口：经 <unisacc_ffi.h> 调宿主库（libcurl、libc），上层不必 #include。

- 类型：SY_FFI_* 种类常量（数值与 UFFI_* 相同）

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_ffi_default` | `void * (void)` | 默认搜索域句柄（RTLD_DEFAULT 的平台差异由函数给出） |
| `sy_ffi_now_global` | `int (void)` | RTLD_NOW|RTLD_GLOBAL 的平台常量 |
| `sy_ffi_dlopen` | `void * (const char *path, int mode)` | 经 ffi 层打开宿主库 |
| `sy_ffi_dlsym` | `void * (void *handle, const char *name)` | 经 ffi 层取符号地址 |
| `sy_ffi_dlclose` | `int (void *handle)` | 经 ffi 层关闭句柄 |
| `sy_ffi_dlerror` | `const char * (void)` | 上一次 ffi 错误的文本 |
| `sy_ffi_call` | `int (void *fn, int ret_kind, int *kinds, void **vals, int n, int nfixed, void *ret)` | 按 SY_FFI_* 种类表做一次宿主调用，支持变参 |

### `cx/sys/fs.cx`（31 个导出）

文件与目录：打开/读写/元数据/改名/链接/复制；错误以 -errno 表示。

- 类型：sy_stat_t（kind、perm、size、时间等字段）

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_fstat` | `int (int fd, sy_stat_t *out)` | 按 fd 取元数据（类型/大小/模式/时间） |
| `sy_reg_size` | `long (int fd)` | 普通文件 fd 的大小，非普通文件返回 -1 |
| `sy_open` | `int (const char *path, int flags, int mode)` | 打开路径（flags/mode 同 open） |
| `sy_open_new` | `int (const char *path, int mode)` | 创建或截断文件供写 |
| `sy_open_read` | `int (const char *path)` | 只读打开 |
| `sy_realpath` | `int (const char *path, char *dst, int cap)` | 规范化绝对路径到调用方缓冲 |
| `sy_self_path` | `int (char *dst, int cap)` | 当前可执行文件的路径 |
| `sy_lseek` | `long (int fd, long off, int whence)` | 改变 fd 偏移，返回新偏移 |
| `sy_read` | `long (int fd, void *buf, long n)` | 一次读，返回读到的字节数或 -errno |
| `sy_read_once` | `long (int fd, void *buf, long n)` | 单次读（管道与字符设备的语义） |
| `sy_read_full` | `int (int fd, void *buf, long n)` | 读满 n 字节或报错（短读即错误） |
| `sy_write_all` | `int (int fd, const void *buf, long n)` | 写满 n 字节或报错 |
| `sy_fsync` | `int (int fd)` | 落盘刷写 |
| `sy_fdatasync` | `int (int fd)` | 数据落盘刷写（不刷元数据） |
| `sy_truncate` | `int (int fd, long size)` | 按 fd 截断到给定长度 |
| `sy_close` | `int (int fd)` | 关闭 fd |
| `sy_mkdir` | `int (const char *path, int mode)` | 建目录 |
| `sy_copy` | `int (const char *src, const char *dst, int mode)` | 文件复制（源目标路径） |
| `sy_mkfifo` | `int (const char *path, int mode)` | 建命名管道 |
| `sy_chmod` | `int (const char *path, int mode)` | 改模式位 |
| `sy_chdir` | `int (const char *path)` | 改当前目录 |
| `sy_stat` | `int (const char *path, sy_stat_t *out)` | 取路径元数据（跟随符号链接） |
| `sy_lstat` | `int (const char *path, sy_stat_t *out)` | 取路径元数据（不跟随符号链接） |
| `sy_is_reg` | `int (const char *path)` | 是否普通文件 |
| `sy_rename` | `int (const char *from, const char *to)` | 重命名/移动 |
| `sy_unlink` | `int (const char *path)` | 删除文件 |
| `sy_rmdir` | `int (const char *path)` | 删除空目录 |
| `sy_symlink` | `int (const char *target, const char *link)` | 建符号链接 |
| `sy_readlink` | `long (const char *path, char *dst, long cap)` | 读符号链接目标到调用方缓冲 |
| `sy_exists` | `int (const char *path)` | 存在性检查（仅 ENOENT 记为不存在） |
| `sy_access` | `int (const char *path, int mode)` | 按模式探测访问权限 |

### `cx/sys/io.cx`（9 个导出）

标准流格式输出、终端判定、errno 文本、fd 标志。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_vprintf` | `int (const char *fmt, va_list ap)` | 标准输出格式化（va_list 形式） |
| `sy_veprintf` | `int (const char *fmt, va_list ap)` | 标准错误格式化（va_list 形式） |
| `sy_printf` | `int (const char *fmt, ...)` | 标准输出格式化 |
| `sy_eprintf` | `int (const char *fmt, ...)` | 标准错误格式化 |
| `sy_puts` | `int (const char *s)` | 输出一行 |
| `sy_isatty` | `int (int fd)` | fd 是否终端 |
| `sy_strerror` | `const char * (int e)` | errno 到错误文本 |
| `sy_fd_nonblock` | `int (int fd, int on)` | 设置/清除非阻塞 |
| `sy_fd_cloexec` | `int (int fd, int on)` | 设置/清除 close-on-exec |

### `cx/sys/kernel.cx`（1 个导出）

汇编执行内核：与 core.execute 同名同签名，产品 entry 链接的是它而不是 CX 版。

- 类型：CoreModel、CoreResult（与 execute 同一组）

| 导出 | 签名 | 用途 |
|---|---|---|
| `core_run` | `int (const CoreModel *m, unsigned char *input, int inputn, const char *src, I maxsteps, CoreResult *result)` | 执行器总入口/汇编内核入口：模型+输入→结果，无编译与 OS 语义 |

### `cx/sys/mem.cx`（4 个导出）

匿名映射与页权限改置；Windows 走 -ENOSYS。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_map_none` | `int (void **out, long n)` | 匿名私有映射，返回首地址 |
| `sy_unmap` | `int (void *p, long n)` | 解除映射 |
| `sy_protect_rw` | `int (void *p, long n)` | 改为可读写 |
| `sy_protect_rx` | `int (void *p, long n)` | 改为可读可执行 |

### `cx/sys/posix.cx`（19 个导出）

POSIX 原语：相对目录 fd 系列、终端属性与前后台进程组、文件锁、会话、socketpair。

- 类型：sy_tty_t

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_at_fdcwd` | `int (void)` | AT_FDCWD 的平台常量 |
| `sy_setsid` | `long (void)` | 新建会话 |
| `sy_getpgid` | `long (int pid)` | 取进程组 id |
| `sy_killpg` | `int (int pgrp, int sig)` | 向进程组发信号 |
| `sy_socketpair` | `int (int sv[2])` | 建一对已连接 socket |
| `sy_tcgetpgrp` | `long (int fd)` | 终端前台进程组 |
| `sy_tcsetpgrp` | `int (int fd, int pgrp)` | 设置终端前台进程组 |
| `sy_openat` | `int (int dfd, const char *path, int flags, int mode)` | 相对目录 fd 打开 |
| `sy_linkat` | `int (int oldd, const char *oldp, int newd, const char *newp, int flags)` | 相对目录 fd 建硬链接 |
| `sy_mkdirat` | `int (int dfd, const char *name, int mode)` | 相对目录 fd 建目录 |
| `sy_unlinkat` | `int (int dfd, const char *name, int dir)` | 相对目录 fd 删除（dir 区分文件/目录） |
| `sy_dir_names` | `int (int dfd, char *out, int cap)` | 把目录条目名以 NUL 分隔写进调用方缓冲 |
| `sy_tty_get` | `int (int fd, sy_tty_t *t)` | 读终端属性 |
| `sy_tty_set` | `int (int fd, const sy_tty_t *t, int flush)` | 写终端属性 |
| `sy_tty_same` | `int (const sy_tty_t *a, const sy_tty_t *b)` | 两份终端属性是否相同 |
| `sy_flock` | `int (int fd, int op)` | flock 文件锁 |
| `sy_lockf` | `int (int fd, int write, int wait)` | lockf 区间锁 |
| `sy_unlockf` | `int (int fd)` | 解除 lockf 锁 |
| `sy_rlimit_as` | `long (void)` | 地址空间软上限（无上限返回 0） |

### `cx/sys/proc.cx`（12 个导出）

进程：spawn/管道/等待/信号/进程组；nohang 等待与退出码/信号二分解。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_run` | `int (char **argv, char *out, int cap, int *cut)` | 跑一个子进程并把标准输出收进缓冲 |
| `sy_spawn` | `int (char **argv)` | fork+exec 一个子进程并等它结束 |
| `sy_spawn_inherit` | `int (char **argv, int keep)` | 同上，但把 keep 之后的标准流描述符继承过去 |
| `sy_fork_session` | `int (int (*fn)(void *), void *ud, int fds[2])` | 在独立会话里跑函数，并把一条 pipe 交给父进程 |
| `sy_pipe` | `int (int fds[2])` | 建管道 |
| `sy_spawn_pipe` | `int (char **argv, const char *cwd, int flags, int *out_fd, int *err_fd)` | 跑子进程并把它的标准输出/错误接到管道 |
| `sy_wait_status` | `int (int pid, int nohang, int *exited, int *code, int *sig)` | 等子进程并给出退出码或信号；nohang 时不阻塞 |
| `sy_signal` | `int (int pid, int sig)` | 向进程发信号 |
| `sy_wait` | `int (int pid, int nohang)` | 等子进程退出 |
| `sy_kill` | `int (int pid)` | 终止进程 |
| `sy_pid` | `int (void)` | 当前进程 id |
| `sy_setpgid` | `int (int pid, int pgid)` | 设置进程所在进程组 |

### `cx/sys/sha.cx`（1 个导出）

SHA-256 十六进制摘要：Linux x86_64 走 AF_ALG，失败返回非 0 供上层回退到 cx_sha256_hex。

- 类型：无

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_sha256_hex` | `int (const void *data, long n, char hex[65])` | SHA-256 的 64 位小写十六进制；失败返回非 0 且 hex 为空 |

### `cx/sys/time.cx`（9 个导出）

时钟、时间戳、格式化、睡眠、fd 可读性等待、文件时间设置。

- 类型：sy_ts_t

| 导出 | 签名 | 用途 |
|---|---|---|
| `sy_now_ms` | `long (void)` | 单调时钟的毫秒数 |
| `sy_wall_s` | `long (void)` | 墙上时钟秒数 |
| `sy_mono` | `int (sy_ts_t *t)` | 单调时间戳结构 |
| `sy_wall` | `int (sy_ts_t *t)` | 墙上时间戳结构 |
| `sy_ts_ms` | `long (const sy_ts_t *from, const sy_ts_t *to)` | 两个时间戳之间的毫秒差 |
| `sy_local_fmt` | `int (char *out, int cap, const char *fmt, long sec)` | 本地时间格式化（fmt 为 strftime 风格） |
| `sy_sleep_ms` | `int (long ms)` | 睡眠给定毫秒 |
| `sy_readable` | `int (int fd, long ms)` | fd 在给定毫秒内可读则返回 1 |
| `sy_set_mtime` | `int (const char *path, long sec)` | 设置文件修改时间 |

## 五、裁定结果（cc，2026-11-02）

- a. `core_run` 的两个实现合并为一行：`cx/core/execute.cx` 是参考实现，产品链接 `cx/sys/kernel.cx`；基线行标 `DUAL-IMPL linked=cx/sys/kernel.cx`，两行同名不再各占一行。
- b. 第一阶段只冻函数名与签名，类型字段级冻结留作第二阶段单列提案。
- c. libc 语义镜像（`sy_printf`/`sy_strerror`/`mm_*`/`tx_*`）算契约：所有上层按它写，改动需与上层一起评审。
- d. `cxtn_transition` 不删，标 `DEPRECATED-ALIAS sites=1`：行内记录允许的调用方文件数，超出即红，因此它可以继续收缩但不会再扩张。
- e. 同名多实现合并判定：棘轮按符号集合比较，一份符号最多一条 `CORE-SIG`；分叉的签名在归一后必然不同，仍会红一次。

仍未决：类型字段级冻结（b 的第二阶段）；`DEPRECATED-ALIAS` 收缩到 `sites=0` 后是否删行，等调用方归零再议。

## 六、复现方式

```
sh ./unisacc.com tests/coreapi.cx -- --check            # 基线比对，末行 CORE_INTERFACE
sh ./unisacc.com tests/coreapi.cx -- --fixtures         # 八例自证，末行 COREAPI_FIXTURES
sh ./unisacc.com tools/ratchets.cx                      # 八项棘轮，含 coreapi
sh ./unisacc.com tools/check-core-run-diff.cx -- REPO KERNELS   # core_run 两链接差分，末行 CORE_RUN_DIFF
sh ./unisacc.com tools/symindex.cx -- dump cx/sys/fs.cx | awk -F'\t' '$1=="D" && $5=="0"'
```

`KERNELS` 是含 `x86_64.kernel` 的目录（本文用 `out/c43-native-kernel-06`，x86_64.kernel 的 sha256 为 `32cecc8a48dd6ccb30be186a161325a3537cce743291752eab427046ff262d97`）。

对 `tests/cx-module-freeze.tsv` 的每行路径重复 dump 即可重算本清单：模块 26 个、导出符号 191 个，`static` 定义全部不在其中；`--write` 只用于评审后下调或受控加行。
