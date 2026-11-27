# 0.1.6 本机根噪声带

单作业, `nice -n 19`. 只跑已装根 `/home/devali/repos/unisacc/unisacc.com` (`unisacc 0.1.6`). 不跑私有 APE. 结果是 LOCAL 测量, 不是 PASS.

三条命令来自 w-agy-h `.work/m2-pm/m2_wall_bench.cx`, 不是 m1-ape 的表. m1-ape `wall-bench.tsv` 只有 stream 两行, 没有 `cli_compile_c1`. m2 三条都带 `--no-external-nn`, 并 `unsetenv UNISACC_NN_DIR`. stdin 仍用 m1-ape 的 `priv-c1-e3.out` 与 `priv-c1-e1.out`.

(1) `sh ROOT stream e4 --level 2 --source case1.c --no-external-nn`
(2) `sh ROOT stream e3 --level 2 --source case1.c --no-external-nn`
(3) `sh ROOT --no-external-nn -O2 case1.c -o ELF`

每个会话先丢 1 次热身, 再记 9 次. 三次会话是三次进程. 中位是排序后第 5 个. IQR 是第 7 个减第 3 个. 噪声带是三次中位的 (最大-最小)/最小, 百分数. load1 读 `/proc/loadavg` 第一字段, 本次全程 2.60.

| bench | s1 med | s1 IQR | s2 med | s2 IQR | s3 med | s3 IQR | spread |
|---|---:|---:|---:|---:|---:|---:|---:|
| stream_e4_c1 | 21.934 | 2.320 | 21.901 | 0.423 | 22.512 | 1.183 | 2.790% |
| stream_e3_c1 | 41.726 | 0.572 | 41.868 | 2.384 | 41.706 | 1.989 | 0.388% |
| cli_compile_c1 | 6.152 | 0.953 | 5.558 | 0.179 | 5.556 | 0.282 | 10.727% |

单位毫秒. 原始 90 行在 `plans/noise-band-016-samples.tsv`. s1 的 stream_e4 九个数排序后中位 21.934, IQR 2.320, 与表一致.
