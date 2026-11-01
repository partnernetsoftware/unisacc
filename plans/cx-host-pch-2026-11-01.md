# 宿主 gcc_cx/clang_cx：trimmed prelude PCH

## 事实

`tools/cxcc.cx` 对不含 `cx_*` 且不含 `#include` 的目标单元加 `-D__UNISA_FTRIM_LIBC=1`，前奏仍要每次从 `include/cx.h` 解析。`tools/build-cxcc-host.cx` 之前只生成驱动，没有预编译前奏。

## 改动

- `build-cxcc-host` 在 `out/cxcc-host/{gcc,clang}-trim/` 复制 `cx.h`、`cx-nested.h` 和两个被包含头，生成：
  - gcc：`cx.h.gch`
  - clang：`cx.h.pch`
- `cxcc` 在目标单元满足 trim 条件且 PCH 存在时：
  - gcc：`-include out/cxcc-host/gcc-trim/cx.h`（自动选 `.gch`）
  - clang：`-include-pch out/cxcc-host/clang-trim/cx.h.pch`
- 非 trim 单元继续用原始 `include/cx.h`，不接触 PCH；PCH 缺失或失效时回退原路径。
- PCH 是构建产物，不入 git；由 `tools/build-cxcc-host.cx` 重建。

## 实测

同机、10 个唯一源、`-o` 冷路径：

```text
无 PCH：gcc 754ms / 10 = 75.4ms；clang 1365ms / 10 = 136.5ms
有 PCH：gcc 591ms / 10 = 59.1ms；clang 1114ms / 10 = 111.4ms
```

冷路径 gcc 约 -22%，clang 约 -18%。热路径仍约 34ms / 68ms（对象缓存主导），PCH 不冒充热路径收益。

## 验收

- `gcc_cx.com tests/cxcc/suite.cx`：11 units × 2 drivers fails=0。
- `cx-host-run` 批次 PASS。
- `cx0-unit` 28/0。
- `ratchets` 7/7。
- full prelude（调用 `cx_read`）gcc/clang 均编译运行成功，未被 trim PCH 污染。

## 边界

- PCH 由 `build-cxcc-host` 生成；私有测试驱动没有 PCH 时走原 header，仍正确但不享受冷路径收益。
- PCH 失效依赖编译器自身的 header/flags 校验；当前没有独立 stamp 校验，换头后应重建驱动/PCH。
- 只优化宿主，不改变 unisacc 默认运行；Windows 分支不生成 PCH，仍走原路径。
