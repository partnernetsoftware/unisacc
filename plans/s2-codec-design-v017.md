# v0.1.7 S2 `codec` 接入与 `sy_protect_ro` (`kind=3 PROT_READ`) 设计稿

仅设计文档 (`DESIGN_ONLY`): 当前处于 v0.1.5 预跑/冻结期, 未运行任何编译、冷测、生成器或批次命令; 所有实跑证据、替代体击杀、macOS、loader 页合同实测、五态 `e2` 与集成均标为 `UNMEASURED / UNCOVERED`。

## (1) 文件布局 (File Layout)

- L0 只读保护原语:
  - `cx/sys/mem.cx`: 新增 `sy_protect_ro(void *p, long n)` (`kind == 3 -> PROT_READ`), 与 `sy_protect_rw` (`kind == 1`)、`sy_protect_rx` (`kind == 2`) 同形。
  - `cx/sys/README.md`: 第 15 行 `sys.mem` 能力列表补入 `sy_protect_ro`。
- 零 `sys.ffi` 装载器:
  - `cx/driver/tape_soload.cx` (基于 `855fa3dfd` `tsl_img_t` 扁平数组 `TSL_I_SEG0 + i * TSL_S_STRIDE` / `TSL_S_FL`, `:210` `sy_protect_rw`, `:228` 最终段保护)。
- 纯可切片单元与宿主影子包装分离 (仿 `runtime.stream_attributes` + `driver.stream_s1`):
  - `cx/runtime/codec.cx`: 纯可切片 `codec` 逻辑, 不含宿主系统调用, 保证 `tapepack --export` 满足 M1 8-slot mask `<= 383`。
  - `cx/driver/codec_s2.cx`: 宿主装载、容器校验与 fail-open 影子对拍包装。
- 旁置资产布局:
  - `<parent>/tape/codec.tapeslice`
  - `<parent>/tape/tapemode.so`
  - `<parent>/tape/manifest.tsv`
  - 路径解析与安全检查复用 `stream_s1` (`UNISACC_APE_SELF` 解析、`O_NOFOLLOW`、`sy_stat_trusted_owner`、`022` mask)。
- 页对齐约束:
  - `codec` 只读权重/常量表页使用独立 `4096B` 页对齐映射, 不与可写或可执行数据共享同一物理页。

## (2) Loader 与 `kind=3` (`PROT_READ`) 合同

- `cx/sys/mem.cx` (`sym_prot`):
  - 显式分派: `kind == 1` -> `PROT_READ | PROT_WRITE`; `kind == 2` -> `PROT_READ | PROT_EXEC`; `kind == 3` -> `PROT_READ`; 其它未知 `kind` 返回 `-EINVAL` (不静默落为 RO)。
  - 参数校验: `if (!p || n < 0) return -EINVAL;`。
  - Windows (`_WIN32`): 有效输入返回 `-ENOSYS` (与 `cx/sys/refusals.cx:69` 一致), 无效输入 (`!p || n < 0`) 返回 `-EINVAL`。
- `cx/driver/tape_soload.cx` (`:210` 写入后 -> `:228` 最终段保护):
  - `s_fl & 2U` (`PF_W`): 保持 `RW` (若同时置 `PF_X` 则拒绝, 强制 W^X)。
  - `!(s_fl & 2U) && (s_fl & 1U)` (`PF_X`): 调用 `sy_protect_rx`。
  - `!(s_fl & 2U) && !(s_fl & 1U) && (s_fl & 4U)` (`PF_R` only): 调用 `sy_protect_ro`。
  - `(s_fl & 7U) == 0` (无 `PF_R`/`PF_W`/`PF_X`): 拒绝无效 `PT_LOAD` 段。
  - 页粒度 (`4096B`): 禁止不同权限段共享同一 `4096B` 页, 重叠则拒绝。
- `codec_s2` 容器校验与默认行为:
  - 校验 UTAPEBIN v1.3 布局、M1 8-slot mask (`383`)、ABI 标识与 `cx_sha256` 封口。
  - 开关 `UNISACC_CODEC_S2` 默认关闭 (`native`); 显式开启时走影子对拍、始终采纳原生结果、`stdout` 零污染; 异常时 fail-open 回落原生并在 `stderr` 输出 `CODEC_S2_FALLBACK\treason=...`。

## (3) 五态 `stream e2` 与候选绑定

- 绑定已装候选 APE (`release-ape pack --with-tape-sidecar`)、`--g5-candidate` 子模式、显式 `UNISACC_APE_SELF` 与 `stream e2` 入口。
- 五态验收:
  1. **开关关闭 (`UNISACC_CODEC_S2=0`)**: 基线原生输出, 零回退。
  2. **开关开启 (`UNISACC_CODEC_S2=1`, 有效 `tapemode.so` + `codec.tapeslice`)**: 与 OFF 输出逐字节一致, `stderr` 为空, 零 `CODEC_S2_FALLBACK`, 并核查只读段实际命中 `sy_protect_ro` 分支。
  3. **缺库 (`tapemode.so` 缺失)**: fail-open 回落并输出预期 `reason`。
  4. **损坏容器 (`codec.tapeslice` 损坏)**: fail-open 回落并输出预期 `reason`。
  5. **权限过宽 (`chmod 0666`)**: fail-open 回落并输出预期 `reason`。

## (4) 负控矩阵 (区分 `RO` 与 `RW` / `RX`)

- `sys.mem` 隔离测试子进程:
  1. 正常路径: `sy_map_none` -> `sy_protect_rw` 写入 -> `sy_protect_ro` 返回 `0` -> 读回原字节一致。
  2. 写保护负控 (击杀误接 `RW` 替代体): 向 `sy_protect_ro` 页写入必须触发 `SIGSEGV`。
  3. 无执行权限负控 (击杀误接 `RX` 替代体): `/proc/self/maps` 核对权限为 `r--p`, 且隔离执行探针触发 `SIGSEGV`。
  4. 参数与平台负控: `sy_protect_ro(0, 4096) == -EINVAL`、`sy_protect_ro(p, -1) == -EINVAL`、未知 `kind == -EINVAL`; Windows 有效输入 `-ENOSYS`、无效输入 `-EINVAL`。
- Loader 负控: `PF_W | PF_X`、`flags == 0`、跨权限混页均须拒绝。

## (5) 当前验证状态

- `DESIGN_ONLY`: `runtime run`、`substitute bodies` (替代体击杀)、`macOS`、`loader page contract` 实测、五态 `e2` 与集成均标 `UNMEASURED / UNCOVERED`, 待解冻后按上述矩阵补齐。
