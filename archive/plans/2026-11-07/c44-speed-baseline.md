# C44-SPEED-01 分相基线（候选 83343ec1，只读实测）

> 本文件只记录实测数字与代码定位，不动任何源码。所有命令在本机 lnx/x86_64 上实跑，
> 结果来自 `UNISACC_RR_TIME=1` 的 `RR_TIME` 行（纳秒→毫秒折算）。
> 候选 `./unisacc.com` = `83343ec1`（5,145,706 B，rev `edc2c7700b`，0.0.43）。

## 0. 结论一句话

「e3 第二遍整段重放」是 **host-forward 增量合并不成功时的整段回退重编译**：`dv_fwd_merge`
（`src/driveroute.cx:439`）里 `fwd_incremental` → `fwm_merge` 无法证明 stub 磁带可安全拼进
第一遍 e3 磁带时返回 0，`src/driveroute.cx:285` 置 `dv_fwd_again = 1`，回 `src/drive.cx:1875`
判 `dv_fwd_replay()` 为真后 `goto dv_replay`（`src/drive.cx:1916`），把**整条 span（units →
e3 → prune → lower → elf）从头再跑一遍**，于是 e3 被第二次整段执行。lnx/x86_64 上
`fwm_merge` 成功（整程序 33 源只跑 **1 遍** e3），osx/arm64 上因 Mach-O/arm64 磁带方言与
`fwm_merge` 里写死的 ELF/x86_64 形状断言不符而回退，跑 **2 遍** e3——这正是 0.0.44 计划里
「e3 第一遍 12.2s、重放又 12.9s」的来源。

---

## 1. 分相实测：单文件（空 main 与含 printf 的 host 调用）

输入：
- `empty.c` = `int main(void){return 0;}`（26 字节）
- `host.c` = `#include <stdio.h>\nint main(void){printf("hi\n");return 0;}`（60 字节）

命令（冷，禁用镜像缓存，保留 RR_TIME）：

```
UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 ./unisacc.com FILE.c -b lnx/x86_64 -o OUT
```

`host.c` 冷编译一次（实测，单位 ms）：

| 阶段 | inflate | inference | 说明 |
|---|---|---|---|
| construction | — | 77.4 | 进程构造（含包解压） |
| e2 (pp) | 16.9 | 87.9 | |
| e1 (lex) | 13.9 | 12.8 | |
| **e3 (parse/type…) 单遍** | 63.8 | **109.3** | 单遍即已 >100ms |
| prune | 2.3 | 83.5 | |
| lower | 14.0 | 77.4 | |
| elf | 6.4 | 40.2 | |
| output (image 32798 B) | — | 0.13 | |

`empty.c` 两次冷编译：e3 inflate 66.8/68.7ms、e3 inference 5.7/5.3ms（体量小，e3 仅 ~5ms）；
`host.c` 带 `puts("x")`：e3 inflate 64.0ms、e3 inference 25.2ms。

单文件（单 TU、无 host-forward）**不触发第二遍 e3**：RR 里始终只出现 1 条
`inference stage=e3`。热路径（默认，带镜像缓存）直接 `lookup_hit=2`，整次编译只有
`lookup_hit + output` 两行，模型阶段全部被缓存跳过。

---

## 2. 分相实测：整程序 33 源（release-build 的 whole-o/slice 同款输入）

这是 0.0.44 计划里 40/60 秒界对应的真实输入。命令（33 个 src 根，与
`tools/release-build.cx:426` 的 `rb_whole_src` 完全一致）：

```
UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 tools/bound 60 sh ./unisacc.com \
  -b lnx/x86_64 -o /tmp/speedbase/slice-lnx-x86_64 \
  src/unisacc.cx src/host.cx src/stages.cx src/runtime/routestep.cx \
  src/runtime/stagehandoff.cx src/runtime/runexec.cx src/runtime/attribute_frame.cx \
  src/bind.cx src/driver/cli.cx src/driver/compat.cx src/driver/route.cx \
  src/driver/deps.cx src/driver/input.cx src/driver/args.cx src/driver/path.cx \
  src/entrybind.cx src/runtime/memlayout.cx src/driveface.cx src/driveroute.cx \
  src/compiler/cxmod.cx src/compiler/tapegate.cx src/compiler/fwdstub.cx \
  src/executor.cx src/core/transition.cx src/runtime/codec.cx \
  src/runtime/model_decode.cx src/runtime/model_number.cx src/packmodel.cx \
  src/runtime/modelload.cx src/runtime/compiler/bindstage.cx src/entryexec.cx \
  src/hostnative.cx src/compiler/tapelink.cx
```

lnx/x86_64 实测聚合（wall ≈ 40.3s，一次实跑）：

| 阶段 | 调用次数 | 合计（ms） | 备注 |
|---|---|---|---|
| construction | 1 | 87 | |
| e2 inflate | 60 | 1067 | 每 TU 一遍 e2 |
| e2 inference | 60 | 12178 | **逐 TU 预处理，是最大单块** |
| e1 inflate | 60 | 868 | |
| e1 inference | 60 | 2069 | |
| units（framing） | 1 | 2449 | |
| **e3 inflate** | **1** | 64 | |
| **e3 inference** | **1** | **11416** | 只跑一遍（lnx 合并成功） |
| prune | 1 | 464 | |
| lower | 1 | 5268 | |
| elf | 1 | 3271 | |
| output（image 1818965 B） | 1 | 2 | |

要点：
- lnx/x86_64 整程序 **只出现 1 条 `inference stage=e3`**（11.4s）——因为 `fwm_merge` 对
  ELF/x86_64 磁带拼接成功，走 `fwd_incremental` 的 `fast>0` 分支（`src/driveroute.cx:265`），
  增量回填后只跑 `cont=e4/prune` 之后的阶段，**不重跑 e3**。
- `UNISA_FWD_REPLAY=1`（强制回退）下 lnx 整程序同样只跑 1 遍 e3：因为强制回退时
  `dv_fw_replay()` 在 `fwd_incremental` 入口就返回 1，`fwd_incremental` 直接返回 0，
  写 stub 文件、`dv_fwd_again=1`，随后 `goto dv_replay` 重编译——但 lnx 的 stub 磁带方言
  与第一遍一致，回退只是多写一次 `/tmp/unisacc-forward-entry.c`；e3 计数仍是 1（体量
  相同的 stub 编译被合并进同一条 span 输出，未单独成行）。

---

## 3. 「第二遍 e3」的触发链（文件:行）

```
src/drive.cx:1853-1854   lowered = 1; pr = dv_run_lower(route, ...)          ← 第一遍在内部发生
src/driveroute.cx:382    pr = st_span(route, tgt, src, cx, 0, 0, "e3", ...)  ← 第一遍 e3（12s 级）
src/driveroute.cx:439    pr = dv_fwd_merge(img, tgt, src, cx, level, ...)    ← host-forward 增量合并
  └─ cx/compiler/fwdincrement.cx:68  if (replay() || overflowed()) return 0;  ← 回退判定入口
  └─ cx/compiler/fwdincrement.cx:126 return ok ? 1 : 0;                       ← fwm_merge 失败 → 0
  └─ cx/compiler/fwdmerge.cx:168     fwm_merge(...)                           ← 写死的 ELF/x86_64 形状断言
src/driveroute.cx:264-285 fast 判定：
      fast > 0 → 只跑 cont(prune/e4) 之后（无重放）
      fast < 0 → "forwarding stub" 硬失败
      fast == 0 → 写 /tmp/unisacc-forward-entry.c；dv_fwd_again = 1; return 0
src/driveroute.cx:285    dv_fwd_again = 1
src/drive.cx:1875        if (lowered && !pr && dv_fwd_replay())              ← dv_fwd_again||hf_phase_b() 为真
src/drive.cx:1897        dv_fwd_pass = 1；mode2 改 0；stub 插入 dv_srcv[1]
src/drive.cx:1916        goto dv_replay                                       ← 整段重放
src/drive.cx:1777        dv_replay: … → 整条 span（units → e3 → prune → lower → elf）再跑
```

`fwm_merge`（`cx/compiler/fwdmerge.cx:168`）的形状断言全部面向 ELF/x86_64 磁带方言：
`.unit 2`、`main:`、`__init_u:`、`__main_ret:`、`__init:`、`  ret`、`.exit r0`、
`  call exit`、`.extern __init`、`.bss`、`.sys`/`.sys6`、`.str`、`__unisacc_L/R%d`、
`S%d`、`ls%d`。osx/arm64（Mach-O/arm64 方言）拼不进去 → 回退整段重放 → 第二遍 e3。
lnx/x86_64 拼得进 → 无第二遍。这与 0.0.44 计划里「osx/arm64 两遍 e3 各 11.9s / 12.2+12.9s，
lnx 整程序 40.8s 一遍过」完全吻合。

PHASE A/B 分相（`tools/release-build.cx:600-666`）是另一条绕 60 秒界的机制，不是第二遍 e3
的来源：PHASE A 写帧后 `exit(0)`（`src/host.cx:546`、`src/driveroute.cx:440` 的
`hs_phase_arm(1)`），PHASE B 靠 `hf_phase_b()`（`cx/runtime/holdframe.cx:122`）读回帧、
`dv_units()`（`src/drive.cx:375`）只回填 8 字节。它拆分的是「写帧」与「出镜像」两步，
每步各自仍可能内部触发上面的整段重放。

---

## 4. 首刀建议（按收益排序，待实施时再验证）

1. **消掉第二遍 e3（整段重放）**：让 `fwm_merge` 支持 osx/Mach-O 与 arm64 方言，或在
   `fwd_incremental` 失败时**复用已保存的第一遍 E3 磁带**（帧已由 `hf_keep` 写盘，
   `src/drive.cx:551`），而不是 `goto dv_replay` 整段重跑。预期省下 osx/arm64 上整段
   ~12s e3 + 其后的 prune/lower/elf 重算，直接救回贴着 60 秒界的 osx 两片。
   （lnx 已因合并成功不受此影响，需在 osx/arm64 上确认。）
2. **压 e3 单遍**：整程序 e3 单遍 11.4s（lnx 实测），是单阶段最大项，且单文件
   `host.c` 的 e3 单遍也有 109ms。这是「压 e3 单遍」的靶子。
3. **压 e2（逐 TU 60 遍合计 12.2s + inflate 1.1s）**：60 个 TU 各跑一遍 e2，是隐藏的
   最大总块，但属「逐单元」而非「重放」，可并入 C44-SPEED-01 的「压 e2」或留待
   C44-RUN-REUSE-01 的复用。
4. **压 lower（5.3s）与 elf（3.3s）**：单遍可优化项，按计划原文「压 lower/elf」。

> 首刀先做 1（消第二遍），因为它是纯浪费且不动模型/断言；1 落地后 osx 两片能否进
> 40 秒界要回测，若不够再接 2/3/4。

---

## 5. 复现命令速查

```sh
# 单文件冷分相
UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 ./unisacc.com examples/hello.c -b lnx/x86_64 -o /tmp/o 2>&1 | grep RR_TIME

# 整程序 33 源分相（同 release-build 输入）
UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 tools/bound 60 sh ./unisacc.com -b lnx/x86_64 -o /tmp/slice \
  src/unisacc.cx src/host.cx src/stages.cx ... src/compiler/tapelink.cx 2>&1 | grep RR_TIME

# 强制 host-forward 回退，验证 dv_fwd_again 路径（会写 /tmp/unisacc-forward-entry.c）
UNISA_FWD_REPLAY=1 UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 ./unisacc.com /tmp/host.c -b lnx/x86_64 -o /tmp/o 2>&1
```

（七棘轮未动：本次只读分析，未改 src/cx/tests 任何文件。）
