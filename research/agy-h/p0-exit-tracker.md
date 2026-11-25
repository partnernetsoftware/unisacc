# P0 与三版整合对照页 (0.1.1 -> 0.1.2 -> 0.1.3)

维护窗口: `agy-h` | 最近更新: 2026-11-25 14:22 | 上游依据: `three-versions.md` (三版整合令) | V1 最终头: `b3e9731b6a` (`004d50e3cf` 起跳)

---

## 1. P0 九行主表对照状态

| 序号 | P0 项 | 写域与负责人 / 复核 | 当前状态 | 已落主线提交 SHA / 证据 | 在飞或待办事项 |
|---:|---|---|---|---|---|
| 1 | T11: 工具自动选 `W0000` 与 `prerun-start` 封装 | `tools/prerun-inputs.cx`、`tools/prerun-drive.cx`、`tools/prerun-start.cx` (`agy-flow` / `cdx`, `agy-h` 只读耗时复核) | 部分已落+在飞复核 | `93a4a562e2` (`P0-(2)` 已推 main); `research/agy-flow/prerun-start-d1.patch` (`sha256: 078d8cadda3b...`, `agy-h` 独立只读冷态实测 `22759 ms <= 25000 ms`, `CHECK_PRERUN_START pass=28 fail=0`) | 等 `cdx` 独立复核并由 `agy-flow` 落库 `prerun-start.cx` 及 `check-prerun-start.cx` |
| 2 | `premerge-selfcheck-main`、`-rd` 再拆与余量核查 | `tools/premerge-selfcheck.cx`、分片登记 (`ompgpt` / `cdx`) | 在飞 | `待核` (见 `ompgpt` 分片/冷态三次实测报告) | 确保全部末级分片冷态三次最大值 `<= 27.5s` 且断言多重集守恒 |
| 3 | `slow-split` 拆组冷态达标 | `tools/check-slow-split.cx`、登记 (`ompgpt` / `cdx`) | 在飞 | `待核` (107 旧批 51144ms 仅热/旧态) | 四组冷态三次最大值 `<= 27.5s` |
| 4 | `precommit-list-cap` 四组独立 fixture 拆分 | `tests/check-precommit-list-cap.cx` (`ompgpt` / `cdx`) | 在飞 | `待核` (107 W0213 加表，本轮须独立冷态拆组) | 确保超 1 MiB 拒与去上限负控有效且冷态达标 |
| 5 | `tsv-lint-root` 材料相与扫描边界 | `tools/check-tsv-lint.cx` (`ompgpt` / `cdx`) | 在飞 | `待核` (107 材料命中 11285ms) | 分开材料相绑定并核冷态 `<= 27.5s` |
| 6 | 第五项 `notcovered` (`P0-X` 第 5 项) | `notcovered` 检查与材料登记 (`ompgpt` / `cdx`; `agy-h` `P0-X` 已归因 `research/agy-h/p0-x-rc124-census.md`) | 在飞 (`P0-X` 定因完备) | `0c96607ef5` (`research/agy-h/` 普查入库: 0.1.0 W0094=54268ms / 107 W0091=46470ms) | `ompgpt` 低负载独占冷跑三次定材料构造与分片方案 |
| 7 | 六件工具修复 (1..6) 与 `P0-Y` / 候选 17 `affected` | 各件写域见第 2 节明细 | 4 件已落 / 2 件在飞 | 见第 2 节逐件 SHA (`ab3e335580`, `93a4a562e2`, `7bdc59041e`, `b9566614d0`) | 余 `(4) domain`、`(5) #15`、`(6) r39e`、候选 17 `affected` 侧 |
| 8 | D1: 冻结入场门 (`admission` + `chain-ready` 校验) | `tools/prerun-start.cx`、`tools/chain-ready.cx` (`agy-flow` / `cdx`) | 在飞复核 | 同第 1 行 (`prerun-start-d1.patch` 含 D1 入场/核验, `CHECK_PRERUN_START pass=28 fail=0`) | 等 `cdx` 复核后落库 `chain-ready` 接线 |
| 9 | 版本形态收口 (`P0-Y`) 及 `0.1.1` 离线彩排 | `tools/cxlib/releaseseq.cx`、消费者检查、`cdx` 彩排 (`cdx` / `ompgpt`) | `P0-Y` 已落 / 彩排待末段 | `b9566614d0` (`P0-Y` 已推 main); `864de15886` (`P0-W` 已推); `e8ae119856` (`src/v0.0.NN` 素材分支改名已落) | 全部工具落 `main` 后用 `0.1.1` 私有归档跑离线全绿彩排 |

---

## 2. 六件与追加 P0/P1 项逐件状态

| 项 | 状态 | 已落提交 SHA / 依据 | 当前动作 / 降级归属 |
|---|---|---|---|
| (1) watch v3 (`tools/prerun-watch.cx`) | 已落 main | `ab3e335580` | 稳定已用 (`v0.1.0` 全程实跑验证) |
| (2) 参数对齐 (`tools/prerun-inputs.cx`) | 已落 main | `93a4a562e2` (2026-11-25 推 main, `CHECK_PRERUN_INPUTS pass=28 fail=0`) | 完成 |
| (3) version-seq 语义最大值 (`tools/release-chain.cx`) | 已落 main | `7bdc59041e` (`CHECK_RELEASE_CHAIN pass=69 fail=0`) | 完成 |
| (4) 域声明提交门 (`tools/precommit.cx`) | 在飞 | `eb57262f30` (先落 `check-releaseseq` 域) | `ompgpt` 串行落库 `precommit.cx` |
| (5) `#15` release-exec notes 正文写入回读 | 待 `P1-b` 后 | 依赖 `ompgpt` 串行链 | 待落库 |
| (6) `r39e` `tests/runcache-units.cx` | 待/在飞 | `cdx` 负责、`ompgpt` 复核 | 待提交 |
| 候选 17 `affected.cx` 下游闭包修复 (`%s/` + `NAME-tool:path`) | 已撤出暂存区降 `P1` (13:58 `cdx` 终审裁定) | 完整补丁已存档 `~/.unisacc-worktree-backups/cand17-affected-66748b46.patch` 与 `~/.local/share/unisacc/research/agy-h/cand17-affected-66748b46.patch`; 暂存区/工作树已清理并释放 `cand17-affected` 锁 | `0.1.1` 起链后按 `P1` 另行登记 `cand17`+`list3` 并把 `list` 控入 `<=23s` 重提; `agy-h` 现转 `V2` `B2` 重放 |
| 候选 19 `guard-hook` prerun 证据保护 | 降 P1 (`0.1.1` 后) | `7e94f02524` 已 revert 为 `a84b2075fd`; 私有收窄补丁 `2de1ef3c...` 记入 `research/agy-h/cand19-p1-notes.md` | 暂不落库, `P0` 出口不等它 |
| 候选 20 / 21 / 22 (`check-release-post` / `constexpr-sizeof` / 封存工具) | `P0-W` 已落 / 余单列 (`P1`) | `864de15886` (`P0-W` 已入 main); `research/agy-h/car2-108-candidates.md` (第 20-22 条) | 不阻塞 `0.1.1` 开车 (`P1`) |
| `V1 (0.1.1)` 冻结头与静默窗 | 最终头 `b3e9731b6a` 待起链 | `14:21 cc` 静默窗至 `14:45`, `cdx` 单跑 `2a shard2` 与门 6 后 `agy-flow` 起链 | V1 公开前 `main` 冻结; `agy-h` 只做只读/文档/补丁文本 |
| `V2 (0.1.2)` `B2` 前端事实表化 | 首件补丁 `2.1-2.3` 就绪 | `~/.local/share/unisacc/research/agy-h/b2-k2-caps-rebased.patch` (`sha256: 46993021...`, 独立验证全绿) | 等 V1 起链后分出 `v0.1.2` 树与机器窗口许可时重放 |
| `tape` `M0` `T0.2` (`hoststub`) | 初版完成送审 `T0.4` | `.claude/worktrees/tape-m0` (`CHECK_HOSTSTUB passed=9 failed=0`, `CX_LINT findings=0`), 补丁 `tape-t02-hoststub-draft.patch` (`sha256: bf80f1ee...`) | 等 `1:cdx` 审阅(`T0.4`)及静默窗后登记 `register.cx` 并提交 `tape/m0` |
