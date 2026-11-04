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
