# A2 预注册路径存在性封口（2026-10-09）

只读审计：核对 `research/paper-notes-20261009.md` 所称冻结预注册是否在当前 tip 可引用，不改动论文正文、产品或 gold 数字。

## Tip

| 项 | 值 |
|---|---|
| 分支 | `main`（审计时检出；本封口 PR 自 `cursor/seal-a2-prereg-presence-d10f`） |
| HEAD 短 sha | `2ec7664` |
| 完整 sha | `2ec7664c56f1ea83420cb16acfe610c2d60da709` |

## Git 结论（presence）

**曾在仓内存在，已从 `research/` 迁至 `archive/research/`（内容未改，非删除）。**

| 路径 | tip 状态 |
|---|---|
| `research/a2-preregistration.md` | **缺失**（404） |
| `archive/research/a2-preregistration.md` | **存在** |

### 提交链（`research/a2-preregistration.md`）

1. **c8978b90**（2026-10-02 22:27 +0800）— `create mode 100644 research/a2-preregistration.md`；首版 blob `e8172fb4a876115eccbabf504fcb13a068299426`（7413 字节）。
2. **024a84ba**（2026-10-02 22:38 +0800）— 开跑前修订；blob `f40e850f94b1b90d65da678c030165a947650689`。
3. **31244660**（2026-10-09 09:08 +0800）— `archive: move 12 unreferenced research files … to archive/research, contents unchanged`；`{research => archive/research}/a2-preregistration.md`（0 行 diff，纯重定位）。

tip 上归档副本 blob 仍为 **f40e850f94b1b90d65da678c030165a947650689**（与 024a84ba 一致）。

全历史检索：`git log --all --full-history -- research/a2-preregistration.md archive/research/a2-preregistration.md` 仅上述三提交；无其它分支/标签上的并行副本；未发现其它文件名（`a2-prereg`、`预注册` 独立路径）承载同一文档。

## paper-notes 声称 vs git

| 来源 | 声称 |
|---|---|
| `research/paper-notes-20261009.md`（**85f88d03**，2026-10-08 23:13 UTC） | A2 预注册在 `research/a2-preregistration.md`，2026-10-02 冻结，无正文论文 |
| git tip | 该路径自 **31244660** 起不在 `research/`；正文在 `archive/research/a2-preregistration.md` |

时间序：**paper-notes 写入早于归档搬迁约 10 小时**（同日 SGT 上午搬迁）。笔记中的路径在 85f88d03 当时仍正确，在当前 tip 已过时。

`archive/plans/v0.0.22.md`、`v0.0.23.md` 仍写 `research/a2-preregistration.md`（计划快照，本封口不改）。

## Paper A §7.3 与 A2 分工（仅引用，未改稿）

下列文件存在且未在本 PR 修改：

- `research/unisacc-paper.md` §7.3（RQ3：构造与训练）
- `research/unisacc-paper.en.md` §7.3
- `research/arxiv-paper-a/main.tex` 对应节

§7.3 写明：产品与主张不依赖训练；Adam 对照为历史示意；**非**预注册训练研究；**配套工作 A2** 保留改写相关表述的权利；合并/扩表/多目标上的系统性 Adam/SGD 论断**留给预注册研究 A2**。paper-notes 意图是把该块「构造对训练」的系统性证据外置到 A2，SGD 仅作对照组。

## 含义

- 在 **当前 tip**，不能把「冻结于 `research/a2-preregistration.md`」当作对外可点击的仓内锚点；引用需改为 `archive/research/a2-preregistration.md`，或恢复/重定向到 `research/` 并经政委确认路径政策。
- 预注册**科学内容**仍在仓内（归档路径），本封口**不**复述 RQ/判据全文，避免与笔记混淆为二次发明。

## 下一拍（选项，非本 PR 执行）

1. **恢复路径**：若政委确认对外引用仍应为 `research/a2-preregistration.md`，从 `archive/research/` 移回或加只读 symlink/指针文件（需单独 PR，不改正文）。
2. **更正引用**：批量改 paper-notes、计划文档中的路径为 `archive/research/a2-preregistration.md`（或统一「归档研究」约定）。
3. **仓外副本**：若政委持有未入库修订，以外部 sha 对账后再提交；**禁止**在无源稿时起草四问/推翻条件。
4. **实验结果目录**：`research/a2-results/` 在 tip 上未见；开跑前仍依赖 024a84ba 修订版预注册与偏离记录流程。

## 机器可读摘要

见同目录 `seal-a2-prereg-presence-20261009.json`。
