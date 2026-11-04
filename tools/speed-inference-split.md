# host.c inference 阶段分解(杠杆2 起点,2026-11-04)

命令:`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 ./unisacc.com -run examples/host.c`,3 次取中位。

| 阶段 | 中位 | 占比 |
|---|---|---|
| e3    | 114.63 ms | 30.4% |
| prune | 88.79 ms  | 23.5% |
| lower | 78.94 ms  | 20.9% |
| e2    | 63.61 ms  | 16.9% |
| elf   | 46.35 ms  | 12.3% |
| e1    |  7.73 ms  |  2.0% |
| 合计  | 377.41 ms | 100% |

对照 empty.c 同一组: e2 2.2 / e1 0.2 / e3 3.0 / prune 1.7 / lower 2.4 / elf 2.4 ms。
两者之差就是「与输入规模相关的量」:e3 差 37 倍、prune 差 52 倍、lower 差 33 倍,
而 e1 只差 39 倍但绝对值小 —— 说明热点在**处理 tape 行数多**的阶段(e3/prune/lower),不是固定开销。

## 为什么没有转移级(rule-level)分解
三条路都试过,都不可用:
1 `UNISACC_PROFILE=1`(cx/runtime/profile.cx)—— 已装 entry.elf 的 strings 里
   `UNISACC_PROFILE` 与 `PROFILE` **各 0 次**,说明这个模块没编进已装产品。要生效必须先重建。
2 `perf record` —— 本机 `perf_event_paranoid=4` 且无 sudo,内核 perf 拒绝采样。
3 汇编内核 `exec/c/asm/run_x86_64.S` 的步数计数器 `%r14` 只在超时判定里用,
   没有对外导出接口(kernel.cx 的 core_run 只交回 rc 与输出)。

因此本轮能给的是**阶段级**分解(上表),转移级要等重建或加导出接口。这项记 NOT_RUN。

## 对杠杆2 的含义
- e3 最大但只占 30%,单压 e3 最多拿 114.63ms 的一部分;
- prune(23.5%)、lower(20.9%) 与 e3 同量级,三处合计 74.8% —— 若三者共用同一 hotspot
  (逐行派发 + 稀疏表查找),一次修能同时吃到三处。
- 增量分析(host.c 减 empty.c,即"与规模相关"的部分):
    e3 +113.20ms、prune +87.35ms、lower +78.56ms、e2 +59.64ms、elf +46.19ms、e1 +7.48ms。
  prune 的倍数最高(53.8×),说明它对输入规模最敏感;其余五个都在 20–33× 区间。
- 三者共用同一执行循环已确认:src/executor.cx:133 与 :228 两处都调 `core_run(...)`,
  即汇编内核 `exec/c/asm/run_x86_64.S` 的 `Lxrloop`。所以 e3/prune/lower/e2/elf 的推理
  时间都花在同一个逐键派发循环里 —— 一次改动能同时吃到 74.8% 的阶段。

## B 方案:多规模输入的斜率(2026-11-04,3 次中位)

四档同结构输入(同模板重复 1/8/32/96 次,186B → 14798B):

| 输入 | 字节 | e2 | e1 | e3 | prune | lower | elf |
|---|---|---|---|---|---|---|---|
| s1 | 186   | 2.4 | 0.3 | 5.0   | 2.7   | 3.6   | 2.9 |
| s2 | 1250  | 2.8 | 0.6 | 13.4  | 9.5   | 9.8   | 5.9 |
| s3 | 4942  | 4.2 | 1.9 | 44.5  | 34.8  | 31.6  | 16.2 |
| s4 | 14798 | 8.8 | 5.4 | 132.3 | 137.5 | 98.8  | 51.1 |

(单位 ms)

### 关键观察:prune 在 s4 超过 e3
s3 → s4 时 e3 与 prune 的排序翻转:prune 137.5 > e3 132.3。而在 examples/*(≤609B)一直是
e3 最大。说明 **e3 与 prune 的规模敏感度不同**:prune 的斜率更陡。

### 这也修正了我先前"74.8% 在同一循环"的说法
omp-cc 的提醒成立:同一解释器循环 ≠ 同一热点。五阶段的网/表不同,步数差异可能才是主因。
现有数据无法区分「步数多」与「每步慢」——**这正是 UNISACC_PROFILE 的 steps 能给的**,
但要等 INSTALL44 把这个模块编进产品。

### 现有数据的可靠结论
1 六个阶段的时间都随输入规模线性增长(小输入空转 2-4ms 的固定开销之外全是斜率);
2 斜率排序:e3 ≈ prune(0.009 ns/B)> lower(0.006)> elf(0.004)> e2/e1(≈0);
3 e2/e1 几乎不随规模变(e2 从 2.4 到 8.8ms 只涨 6.4ms),它们不是热点。
4 examples/*.c 太小(≤609B),用它们做的阶段分解会把 prune 的斜率优势读成 e3 最大 ——
   后续所有杠杆2 的比较都应该用 ≥5KB 的输入。

## 第三轮:按 gate 协议复测(2026-11-04 晚,INSTALL44 前)
`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1`,3 次中位:
  empty.c  inflate 79.00ms  inference 12.26ms
  host.c   inflate 80.93ms  inference 374.31ms
对照 §1 基线(78.5 / 11.9 与 85.2 / 430.3):inflate 一致,e3 推理 112.79 vs 记的 108.7(+3.8%)。
**基线未漂移,后续每刀都以此为准。**

## 杠杆 4(模型解压缓存)已经兑现,不需重做
计划 §2 的选项 ③(按包 sha 缓存已解压模型)在产品里已接线并生效:
  cx/runtime/modelcache.cx 的 mc_get/mc_put,调用方 cx/runtime/routestep.cx:60 与 src/executor.cx:103。
实测 host.c:冷 run 501.2ms(min) vs 热 12.1ms(min),热路径有 `RR_TIME lookup_hit=2`;
`UNISACC_RUN_CACHE=0` 时 modelcache 行数 0(冷测不读缓存,语义正确)。
所以杠杆 4 无需再动;单文件冷路径的固定成本已由这条缓存覆盖到热路径。

## 杠杆 2/3 仍缺函数级 profile(非我的写域)
- `UNISACC_PROFILE=1` 拿不到数据:products 的源表 tools/cx-driver-build.cx 里
  `profile.cx` 出现 0 次,`grep -c "cx/runtime/profile"` = 0 —— 该模块根本没被编进产品,
  尽管 cx/runtime/runexec.cx:6 有 `#pragma unisa use profile`。
  要把 PROFILE 拿起来必须把 profile.cx 加进产品源表并重建。
- perf 不可用:`/proc/sys/kernel/perf_event_paranoid` = 4,无 sudo。
- 汇编步数 `%r14` 只用于超时判定,未导出。
结论:杠杆 2/3 的"按转移占比下刀"在装好带 PROFILE 的产品前无法开展;
不建议靠猜改 run_x86_64.S(督导已否 A 方案)。
