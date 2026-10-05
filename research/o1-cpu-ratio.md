# O1：产品编出的程序与宿主 cc 的纯 CPU 比率（0.0.27，2026-10-05）

同一份 C 源码，分别用宿主 cc（Apple clang）`-O2` 和 unisacc.com 0.0.26 `-O2 -o` 编成 macOS arm64 镜像，各跑 3 次，取 `/usr/bin/time -p` user 时间的中位数；两边输出逐字节相同。机器：本机，负载约 2。

**复现**：源码在 [bench/](../bench/)（fib、sieve、matmul、hash、qsort），脚本 `UA=./unisacc.com RUNS=3 bench/cpu_ratio.sh` 直接打印下表；也可以只跑一个，例如 `bench/cpu_ratio.sh fib`。

| program | cc -O2 user s (median of 3) | unisacc -O2 user s (median of 3) | ratio | same output |
|---|---|---|---|---|
| fib | 0.02 | 0.08 | 4.0 | yes |
| sieve | 0.19 | 0.48 | 2.5 | yes |
| matmul | 0.04 | 0.26 | 6.5 | yes |
| hash | 0.27 | 1.59 | 5.9 | yes |
| qsort | 0.39 | 2.18 | 5.6 | yes |

| 程序 | 内容 |
|---|---|
| fib | 递归 fib(36) |
| sieve | 埃氏筛 6000 万 |
| matmul | 400×400 double 矩阵乘 |
| hash | FNV 风格整数循环 3 亿次 |
| qsort | 500 万个 int，用自带 qsort 排序 |

结论：产品编出的代码比 clang -O2 慢约 2.5–6.5 倍，内层整数循环和浮点循环差得最多；与第一次单次计时的结果一致。0.0.26 里 seed/gen.c 慢约 9 倍，主要原因是重复读 facts（cdx 已用缓存解决），不是这个比率本身。

后续可做（未排期）：内层循环的寄存器分配、循环不变量外提，以及 qsort 比较回调的通用调用开销。
