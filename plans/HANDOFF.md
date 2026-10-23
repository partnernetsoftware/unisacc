# unisacc 交接（2026-10-23，cc → 下一位开发者 / Codex）

> 先读：`prd.md` §0（术语权威）、`plans/v0.0.43-finish-line.md`（0.0.43 终点线）、`research/c43-workflow-lessons.md`（今天两次清空事故后的运行纪律）、`plans/cx-modules.md`、`plans/unisa-detach.md`、`plans/m4-flownet-cx-module.md` §9、`plans/flownet-consumer-configs.md`。

## 1. 现状

- **0.0.43 终点线**（`plans/v0.0.43-finish-line.md`）：M0/M1/M3/M5/M6 已完成；静默错码已修（`(*x[N])[M]` 具名拒绝 9df1417c、bit-field 复合赋值 0fc2428b）。
  - **F1 产品修复批**已在 scratch 构建并通过 `ident` 校验，补丁在 `plans/pending/f1/0001–0007`（6 刀自带 libc + `enum E;` 回归修复），**尚未落地**。
  - F2–F6 未开始。M9（Draft/Latest/签名/公开）归主人。
- **零 Python**（prd.md §1 硬目标；台账 `tests/zeropython.cx`）：调用点 1789 / unisa 依赖 92 / `.py` 411（起点 2253 / 163 / 452）。剥离计划 `plans/unisa-detach.md`：W1 全部落地；W2 起未落。
- **.cx**：内置前奏 edition 1（`include/cx.h`）、`#pragma unisa use` 模块（`src/cxmod.c`）、唯一删除守卫 `include/unisa_safedel.h`（台账 `tests/deletes.tsv`，闸 `deletecheck`）。
- **flownet**：W0–W4 绿；W6 B1 收口已落（054855e0、bd73c837），验证未完成；W5 在作者本机 scratch 已完成未落。
- **产品**：根 `unisacc.com` 不入库，由 `make model-com` 从源码确定性重建（只需宿主 cc；不依赖旧 `unisacc.com`）；`ident check` 校验来源。

## 2. 下一步（按优先级）

1. **F1 落地**：在当前 HEAD 依次 `git am plans/pending/f1/0001…0007`；用仓内工具重生成派生文件（`kernel/unisa_headers.inc`、`exec/facts/{pp-autoinc-gen,k2-gen2,k2-libraryenv}.tsv`——目前只有 Python 生成器：unisa 的 emit-kernel 子命令与 `exec/facts/export.py`，命令行见 `tests/headerchain.cx` 第 1–2 步；它们是剥离 W3 的硬核项）；重建根产品；`.c` 语料（examples/ + tests/ 共 653 个）新旧差分只允许预期文件变化；重封 `tests/seals/exec-e3`（`tests/reseal-e3.sh`）与 `tests/seals/exec-pp-literals`（`tests/reseal-pp-literals.sh`）；更新 seedmemory 键（`tests/seedmemorykey.cx -- gen parse2`）；**新 `include/cx.h` 必须与新根一起落**（旧根编不了它）。
2. F2（M2 余项：spec TP-9 下划线句、`exec/tapecheck/README.md`、P4a 探针移入 tests/c）→ F3（CONTRACT 层全扫）→ F4（`tests/release.sh --com` 全量队列）→ F5（`research/r43-draft-notes.md` 填实）→ F6（候选构建 + comboot）。
3. 长期轨道（不阻塞 0.0.43）：零 Python（剥离 W2→W3→W4；「闸即数据」：5 个通用 .cx 执行器取代逐个移植）、删除守卫第 3 轮加固（TOCTOU 改 fd 遍历、TMPDIR 祖先、天花板旗标、扫描器漏项、豁免行内容哈希）、flownet W5→W6 B2…→W7、CX2b 模块根、0.0.44 分段产品入口 `unisacc-{stage}.com`。

## 3. 必守的运行纪律（今天付出了代价）

- 工具与测试只在 scratch 导出（`git -c tar.umask=022 archive HEAD | tar -x`）里跑，**不在活检出里跑**；任何工具都不得收到 `''`、`.`、`/`、仓库根、`$HOME` 作路径参数（05:22 `warnprep.py --tools=` → `rmtree('.')` 清空过整个检出）。
- 破坏性测试只在隔离命名空间里跑（`unshare -rm`，用空 tmpfs 遮住家目录与 `/tmp`，只把沙箱挂回）。
- 共享文件（`tests/gate.sh`、`gatelayers.py`、`stagemap.c`、`INVENTORY.md`、`gatedeps.json`、`zeropython.tsv`）只打自己的补丁块；落地用：
  `tests/landlock sh -c 'git apply --cached P && git apply P && tests/landlock --derive && release/tools/commitgate.sh --index -m MSG --suite S… -- 路径… tests/INVENTORY.md tests/gatedeps.json tests/zeropython.tsv'`
- 一律经 `release/tools/commitgate.sh` 提交；不新增/扩展 Python（`tests/zeropython.cx` 只降不升，`tests/glossarycheck.cx` G5）；删除只经守卫（`tests/safedel.cx` / `cx_rmtree`）。
- 主人保留：Draft、Latest、签名、公开发布、resume7000；`prd.md` §0 语义不改，要改先报。
