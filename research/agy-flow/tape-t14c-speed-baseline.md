# v0.1.3 T1.4c 速度基线与正式门口径 (102 对象同源 tl_dce 驱动)

## 1. 基线身份与冻结约束

- 基准提交: `tape/m0` `HEAD=4852f20a1a` (40 码 `0..31, 37, 38, 42..47`, 含 4 种除法/取余、`.argc/.argv`、fstat 投影、CODE 取址边界校验、`callr` 与有界 `callm`)
- 打包工具: `tools/tapepack.cx` (`sha256=816bc690fd41534489d7487373ce3b8231918eda752624439e25816f6c1a4549`, 含 `TP_MAX_STRS=8192` 扩容与函数 `.lea` SLEB128 `tag=0` 修复)
- 夹具脚本: `~/.local/share/unisacc/research/agy-flow/check-tape-tldce-speed.cx` (同步副本 `~/repos/unisacc/.claude/worktrees/v0.1.3/research/agy-flow/check-tape-tldce-speed.cx`)
- 宿主槽约束: 严格守住 M1 冻结 8 槽 `{0=exit, 1=read, 2=write, 3=mmap, 4=munmap, 5=open, 6=close, 8=fstat}`, 要求 `(mask & ~383) == 0`, 绝不扩 383
- 负载门槛: `/proc/loadavg` 1 分钟均值 `load1 <= 6.00` (`scope=CLEAN`), 超过 6.00 时夹具默认 `rc=2` 拒绝 (`REFUSE load-over-gate`); 仅显式传 `--allow-loaded` 可记录 `scope=ADVISORY_LOADED` 且绝不宣称洁净 PASS

## 2. 按 ompgpt 正式口径收紧的四项规则 (v2 夹具)

1. 显式固定清单与逐件 SHA-256 核验: 禁用隐式目录扫描; 运行必须传入 `--manifest FILE --obj-dir DIR` (支持标准 `sha256sum` 格式 `<64hex>  <name>` 或 TSV `<name><TAB><64hex>`, 另提供 `--write-manifest FILE --obj-dir DIR` 一次性固化清单)。夹具严格按清单行序喂给驱动, 在任何计时开始前逐件读取 `.o` 核对 64 位 SHA-256, 任一缺件或失配立即失败; 同时通过 `--profile-tag` 显式区分纯 102 件口径 (`pure102`, `cut=1478`, `SHA=1ebd433a7a4f394c06eb7cc2187bd98f362dc563674efdaf644b7475be841a9d`) 与 ompgpt 合成死组口径 (`synthetic-dead`, `cut=1479`, `SHA=8972c2cfc069ba3d221a659d1c96100b0687dbe29b4e9c4c95a27f7507c2e76a`), 两套口径不混算。
2. 预热前/预热后样本命名: 本机无 root `drop_caches` 页缓存隔离, 不冒称 OS 冷页缓存。两段样本统一命名为 `PREWARM_SUMMARY` (`prewarm_median_ms`) 与 `POSTWARM_SUMMARY` (`postwarm_median_ms`), 并显式标注 `page_cache=NOT_DROPPED`。
3. 15 倍门精确乘法不等式: 判定一律使用 64 位无符号乘法不等式 `tmed_ms <= postwarm_median_ms * 15` (`rule=tape_median_ms<=postwarm_median_ms*15`), 绝不使用整数除法 `tmed_ms / postwarm_median_ms <= 15` (杜绝 `15.9x` 被截断误放)。
4. 正式资格门 (默认 `--require-complete`) 与速率探针 (`--probe-rate`) 分离:
   - 默认正式资格门: 必须同时满足外层 `rc == 0`、`status == 0`、`exit_code == 0`、`TLDCE_DRIVER` 全部字段齐全且 `dce_cut == ref_dce_cut && dce_cut > 0`、tape 输出文件与原生参考输出**逐字节 `memcmp` 完全相等且 `sha256` 完全相等** (`cmp=MATCH`)、`tmed_ms <= postwarm_median_ms * 15`、`load_gate=CLEAN`。若步数预算耗尽 (`status == 3`) 或任一条件不满足, 必须输出 `T14C_VERDICT status=FAIL_INCOMPLETE_BUDGET` (或 `FAIL`) 并返回非零 `rc=1`。
   - 速率探针模式 (`--probe-rate`): 仅用于采集未完成全量闭包时的稳定 `steps/s` 速率与 15 倍门步数容量估计, 输出 `T14C_VERDICT status=PROBE_INCOMPLETE_BUDGET`, 绝不输出 `T14C_VERDICT status=PASS`。

## 3. 同源驱动关键实现事实 (实测排雷记录)

1. 零 `stdio.h` 与零 `sys.posix` 依赖: 驱动仅使用 `compiler.tapelink`、`sys.fs`、`mem.heap`、`mem.bytes`, 本地提供 `long sy_rlimit_as(void) { return -1; }` 满足 `tl_tables()` 外链, 并用 `mm_alloc` + `mm_copy` + `mm_free` 替代 `mm_grow` (避免 `realloc` 引入 `.sys6 syscall` slot 12 `mremap`)。打包后实测 `DRIVER_SLICE bytes=88029 host_slots=8 mask=383 subset_m1_383=1`。
2. `open` 标志同源兼容: `sy_open_read` 在 Linux 下传 `O_NONBLOCK=0x800`, 会被 `hoststub_lnx_x86_64.S` 冻结标志检验 (`testq $-0x400, %rdx`, `HS_O_NONBLOCK=0x200`) 拒为 `-EINVAL`。驱动改用 `sy_open(path, O_RDONLY, 0)` (`0`) 读对象, 并由外层预建空输出文件、驱动以 `sy_open(out_path, O_WRONLY, 0)` (`1`) 写结果, 两端标志数值完全一致且不与 `tapemode-run` 的 stdout 统计行混淆。
3. 完整链接序列: `tl_bind` -> `tl_begin` -> 逐对象 `tl_tape`/`tl_adopt_first`/`tl_target_differs`/`tl_unitver`/`tl_unit`/`tl_note_links` -> `tl_dce` -> `tl_check_undef` -> `tl_entry_missing` -> 断言 `tl_dce_cut() > 0` -> `tl_finish`。

## 4. 已实测洁净基线原行 (`pure102`, `4852f20a1a`, `scope=CLEAN`, 未清 OS 页缓存)

注: 以下洁净基线原行采自 v1 夹具 (与 v2 内嵌驱动源码逐字节一致, `mask=383`, `88029 B`); v2 收紧后的 5 项正负控与 `cx-lint` 零违规实测见第 4.4 节。

### 4.1 构造相位 (`reused=0`, `load1=5.04`)

```text
LOAD_GATE	load1=5.04	max_load=6.00	scope=CLEAN
DRIVER_SLICE	bytes=88029	host_slots=8	mask=383	subset_m1_383=1
BUILD_TIMING	reused=0	native_build_ms=1791	tape_emit_ms=1780	pack_ms=70	check_ms=98	run_build_ms=21	so_build_ms=53
```

构造总计 3813 ms, 与执行计时完全分离。

### 4.2 原生 102 对象预热前 5 次 + 预热 1 次 + 预热后 5 次 (`load1=5.04`, `page_cache=NOT_DROPPED`)

全部 11 次运行 `rc=0`, `bytes=9540061`, `dce_cut=1478`, 输出摘要完全一致:
`sha256=1ebd433a7a4f394c06eb7cc2187bd98f362dc563674efdaf644b7475be841a9d`

| 相位 | n | min_ms | p25_ms | median_ms | p75_ms | max_ms | iqr_ms | 15 倍门上限 (ms) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 预热前 (pre-warm, 未清页缓存) | 5 | 1907 | 1976 | 1988 | 1993 | 2086 | 17 | 29820 |
| 预热后 (post-warm, 未清页缓存) | 5 | 1949 | 1953 | 1958 | 1970 | 1973 | 17 | 29370 |

逐次原行:

```text
PREWARM_RUNS_MS:  1907 / 1976 / 1988 / 1993 / 2086  (median=1988, iqr=17)
POSTWARM_RUNS_MS: 1970 / 1958 / 1949 / 1973 / 1953  (median=1958, iqr=17)
GATE_15X:         postwarm_median_ms=1958 -> gate_15x_postwarm_ms=29370 (1958 * 15)
```

注: 预热后中位 1958 ms (IQR 17 ms, < 0.9%) 与 ompgpt 同类 3 次原生中位 1960 ms 吻合 (差 2 ms); 两套驱动输入口径 (`pure102` `cut=1478` vs `synthetic-dead` `cut=1479`) 严格分列、不混算。

### 4.3 tape 态吞吐探针与步数线性度 (`4852f20a1a`, `scope=CLEAN`)

所有运行均为 `status=3` (预算耗尽)、`exit_code=0`、`fault_reason=0` (零故障), 同预算下停止 PC 确定性完全一致。

| 步数预算 | load1 | 逐次 wall_ms | 中位 wall_ms | 中位速率 (steps/s) | 停止 PC | 15 倍门可容最大步数 |
| --- | --- | --- | --- | --- | --- | --- |
| 100,000,000 (1e8) | 5.04 | 1590 / 1555 / 1636 | 1590 | 62,893,081 | 19806 | 1,847,169,788 |
| 300,000,000 (3e8) | 4.25 | 4897 / 5729 / 5161 | 5161 | 58,128,269 | 19492 | 1,687,173,007 |

量化结论:
1. 现有逐指令解码解释器 (`4852f20a1a`) 在洁净负载下稳定速率为 **58.1M - 62.9M steps/s** (与 ompgpt 15 亿步耗尽约 26 s / ~57.7M steps/s 吻合)。
2. 对应 15 倍门 **29370 ms**, 逐指令解码解释器在 15 倍门内至多容纳 **~16.9 亿 - 18.5 亿步**; 结合 ompgpt 实测 17 亿步仅完成 18/102 对象 (尚未进 DCE), 量化支持进入 T1.6 预解码紧凑指令记录方案 (同时保留逐指令解码入口作对拍)。
3. 本次未宣称 T1.4c 官方通过: tape 未完成 102 对象输出, 正式门口径下必然返回非零 `FAIL_INCOMPLETE_BUDGET`。

### 4.4 v2 收紧夹具正负控与 lint 实测原行 (`CX_LINT findings=0`)

- 静态门: `CX_LINT files=1 findings=0 elapsed_ms=28` (通过 `tools/devsvc-repo.tsv` 读产品路径并支持 `--product`, 零 `R1-product` / `R4-source-anchor`)。
- 缺 `--manifest` 负控: `check-tape-tldce-speed: REFUSE explicit --manifest FILE required (implicit directory scan disabled)` (`RC=2`)。
- 清单篡改负控 (篡改首条 `args.o` 摘要首字符): `manifest-mismatch entry=args.o`, `INPUT_MANIFEST profile=pure102 count=101 bad=1`, `FAIL check-tape-tldce-speed.input-manifest-sha256-verified`, `RC=1`。
- 正式资格门 (默认 `--require-complete`, 1e7 步未跑完): `T14C_VERDICT status=FAIL_INCOMPLETE_BUDGET last_status=3`, `FAIL check-tape-tldce-speed.t14c-official-complete-and-15x-gate`, `CHECK_TAPE_TLDCE_SPEED passed=10 failed=1`, `RC=1`。
- 速率探针模式 (`--probe-rate`, 复用产物 `BUILD_TIMING reused=1`, `DRIVER_SLICE bytes=88029 host_slots=8 mask=383 subset_m1_383=1`): `PREWARM_SUMMARY`/`POSTWARM_SUMMARY` (`page_cache=NOT_DROPPED`), `T14C_GATE_15X ... rule=tape_median_ms<=postwarm_median_ms*15`, `T14C_VERDICT status=PROBE_INCOMPLETE_BUDGET`, `PASS check-tape-tldce-speed.tape-probe-rate-no-fault`, `CHECK_TAPE_TLDCE_SPEED passed=4 failed=0`, `RC=0`。

## 5. T1.4c 正式验收命令 (供 ompgpt 过 T1.4b/T1.6 后直接运行)

```sh
# 1. 固化显式 102 对象顺序与逐件 SHA-256 清单 (一次性生成并核对 manifest_sha)
sh ./unisacc.com tools/check-tape-tldce-speed.cx -- \
  --write-manifest /tmp/g2-102-sha256.txt --obj-dir <G2_LNX_X86_64_DIR>

# 2. 正式资格门 (默认 --require-complete, 要求 load1 <= 6.00、逐件 SHA 核对、字节级 cmp 一致、
#    dce_cut 齐全匹配、tmed_ms <= postwarm_median_ms * 15, 未跑完即非零失败)
sh ./unisacc.com tools/check-tape-tldce-speed.cx -- \
  --manifest /tmp/g2-102-sha256.txt --obj-dir <G2_LNX_X86_64_DIR> \
  --reuse-dir /tmp/tape-t14c-art --profile-tag pure102 \
  --runs 5 --tape-runs 3 --tape-budget <B> --max-load 6.0

# 3. 速率探针模式 (仅量 steps/s, 绝不输出 T14C_VERDICT status=PASS)
sh ./unisacc.com tools/check-tape-tldce-speed.cx -- \
  --probe-rate --manifest /tmp/g2-102-sha256.txt --obj-dir <G2_LNX_X86_64_DIR> \
  --reuse-dir /tmp/tape-t14c-art --profile-tag pure102 \
  --runs 5 --tape-runs 3 --tape-budget 100000000 --max-load 6.0
```
