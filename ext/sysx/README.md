# ext/sysx — OS 扩展包(非 core,非 unisacc)

按 `plans/cx-tiers-2026-11-02.md` 的三层划分,这里放**操作系统能力扩展**:
net、tty、poll、guard、fd、process、process_group、process_group_osx、reaper、childdepth。

- **产品与 core 禁止 `use sysx.*`**:这一层是工具/应用可选的库,依赖方向只能是它依赖 core。
- 模块名保持原样(`#pragma unisa module net`),只有包的路径变;调用方写成 `#pragma unisa use sysx.net`。
- 解析按使用者目录走:每个消费目录有一个 `sysx` 符号链接指向这里(与既有的 `sys` 链接一一对应)。
- `ffi`、`dl`、`dlcall` **不在**这里,它们属 core。
- 迁移分批见 `plans/w2-sysx-migration-2026-11-02.md`。

本目录由 `plans/w2-sysx-migration-2026-11-02.md` 的 B0 批建立;模块在 B1 起逐批搬入。
