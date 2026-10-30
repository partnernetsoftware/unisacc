# cx/mem — 内存库（L0/L1，Rust 所有权/借用体系）

一组带长度/容量/所有权合同的字节与分配模块，是「向 Rust 学习内存安全」的库层。
上层通过 `#pragma unisa use mem.NAME` 消费，不直接 malloc/memcpy。

## 模块与用途

| 模块 | 文件 | 语义 | 对应 Rust |
|---|---|---|---|
| `mem.heap` | `heap.cx` | malloc/calloc/realloc/free/strdup 的显式长度封装（libc 语义，机械改名） | `alloc::alloc` |
| `mem.bytes` | `bytes.cx` | 带长度/容量的 copy/move/fill/compare，越界拒绝、失败原子 | `slice` 的带界原语 |
| `mem.arena` | `arena.cx` | 句柄式分组分配，一次释放一组（编译阶段临时表） | `bumpalo::Bump` |
| `mem.grow` | `grow.cx` | 可增长字节缓冲区（倍增扩容），`mg_move`/`mg_take` 所有权转移 | `Vec<u8>` |
| `mem.slice` | `slice.cx` | 不可变借用切片，子切片越界拒绝、相等、查找 | `&[u8]` |

## 所有权生命周期（grow）

`mg_init`（出生）→ `mg_push/mg_append`（增长）→ `mg_move/mg_take`（转移/交出）→ `mg_free`（终结）。
`mg_move` 后旧句柄失效，`mg_take` 后调用者成为唯一所有者。借用视图见 `mem.slice`（不拥有、只读）。
尾部操作 `mg_pop`（Vec::pop，返回值不悬垂）、`mg_truncate`（Vec::truncate，原地缩短）、
`mg_clear`（Vec::clear，保留分配）都不 realloc，是 O(1)。中段操作 `mg_insert`（Vec::insert）、
`mg_remove`（Vec::remove）按 index 位移尾段，O(len - index)，插入可触发倍增扩容。

## 速度

`grow` 从 16 起倍增扩容（16→32→64…），N 次 push 总成本 O(N)，不逐字节 realloc。
`mg_pop`/`mg_truncate`/`mg_clear` 是 O(1) 尾部操作、不缩容，regrow 复用已分配块。
`slice` 零分配（指针+长度），O(n) 操作不复制字节。

## 检查

- `tools/check-cx-grow.cx`（22 用例）、`tools/check-cx-slice.cx`（12 用例）、`tools/check-cx-arena.cx`。
- 棘轮 `tests/cxraw.cx`：`cx/mem/` 在 L0 白名单，不计；上层经 use 消费。
