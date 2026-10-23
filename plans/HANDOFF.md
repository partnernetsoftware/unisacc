# unisacc 交接（2026-10-23，cc → 下一位开发者 / Codex）

> 先读：`prd.md` §0（术语权威）、`plans/v0.0.43-finish-line.md`（0.0.43 终点线）、`research/c43-workflow-lessons.md`（今天两次清空事故后的运行纪律）、`plans/cx-modules.md`、`plans/unisa-detach.md`、`plans/m4-flownet-cx-module.md` §9、`plans/flownet-consumer-configs.md`。

## 1. 现状

- **0.0.43 终点线**（`plans/v0.0.43-finish-line.md`）：M0/M1/M3/M5/M6 已完成；静默错码已修（`(*x[N])[M]` 具名拒绝 9df1417c、bit-field 复合赋值 0fc2428b）。
  - **F1 产品修复批已落地**：a21a7ed1..ffb1fdbf，根产品 SHA256 `4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda`，ident verified；657 文件差分、E3/PP 重封与 seedmemory 键见 `research/c43-f1-evidence/README.md`。cx.h 与根产品同锁安装。
  - F2 已完成：文档与探针迁移见 `research/c43-f2-evidence.md`。F3 完整187项：186 PASS、0失败、1 Darwin 宿主未验证；按终点线宿主例外单列，原 rc4 保留，见 `research/c43-f3-final-evidence/README.md`。F6 候选与三阶段 comboot 实际同 SHA、官方 fixedpoint 和双 ident 通过，见 `research/c43-f6-final-evidence/README.md`。F4 全量1118项队列仍待完成；F5 说明仍待最终闸计数。M9（Draft/Latest/签名/公开）归主人。
- **零 Python**（prd.md §1 硬目标；台账 `tests/zeropython.cx`）：调用点 1789 / unisa 依赖 92 / `.py` 411（起点 2253 / 163 / 452）。剥离计划 `plans/unisa-detach.md`：W1 全部落地；W2 起未落。
- **.cx**：内置前奏 edition 1（`include/cx.h`）、`#pragma unisa use` 模块（`src/cxmod.c`）、唯一删除守卫 `include/unisa_safedel.h`（台账 `tests/deletes.tsv`，闸 `deletecheck`）。
- **flownet**：W0–W4 绿；W6 B1 收口已落（054855e0、bd73c837），验证未完成；W5 在作者本机 scratch 已完成未落。
- **产品**：根 `unisacc.com` 不入库，由 `make model-com` 从源码确定性重建（只需宿主 cc；不依赖旧 `unisacc.com`）；`ident check` 校验来源。

## 2. 下一步（按优先级）

1. **F1 已完成**：七个补丁逐刀提交，派生文件重生成、根产品重建及 ident、完整差分、双重封和键更新已完成；记录见上。下一步从 F2 验收继续。
2. F2（M2 余项：spec TP-9 下划线句、`exec/tapecheck/README.md`、P4a 探针移入 tests/c）→ F3（CONTRACT 层全扫）→ F4（`tests/release.sh --com` 全量队列）→ F5（`research/r43-draft-notes.md` 填实）→ F6（候选构建 + comboot）。
   - F4 的 carrier import 输入需先在同一冻结 scratch 环境单独执行 `tests/bound 55 sh tests/cxbin tests/carrierimportprepared.cx --prepare`，再开 fresh `tests/release.sh --com` 完整队列；这是有界预建，不是套件通过。正式 prepare 仍核完整缓存身份和八项输出，两个消费者必须实际执行。`gate.sh` 默认的 `UNISA_JOBS=2` 会产生另一键，不得把不同环境的预建收据混用。保留旧 full41 超时；source-only 导出字节与原包装源码相同，仅省未消费的 ref 编译。

3. 长期轨道（不阻塞 0.0.43）：零 Python（剥离 W2→W3→W4；「闸即数据」：5 个通用 .cx 执行器取代逐个移植）、删除守卫第 3 轮加固（TOCTOU 改 fd 遍历、TMPDIR 祖先、天花板旗标、扫描器漏项、豁免行内容哈希）、flownet W5→W6 B2…→W7、CX2b 模块根、0.0.44 分段产品入口 `unisacc-{stage}.com`。

## 3. 必守的运行纪律（今天付出了代价）

- 工具与测试只在 scratch 导出（`git -c tar.umask=022 archive HEAD | tar -x`）里跑，**不在活检出里跑**；任何工具都不得收到 `''`、`.`、`/`、仓库根、`$HOME` 作路径参数（05:22 `warnprep.py --tools=` → `rmtree('.')` 清空过整个检出）。
- **政委补充口径（2026-10-23）**：上条路径禁项针对活检出；工具仍只在 scratch 导出中跑，scratch 导出根作为现有 ident/seal/`--check-core` 的 ROOT 是规定做法，允许。保留现有接口，不改无 ROOT 接口、不再询问此口径。
- 破坏性测试只在隔离命名空间里跑（`unshare -rm`，用空 tmpfs 遮住家目录与 `/tmp`，只把沙箱挂回）。
- 共享文件（`tests/gate.sh`、`gatelayers.py`、`stagemap.c`、`INVENTORY.md`、`gatedeps.json`、`zeropython.tsv`）只打自己的补丁块；落地用：
  `tests/landlock sh -c 'git apply --cached P && git apply P && tests/landlock --derive && release/tools/commitgate.sh --index -m MSG --suite S… -- 路径… tests/INVENTORY.md tests/gatedeps.json tests/zeropython.tsv'`
- 一律经 `release/tools/commitgate.sh` 提交；不新增/扩展 Python（`tests/zeropython.cx` 只降不升，`tests/glossarycheck.cx` G5）；删除只经守卫（`tests/safedel.cx` / `cx_rmtree`）。
- 主人保留：Draft、Latest、签名、公开发布、resume7000；`prd.md` §0 语义不改，要改先报。
