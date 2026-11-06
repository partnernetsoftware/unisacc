# C44-PACKAGE-01 按功能拆解 drive.cx 大模块：方案（2026-11-01，只读调研）

目标：把 `src/drive.cx`（2016 行）按功能拆成若干新模块，重点拆 `dv_main`（771 行、105 个 `if`）。
本文只读，不改 `src/`/`cx/`/`tests/` 任何源码。结论给「实测数字 + 可复现命令」。

---

## 0. 现状实测（2026-11-01，工作区 HEAD 12f4e8d036，已装 unisacc.com 83343ec1）

### 0.1 文件体量

```sh
wc -l src/drive.cx src/driveroute.cx src/stages.cx src/driveface.cx src/stream.cx src/network.cx src/bind.cx
#   2016 src/drive.cx
#    443 src/driveroute.cx
#    395 src/stages.cx
#    363 src/driveface.cx
#     17 src/stream.cx
#     28 src/network.cx
#      7 src/bind.cx
```

`dv_main` 从第 1246 行到 2016 行（文件尾），**771 行**；其内 `if (` 共 **105** 处（`awk 'NR>=1246&&NR<=2016&&/if \(/{c++}END{print c}' src/drive.cx`）；全文件 `if ` 267 处。`dv_main` 有 **1 个 goto 标签**（`dv_replay:` 在 1777 行，唯一后跳），**61 个**块内 `extern` 前向声明的函数名（`awk` 计数）。

### 0.2 drive.cx 的符号结构（symindex 实测）

```sh
sh ./unisacc.com tools/symindex.cx -- dump src/drive.cx
```

`drive.cx` 共 **34 个顶层函数，33 个 static + 唯一导出 `dv_main`**（其余 33 个全是文件内 static，外部完全不可见）：

| 函数 | 行 | 归属（本方案） |
|---|---|---|
| dv_word | 111 | 参数解析 |
| dv_compat_word | 132 | 参数解析 |
| dv_fail | 136 | 核心（共享错误出口） |
| dv_tapebin_sha_bad | 147 | tapebin 校验 |
| dv_cx | 159 | 核心（共享） |
| dv_time | 167 | 核心（共享） |
| dv_cxm_open | 191 | plan 展开 |
| dv_cx_flags | 211 | plan 展开 |
| dv_expand | 250 | plan 展开 |
| dv_len | 289 | 核心（共享） |
| dv_put_key | 296 | 核心（cli 发布） |
| dv_cli | 302 | 核心（cli 发布） |
| dv_unit_route | 328 | 多源 framing |
| dv_need | 349 | 多源 framing |
| dv_le | 365 | 多源 framing |
| dv_units | 373 | 多源 framing |
| dv_move | 494 | 多源 framing |
| dv_hold_drop | 503 | 多源 framing |
| dv_why | 507 | 核心（共享） |
| dv_rd32 | 516 | 多源 framing |
| dv_recs_free | 521 | 多源 framing |
| dv_fwd_keep | 537 | 多源 framing |
| dv_compile_frame | 561 | 多源 framing |
| dv_fwd_splice | 642 | 多源 framing |
| dv_join | 860 | 运行 |
| dv_bind_args | 881 | 运行 |
| dv_memory | 915 | 运行 |
| dv_link_emit | 1014 | 链接 |
| dv_link_die | 1037 | 链接 |
| dv_take_unit | 1044 | 链接 |
| dv_load | 1068 | 链接 |
| dv_link_objects | 1109 | 链接 |
| dv_store_exec | 1238 | 运行 |
| **dv_main** | **1246** | **核心（唯一导出）** |

`drive.cx` 的 4 条 `#pragma unisa use`：`text.text`、`runcache`、`sys.env`、`runtime.holdframe`。

### 0.3 dv_main 的功能块（实测行号，块边界）

```
1246  函数开门 + 84 行 extern 前向声明
1331-1342  门禁 + 工具分发：ex_cap_gate、dv_word → dv_ar/dv_as/dv_view
1343-1427  参数走查：cxc_default_run / cxc_step / cxp_step(compat) / cxm_flag / 收源
1428-1436  dv_cx_flags（-cx-interface / -cx-plan 早退）
1437-1443  shown/gotb/gotn/dv_fwd_* 初始化
1445-1530  决策：cxt_object →（对象则 dv_link_objects 合带）→ CxtIn/CxtOut → cxt_decide
           → bx_diag_opt/run → cxt_text → image_fwd → cxd_default_target
1532-1563  读源：cxi_read + bx_read_act 诊断
1564-1648  tapebin 门禁（85 行内联 tb2_* 校验）
1650-1710  开包：sy_setenv(container) → hs_container →（cache: rc_open/rc_lookup_elf）
           → ex_ri_arm → ex_touch → bd_touch → st_touch → hs_read → pm_open
1711-1761  st_gap + e2 早路由：dv_cli → pm_model("e2") → ex_pp → fwrite
1763-1776  attr reset + cli 发布（st_extra_put / st_dirs）
1777-1843  dv_replay 标签 + argv 发布 + ccw + 多源 dv_units
1844-1934  产出 body：dv_run_lower / tape span / 普通 st_span；fwd replay → dv_fwd_splice/goto dv_replay
1936-1943  dv_fwd_exec → dv_store_exec
1944-1987  mode3 运行：dv_store_exec / dv_memory
1990-2015  输出模式：mode4 fwrite / mode2/0/5 ex_ri_emit + rc_store_elf
```

### 0.4 已有邻接对象（drive 层已经拆出去的两块）

`drive.cx` 之外，六层里「drive 层」已经有三个独立对象，且都是 `drive.cx` 按名调用：

| 对象 | 行 | 导出 | 用途 | use |
|---|---|---|---|---|
| `driveroute.cx` | 443 | `dv_run_lower`、`dv_fwd_replay`、`dv_fwd_file` | 转发 lowering | compiler.fwdsig/fwdmerge/fwdincrement + runtime.holdframe |
| `driveface.cx` | 363 | `dv_as`、`dv_view`、`dv_ar` | as/nm/objdump/ar | compiler.asmtext + driver.archive |
| `runcache.cx` | 688 | `rc_open/rc_arm/rc_lookup_elf/rc_lookup_e3/rc_store_elf/rc_store_e3` | 映像缓存 | sys.fs/env/sha + runtime.rcident |

也就是说，`dv_main` 的「工具分发」和「转发 lowering」已经拆走；剩下的 771 行是**参数、决策、开包、tapebin、链接、多源 framing、运行、输出**八段杂烩。

---

## 1. 三个「空壳」文件的实测结论

### 1.1 stream.cx（17 行）、network.cx（28 行）：死代码，删除

```sh
# 两个文件都不在 33-object 入口列表里
grep -cE 'stream\.cx|network\.cx' tools/entry-build.cx   # → 0

# 全仓库没有任何调用者（只有自身定义）
grep -rnE '\b(st_count|st_at|nw_count|nw_name)\s*\(' src/ cx/ exec/ | grep -v '^src/stream.cx\|^src/network.cx'
#   → 空
```

成因（git 实测）：`c2ecf4eb22`「让 -E 点出 e2 流和 pp 网，入口不再 include」把当时 `src/unisacc.cx` 里 `-E` 的调试 `printf` 路径改成读 `st_at`/`nw_name`，同 commit 新建了这两个文件。此后 `src/unisacc.cx` 被重写为只 `#pragma unisa use drive` 把一切交给 `dv_main`（见 git 该 commit 的 diff），`stream`/`network` 的 `use` 随之中断 → 两个文件彻底脱链。

`tools/check-cx-*.cx` 里同名 `nst_count`/`nst_at`/`rst_count` 是**别的模块**（`streamname`/`streamrt` 等），与 `src/stream.cx`/`src/network.cx` 无关（grep 命中的是这些，不是这两个空壳）。

**处置：删除（或归 `archive/`），不并入、不填实。** 它们承载的「7 条字节流、18 张决策网」枚举名早已不是入口的责任——真实枚举在 `exec/pipeline/*.tsv` + `stages.cx` 的 `st_gap`（cx/runtime/runexec.cx:709）+ `st_span` 路由里。删除这两个文件 + 从 `tools/entry-build.cx` 确认无引用后，`unisacc.com` 行为不变。

### 1.2 bind.cx（7 行）：占位 hook，待 G 模块落地时填实或删

```sh
grep -c 'bind.o' tools/entry-build.cx      # → keys[7] 在列（bind.o = src/bind.cx）
grep -n 'bd_touch' src/drive.cx            # → 仅 dv_main:1686 一处调用
```

`bd_touch()` 恒 `return 0`，注释自称「映像文件与内存入口绑定，这一步不写映像」。真实绑定工作有两处：`dv_memory`（drive.cx，本方案归「运行」模块）和 `src/runtime/compiler/bindstage.cx`（keys[29]，另对象）。`bd_touch` 只是 `st_touch`/`ex_touch` 同款的自检哨兵，当前无任何实现。

**处置：本方案不删、不并入，留占位。** 待第 3 节「运行」模块落地时，二选一：① 把 `dv_memory` 的内存映像绑定迁进 `bind.cx` 填实 `bd_touch`（正名）；② 若 `bindstage.cx` 已覆盖，则删掉 `bd_touch` 调用 + `bind.cx`。首刀不碰它（避免把「拆解」和「填实」两件事耦合）。

---

## 2. 拆解方案：dv_main 切成 7 个新模块 + 1 个核心

原则：
- **按 dv_main 的功能块切，不是按行均分**。每块一个 `#pragma unisa module`，导出一个入口函数，`dv_main` 退化成约 150–200 行的编排器，只做「按模式把字节交给下一块」。
- **先移「已经独立成函数的叶子」，再包「内联块」成函数**。`dv_link_objects`、`dv_units`、`dv_memory` 已经是完整函数，机械可移；tapebin 门禁、输出尾是内联块，要先包成函数再移。
- **共享的小 helper 保留在每个对象里各一份**（`dv_why`/`dv_len`/`dv_eqz` 这类 5–15 行的带界 z-string 助手）。这是 `driveface.cx`（自带 `dv_eqz`/`dv_len`）、`driveroute.cx`（自带 `dv_setwhy`）、`stages.cx`（自带 `st_put`）已有的既定惯例：避免跨对象 `use` 撞软链归一缺陷（见 plans/cx-layers.md「迁移试点阻塞」）。

### 2.1 模块总览与函数归属（「哪些函数进哪个新模块」）

| 新模块（建议路径） | 一句话职责 | 迁入的函数（+ 新入口） | use（依赖） | 被谁依赖 |
|---|---|---|---|---|
| `src/driveargs.cx` | 把命令行走成 `src/nsrc/dv_srcv[]/argstart` + 早退码 | `dv_word`、`dv_compat_word`；新 `dv_parse_args(CxcState*, CxpState*, int argc, char **argv, …)` | `cx/driver/cli.cx`(cxc_default_run/cxc_step)、`compat.cx`(cxp_step)、`src/cxmod.c`(cxm_flag)、`entrybind.cx`(bx_*)、`runcache.cx`(fwd_lib_slot/fwd_lib_count) | drive 核心 |
| `src/driveplan.cx` | -cx-plan/-cx-interface 的 use 闭包展开与早退 | `dv_cxm_open`、`dv_cx_flags`、`dv_expand`；新 `dv_plan_or_expand(int *nsrc, int funit)` | `src/cxmod.c`(cxm_*)、`host.cx`(hs_cxm_*)、`stages.cx`(读 dv_srcv) | drive 核心 |
| `src/drivetapebin.cx` | 校验 UTAPEBIN 并把文本带交回 | `dv_tapebin_sha_bad` + 内联 tb2_* 块（1564–1648）；新 `dv_tapebin_check(const char *tgt, unsigned char **gotb, long *gotn, int *as_tape)` | `cx/compiler/tapegate.cx`(tb2_*)、`driver.path`(cxp_tapebin_target) | drive 核心 |
| `src/drivelink.cx` | 把 `.o/.obj/.a` 合进一条带 | `dv_load`、`dv_take_unit`、`dv_link_emit`、`dv_link_die`、`dv_link_objects`（改签名为带 `const char **srcs`） | `cx/compiler/tapelink.cx`(tl_*)、`driver.archive.cx`(cxr_member)、`driver.path.cx`(cxp_archive_path) | drive 核心 |
| `src/driveunits.cx` | 多源 framing + 转发 stub 拼接 | `dv_unit_route`、`dv_need`、`dv_le`、`dv_units`、`dv_move`、`dv_rd32`、`dv_recs_free`、`dv_compile_frame`、`dv_fwd_keep`、`dv_fwd_splice`、`dv_hold_drop`；新 `dv_units_entry(...)` / `dv_fwd_splice_entry(...)` | `stages.cx`(st_span/st_attr_*/st_extra_*)、`runtime.holdframe`(hf_*)、`cxaf_merge` | drive 核心、driveroute(读 hf) |
| `src/driverun.cx` | -run 的 argv 绑定、内存映像执行、forward exec | `dv_join`、`dv_bind_args`、`dv_memory`、`dv_store_exec`；新 `dv_run_image(...)` | `host.cx`(hs_*)、`stages.cx`(st_span)、`mem_*`(mem_image/mem_place/mem_span_ok) | drive 核心 |
| `src/drive.cx`（剩） | 编排核心：门禁/决策/开包/e2/选路/输出 | `dv_main`（瘦身）+ `dv_fail`、`dv_cx`、`dv_time`、`dv_len`、`dv_put_key`、`dv_cli`、`dv_why` | 上述 6 个新模块 + 现有 runcache/host/stages/packmodel/executor/driveface/driveroute/entrybind | `src/unisacc.cx` |

> `dv_memory` 归 `driverun.cx` 后，`bind.cx` 的 `bd_touch` 占位见 1.2，二选一填实或删，不占首刀。

### 2.2 每个模块的边界（输入 / 输出）

| 模块 | 输入 | 输出 | 副作用 |
|---|---|---|---|
| driveargs | `argc, argv`、`CxcState*`(opts)、`CxpState*`(compat) | `nsrc, src, argstart, dv_srcv[]`（`dv_srcv` 随模块走，供核心经访问器读）；早退码（version/help/`--`/工具） | 填 opts.res/ops、`fwd_lib_slot/fwd_lib_count` 兼容库列表 |
| driveplan | `nsrc, dv_srcv[], funit` | `-1`（继续）、或早退 rc（interface/plan 打印后） | 展开后回写 `dv_srcv[]/nsrc` |
| drivetapebin | `tgt, gotb, gotn` | 更新后的 `gotb/gotn`（换成文本带）+ `as_tape=1`；坏带 → 早退 | free 旧 gotb |
| drivelink | `nsrc, srcs[]`（显式传，不再读 file-static） | `gotb/gotn`（合成带） | 内部 `dv_linkb` 状态归零 |
| driveunits | `tgt, nsrc, dv_srcv[]` | 多源 framed 字节 / fwd splice 后的 joined 字节 | 经 `st_attr_exchange` 与 stages 交换属性表、`hf_keep` 保存帧 |
| driverun | `tgt, src, cx, body/bodyn, base/reserved, dv_runargs` | rc（执行结果） | `hs_reserve/hs_commit/hs_enter`、forward exec 替代本进程 |

### 2.3 依赖方向（不破坏六层）

六层是 `host → packmodel → executor → stages → drive → bind`（ARCHITECTURE.md §2 的「六层」）；`drive.cx` 是其中「drive」层。拆出的 6 个新模块**都在 drive 层内**，互相只调用对方的导出入口，**不产生新的跨层反向边**：driveargs/driveplan/drivelink 依赖 `cx/driver`(cli/compat/route/path) 与 `cx/compiler`(cxmod/tapelink/tapegate)，driveunits/driverun 依赖 `stages`(st_span) 与 `host`(hs_*)，全部是既有向下的 `use`，与 `tools/check-cx-layers.cx` 的「编号层之间向上边 = 0」基线一致。

---

## 3. 首刀（最小、可独立验证的一刀）

**首选首刀：把 `dv_link_objects` + 5 个 helper 移进 `src/drivelink.cx`。**

理由（实测）：
- `dv_link_objects`（1109–1237）**已经是完整函数**，`dv_main` 只在一处调用（1478：`if (dv_link_objects(nsrc, &gotb, &gotn)) return 1;`）。
- 它的 5 个 helper `dv_load`/`dv_take_unit`/`dv_link_emit`/`dv_link_die` + 状态 `dv_linkb/dv_linkn/dv_linkcap` **只被它用**，无其他引用点。
- 是**纯 I/O 叶子**：进 `nsrc + 源数组`，出 `gotb/gotn`，不写模块外状态、不碰 goto/重放。
- 唯一改动接口：`dv_link_objects(int n, const char **srcs, unsigned char **out, long *outn)`，把对 file-static `dv_srcv` 的读改为显式参数，drive.cx 传 `dv_srcv`。零语义变化。

**验证（可复现）**：
```sh
# 对象链接路径：编一个 .o（-c -b lnx/x86_64 -funit），再链接，字节逐一对拍
# （现有批 link.malformed-input 与 .tape join 路径已覆盖 dv_link_objects）
sh ./unisacc.com -c -b lnx/x86_64 -funit examples/hello.c -o /tmp/h.o   # 产出对象
sh ./unisacc.com /tmp/h.o                                              # 走 dv_link_objects
# 断言：入口 33-object 列表把 drive.o 拆成 drive.o + drivelink.o 后，
#       `tools/entry-build.cx -- ROOT P3 OUT` 产出候选，行为与当前 drive.o 逐字节一致
```
验收标准：拆分前后，`dv_link_objects` 的调用点从 `drive.cx` 变 `use drivelink` + 传 `dv_srcv`，`tests/link-malformed-input.cx` 与对象/归档 join 路径 rc 不变、输出字节不变。

**次选（同为最小叶子）**：`drivetapebin.cx`——但需先把 85 行内联 tb2_* 块**包成函数**再移，机械风险略高，作为第二刀。

### 刀序（每刀独立可验证）

1. `drivelink.cx`（首刀，见上）。
2. `drivetapebin.cx`：内联 tb2_* 块包成 `dv_tapebin_check`，移出。验：`tests/fixtures/tapebin/return7.tapebin`（冻结真带，`int main(){return 7;}`）+ `tools/link-malformed-input.cx` 派生的 3 种损坏带，解码/拒收逐一对拍。
3. `driveargs.cx`：参数走查块 + `dv_word/dv_compat_word`，`dv_srcv` 随模块走 + 访问器。验：`--version/--help/--`、`-D/-U/-I/-l/-L/-x`、多源、`-run` 的 `--` 边界，全 rc 不变。
4. `driveplan.cx`：`dv_cxm_open/dv_cx_flags/dv_expand`。验：`-cx-plan`/`-cx-interface` 输出与现在逐字节一致。
5. `driveunits.cx`：`dv_units` + 10 个 helper（framing + fwd splice）。验：多源编译、`-run` 转发 stub 重放路径。
6. `driverun.cx`：`dv_memory/dv_bind_args/dv_store_exec/dv_join`。验：`-run` 内存执行、`-o` 映像写、forward exec。此刀顺带裁定 `bind.cx` 填实或删。
7. 收尾：删 `stream.cx`/`network.cx`（1.1），`dv_main` 瘦身，`tools/entry-build.cx` 的 33-object 列表加 6 个新对象，`src/unisacc.cx` 的 `use drive` 不变。

---

## 4. 明确不改 / 不碰

- 不改 `cx/` 任何文件（`cli.cx`/`compat.cx`/`route.cx`/`path.cx`/`cxmod.c`/`tapelink.cx`/`tapegate.cx` 全部只被新模块 `use`，原样消费）。
- 不改 `driveroute.cx`/`driveface.cx`/`runcache.cx`/`stages.cx`/`host.cx` 的导出签名（`dv_run_lower`、`dv_as/view/ar`、`rc_*`、`st_span`、`hs_*` 保持）。
- 不重排 `dv_main` 的控制流（`dv_replay:` goto、模式判定顺序）——本方案只**搬运功能块**，不改状态机语义，避免「拆解」和「重构逻辑」混做。
- 不填实 `bind.cx`（1.2），不改 `packmodel/executor/entrybind/entryexec/hostnative` 这些同层其他对象。

## 5. 验收口径（对应本票 acceptance）

- `plans/c44-package-plan.md` 存在，拆解到「哪些函数进哪个新模块」（见 2.1 表），不是泛泛说「要拆」。
- 每个新模块边界清晰：2.2 给了输入/输出/副作用，2.3 给了依赖/被依赖方向。
- 首刀（3.`drivelink.cx`）是「已独立成函数、单一调用点、纯 I/O、一处签名微调」的最小可独立验证的一刀。
- 空壳判定有实测：stream/network 不在 33-object 入口列表 + 全仓库 0 调用者（grep 命令可复现），建议删；bind.cx 在入口列表、唯一 `bd_touch` 调用恒 0，留占位待 driverun 落地时填实或删。
