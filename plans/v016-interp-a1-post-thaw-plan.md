# v0.1.6 解释器 A1 解冻后性能实测协议与 UNCOVERED 夹具闭合计划

- 对象提交: `8d9802c64dfea5dc484882beab7171ae986573c4` (`tools/tape/asm/tape_x86_64.S`, 基点 `85d271c2d`)
- 对应复核: `w-cdx/.work/review-interp-a1-r2.md` (Round 2 静态复核)
- 状态声明: 本文在 `v0.1.5` 冻结期编写, **不执行任何编译、冷测试、生成器或批次**; 所有运行态项与性能数据当前保持 **`UNMEASURED / UNCOVERED`**, 待解冻且满足低负载前置条件后逐项执行.

## (1) A1 `ns/step` 前后对比采样协议 (BASE `85d271c2d` vs A1 `8d9802c64`)

### 1.1 硬性前置条件与 `UNMEASURED_HIGH_LOAD` 拒报规则
1. **双采负载门禁 (`load1 < 4.0`)**:
   - 每轮样本执行前读取 `/proc/loadavg` 记为 `load1_before`, 执行后立刻再读记为 `load1_after`.
   - 只要 `load1_before >= 4.0` 或 `load1_after >= 4.0`, 本轮样本**立即作废**, 输出 `G1_BENCH UNMEASURED_HIGH_LOAD load1_before=<x> load1_after=<y>` 并以非零/未测状态退出, 严禁输出任何提速比数字.
   - 同时确认无在途整链/重构建进程 (`pgrep` 核实重槽位空闲).
2. **输入与制品强绑定**:
   - 同一份只读容器字节与预初始化线性内存分别跑 `base.so` (由 `85d271c2d:tools/tape/asm/tape_x86_64.S` `ec59f17e...` 经 `tape-sidecar-build` 同参数编出) 与 `a1.so` (由 `8d9802c64:tools/tape/asm/tape_x86_64.S` `c34de38c...` 编出);
   - 日志首行打印双方 `.S` 与 `.so` 的完整 64 位 SHA-256 及 `hoststub_lnx_x86_64.S` SHA-256.

### 1.2 计时边界、工作负载与重复轮数
1. **剥离外壳噪声**:
   - 线性内存 `mem` (`64KB`) 与指令位图 `ibitmap` 在计时窗外预分配;
   - 通过独立 C/ASM 计时桩直接调用 `tape_run_x86_64(args)`, 计时窗仅覆盖单次 `tape_run_x86_64` 调用 (`CLOCK_MONOTONIC`), 剔除 `mm_alloc`/`mm_free`/`sy_ffi_call` 与容器构造开销;
   - 单独记录第 1 次冷调 (含首趟 `tape_bitmap_x86_64` + 缺页) 与稳态热调, 禁止混报.
2. **三档 G1 工作负载**:
   - `G1-arith-loop`: 算术+循环 (`add/xor/sub/mov/jumpz/jump`, `1,000,004` 步, 约 2-10ms/轮);
   - `G1-cmp-branch`: 全部 6 个比较 opcode (`15..20`) + 条件分支循环 (`500,000` 步);
   - `G1-mem-call`: `frame/store64/load64/call/ret` 混合循环 (`500,000` 步).
3. **重复轮数、交替采样与离散度门禁**:
   - 每档先跑 `2` 轮不计时热身 (`warmup=2`);
   - 随后交替采样 `N = 9` 轮 (`BASE -> A1 -> BASE -> A1 ...`, 消除热降频与后台抖动偏差);
   - 每轮均核对 `status == 0`、`exit_code` 与 `steps` 精确一致后才计入有效样本;
   - 统计输出每档 `min / median / p90 / max` (`ns/step` 与 `Msteps/s`)、提速比 `median(BASE) / median(A1)` 以及相对四分位距 `IQR / median`; 若 `IQR / median > 5%`, 自动标记 `G1_BENCH NOISY_UNMEASURED` 并拒绝采信.

## (2) `cdx` 第 2 轮全部 `UNCOVERED` 项 -> 夹具改造映射

| 编号 | `cdx` Round 2 `UNCOVERED` 原项 | 现有夹具缺口 | 解冻后夹具改造与新增断言 |
|---|---|---|---|
| `U1` | 线性内存逐字节对拍与客体成功输出观测 (`memory digest only`) | `.work/check-tape-interp-diff.cx:150-161,174-178` 仅算 SHA-256 后即 `mm_free(mem)` 且日志只印 16 位前缀; `v11` 用 `fd=-1` 只测 `-EBADF` 返回 | (1) `compare_vec` 同时保留 `mem_base` 与 `mem_cand`, 执行 `mm_same(mem_base, mem_cand, memsz)` 逐字节比对; 失配时打印 `first_diff_off` 与前后 16B 十六进制明文对照, 同时记录完整 64 位 SHA-256. (2) 新增 `v11b_hoststub_sys_write_pipe`: 宿主建 `pipe()` 将写端 fd 传入客体, 客体 `sys(write)` 写固定载荷后宿主读出全量字节, 对拍 `out_len` 与全部明文字节. |
| `U2` | 独立回调入口 16B 动态对齐哨兵与错栈故障诊断 (`callback entry alignment`) | `.work/abi_sentinel.S` 仅测外层调用返回后的 `rsp`, 未在 `hoststub` 回调入口动态读 `(%rsp & 15)`, 且错栈直接崩溃无诊断 | (1) 在 `.work/abi_sentinel.S` 新增回调探测桩 `abi_hostcall_align_probe` 挂接至 `args[8]`: 入口检测 `(%rsp & 15) == 8` (对应 `call *%r11` 前 16B 对齐), 累加 `cb_count` / `cb_misalign_mask` 后转发真实 `hoststub_lnx_x86_64`. (2) 外层 `abi_call_sentinel` 将预期 `rsp` 备份至专用寄存器/只读槽位以防错栈崩溃. (3) 在 `v11b` (`sys write`) 与 `v12` (`sys6 mmap/munmap`) 断言 `cb_count > 0 && cb_misalign_mask == 0`, 并新增第 7 个替代体 `M7_cb_misalign` (`.Lrun_sys_call` 将 `subq $16,%rsp` 改为 `subq $8,%rsp`) 校验动态必杀. |
| `U3` | 固定 6/6 (扩充后 7/7) 替代体必跑门禁与制品身份复验 (`0/0 fixtures`) | `.work/check-tape-interp-diff.cx:449-471` 允许 0 替代体返回 `0/0` 且 `rc=0`, 允许重复名与未配对尾参数, 替代体由临时命令行生成未留内容摘要 | (1) `check-tape-interp-diff.cx` 强制: 尾参数必须成对 (奇数报错 `rc=2`); 禁止重复替代体名 (`rc=2`); 默认模式强制核验必需替代体全集 (`M1_drop_step`..`M6_bad_op48_class` + `M7_cb_misalign`, 缺失或 `0/0` 一律 `rc=1` 失败). (2) 用独立 `.cx` 构建器固化全部替代体源码生成逻辑, 逐件输出 `.S` 与 `.so` 的 64 位 SHA-256 供独立复验. |
| `U4` | 合同外外部调用方与独立复跑 | 当前证据仅由作者单窗口跑出; 外部未审计调用方可能违反 `args`/`CODE` 非别名合同 | (1) 解冻后由 `cdx` 或独立窗口基于固化构建器一键独立复跑 `23/23` 向量 + `7/7` 替代体 + 低负载 `G1_BENCH`. (2) 仓内既有调用方 (`tapemode-run.cx`, `stream_s1.cx`, `tapecall.cx`) 逐一加注非别名静态核对; 合同外恶意别名调用方明确保持 `UNCOVERED` 边界说明. |

## (3) 解冻后执行顺序 (总耗时 < 2 分钟, 仅当 `load1 < 4.0`)

1. 按 `U1`/`U2`/`U3` 改造 `.work/abi_sentinel.S` 与 `.work/check-tape-interp-diff.cx` (解冻后可按 `tools/register.cx` 正式入 `tools/` 并登记批次).
2. 跑全量 `23/23` 逐字节明文差异 + 动态回调对齐哨兵 + 固定 `7/7` 替代体必杀, 输出全部 `.S`/`.so` 64 位 SHA-256.
3. 核实 `load1 < 4.0` 且无重任务后, 跑 3 档 `G1_BENCH` (`warmup=2`, `N=9` 交替采样), 产出冷/热与 `median/IQR` 前后对比表.
4. 交由独立复核窗口按同一命令独立复跑并签收.
