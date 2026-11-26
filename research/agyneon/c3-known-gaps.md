# C3 已知缺口清单 (`v0.1.3`)

- 维护者: `agyneon` | 来源: gatekeeper 复核结论 (`v0.1.3`)

1. **I9 (`ext/sysx/reaper.cx`)**: 扫描 `/proc` 达到 `SY_KT_MAX` 容量上限时 `break`, 超出容量的后代未被纳入树清理 (随 S4 推 `v0.1.4` 解决)。
2. **`cx/sys/proc.cx` 外的裸 `fork`/`waitpid`**: `processcore.cx:59/77/126`、`trace.cx:230`、`runlaunch.cx` 及 `tools/cxlib` 多处仍保留直接 `fork`/`waitpid`, 未经 L0 统一进程所有权表。
3. **跨平台覆盖 (`osx` / `win`)**: macOS 与 Windows 平台测试覆盖当前为 `UNCOVERED`。
4. **32 位 `long` 移位未定义行为**: `cx/sys/proc.cx` 中 `c->deadline_at_ms |= (1L << 32)` 在 32 位 `long` (LLP64/ILP32) 上属未定义行为 (后续改为 `int64_t` 或独立字段)。
