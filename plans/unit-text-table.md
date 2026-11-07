# 按单元 .text 全表（k2-precheck-04 sl-lnx）

身份：81 个 ET_REL，`entry.elf` 1860142 B，RX=1821462。镜像仍 UNATTRIBUTED。
口径：ELF `.text` `sh_size`（不是符号跨度、不是最终镜像）。
asmtext 对照：`m27.o` .text **223602** = 旧单片 223,602B，`default_path=NO`。

真源行：`plans/unit-text.tsv`。

## 各类 .text 合计（下界=该类 Σ.text；上界相同，因整节计入；镜像上界 ≤RX）

| kind | n | .text 合计 | 占 object .text 1845680 |
|---|---:|---:|---:|
| compiler | 44 | 1089281 | 59.0% |
| drive | 24 | 470770 | 25.5% |
| host | 12 | 241146 | 13.1% |
| model_decode | 9 | 155084 | 8.4% |
| runtime | 4 | 31460 | 1.7% |
| libc 独立单元 | 0 | 0 | 0 |

host 行与 compiler 有重叠分类修正后以 tsv 为准；上表按 tsv 再汇总于提交说明。

**无 libc 独立 .o**；libc/runtime 副本见 b6db5a7944 区间（合计上界 ~146KB ≈8% entry）。

## 前 20（.text 降序）

| # | key | .text | kind | default_path | migrability |
|---|---|---:|---|---|---|
| 1 | m27.o | 223602 | compiler | NO | UNBOUNDED asmtext |
| 2 | cxmod.o | 120061 | compiler | YES | UNBOUNDED |
| 3 | m22.o | 93049 | compiler | YES | UNBOUNDED source_frame |
| 4 | host.o | 92561 | host | YES | SYS-IO |
| 5 | tapegate.o | 78680 | compiler | YES | UNBOUNDED |
| 6 | driveroute.o | 69288 | drive | YES | SYS-IO |
| 7 | driveface.o | 61641 | drive | NO | MIXED（根，闭包含 asmtext） |
| 8 | m0.o | 51361 | drive | YES | MIXED dv_main |
| 9 | runexec.o | 48286 | drive | YES | SYS-IO |
| 10 | m2.o | 43791 | compiler | YES | UNBOUNDED |
| 11 | stages.o | 39347 | drive | YES | MIXED |
| 12 | driveunits.o | 39305 | drive | YES | MIXED |
| 13 | drivelink.o | 35019 | drive | YES | MIXED |
| 14 | executor.o | 33978 | model_decode | YES | UNBOUNDED |
| 15 | m20.o | 33329 | compiler | YES | SYS-IO |
| 16 | m5.o | 30660 | compiler | YES | UNBOUNDED |
| 17 | hostout.o | 30076 | host | YES | SYS-IO |
| 18 | m29.o | 30040 | compiler | YES | MIXED |
| 19 | tapelink.o | 29890 | compiler | YES | UNBOUNDED |
| 20 | m32.o | 26441 | model_decode | YES | UNBOUNDED core_run |

## 重选试点

asmtext（m27）暂缓：不在默认 `-o` 路径。

默认路径 ∩ FINITE 按体积：`route.o` 13399、`m14.o` **rts_at / rtsharedname** 8420、`driveargs.o` 6374、`m13.o` tape_shape 8543、`m36.o` cel_at 4072。

选定 **m14 / `cx/compiler/rtsharedname.cx`**：已有 `exec/facts/rt-shared.tsv` → `rt-shared-gen --check` 文本合同；2 组有限键；默认链接诊断路径会执行。见 `plans/rtshared-nn-pilot.md`。
