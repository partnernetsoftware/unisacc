# 代裁：Darwin B139 双红 — cc 主刀安排解决（机房主任 1349）

墙钟：2026-10-11 13:50 SGT。授源：政委 13:49 SGT 原话「让cc去安排解决啊」。
对象：CI https://github.com/partnernetsoftware/unisacc/actions/runs/38111789528 @`82e1a9f0` — macos-14 + macos-15 双红（bootstrap `B=139 U=0`，139≈SIGSEGV），挡 Draft。

## 主责分工
1. **cc（tmux 0:1）主刀安排**：取证 → 根因假设 → 最小修 / 重跑策略（含是否勾同 SHA flaky 探针、是否需抓 core/PC、窄修域边界）。材料进 `research/` 或 `/tmp/unisacc-cdx2/observe-10m/`。回执具名页。
2. **cdx（0:2）旁核**：只读复核 cc 取证链与根因/策略是否越界；短记 `*-review-cdx-1349.md`（或后续时间戳）。
3. **cdx2（0:3）续盯 intel**：`macos-15-intel`（114388705401）至 conclusion；出口后补五 job 终表；若 intel 红摘 FAIL 行。不把长时运行当结论。

## 硬禁（本刀不破）
- 禁 bump / Draft / 公开 / Latest / seal / 强制签
- 禁放宽验收 / 删测 / 以 U 成功替代 B
- 禁依赖 m4pro 私有宿主（Darwin 证据仍走 GitHub CI 原生 runner）
- 禁把一次重跑绿写成根因闭合或 40 六格 PASS
- 禁把本机 B.2 candidate 绿（`50ab4c21…`）替代 CI Darwin 红

## 可安排范围（cc 主刀，先材料后动手）
- 保全原红日志与三项红账（acceptance10-Ctape / acceptance10-compare / bootstrap B139）
- 交叉对照 14/15（已有 cdx 1340 共证页）；记缺 core/异常 PC
- 起草最小修候选域与重跑策略（勾选表，未勾不跑）
- intel 一结后：若需 flaky 探针，在策略页勾选并**另短报政委/机房主任确认后**才触发同 SHA 失败 job 各一次（材料见 `macos14-15-flaky-rerun-options-cdx-1327.md`）
- 窄修改源：须根因定位 + 具名文件域后**另裁**；本裁定只授「安排/取证/策略」，不自动授改源落地

## 身份（采样）
- 主树 tip = origin = `ef35e718`（干净）；CI 钉 `82e1a9f0`；B.2 WT `82e1a9f0` 净；candidate `50ab4c21…` 本机构造绿
- Latest 仍 **v0.0.39**；无 Draft v0.0.40；`src/version.h`=0.0.40
- linux-arm / linux-latest：**success**；macos-14 / macos-15：**failure**（B=139 U=0）；macos-15-intel：**in_progress**（1347 采样仍未结）

## 当前假设（工作假设，非定论）
- 两格同属 bootstrap B 支崩溃症状族（exit 139≈SIGSEGV）；U=0；前序 Ctape/compare 亦红，不得自动合并为同一根因
- 缺 core / 异常 PC → 产品生成/执行缺陷 vs 宿主加载/签名适配 **未定**
- 统计镜像同体积/指令数不证明字节相等；bootstrap 未进入 B tape vs C/UT cmp

## 下一刀（cc 起）
1. 汇总取证包（原日志 SHA、三项红账、缺项清单）→ 具名页
2. 根因假设分栏（产品 / 宿主 / flaky）与否证条件
3. 最小修或同 SHA 探针的勾选策略页；intel 未结前不触发重跑
4. cdx 旁核；cdx2 盯 intel

## 产物
- 本页；paste-cc/cdx/cdx2-1349.txt；cc 回执；cdx 旁核短记；cdx2 续盯/终表
