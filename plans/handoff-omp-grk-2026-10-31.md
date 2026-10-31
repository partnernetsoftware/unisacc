# 交接：omp-grk 接手 0.0.43 收尾（2026-10-31 晚，cc 安排，主人裁定）

主人原话：「你来安排。我要的是结果；等 grk 空下来后不要给它新任务了，你转为安排 omp-grk 去跟。」随后主人把 grk 关闭了：**全部由你（omp-grk）接，包括候选第 2 项。**cc 继续巡检、补工具、记账，不抢刀。

先读：仓库根 `CLAUDE.md`（规则，必须遵守）、`prd.md` 的思维树和能力速查、[release-0.0.43.md](release-0.0.43.md) 的 STATE43 块、[census-2026-10-31.md](census-2026-10-31.md)、[v0.0.44.md](v0.0.44.md)。

## 目标

把 0.0.43 做到 `sh ./unisacc.com tests/run.cx -- --all` 输出 QUALIFIED。当前（10-31 晚）：必需格 724，已覆盖 547，缺 58，待外平台 73，待产品改 7。发布、签名、Draft、Latest、公开一律不碰，等主人授权。

## 你的车道

`src/` 和产品层、`cx/runtime/`、`include/`、发布链（`tools/release-build.cx`、`release-ape.cx`、`com-tree.cx`）。别的车道：`cx/sys|mem|text|core|cxmod` 等归 bdy（窗口 `omp-bdy`），`apps/csih/` 归 csih，`ujs/` 不碰。要动别人的文件先发信封（`mux envelope "1:omp-bdy" 标题 正文`）。

## 最后一只候选：9 项产品改动（按风险排序；每项走快车道）

快车道 = 改 → 对应红批次转绿，一小时内，不触 SID/PC/GEN；验证用当前已装产品加私有网/探针/表 lint/字节等价回归。一小时做不动就停，告诉 cc，由 cc 豁免进 0.0.44。

1. `link-undefined-function`（**已完成**，提交 1214ea7c41，在私有候选 3b00b4c8 上 6/0）。
2. image 路线补 `source` 阶段和 `e3-identity.tsv`（RI 迁移漏项）。**grk 已被主人关闭，这项现在归你（omp-grk），没有任何半成品留在工作区。** 授权你动 `drive.cx`，条件：净行数不增（先把 drive.cx 里自成一段的代码搬进 `cx/runtime/` 腾出行，再加 ex_ri 的前端后/e3 前调用），最终 ≤ 2035 行、最长函数和 if 数不升、只加调用不加逻辑、棘轮 7/7；限 90 分钟，做不成告诉 cc（cc 豁免并退役三个批次，会连带降 7 个以上的格，所以先尽力）。已知事实（grk 的结论）：image 路线要的 e3 身份是前端跑完后的 tokens 和属性，不是源码原文；runexec 只在 drive.cx 第 1700 行附近那一次 `ex_ri_arm` 看到已读好的源码，`st_span` 不交出中间字节；旧编译器（79fb84e6）image 先开 source 阶段，tape 才保持 e3；伴随文件 e3-identity.tsv 在前端之后、e3 之前写，账本行绑的是空字节加 `ri/e3-identity`；读和比对的判定已在 `cx/runtime/source_frame.cx`；`ex_ri_arm` 现在对 tape 和 image 都盖成 e3 加路线。验收：`ri-image-single`（对照 79fb84e6 上 30/0）、`ri-image-multi`、`ri-image-pem` 转绿。
3. 加载器校验：页脚长度翻位、D 行阶段名（lower→mower）、terminal elf 行离开 image/O0，新模型驱动现在接受坏包。修在 `cx/runtime` 的包解析里，不加 drive.cx。验收：`negative-model-loader`（现 99 条断言 7 条失败）、`negative-model-loader-terminal`。
4. `-ferror-limit=N` 生效：资源 `\0cli/error-limit` 账本里有，诊断路径没读；期望只打第一个错误然后 `too many errors emitted, stopping now`。验收：`negative-diagnostics`（只剩 2 条失败）。
5. `-nostdinc` 不再展开自带头（`-nostdinc -E` 仍展开 stddef.h）。验收：`pp-cli-includes`。
6. `-S --tapebin` 写 UTAPEBIN（现写出的仍是文本磁带）。验收：`formats-tapebin`。
7. `-run bad.tape` 要报 unknown opcode（现报 top-level construct）；不要改测试期望。验收：`link-malformed-input`。
8. phase-timing 六个 lnx 机内打点：`construction.cold/hot`、`loading.model-inflate.hot`、`inference.in-product.hot`、`output.duration.cold/hot`。空闲机器跑三次，不设 UNISACC_RR_TIME，不能让编译变慢。验收：`phase-timing-stamps`（现 stamp_lines=0）。
9. 启动器 `ulimit -v` 预检（`res.mem.compile-as-limit`）：只改启动器 sh 头，按每片自己的 PT_LOAD memsz 加余量看，不够就印 `run: out of memory`，退出 2，不写输出；不改 `asmtext.cx`、不动切片。

已决定不做：用户程序 malloc（R5）、`UNISA_MAXSTEPS` 校验和预算（R6）、`-Werror`（C44-WERROR-01）。

做完 2–9 之后一次性：`tools/release-build.cx` 打五片 → `release-ape.cx` 打包 → 装 → 冻结。**打包安装前先告诉 cc。**

## 还剩的格（你的）

`elf.pos.exact-image-independent`、`elf.pos.windows-args-template`、`lower.pos.darwin-carry`、`lower.pos.windows-gates`、`lower.pos.arm-narrowing`：对产品出的镜像解码（ARM64 的做法见 `tests/elf-arm64-image.cx`、`elf-x86-image.cx`）。能证明某格只有单流容器才看得到，写一句理由给 cc，cc 并入 R7；证明不了就做测试，不要整批并。`e3.ref.attr-count-overstated` 已改句、仍 MISSING，待测。

## 红批次转绿后的账

引用红批次的矩阵格被 cc 降成了 `STALE-RED`：`ri.frame.hash-mismatch`、`ri.id.sealed-live-header`、`ri.id.e3-companion-states`、`ri.class.source.MISSING`、`ri.class.inc-quote.PRESENT`、`ri.route.image`、`ri.route.multi-tu`、`loader.footer`、`loader.d-row`、`diag.error-limit`、`tapebin.roundtrip`、`e2.cli.include-path`。批次在**已装产品**上转绿后，用 `tools/matrix-flip-gen.cx` 把它们翻回 COVERED_LOCAL（MISSING→COVERED_LOCAL），或告诉 cc 来翻。每次提交只用 `tools/commit-own.cx --keyed` 带自己的行。

## 工具和规矩速记

- 默认直接 `sh ./unisacc.com FILE.cx -- ARGS`；新 `.cx` 不手写 `#include`，不直接调 libc；七个棘轮（`tools/ratchets.cx`）必须 7/7，`srcshape` 只瘦不胖（drive.cx 钉 2035 行）。
- 每条命令 ≤60 秒；单机同时一组重测试；`tools/census.cx` 是在已装产品上跑全部无材料批次的普查（`--only ID,.. --redo ID,..`），每项做完用它复验相关批次。
- 不用 `git stash/reset/checkout/clean`；只提交自己的路径；`out/` 放日志。
- 别通过删断言、缩覆盖、放宽时限变绿。
- 不要等 cc 回话：每项落地提交一次，提交信息写清验收批次。只有三种情况找 cc：①一项一小时做不动；②需要改 drive.cx 且腾不出行；③要动 SID/PC/GEN。
