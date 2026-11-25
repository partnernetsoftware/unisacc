# nn 外置优先加载与热替换设计页 (列入 V3 / 0.1.3 T3.6)

维护窗口: `agy-h` | 日期: 2026-11-25 | 上游依据: 所有者指示(loader 加切片完成后 nn 高速迭代)、`cc` 链期间设计派工 | 基线状态: 只读源码核对，零编译

---

## 0. 一句话结论

现有 `--models PKG` 是「整容器替换 (`UNISA_CONTAINER` -> 完整 P4 `UNIPKG1` 文件)」，无单网粒度覆盖、无 `ENOENT` 回退内置、无外置摘要混入 `run-cache`/`unitcache` 键。最小改造方案为新增 `--nn-dir DIR` / `UNISACC_NN_DIR` / 旁置 `nn/` 三级外置单网解析(`pm_resolve_model`)，以原生 `UNINETB1N` 加上轻量头校验 `kernel_id/abi_ver/stage`，把「实际命中的外置模型摘要」追加进 `USE33 ver=15` 与 `UNIUC3` 缓存键（无外置时键零变化、7 ms 热路径不失效），外置目录置于 `seed/ident.c` 七目录之外从而完全不触 `SOURCES_SID`，并将 `--no-external-nn` 设为正规链与 `release-build` 强制门。

---

## 1. 现有 `--models` 语义实证与五大缺口

依据对仓库源码的直接读取，现有路径如下:

(1) CLI 解析 (`cx/driver/cli.cx:89, 250, 532-539`)
- `cxc_skips` 将 `--models` 标记为「跳过后续参数槽」的选项；`cxc_step` 在第 532-539 行读取下一个参数写入 `st->pkg`（未作存在性或格式探测，仅拷贝指针）。
- 同时 `tools/check-driver-cli.cx:31-62` 锁死了 `CxcState` 的大小与逐字段偏移(`sizeof(CxcState) == 144`，`res` 偏移 128，`ops` 偏移 136)。

(2) 容器绑定与加载顺序 (`src/drive.cx:299-304, 587-601`, `src/drivestream.cx:383-401`, `src/host.cx:107-141`)
- `dv_read_input` 与 `ds_model` 均执行 `sy_setenv("UNISA_CONTAINER", pkg ? pkg : "", pkg && pkg[0] ? 1 : 0)`，然后调用 `hs_container(cont, ...)`。
- `hs_container` (`src/host.cx:107-141`) 的查找优先级严格为三选一、无局部回退:
  1. 若 `getenv("UNISA_CONTAINER")` 非空（包含 `--models` 注入的值），唯一锁定该路径，直接返回；
  2. 否则通过 `hs_self` (`/proc/self/exe`) 检查自身末尾是否有 `UNIPKG1\n` 尾部标记(`hs_trailer`)；
  3. 否则用 `hs_beside` 检查同级目录 `unisacc.com` 是否有 `UNIPKG1\n` 尾部标记。
- 随后 `hs_read` 一次性把整个容器读入内存，`pm_open` (`src/packmodel.cx:53-86`) 调 `packagefooter_locate_bytes` + `cxpk_decode` (`cx/runtime/package_decode.cx:130-197`) 解析整包。

(3) `rb_release_models` 与 `P4` 包装配 (`tools/release-build.cx:1480-1540`, `cx/runtime/package_decode.cx:130-197`)
- `rb_release_models(input, output)` 要求输入必须是 `ver == 4` 的完整 P4 `UNIPKG1` 容器；它通过查找 `lnx/x86_64/image/O1` 的 7 个热阶段模型索引(`selected[7]`)，将这 7 个模型成员解压(校验 `UNINETB1N` 魔数和 CRC32)，把表头从 `codec=1` 改写为 `M raw raw 0 crc`，连同其余全部 `M` 成员、全部 `D` 路由表与全部 `F` 资源(`\0hdr/` 等)原封重新拼接为新的完整 `.pkg` 容器，尾部加 `UNIPKG1\n` 与长度。
- 阶段执行读取(`src/stages.cx:310-360`, `src/drive.cx:631`)通过 `pm_along(route, index, &stage)` 与 `pm_model(route, stage, ...)` 在同一个 `pm_dec.package` 内查找模型 span，再走 `rt_raw`(`compressed == 2`) 或 `rt_open`(`compressed == 0/1`)。

因此，现有 `--models` 做不到「外置优先、缺失回退内置」，具体缺以下五项能力:

| 编号 | 现有机制 (`--models PKG`) | 所需目标 (外置优先 + 回退内置) | 实际缺口 |
|---|---|---|---|
| G1 | 整容器粒度(`UNIPKG1` 全包: `D` 路由 + 全量 `M` + `\0hdr/` 等 `F` 资源) | 单网/按 stage 子集外置(只放 1..N 个 `.nb` 网文件) | `pm_model` 只查单包 `pm_dec.package`，无外部散装模型按 `(route, stage)` 或 `<stage>.nb` 覆盖解析层 |
| G2 | 一旦设 `UNISA_CONTAINER` 即无条件锁死，路径缺失或坏包直接失败 | 外置目录或单网文件 `ENOENT` 时回退内置；文件存在但损坏则拒绝 | `hs_container` 与 `pm_open` 无「局部覆盖 + `ENOENT` 回退 + 非 `ENOENT` 失败即停」分支 |
| G3 | 无目录级发现入口(命令行 `--models` 仅认单个容器文件) | CLI `--nn-dir DIR` / env `UNISACC_NN_DIR` / 可执行旁 `nn/` | 缺目录/文件名映射约定，也缺正式链禁外置参数 `--no-external-nn` |
| G4 | `rc_prod` 与 `uc_prod` 仅绑定容器与 `/proc/self/exe` 的 `UPI1` pid | 外置网字节改动后立即使 `run-cache` 与 `unitcache` 失效 | `rc_key` 与 `uc_keymake` 不感知外置模型摘要，改外置权重会假命中旧缓存 |
| G5 | P4 封包阶段用 `rb_opguard_require` 保证算子不超内置 kernel 能力 | 散装外置网绕过 `release-build`，须运行时轻量验明身份与算子边界 | 仅靠 `UNINETB1N` + CRC32 无法防跨版 op 越界、stage 错挂与签名/哈希漂移 |

---

## 2. 身份、安全与缓存键关系

### 2.1 外置单网格式与绑定头

外置文件建议支持两层组织(兼容现有 `UNINETB1N`):

(1) 可选外包头或单网命名目录结构:
- 目录布局按 `<nn_dir>/<stage>.nb` 或 `<nn_dir>/<route>/<stage>.nb` (其中 `/` 可为分层目录，先匹配精确 `<route>/<stage>.nb`，未命中(`ENOENT`)再匹配阶段通用 `<stage>.nb`，仍 `ENOENT` 则回退内置 P4 槽位)。
- 若外置文件携带 24B 以上可选前导元数据(或放在同目录 `nn-manifest.tsv` 中)，字段最小集合为:
  - `magic`: `UNINETB1N` (直接复用现有二进制网络编码) 或外置封装头 `UNINN1\n` + 定长头后再跟原始 `UNINETB1N`；
  - `abi_ver`: 模型指令集版本(对齐 `cx/runtime/model_decode.cx:11` 当前 `ARITY[57]` 槽数或 V3 扩展表)；
  - `op_max`: 该网实际使用的最大算子编号或 op 位掩码(加载前核对是否 <= 当前 `core_run` 支持集，超界立刻拒绝并给出 `external-nn-op-unsupported`)；
  - `kernel_ref`: 目标内置 `kernel/<isa>` 哈希前缀(或 `"any"` 仅开发态允许)；
  - `sha256` / `crc32`: 全载荷内容校验。
- 为降低开发态生成器成本，支持直接输出裸 `.nb` (`UNINETB1N` 字节，已有 `nw_binary_net` 工具直接可产): 加载时若同目录存在 `nn-manifest.tsv` 则强制核对每行 `(stage, len, sha256, abi_ver)`；若无清单且处于开发默认模式(`--nn-verify=content`)，按字节算 `crc32` / `sy_sha256_any_hex` 并检查 `cxmd_decode` 与 `ARITY` 上界；严格门模式(`--nn-verify=manifest`)无清单或 sha 不合即拒绝。

(2) 存在性与错误处理铁律(不静默吞错):
- `ENOENT`: 外置文件不存在 -> 安全回退内置 `pm_model` 槽位。
- `ENOTDIR` / `EACCES` / `EIO` / 魔数不对 / `sha256` 不匹配 / `abi_ver` 或 `op_max` 不兼容: 立即终止并返回具名失败，绝不因为损坏而静默回退内置去掩盖错误。

### 2.2 与四大缓存层及 `rcident` 的关系

基于对 `cx/runtime/rcident.cx`、`src/runcache.cx`、`cx/runtime/unitcache.cx`、`cx/runtime/modelcache.cx` 的逐行核对，各缓存边界如下:

(1) `modelcache` (`cx/runtime/modelcache.cx:10-31`)
- `mc_get` / `mc_put` 按「输入(压缩)字节块的 sha256」内容寻址。
- 建议: 外置 `.nb` 本身是未压缩 `UNINETB1N` (`compressed=2`, 走 `rt_raw`), 跳过 deflate 也不落 `mc-<hash>` 重复写盘，直接零拷贝验证并传给 `cxmd_decode`；若以后支持 `.nbz` 压缩外置网，则天然复用 `mc_get` 内容寻址，无需特判。

(2) `run-cache` (`src/runcache.cx:245-271, 593-594`) 与 `unitcache` (`cx/runtime/unitcache.cx:225-258`)
- 现状问题: `rc_open` 与 `uc_open` 当前均只调用 `rk_prod(cont, rc_prod)` / `rk_prod(cont, uc_prod)` (`cx/runtime/rcident.cx:86-93`)。在外置模式下，底层 `cont` (`unisacc.com`) 的 `UPI1 pid` 与 `/proc/self/exe` 均不发生任何改变! `rc_key` (`USE33 ver=14`) 与 `uc_keymake` (`UNIUC3`) 完全不含外置模型身份。若用户修改了 `<nn_dir>/e2.nb` 或 `<nn_dir>/e3.nb` 后立即重跑 `-run` 或 `-c`，会直接命中修改前的旧 `.host` ELF、旧 frontend tape 或旧 `uc-<hash>.unit`，产生严重的缓存错用。
- 修复设计 (`active_nn_digest`):
  1. 在 `rc_open` / `uc_open` 之前的初始化阶段，由外置解析器按固定排序列出「本次路由实际命中的外置模型视图」（或当前目录下生效的全部外置覆盖项表），拼接成规范清单字符串 `"stage\tsha256(bytes)\n..."`，用 `rk_hex` 计算 64 位小写 hex 的 `nn_digest`。当「未命中任何外置文件」时，保持 `nn_active = 0`。
  2. 当 `nn_active == 0`: `rc_prod`、`rc_key`(`USE33 ver=14`)、`uc_keymake`(`UNIUC3`) 与现有版本逐字节相同——原有工具 7 ms 热启动缓存不受任何影响。
  3. 当 `nn_active == 1`:
     - `rc_prod` 与 `uc_prod` 更新为 `SHA256(builtin_prod + "\nnn=" + nn_digest)`（同时影响 `rk_sources2` 的 `.plan` 备忘录绑定，见 `cx/runtime/rcident.cx:575, 608`）；
     - 且 `rc_key` 升为 `ver=15` 并显式带入 `\nnn=<nn_digest>`，确保就算 `UNISACC_RUN_CACHE_KEY_OLD=1` 退旧键也不至于错命中未开外置的条目，或在该开关下当 `nn_active==1` 直接旁路 run-cache 写/读保安全。
     - 若外置覆盖了 `rs_span` / `ex_pp`（例如改了 `e2` 预处理器模型），所有经该阶段的缓存条目因为 `rc_prod`/`uc_prod` 改变而自动全量失效并在下一轮更新到新桶，旧桶仍保留不被覆盖。

---

## 3. 对 `SOURCES_SID` 与构建边界的影响

(1) `seed/ident.c:101` 源码实证
- `historical_identity` 明确仅扫描七个目录: `dirs[] = {"exec", "src", "kernel", "include", "weights", "seed", "cx"}` 外加 `iterate/kernel/typekw.tsv` 与若干显式 `support[]` 构建适配器。
- 因此:
  - 只要外置目录不放入以上七个目录下（推荐开发目录采用仓根 `<repo>/nn/`、`<repo>/build/nn/`、`$TMPDIR/nn/` 或可执行文件旁的 `nn/`），改写、增删、反复迭代外置 `.nb` 模型文件「对 `SOURCES_SID` 的影响恒为零」。
  - 绝不可把开发态外置模型目录放在 `weights/` 或 `exec/` 下，否则每次改权重都会破坏产品源身份 `SOURCES_SID`。

(2) 正式链路与开发的隔离红线
- 「外置优先」是开发/实验态加速机制；正式构造链(`candidate-chain.cx`, `release-build.cx`, `kernel-build`, `product-build`, `chain-ready.cx`)及预提交门禁必须强制禁用外置探测（传入 `--no-external-nn` 或在工具父进程设定 `UNISACC_NN_MODE=builtin`）。
- 未带 `--no-external-nn` 时如命中了外置模型，建议在 `--version` / `RR_DEBUG` / `UNISACC_RR_TIME=1` 诊断行显式打印 `NN_SOURCE\texternal\t<nn_digest_8>\tdir=<resolved_dir>`，杜绝在正式发布取证报告中误用外置网而不知情。

---

## 4. 开发迭代回路耗时账

说明: 以下既有基准均引自实测记录文件([build/phase-times.tsv](../../build/phase-times.tsv) 与 `AGENTS.md`)；新方案预期为基于当前阶段拆分的工程推算，标注 `UNMEASURED`，须在 `T3.6` 实测后回填真实数值。

(1) 现有整包重编回路(改 1 个网 -> 验证变更效果)
- 改表/改网文本 -> `42网全键三波` 校验: `7.179s` (42 网共三波，`build/phase-times.tsv:52`，单网校验子部分约 `0.2s` 量级，`UNMEASURED`)。
- 重打路由与 P4 包: `C49-979b56e4-routes` 实测 `29.731s`，`C49-979b56e4-P4` 实测 `58.797s` (`build/phase-times.tsv:49-50`)。
- 重编安装可用产品(`gen1-lnx-x86_64`): 实测 `61.338s` (`build/phase-times.tsv:51`)；且产出新 `unisacc.com` 后改变 `UPI1 pid`，全仓 `.cx` 工具首次冷编译每项触发 ~`4.5s`（`AGENTS.md`）。
- 即使局部手工仅重打包不重编驱动，也需要完整重写数十 MB P4 载体并重走 `release-models` (`rb_release_models`)，整条最小迭代周期仍处于几十秒到两分钟以上。

(2) 外置优先单网热替换回路(设计目标)
- 步 1: 改单网表/网文件 -> 单网表检查 + `nw_binary_net` 产 `<stage>.nb` 落入外置目录 (`nn/<stage>.nb`): 只处理 1 张网，跳过其余 41 张网编解码与 DEFLATE 重压，预计 `< 1s` (`UNMEASURED`)。
- 步 2: 直接运行 `sh ./unisacc.com --nn-dir nn ...` 执行单个测试/探针:
  - 跳过 `P4` 与 `product` 重编重链: 省下 ~`90-120s`；
  - 仅读入该 1 个外置 `.nb`（未压缩 `UNINETB1N`，只跑一次内存 CRC32/SHA256 与 `cxmd_decode`，其他未覆盖阶段继续走内置 P4 + 已有 `modelcache` 热命中）；
  - 因 `nn_digest` 只变了这一目录视图，`unisacc.com` 本体 `UPI1` 与 `SOURCES_SID` 不变，关掉 `--nn-dir` 时所有已有 7 ms 热工具缓存分毫不动；开启 `--nn-dir` 时只有受测用例按新 `nn_digest` 冷跑验证一次（实测墙钟待 `T3.6` 补测，预期单轮「改单网 -> 跑完小验证」从分钟级降至数秒内）。

---

## 5. 最小实现路径与 `V3 (0.1.3) T3.6` 验收

### 5.1 写域规划与零 ABI 破坏接口

为兼容 V3 (0.1.3) 并守住 `tools/check-driver-cli.cx` 对 `CxcState == 144B` 的锁死，最小改动写域集中在以下 6 个模块（全部在版本分支完成，不破既有工具 ABI）:

(1) CLI 参数透传与状态扩展 (`cx/driver/cli.cx`, `src/drive.cx`, `src/drivestream.cx`)
- 新增两个命令行标志:
  - `--nn-dir DIR`: 指定外置模型目录（优先级高于环境变量 `UNISACC_NN_DIR` 与默认旁置 `nn/`）；
  - `--no-external-nn`: 强制只用内置 P4 容器（覆盖 `--nn-dir`、`UNISACC_NN_DIR` 与旁置目录）。
- 兼容 `CxcState == 144B`:
  - 在 `cx/driver/cli.cx:89, 250` 将 `--nn-dir` 加入 `cxc_skips`，在 `cxc_mode_word` 加 `--no-external-nn`（防止模式/源扫描把 `DIR` 当源码路径）；
  - 不往 `CxcState` 结构体加字段，不改 `tools/check-driver-cli.cx` 144B 偏移，而在 `src/drive.cx` (`dv_args` 的 `cxc_step == 1` 分支) 与 `src/drivestream.cx` (`ds_args`) 解析并保存至模块静态状态 `dv_nn_dir` / `dv_no_ext_nn`，在 `dv_read_input` 传给 `pm_nn_init`。

(2) 外置发现、哈希与统一取模解析 (`src/packmodel.cx`, `src/drive.cx`, `src/stages.cx`)
- 在 `src/packmodel.cx` 新增:
  - `pm_nn_open(dir, no_ext, argv0, out_digest_hex, why, cap)`: 按优先级(1. `--no-external-nn`/`UNISACC_NN_MODE=builtin` -> 禁用; 2. `--nn-dir`; 3. `UNISACC_NN_DIR`; 4. 可执行旁 `nn/` 目录若存在) 初始化外置索引并仅在存在可读 `.nb` 文件时构建 `active_nn_digest`；遇到存在却损坏/非 `UNINETB1N`/CRC 错/ABI 越界/目录不可读(`EACCES`/`ENOTDIR`)时立刻返回失败错误码，严禁默默吞错。
  - `pm_resolve_model(route, stage, &mb, &mn, &raw, &crcsum, &compressed)`: 先查外置映射表中是否有 `(route, stage)` 或 `<stage>.nb` 命中；命中则返回 `compressed=2` (raw `UNINETB1N`) 交由现有 `rt_raw` (`cx/runtime/routestep.cx:32`) 路径执行；未命中(`ENOENT`)则回退调用原有 `pm_model`。
- 将 `src/stages.cx:331` 与 `src/drive.cx:631` 两个调用点从 `pm_model(...)` 替换为 `pm_resolve_model(...)`。
- 若 `cx/runtime/` 需要校验 `op_max` 与 `ARITY`，在 `src/packmodel.cx` 校验 `cxmd` 解出的操作码上界或预读轻量头。

(3) 缓存键接入 (`src/runcache.cx`, `cx/runtime/unitcache.cx`, `cx/runtime/rcident.cx`)
- 在 `src/drive.cx` 调 `rc_open` 前确保 `pm_nn_open` 先算出 `nn_digest`（需在 `dv_read_input` 之后、第 560 行 `rc_open` 之前调用一次轻量级 `pm_nn_open`，仅读目录内命中的外置头/小网文件）；
- 把 `nn_digest` 传入 `rc_set_nn(nn_digest)` 与 `uc_set_nn(nn_digest)`，仅当外置生效时将 `nn_digest` 混入 `rc_prod`/`uc_prod` 并把 `USE33 ver=14` 升为 `ver=15` 加 `\nnn=`。

(4) 正规链隔离与导出工具 (`tools/release-build.cx`, `tools/candidate-chain.cx`, `tools/nn-export.cx`)
- `tools/release-build.cx`、`tools/candidate-chain.cx`、`tools/chain-ready.cx` 在调起子进程编译器时统一携带 `--no-external-nn`（或环境设 `UNISACC_NN_MODE=builtin`）。
- 新增小工具 `tools/nn-export.cx` + `tools/check-nn-external.cx`（随批次登记）支持从 `result.net` 或 P4 包一键导出单网 `.nb` 到指定目录并生成可选 `nn-manifest.tsv`。

### 5.2 列入 `V3 (0.1.3)` 任务 `T3.6`

| 任务号 | 内容 | 写域 | 前置 | 验收标准 | 负责人(建议) |
|---|---|---|---|---|---|
| `T3.6` | nn 外置优先加载、缺失回退内置与 `run-cache`/`--no-external-nn` 隔离 | `cx/driver/cli.cx`, `src/drive.cx`, `src/drivestream.cx`, `src/packmodel.cx`, `src/stages.cx`, `src/runcache.cx`, `cx/runtime/unitcache.cx`, `tools/check-nn-external.cx` | `M2`/`T2.3` (`T3.1` 若同改 `rcident` 须串联或同版协调) | 见 5.3 五项实跑验收全部通过；不改 `--no-external-nn` 下所有产出与既有 P4 逐字节一致；`SOURCES_SID` 不受外置目录影响 | 待 `cc` 排派 |

### 5.3 五条可执行验收命令（先红后绿清单，写入 `tools/check-nn-external.cx`）

说明: 以下命令在 `0.1.3` 分支构建并带外置支持的候选 `CAND` 后运行，当前基线尚无 `--nn-dir` 参数故第 (1)-(4) 项先红，完成后转绿；所有临时文件放 `$TMPDIR`。

(1) 缺失回退内置与既有字节零变化:
- `CAND --nn-dir $TMPDIR/empty-nn -b lnx/x86_64 hello.cx -o $TMPDIR/a.elf && CAND --no-external-nn -b lnx/x86_64 hello.cx -o $TMPDIR/b.elf && cmp $TMPDIR/a.elf $TMPDIR/b.elf`
- 预期: 空目录(全 `ENOENT`)安全回退内置，两份产物逐字节 `cmp` 退出码 `0`。

(2) 单网外置优先覆盖生效 (先红后绿):
- 用 `tools/nn-export.cx` 将变体 `e2.nb`（例如可观测差异或受控宏/诊断变更）放入 `$TMPDIR/nn-one/e2.nb`（其余 41 网不放），运行 `CAND --nn-dir $TMPDIR/nn-one -E sample.cx`；
- 预期: 仅替换 `e2` 模型输出结果改变，其余 `-c`/`-run` 缺失阶段全部自动回退内置且完整跑通 rc `0`。

(3) 损坏/越界外置文件严格拒绝（禁止静默回退掩盖错误）:
- 写入破坏 CRC 或错误魔数的 `$TMPDIR/nn-bad/e2.nb`，运行 `CAND --nn-dir $TMPDIR/nn-bad -E sample.cx`；
- 预期: 返回非零退出码并具名拒绝（例如 `raw model CRC` 或 `package requires binary networks`），绝不允许因报错而悄悄回退内置输出 `0`。

(4) `run-cache` 键值感知外置模型（防改权重后伪命中旧缓存）:
- 先带 `--warm` 跑两次 `CAND --nn-dir $TMPDIR/nn-cache test.cx --` 验证第二次 hit；随后覆写 `$TMPDIR/nn-cache/e2.nb` 为新合法模型，不改 `CAND` 也不改 `test.cx`，再带 `UNISACC_RR_TIME=1` 跑 `CAND --nn-dir $TMPDIR/nn-cache test.cx --`；
- 预期: 修改外置 `.nb` 后第一次必定 `lookup_hit=0` 重跑并反映新网行为（不再伪命中旧条目）；且不带 `--nn-dir` 时原有内置缓存仍 `lookup_hit=1`/`2` 且热启动不倒退。

(5) `--no-external-nn` 与 `SOURCES_SID` 隔离门:
- 在含 `$TMPDIR/nn-bad/e2.nb`（或旁置损坏 `nn/e2.nb`）的目录下运行 `CAND --no-external-nn -b lnx/x86_64 hello.cx -o $TMPDIR/clean.elf`；并在仓库内写入 `<repo>/nn/e2.nb` 后比对运行 `seed/ident` 得到的 `SOURCES_SID`；
- 预期: `--no-external-nn` 完全忽略坏外置文件且输出与纯净内置态一致 rc `0`；增删 `<repo>/nn/e2.nb` 前后 `SOURCES_SID` 64 位 hex 保持完全一致。
