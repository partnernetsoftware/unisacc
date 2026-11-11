# 仓库目录地图(从哪里看起)

更新于 2026-11-10。顶层有 29 个目录(含 `.github/`)加若干根文件,按用途分四组。每个目录一行,列出:作用、是否进源 SID、所有者/车道。

**源 SID 规则**:`seed/ident.c` 的 `list-current` 计入的目录集为 `exec src kernel include weights seed cx` 七目录,加 `iterate/kernel/typekw.tsv` 与 `tools/` 下少量支撑文件(文件清单以 `list-current` 实际输出为准,不要手抄数目)。`ext/` 目录本身不在源 SID 内,但 `ext/memx`、`ext/textx` 经 `cx/mem`、`cx/text` 的符号链接参与 SID。带 ★ 的目录是产品输入(冻结面):内容决定候选身份,冻结后不得改动;其余目录可改,但须过棘轮与元数据闸。

## 一、产品输入 ★(冻结面)
| 路径 | 作用 | 进源 SID | 所有者/车道 |
|---|---|---|---|
| `src/` | 编译器驱动与六层源码(.cx),入口 `src/unisacc.cx`、`src/host.cx` | 是 | 主线 |
| `cx/` | 语言运行库与编译器模块:`sys/mem/text`(L0)、`core/driver/runtime/compiler`(L1)、`cx0`、`workspace`,另有 `cx/spec.md`、`cx/prd.md` | 是 | 主线 |
| `exec/` | 各阶段事实表(TSV)与构造入口;改表即改网(`exec/parse2` 为解析/约束表) | 是 | 主线 |
| `include/` | 随 APE 发的 C 头文件(cx.h 前导 + std 头) | 是 | 主线 |
| `seed/` | 生成器种子(`gen.c` 构造网,`ident.c` 算身份,ape 打包) | 是 | 主线 |
| `weights/` | 静态输入:金标 `gold/*.tsv`、`built.uns2` | 是 | 主线 |
| `kernel/` | 生成物,禁止手改(由 `tools/` 生成) | 是 | 生成器 |

## 二、构造、验证与发布(可改,过棘轮)
| 路径 | 作用 | 进源 SID | 所有者/车道 |
|---|---|---|---|
| `tools/` | 开发与构造工具(.cx,约 565 个):生成器 `*-gen`、检查 `check-*`、`release-*`、`commit-own`、`ratchets`、`refactor`;找工具先看 `prd.md` 能力速查 | 仅少量支撑文件 | 主线,新增须带 check 并登记批次 |
| `tools/cxlib/` | 工具共享库(.cx):`rechunk`(自动再分块)、`netpack`、`resolver`、`releasecandidate`、`wholefrontmerge`、`kernelsha` 等,以及 `mem/sys/sysx/text` 子库。`toolsloc.cx` 对应的 toolsloc 已退役,文件仅保留 | 否 | 主线 |
| `ext/` | 扩展包:`sysx`、`memx`、`textx`、`crypto`(aes/gcm/p256/der 等)、`tsvx`(TSV 读写与三方合并 `merge3.cx`,2026-11 新增) | 否(`memx/textx` 经符号链接间接参与) | 主线 |
| `tests/` | 新测试体系:`batches.tsv`、`coverage.tsv`、`run.cx`、域矩阵及棘轮基线 `*.tsv`(管理表,不构造成网);入口 `sh ./unisacc.com tests/run.cx -- --list` | 否 | 主线 |
| `build/` | 构造图与前提规则表(2026-11 新增):`graph.tsv`(一个候选的构造图,205 节点,含 split_dim/split_min,由 `tools/graph-check.cx` 校验)、`hardwire.tsv`(构造工具中「第二真值」硬连线模式清单,G1)、`preflight-rules.tsv`(0.0.50 候选链撞到的隐性前提,9 行,是 0.0.51 PF 预检工具的输入,omp-opus 整理) | 否 | 构造链维护者(G0/G1/PF 工作包) |
| `release/` | 发布材料:`candidate.tsv`(候选身份唯一来源)、读回/布局工具说明,见 `release/README.md` | 否(版本串由 candidate.tsv 注入) | 集成者 |
| `agentwork/` | 管理账:`items.tsv`(任务)、`ledger.tsv`(进度)、`units.tsv`、`freeze.tsv`、`phase-times.tsv`(超限相位与整改)、`pkg-gaps.tsv`(库包缺口)、`WORKFLOW.md`、`roles/`(角色提示词)。不是产品事实表 | 否 | 集成者写 main,各窗口按行追加 |
| `.github/` | CI 工作流:`candidate-native.yml`、`ci.yml`、`candidate-raw-export.yml`、`admission-capsule.yml`、`defender-scan.yml`、`macos-sign.yml`、`windows-signing.yml` | 否 | 集成者 |
| `scripts/` | 信任包/Authenticode 检查脚本(3 个) | 否 | 可动 |

## 三、文档与计划
| 位置 | 作用 | 进源 SID | 所有者/车道 |
|---|---|---|---|
| `prd.md` | 当前状态与路线(思维树、记忆宫殿、能力速查),先读它;§8 为目录表 | 否 | 多人,用 `commit-own --keyed` |
| `prd_hist.md`、`prd_release.md` | 状态历史与发布段落 | 否 | 主线 |
| `plans/` | 版本计划与设计记录(约 90 个文件)。现行:`v0.0.50-unisacc.md`(0.0.50 记录)、`v0.0.51-unisacc.md`(增量、自含、一键)、`v0.0.52-unisacc.md`(模型层迭代占位)、`y-design-050.md`(Y 包设计)、`rechunk-design.md`、`standing-goal.md`、`design-review-log.md`;`release-notes-0.0.46` 至 `0.0.48` 与 `0.0.53` 至 `0.0.55`。过期的进 `archive/plans/`,2026-11 文档清理移走的在 `archive/docs-2026-11/plans/`(见下) | 否 | 主线,交接单不再写带日期的 plans |
| `docs/` | 契约与说明:`glossary.md`、`toolchain.md`、`tapebin-v1*.md/tsv`、`tape-vs-wasm.md`、`host-interop.md`、`cx-spec-draft.md`、`docs/cx/`、本页 | 否 | 主线 |
| `README.md`、`spec.md`、`ARCHITECTURE.md` | 用户视角入口、条款全文、早期架构叙述(含历史段落,以 `prd.md` 为准) | 否 | 主线 |
| `AGENTS.md`、`CLAUDE.md` | 协作与工作规则 | 否 | 主人/集成者 |
| 根文件 `cxroot.tsv`、`Makefile`、`ape-receipt.txt` | cx 根登记、薄入口 make、APE 回执 | 否 | 主线 |

## 四、车道、实验与归档
| 路径 | 作用 | 进源 SID | 所有者/车道 |
|---|---|---|---|
| `apps/` | 仅 `apps/csih/` 应用车道 | 否 | 归 csih 车道,先发信封再改 |
| `ujs/` | UJS 论文车道 | 否 | 归论文 B,unisacc 侧不碰 |
| `adapter/` | `adapter/ujswasm` 适配 | 否 | 少量被产品引用 |
| `m4/`、`flownet/` | m4 探针与 flownet 通用流水线实验(`.cx`) | 否 | 可动 |
| `iterate/` | 早期构造器 `construct/` 与 `kernel/`;其中 `iterate/kernel/typekw.tsv` 进源 SID | 仅 typekw.tsv | 移动会改产品面 |
| `bench/`、`examples/`、`skills/` | 基准程序、示例(被 apps 引用)、`skills/unisacc-cx` 技能 | 否 | 可动 |
| `research/` | 历史研究(约 930 个文件;2026-11 清理移走的在 `archive/docs-2026-11/research/`),被 `cx/`、`exec/`、`prd.md` 引用 | 否 | 按需读,不追加流水账 |
| `archive/` | 归档(约 4500 个文件):旧计划 `plans/`、2026-11 文档清理 `docs-2026-11/`(543 个,索引 `INDEX.md`)、旧测试 `testing-c43-legacy/`、参考实现、历史 prd;登记表 `archive/ARCHIVE.tsv` | 否 | 只读参考 |

## 生成物与本地残留(被 git 忽略)
- `out/`:构建与测试产物、日志、候选、材料。**交接单约定**:各窗口把交接单写成 `out/<窗口>-handoff-*.md`,不入库、不写带日期的 plans;集成者按此路径取。清理:`find out -mindepth 1 -maxdepth 1 -mmin +1440 -exec rm -rf {} +`(保留最近一天)。
- `unisacc.com`(已装产品)、`unisacc-dev`(安装回滚脚本)、`*.split-prev`、`tmp/`、`node_modules/`(按需出现,当前检出没有 tmp 与 node_modules)。

## 我想做 X,去哪里
| 想做 | 去 |
|---|---|
| 看现在做到哪、有什么阻塞 | `prd.md` §9、`plans/v0.0.51-unisacc.md`、`agentwork/items.tsv`、`agentwork/ledger.tsv` |
| 找一个工具 | `prd.md` 能力速查,或 `ls tools | grep 关键字` |
| 看候选构造图与隐性前提 | `build/graph.tsv`、`build/preflight-rules.tsv`、`tools/graph-check.cx` |
| 改一张解析/约束表 | 改 `exec/parse2/*.tsv` -> `tools/a12-cycle.cx` -> `tools/affected.cx --files 表` |
| 跑测试 | `sh ./unisacc.com tests/run.cx -- --list` / `tools/ratchets.cx` / `tests/run-batch.cx` |
| 发布 | `release/README.md`、`release/candidate.tsv`、`plans/release-notes-*.md` |
