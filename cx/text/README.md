# cx/text — 文本库（L0/L1，Rust str / Node Buffer 体系）

一组带长度的字节串与字符串构造模块，是「向 Rust 学习」的文本层。
上层通过 `#pragma unisa use text.NAME` 消费，不直接 strlen/strcmp/strcat。

## 模块与用途

| 模块 | 文件 | 语义 | 对应 Rust / Node |
|---|---|---|---|
| `text.text` | `text.cx` | 带长度的 z-string/span 比较、前缀、查找、字段分割 | `str` / `String.prototype.split` |
| `text.build` | `build.cx` | 不透明字符串构造器（`sb_*`），append/join/cstr，倍增扩容 | `String` / `Buffer` |
| `text.deflate` | `deflate.cx` | deflate/gzip 压缩（调用方提供输出缓冲） | `flate2` / `zlib` |

## 关键接口

- `text.text`：`tx_eqz`/`tx_prefixz`（z-string 带界比较，无 cap 上限固化）、`tx_prefix`/`tx_suffix`（starts_with / str::ends_with）、`tx_fields`/`tx_field`（字段分割）、`tx_find`/`tx_copy`。
- `text.build`：`sb_new/sb_free/sb_puts/sb_putn/sb_len/sb_clear/sb_cstr/sb_join`。`sb` 是不透明类型（`void* inner`），CXIF 不泄漏内部 Vec 类型。

## 速度

`build` 复用 `mem.grow` 的倍增扩容，N 次 append 总 O(N)。`sb_cstr` 只在需要时补 NUL（不每次 realloc）。

## 检查

- `tools/check-cx-text.cx`（16 用例）、`tools/check-cx-build.cx`（16 用例）、`tools/check-cx-deflate.cx`。
- 棘轮 `tests/cxraw.cx`：`cx/text/` 在 L0 白名单，不计。

## 驱动消费者

- `tools/csv2tsv.cx`：读文件 → `tx_fields`/`tx_field` 分割 → `sb_*` 重组，一条 CSV→TSV 文本管道。
