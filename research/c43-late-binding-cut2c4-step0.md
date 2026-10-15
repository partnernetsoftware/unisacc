# 晚绑定 2c4 第0步｜callback 类别设计（只是设计）

**证据边界：** 现状为读源码所得（行号为读取时的工作树），未跑实现、未改仓库、未出候选。下列建议均为“只是设计”。依据：`research/c43-late-binding-cut2c-step0.md` §2.4 与 §5（callback 为拆分顺序第 4 步）、`research/c43-late-binding-cut2c2-step0.md` 与 `research/c43-late-binding-cut2c3-step0.md`（owner1 复用、verify 回调、kind 取值的先例）。

## 1. 现状（读源码）

- **W 类别表。** `exec/c/librarylatebind.h:207` 的 `UsLateClass` 行含 `kind, wire_kind, owner, partition, declare, verify…`，`:219` 由 `US_LATE_CLASSES` 展开；`:222` `us_late_class(kind)` 查表，`:225` `us_late_origin_class` 按名字与 wire 分派。callback 当前不在表内。
- **callback 图的数据现状。** `exec/facts/librarycallbackgraph.tsv` 只有 5 行常量（`SIGWIREID`、`SIGWIREMARK` 等，银行号 `n<<40`，源自 `exec/parse2/librarycallbackgraph.py` 的字面量）。图的构造在 `exec/parse2/librarycallbackgraph-manifest.tsv` 与 `librarycallables-*.tsv`，即 E3 模型阶段；运行期不解析签名图。
- **closure 构造时机（两处，均在绑定之后）。**
  - `exec/c/librarycallables.h:294` `us_callable_callback` 是 libffi 闭包回调；`:375` 仅在 `p->closure` 为空时 `us_ffi_closure_make` 创建，即 **plan 实例化时**，不在编译期。
  - `exec/c/librarylatebind.h:141` `us_late_typed_callback`，`:197` 在 typed 函数帧里创建闭包，即 **W 的 typed 数据阶段**（绑定期）。
  - `exec/c/librarycallables.h:12–14` `us_ffi_closure_make` 为公共机制；`:8` 注释写明“caller retains callback semantics, registry and lifetime”，即语义与生命周期不归它。
- **释放钩子。** `exec/c/librarylatebind.h:53` `us_late_release` 释放 contract、selection、各 phase/partition 缓冲；`libunisacc.c:698` 在 `us_late_release(&prior)` 处替换旧状态；`libunisacc.c:239–242` 的上下文释放负责 call_stack、guest_maps、image。闭包当前由 plan 与 typed 帧各自释放，**无统一归属**。
- **native prepare。** `libunisacc.c:922` `us_native_prepare(plan,…)` 在调用期使用；`librarylatebind.h:169` `us_carrier_certificate_native_prepare` 在 W 的证书阶段使用。两者都接受 plan 与证书，不区分函数种类。

结论：callback 今天是运行期旁路（plan 闭包 + typed 帧闭包），没有模型阶段的图，也没有 W 类别。

## 2. 逻辑类别与物理 owner

- **逻辑类别：kind=4**（沿 2c3 取 3 为 variadic 之后的下一个值；以 `US_LATE_CLASSES` 的实际末值为准，本设计只占位，实施前核对）。wire 与函数相同（`wire_kind=0`），**物理 owner 复用 owner1**（函数），partition 与普通函数相同。
- **能力由阶段名显式声明。** 与 2c2/2c3 一致：`ownership-callback` 作为能力名出现在分区模型表中（`(kind=函数, callkind=3, V3 启用)`），模型输出仍是每 ID 唯一归属，缺归属或重复归属拒绝。
- **不把 callback 校验写进 owner1 的通用分支。** 与 2c3 的修正一致：callback 的图与约定校验放进独立 `verify` 函数指针，不依赖 relocate 的通用 extent/类型检查（否则迫使 `libraryrelocate.h` 改动，与 W 零改动冲突）。

## 3. 嵌套签名图与生命周期放哪个模型阶段

- **E3（模型阶段）输出稳定图，不输出地址。** 每个 callback 参数输出：嵌套签名图（节点按稳定局部序号）、调用约定、同步/可重入/生命周期能力位、所需 ABI 证书。**图的局部序号不是 ABI**：绑定期只用“签名字节的内容哈希”对照（与 2c2 的 `USLCON1 callkind` 同一做法）。
- **lower（模型阶段）：** 只消费 callback 的 span（与普通函数同形态的“未绑定 span”），不生成闭包，不决定生命周期。
- **绑定期（E3 释放之后）：** 以 `us_ffi_closure_make` 建闭包；闭包归属由 **W 的 typed 数据阶段**统一持有，经 `us_late_release` 释放（`librarylatebind.h:53`）。plan 闭包（`librarycallables.h:375`）只作为现有路径保留，不新增归属。

## 4. 可直接复用的部件

| 部件 | 复用方式 |
|---|---|
| W 类别表与 `us_late_class` | 新增一行（kind 占位，见 §2） |
| `us_ffi_closure_make` | 原样复用（`librarycallables.h:12–14`） |
| `us_late_release` 与 `libunisacc.c:698` 替换点 | 闭包归属并入现有释放（B0 钩子） |
| `us_native_prepare` / 证书 native prepare | 原样复用（fixed/variadic/callback 同一组件，不分叉） |
| callsite layout 表模式（2c3 的无地址帧） | callback 的嵌套图帧仿此：无地址、稳定 ID、需求列表 |
| 能力集参数 | `ownership-callback` 仅作声明，不引入新能力机制 |

## 5. 验收件骨架与复用

- 骨架沿 2c3：`us_add_symbol_typed` / `us_declare_import_typed` 给 `.sig` 字节；verify 行（有/无 provider、签名图一致/不一致、同步与可重入位）。
- 不新造 P6：普通函数路径不变，由 `latebind-p6`（26 源 130 件）守护；W 零改动由 `lib-latebind-W`（67 断言）守护。
- 实际 callback 调用与闭包释放用 ASan 复跑（沿 2c3 的 `-fsanitize=address`，不加 `-Iinclude`）。

## 6. 五问

1. **复用：** 是。owner1、W 表、`us_ffi_closure_make`、`us_late_release`、native prepare 全部复用，无新 owner 或新闭包机制。
2. **新机制：** 否。仅新增一个类别行与一个 verify 函数；闭包归属统一到现有释放点。
3. **默认输出：** 应不变。普通函数与 P6 路径不动；验收以 `latebind-p6` 与 `lib-latebind-W` 的字节同对为准。
4. **未定义 / 缺失 / 空：** 缺签名图 → 具名拒绝；图局部序号不匹配 → 拒绝（不当作 ABI）；空图（零参数 callback）→ 合法，与缺失区分。
5. **边界：** 不改 2c2/2c3 文件，不改 `libraryrelocate.h`，不动主检出其它文件。

## 7. 待决（需 cc 裁）

- kind 的具体编号（以 `US_LATE_CLASSES` 末值为准，实施前核对）。
- 嵌套图的稳定序号规则：沿用 2c2 的内容哈希对照，还是新增图哈希（两者都不能把局部序号当 ABI）。
- 闭包归属统一到 W 之后，plan 路径的闭包是保留还是迁移（本设计只建议保留，不迁移）。
