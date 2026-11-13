# 提速热点表(只读,2026-11-02)

数据来源:`out/c44-five-lnx/entry.elf.rr`(唯一一份可用 receipt,单对象 `entry.elf`)。
**67 对象逐项耗时:UNMEASURED** —— 现有 out/ 无逐对象 receipt。
补测命令(纪元后跑):`for o in <67 entries>; do ... -o ... --time-receipt out/speed/$o.rr; done`
再用 `awk -F'\t' '$1=="RR_TIME"&&$2=="inference"{split($3,a,"=");t[a[2]]+=...}'` 汇总。

## 单对象三相占比(实测,总 ≈ 6.92 s)

| 相 | ns | 占比 |
|---|---|---|
| lower | 3,497,207,341 | **50.5%** |
| elf | 2,359,096,594 | **34.1%** |
| tapecheck | 753,788,385 | 10.9% |
| prune | 246,544,617 | 3.6% |
| construction | 45,683,629 | 0.7% |
| output | 1,215,878 | 0.02% |

## 与目标差距

- 目标:整程序 `-o` 五片 ≤ 40 s;参考 `drive.cx -c -funit lnx/x86_64`:0.0.43 = 8.14 s,79fb84e6 = 6.5 s。
- 单对象 6.92 s 里 lower+elf = 5.86 s(84.6%);**任何"整段重放"都只会放大这两相**,所以先砍重放。

## 每个热点的一刀

1. **lower 3.50 s**:消掉第二遍整段重放(一次性把可见集算完再 lower),[推断] 预期砍到一半以下;改动点在 lower 阶段驱动,不在各 arch 后端。
2. **elf 2.36 s**:`out/c44-five-lnx/` 里 67 个 `.o` 已先例化,优先查"是否重复写/重复定位段";[推断] 预期 20–30%。
3. **tapecheck 0.75 s**:稀疏表,可缓存每 unit 的 tape 指纹跨对象复用;[推断] 预期 40%。
4. 其余(prune/construction/output)合计 7%,不动。

## 2026-11-13 补测(SPEED-1,候选 unisacc.com sha e43ef36832939c7f,全表见 `build/speed-baseline.tsv`)

`UNISACC_RR_TIME=1 sh ./unisacc.com tools/ratchets.cx -o IMAGE`(10 单元,单元缓存命中,2 次,最小值):

| 相 | ms | 占五相合计 |
|---|---|---|
| e3 | 2460.4 | 55.2% |
| lower | 798.8 | 17.9% |
| elf | 464.7 | 10.4% |
| units | 368.7 | 8.3% |
| prune | 334.9 | 7.5% |

- 与上表不同:工具整程序 `-o` 路由下最大相是 e3,不是 lower;上面"每个热点的一刀"的排序只对单对象 `entry.elf` 成立。
- 冷 e2/e1、tapecheck:UNMEASURED。67 对象逐项:仍 UNMEASURED。
- `tools/speed-probe.cx` 对 3 个 `-o` 作业三次均 rc=0 但四列全 0、`per-object.tsv` 未落盘;同命令直接跑有回执。取数缺陷未修。
- 优化补丁:未写。前后对比:无。

## 补测工具(已写,纪元后用)

`tools/speed-probe.cx`(批次 `speed-probe`,check 6/6 PASS):输入 `JOBS.tsv`(每行 `NAME<TAB>ARGV...`,
经 `sh ./unisacc.com <ARGV...>` 跑,`UNISACC_RR_TIME=1`,stderr 收 receipt),输出到 stdout 的
`per-object` TSV(name/total_ms/lower_ms/elf_ms/tapecheck_ms/rc)与 `top10` 排序。零 shell、零 libc,
只走 `sy_spawn_pipe`/`sy_read`/`cx_read`/`tx_*`。
67 个对象的 `JOBS.tsv` 由调用方按同一 recipe 生成(纪元后跑):
`for o in out/c44-five-lnx/*.o; do ...; done` → 再 `sh ./unisacc.com tools/speed-probe.cx -- JOBS.tsv out/speed`。
