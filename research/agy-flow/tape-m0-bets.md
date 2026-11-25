# T0.a 三个赌注证伪实验报告 (`tape-m0-bets`)

日期: 2026-11-25
对应设计依据: `~/.local/share/unisacc/research/fable-v01x-flow/tape-car-tasktree.md` 与 `slice-tiny-arch.md` 第 0、D、F 节
实验工具: `/tmp/lane-v2-work/tools/tape-m0-bets.cx` (`sha256=b1282fb571bfcf1a0a2ce4221bc1194a600f80f78e2e74627def3ca1710d03c3`)

## 1. 总结与判据裁决

本实验用纯 `.cx` 工具实测了 `tape-m0` 架构开工前的三个可证伪赌注,结论如下:

| 赌注与判据 | 实测指标 | 判据阈值 | 结论 |
| --- | --- | --- | --- |
| 赌注 (1) 二进制 tape 字节 | `full.tapeslice` = `521,394 B` raw / `68,575 B` `gzip -9` (对比原生 `check-tapeshape.bin` `571,117 B` raw / `99,530 B` `gzip -9`: raw 比 `0.913x`, `gzip -9` 比 `0.689x`) | 观察打包密度 | 优于原生 (`< 1.0x`) |
| 赌注 (2) `tl_dce` 裁剪后字节比 (判据 1) | `pruned.tapeslice` = `145,350 B` raw / `20,323 B` `gzip -9` (函数 `844 -> 188`, 指令 `137,337 -> 36,636`); 对未裁剪原生 `gzip -9` 比 `0.204x`; 按同口径裁剪与全量编译器切片 (`1,905,313 B` 原生) 折算 `gzip -9` 比 `0.69x ~ 0.73x` | 压缩字节比 `<= 2.0x` | PASS (大幅优于阈值) |
| 赌注 (3) `.cx` 解释器性能与 M10b 开销 (判据 2) | `12,000,012` ops 混合负载 (算术+哈希+`LD64`/`ST64`), 3 次实跑 `checksum=d54efc9a510ee03d` 逐位全等 (`match=1`, `err=0`): 原生中位数 `1.154 ns/op`; 无检查解释器 `14.169 ns/op` (`12.28x`); 带 M10b 步数预算+线性内存边界/对齐检查 `18.352 ns/op` (`15.90x`, M10b 附加开销 `+29.52%`) | 慢速比 `<= 40.0x` | PASS (带检查仍留 2.5 倍余量) |

总判定: 两项核心可证伪判据全部 PASS, 支持按 `slice-tiny-arch.md` 继续推进 `T0.3` (`.tapeslice` 解码器与解释器) 与 `T0.4` (打包与 `tl_dce` 裁剪器)。

---

## 2. 实验环境与约束核验

(1) 负载与调度约束:
- 实测前检查 `uptime`: 生成基准样本时 `load1 = 6.20 < 8`; 编译与运行 `tape-m0-bets` 时 `load1 = 2.16 < 8`。
- 全部编译与实测命令均按单进程串行执行, 强制加 `nice -n 19 taskset -c 10-15`, 且显式 `CC=/usr/bin/cc` 规避宿主 `PATH` 包装器干扰。

(2) 基准输入样本 (本机根 `./unisacc.com` 现场生成自 `tools/check-tapeshape.cx`, 22 行, 闭包含完整 cx 运行库):
- 文本 tape `/tmp/t0a-exp/check-tapeshape.s`: `2,651,506 B`, `146,915` 行, `gzip -9` = `162,438 B`, `sha256=f956e9a79cbaa45e6329cb56d2f9c303218691510d16603dcedf526eb0438691`。
- 原生可执行 `/tmp/t0a-exp/check-tapeshape.bin`: `571,117 B`, `gzip -9` = `99,530 B`, `sha256=f52bdd8ee3a0ce99ac2c973fa93145ee0930364ac4b573b04dad28b0c13b091d`。
- ELF Program Headers 核验: `LOAD 0 (R E) = 0x89197` (`561,559 B`), `LOAD 1 (RW) FileSiz = 0x16ed` (`5,869 B`), `MemSiz = 0x107360` (`1,078,112 B`), 说明当前 `unisacc.com -o` 默认编入全量闭包未做 `tl_dce`, 与 `full.tapeslice` 的 `DATA = 5,882 B` (`5,870 B` 字符串 + `12 B` 入口符号名) 精确对应。

---

## 3. 赌注 (1): 二进制 tape 容器 (`UTAPEBIN` v1.3) 字节实测

### 3.1 编码方案

`tape-m0-bets.cx` 按 `UTAPEBIN` v1.3 容器规则将文本 `-S` 打包为 8 字节对齐的 6 段二进制文件:
- 文件头与 6 段目录: `64 B` 头 (`magic="UTAPEBIN"`, `major=1`, `minor=3`, `endian=1`, `nsect=6`, `dir_off=64`, `ident` 32B SHA-256 封口, `origin=1`) + `6 x 24 B = 144 B` 目录, 共 `208 B` (含 8B 对齐填充)。
- `CODE`: 每条指令 `1 B` 操作码 (bit7 标示是否含符号引用) + 压缩寄存器半字节 (0..2 字节) + SLEB128 编码立即数/偏移 + (若含符号) `4 B` 重定位槽位。
- `DATA`: 反转义后的原始 `.str` 字节串 + 入口符号名 (`_start\0main\0`)。
- `BSSZ`: 定长 `8 B` u64, 记录总 `.bss` 字节需求。
- `SYMS`: 定长 `16 B` 行, 仅保留入口符号 (`_start`, `main`, 共 `32 B`)。
- `RELO`: 定长 `16 B` 行 (`pos u64 + kind u8 + width u8 + rsv u16 + addend i32`), 覆盖全部外部 `call`/`.lea`/`.str`/`.bss`/`.sys` 重定位 (函数内局部分支按静态偏移解析)。
- `HOST`: 规范宿主调用槽记录 (`slot u16 + name_len u16 + ASCII name`), 对应 `hoststub-slots.tsv` 37 槽表。

### 3.2 实测字节对照 (`full.tapeslice` vs 文本 tape vs 原生可执行)

| 产物 | 原始字节 (raw) | `gzip -9` 压缩字节 | 相对文本 `-S` (raw) | 相对原生 bin (raw) | 相对原生 bin (`gzip -9`) |
| --- | ---: | ---: | ---: | ---: | ---: |
| `check-tapeshape.s` (文本 tape) | `2,651,506` | `162,438` | `1.000x` | `4.643x` | `1.632x` |
| `check-tapeshape.bin` (原生 x86_64) | `571,117` | `99,530` | `0.215x` | `1.000x` | `1.000x` |
| `full.tapeslice` (未裁剪二进制) | `521,394` | `68,575` | `0.197x` | `0.913x` | `0.689x` |

`full.tapeslice` 各段字节明细:
- 头 + 目录 + 对齐填充: `228 B` (208 B 基础头目录 + 20 B 段间 8B 对齐)
- `CODE`: `439,698 B` (`137,337` 条指令, 平均 `3.202 B/insn`)
- `DATA`: `5,882 B` (`567` 个 `.str` + `12 B` 入口符号名)
- `BSSZ`: `8 B` (声明 `.bss` 需求 `1,751,486 B`)
- `SYMS`: `32 B` (2 行)
- `RELO`: `75,408 B` (`4,713` 行 x `16 B`)
- `HOST`: `146 B` (`16` 个宿主调用槽)
- 合计: `521,394 B`, `gzip -9` 压缩率 `13.15%` (`68,575 B`), 比原生 ELF 的 `gzip -9` (`99,530 B`, 压缩率 `17.43%`) 还小 `31.1%`。

---

## 4. 赌注 (2): `tl_dce` 级可达闭包裁剪与全量编译器切片折算 (判据 1)

### 4.1 裁剪实测 (`full.tapeslice` -> `pruned.tapeslice`)

从根集合 `{_start, main, __init, __main_ret, __init_uN}` 出发, 沿指令体内的函数名、`.str` 名、`.bss` 名做 BFS 可达传播后重新用同一 `UTAPEBIN` 编码器打包:

| 维度 | 裁剪前 (`full`) | 裁剪后 (`pruned`) | 保留比例 | 裁剪率 |
| --- | ---: | ---: | ---: | ---: |
| 顶级函数数 | `844` | `188` | `22.27%` | `77.73%` |
| 指令条数 | `137,337` | `36,636` | `26.68%` | `73.32%` |
| `.str` 个数 | `567` | `241` | `42.50%` | `57.50%` |
| `DATA` 字节 | `5,882 B` | `1,483 B` | `25.21%` | `74.79%` |
| 声明 `.bss` 字节 | `1,751,486 B` | `906,225 B` | `51.74%` | `48.26%` |
| `HOST` 槽位数 | `16` | `9` | `56.25%` | `43.75%` |
| `CODE` 字节 | `439,698 B` | `119,486 B` | `27.17%` | `72.83%` |
| `RELO` 字节 | `75,408 B` | `24,048 B` | `31.89%` | `68.11%` |
| 容器总字节 (raw) | `521,394 B` | `145,350 B` | `27.88%` | `72.12%` |
| `gzip -9` 压缩字节 | `68,575 B` | `20,323 B` | `29.64%` | `70.36%` |

- `pruned.tapeslice` `CODE` 平均密度: `119,486 / 36,636 = 3.261 B/insn`。
- `pruned.tapeslice` 对比未裁剪原生 `check-tapeshape.bin`:
  - raw 比: `145,350 / 571,117 = 0.255x`
  - `gzip -9` 比: `20,323 / 99,530 = 0.204x`
- 若按「两端均做 `tl_dce`」的严格同口径估算 (原生按同指令保留率 `26.68%` 裁剪后约 `152 KB` raw / `26.5 KB ~ 29.5 KB` `gzip -9`), `pruned.tapeslice` 的压缩字节比仍为 `0.69x ~ 0.77x` (`< 1.0x`)。

### 4.2 折算到全量编译器切片

依据 `slice-tiny-arch.md`, 全量编译器切片经原生 `tl_dce` 后为 `1,905,313 B` (约 `1.82 MiB`) raw:
- 按同口径 raw 密度比 (`521,394 / 571,117 = 0.913x`) 折算: 全量编译器单份二进制 `.tapeslice` raw 约 `1,739,551 B` (约 `1.66 MiB`)。
- 按本实验实测原生 `gzip -9` 压缩率 (`99,530 / 571,117 = 17.43%`) 估算, 原生全量编译器 `gzip -9` 约 `332,096 B` (`~324 KiB`)。
- 按本实验实测 `.tapeslice` `gzip -9` 压缩率 (`full` `13.15%`, `pruned` `13.98%`) 折算, 全量编译器单份 `.tapeslice` `gzip -9` 约 `228,751 B ~ 243,189 B` (`~223 ~ 237 KiB`), 相对原生压缩字节比约 `0.69x ~ 0.73x`。
- 即使采用更保守的 15% 可压缩度上限 (`260,933 B`), 压缩字节比仍仅 `0.79x`, 且在 `APE` 多目标包中只需携带 1 份架构无关 `.tapeslice` 而非 3~6 份原生二进制。

判据 1 结论: `0.69x ~ 0.77x <= 2.0x`, PASS (且单份二进制 tape 压缩后小于单份原生 x86_64 可执行)。

---

## 5. 赌注 (3): `.cx` 最小 tape 解释器原型性能与 M10b 检查开销 (判据 2)

### 5.1 负载与实现

- 解释器结构: 16 个 64 位虚拟寄存器 `r0..r15` + `64 KiB` 线性内存 `g_lin_mem`, 标准 `switch (in->op)` 字节码分派循环 (14 条指令: `IMM/ADD/SUB/MUL/XOR/AND/SHR/SHL/LD64/ST64/SLT/JUMPZ/JUMP/HALT`)。
- 计算负载: `1,000,000` 次内循环, 每次执行 12 条 tape 指令 (`SLT`, `JUMPZ`, `AND`, `LD64`, `XOR`, `MUL`, `SHR`, `XOR`, `ADD`, `ST64`, `ADD`, `JUMP`) 加上 9 条初始化与 3 条收尾退出指令, 合计 `12,000,012` 条 tape 指令 (`> 1e7`), 混合了数据依赖链访存 (`LD64`/`ST64`)、64 位乘法、移位、异或与条件跳转。
- 三种模式均用同一编译器 (`unisacc.com` 本机根产物) 编译入同一二进制 `/tmp/t0a-exp/tape-m0-bets.bin`, 每次测量前重置 `g_lin_mem`, 连续实跑 3 轮取中位数, 并校验 64 位输出 `checksum` 逐位全等:
  - `(a)` 原生编译循环 (`run_native_workload`)
  - `(b)` `.cx` 解释器无检查 (`run_interp_unchecked`)
  - `(c)` `.cx` 解释器带 M10b 步数预算 (`budget = 20,000,000`) + PC 范围检查 + `LD64`/`ST64` 线性内存边界与 8 字节对齐检查 (`run_interp_checked`)

### 5.2 三次实跑数据与中位数统计

| 轮次 | 总指令数 (`ops`) | 原生 (`ns`) | 无检查解释器 (`ns`) | M10b 检查解释器 (`ns`) | 64 位 `checksum` | 全等校验 |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| Run 1 | `12,000,012` | `13,734,247` | `170,030,394` | `217,444,588` | `d54efc9a510ee03d` | `match=1 err=0` |
| Run 2 | `12,000,012` | `14,040,686` | `171,517,163` | `228,427,024` | `d54efc9a510ee03d` | `match=1 err=0` |
| Run 3 | `12,000,012` | `13,847,332` | `168,480,430` | `220,221,035` | `d54efc9a510ee03d` | `match=1 err=0` |
| 中位数 | `12,000,012` | `13,847,332` | `170,030,394` | `220,221,035` | `d54efc9a510ee03d` | `3/3 PASS` |

导出指标 (按中位数计算):
- 原生单操作耗时: `1.154 ns/op` (`13.85 ns/iter`)
- 无检查 `.cx` 解释器单操作耗时: `14.169 ns/op`, 慢速比 `12.28x`
- 带 M10b 检查 `.cx` 解释器单操作耗时: `18.352 ns/op`, 慢速比 `15.90x`
- M10b 检查附加开销: `(220,221,035 - 170,030,394) / 170,030,394 = +29.52%` (`+4.183 ns/op`, 即步数预算递减 + PC 越界检查 + 每次 `LD64`/`ST64` 边界与对齐检查的实测总代价)

判据 2 结论: 同编译器口径下无检查 `12.28x`、带 M10b 检查 `15.90x`, 均远低于 `<= 40.0x` 阈值, PASS。

---

## 6. 风险与边界说明 (诚实披露)

(1) Bet 2 裁剪图的保守边界:
- 本实验 `run_reachability_dce` 通过扫描指令行中的函数/字符串/BSS 标识符做 BFS 传播; 对真实 `T0.4` 打包器, 若未来引入 `.data` 里的函数指针表或经 `callr` 间接调用的回调表, 需同步扫描静态数据重定位项并把函数指针目标纳入根集, 避免误剪。本样本 `check-tapeshape.s` 仅含 `31` 条 `callr` 且不含静态函数指针数据表。
- 操作码覆盖核验: `check-tapeshape.s` 共 38 种操作码, 本原型 `g_op_table` 显式映射 33 种高频操作码, 其余 5 种低频操作码 (`lshr64` 51 条、`.mod` 44 条、`.udiv` 39 条、`.zero` 35 条、`.umod` 35 条, 共 204 条, 占 `0.15%`) 走统一 1 字节扩展槽编码, 对字节体积无统计偏差。

(2) Bet 3 编译器档位与硬件延迟边界:
- 本实验的原生循环与解释器均使用当前 `unisacc.com` 自身编译器编译 (即未来真实部署环境), 实测原生每轮 `13.85 ns/iter` (`1.154 ns/op`), 慢速比 `15.90x`。
- 若改用外部重优化编译器 (`gcc/clang -O2`) 编译原生对照循环, 因该循环存在跨轮串行数据依赖 (`idx = acc & 65528` -> `L1/L2 LD64` -> `XOR` -> `IMUL` -> `SHR/XOR`), 原生极限延迟在 `3 GHz` x86_64 上约 `4.5 ~ 6.0 ns/iter` (`0.38 ~ 0.50 ns/op`); 若将未优化的 `unisacc` 编译解释器 (`18.352 ns/op`) 与外部 `-O2` 原生对比, 比值会升至 `37x ~ 48x` 区间。因此在 `T0.3` 正式实现中, 建议把 M10b 步数预算下沉到基本块/回边 (`JUMP`/`JUMPZ`/`CALL`) 检查而非逐条指令扣减, 可进一步将 M10b 附加开销从 `+29.52%` 压至 `< 10%`。

---

## 7. 逐字复现命令与原始输出

### 7.1 复现命令

```sh
# 1. 生成基准文本 tape 与原生可执行
uptime
nice -n 19 taskset -c 10-15 sh ./unisacc.com -S tools/check-tapeshape.cx -o /tmp/t0a-exp/check-tapeshape.s
nice -n 19 taskset -c 10-15 sh ./unisacc.com tools/check-tapeshape.cx -o /tmp/t0a-exp/check-tapeshape.bin

# 2. 编译并运行 T0.a 实验工具
uptime
CC=/usr/bin/cc nice -n 19 taskset -c 10-15 sh ./unisacc.com /tmp/lane-v2-work/tools/tape-m0-bets.cx -o /tmp/t0a-exp/tape-m0-bets.bin
nice -n 19 taskset -c 10-15 /tmp/t0a-exp/tape-m0-bets.bin /tmp/t0a-exp/check-tapeshape.s /tmp/t0a-exp/full.tapeslice /tmp/t0a-exp/pruned.tapeslice

# 3. 核验原始字节与 gzip -9 压缩字节
stat -c '%s %n' /tmp/t0a-exp/check-tapeshape.s /tmp/t0a-exp/check-tapeshape.bin /tmp/t0a-exp/full.tapeslice /tmp/t0a-exp/pruned.tapeslice
for f in /tmp/t0a-exp/check-tapeshape.s /tmp/t0a-exp/check-tapeshape.bin /tmp/t0a-exp/full.tapeslice /tmp/t0a-exp/pruned.tapeslice; do
  echo "$(gzip -9 -c "$f" | wc -c) $f"
done
```

### 7.2 逐字原始输出

```text
BET1_FULL	raw_s=2651506	total=521394	code=439698	data=5882	bssz=8	syms=32	relo=75408	host=146	insns=137337	funcs=844	strs=567	host_slots=16	bss_bytes=1751486
BET2_PRUNED	total=145350	code=119486	data=1483	bssz=8	syms=32	relo=24048	host=78	insns=36636	funcs=188	strs=241	host_slots=9	bss_bytes=906225
BET3_RUN	run=1	ops=12000012	native_ns=13734247	unchecked_ns=170030394	checked_ns=217444588	checksum=d54efc9a510ee03d	match=1	err=0
BET3_RUN	run=2	ops=12000012	native_ns=14040686	unchecked_ns=171517163	checked_ns=228427024	checksum=d54efc9a510ee03d	match=1	err=0
BET3_RUN	run=3	ops=12000012	native_ns=13847332	unchecked_ns=168480430	checked_ns=220221035	checksum=d54efc9a510ee03d	match=1	err=0
BET3_MEDIAN	ops=12000012	native_ns=13847332	unchecked_ns=170030394	checked_ns=220221035	native_ns_op=1.154	unchecked_ns_op=14.169	checked_ns_op=18.352	slowdown_unchecked=12.28x	slowdown_checked=15.90x	m10b_overhead_pct=29.52%
---SIZE---
2651506 /tmp/t0a-exp/check-tapeshape.s
571117 /tmp/t0a-exp/check-tapeshape.bin
521394 /tmp/t0a-exp/full.tapeslice
145350 /tmp/t0a-exp/pruned.tapeslice
---GZIP9---
162438 /tmp/t0a-exp/check-tapeshape.s
99530 /tmp/t0a-exp/check-tapeshape.bin
68575 /tmp/t0a-exp/full.tapeslice
20323 /tmp/t0a-exp/pruned.tapeslice
```

---

## 8. 产物清单与摘要

| 文件路径 | SHA-256 | 说明 |
| --- | --- | --- |
| `/tmp/lane-v2-work/tools/tape-m0-bets.cx` | `b1282fb571bfcf1a0a2ce4221bc1194a600f80f78e2e74627def3ca1710d03c3` | T0.a 纯 `.cx` 证伪实验工具 (961 行) |
| `/tmp/t0a-exp/check-tapeshape.s` | `f956e9a79cbaa45e6329cb56d2f9c303218691510d16603dcedf526eb0438691` | `check-tapeshape.cx` 文本 tape 基准 |
| `/tmp/t0a-exp/check-tapeshape.bin` | `f52bdd8ee3a0ce99ac2c973fa93145ee0930364ac4b573b04dad28b0c13b091d` | `check-tapeshape.cx` 原生 x86_64 可执行 |
| `/tmp/t0a-exp/full.tapeslice` | `c41c6f876bae1bdb10df47052ed55a398eb2d963d064ea47ac84dd46669a5509` | 未裁剪 `UTAPEBIN` v1.3 容器 |
| `/tmp/t0a-exp/pruned.tapeslice` | `5aaec24cc082927f7ab71481c983ad1a8e8d6fd36c168ca79ace6ac4758c2b9e` | `tl_dce` 裁剪后 `UTAPEBIN` v1.3 容器 |
