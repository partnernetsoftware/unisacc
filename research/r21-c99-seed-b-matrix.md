# R20-1 B：C99 网络构造器的全路由对拍（2026-10-02）

本片只比较 `exec/c/tbl.py` 所得 `.tbl` 经 `seed/net.c` 与 `exec/c/net.py` 分别构造的 `.net` 字节。生成器和打包仍由 Python 驱动，故不称 P3 或 APE 种子已迁移。

| 分片 | 实际模型数 | 结果 |
|---|---:|---|
| shared：e2/e1/e3/e4/o1/prune/nativeabi | 7 | 全部字节相同；普通 e3 892,444 B |
| features：tokenpp/tokenlex/warnlex/warnparse/warnunits/errorparse/warnpp-shared | 7 | 全部字节相同；warnparse 1,427,472 B |
| 六目标 lower 与 elf/macho/pe | 12 | 全部字节相同 |
| Linux x86-64/arm64 对象 lower 与编码 | 4 | 全部字节相同 |
| **合计** | **30** | **30/30 字节相同** |

门禁是 `seed-matrix-shared`、`seed-matrix-features`、六个 `seed-matrix-<os>-<arch>`、`seed-matrix-object`。每项自建临时 C99 构造器，重新生成图和表，Python/C99 各构造一次，按原始字节比较；每项总预算 55 秒、每个子进程最多 50 秒。旧 `seed-construct` 的声明返回与 prune 边界探针继续保留。

本机直接运行九组全绿；通过 `TERM_SH=0` 的 gate 入口复核：shared 18 秒、features 35 秒、object 11 秒，六目标各 4–6 秒，旧 seed-construct 1 秒。Terminal.app 接管在本执行环境里超时（`bound.py` 清理时又因 `/bin/ps` 权限报错），因此本片没有把 Terminal 失败写成产品失败。

后续 C：C99 构造 P3 包与 `unisacc-seed.com` 并逐字节对照 Python；D：在全域 `net=table`、N22、六平台验收后切默认。当前产品包与默认构建路径未改。
