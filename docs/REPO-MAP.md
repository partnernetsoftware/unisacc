# 仓库目录地图(从哪里看起)

30 个顶层目录按用途分四组。**规则:带 ★ 的七个目录是「产品输入」,它们的内容决定每个候选的源 SID(身份),冻结后不得改动;其余目录可改,但改动必须过棘轮与元数据闸。**

## 一、产品输入 ★(冻结面,改动会使身份全变)
| 目录 | 是什么 |
|---|---|
| `src/` ★ | 编译器驱动与六层源码(`.cx`),入口 `src/unisacc.cx`、`src/host.cx` |
| `cx/` ★ | 语言运行库与编译器模块(`cx/sys` 系统原语、`cx/compiler`、`cx/runtime` 含 M12 缓存) |
| `exec/` ★ | 各编译阶段的事实表(TSV)与构造入口;**改表即改网**(`exec/parse2` 是解析/约束表) |
| `include/` ★ | 随 APE 发的 C 头文件 |
| `seed/` ★ | 生成器种子(`seed/gen.c` 把事实表确定性地构造成网) |
| `ext/` ★ | 扩展包(`sysx`、`memx`、`textx`、`crypto`) |
| `kernel/` ★ | **生成物**,禁止手改(由 `tools/` 生成) |

## 二、构造、验证与发布(可改,过棘轮)
| 目录 | 是什么 |
|---|---|
| `tools/` | 开发与构造工具(`.cx`):生成器 `*-gen`、检查 `check-*`、发布 `release-*`、`commit-own`、`ratchets`;**找工具先看 `prd.md` 能力速查** |
| `tests/` | 批次表 `batches.tsv`、覆盖表 `coverage.tsv`、`run.cx` 与批次检查;入口 `sh ./unisacc.com tests/run.cx -- --list` |
| `weights/` | 静态输入:金标 `gold/*.tsv`、`built.uns2`(构造网的输入) |
| `release/` | 发布材料:`candidate.tsv`(候选身份唯一来源)、读回/布局工具说明;用法见 `release/README.md` |
| `work/` | 任务与账:`items.tsv`(任务)、`ledger.tsv`(进度)、`units.tsv`(分治索引) ——**管理账,不是产品事实表**:只给流程工具读写,不进「TSV→NN→unisacc」链,与 `exec/`、`weights/` 里的事实表无关 |
| `.github/` | CI 工作流(六目标原生验证 `candidate-native.yml`) |

## 三、文档与计划
| 位置 | 是什么 |
|---|---|
| `prd.md` | **当前状态与路线**(思维树、记忆宫殿、能力速查),先读它 |
| `plans/` | 版本计划与设计记录:现行 `v0.0.48-unisacc.md`、`standing-goal.md`、`design-review-log.md`、`release-notes-*.md`;过期的进 `archive/plans/` |
| `docs/` | 契约与说明(`glossary.md`、`toolchain.md`、本页) |
| `README.md` | 用户视角:下载、Quick start、已知限制 |
| `AGENTS.md`、`CLAUDE.md`、`WORKFLOW.md`、`roles/` | 协作规则与角色提示词 |
| `spec.md`、`ARCHITECTURE.md` | 条款全文与早期架构叙述(`ARCHITECTURE.md` 含历史段落,以 `prd.md` 为准) |

## 四、车道、实验与归档
| 目录 | 是什么 | 能动吗 |
|---|---|---|
| `apps/` | `csih` 应用车道 | 归 csih 车道,不碰 |
| `ujs/` | UJS 论文车道 | 归论文 B,不碰 |
| `adapter/`、`m4/`、`flownet/`、`iterate/` | ujswasm 适配与流网络实验、早期构造器 | 少量被产品目录引用(`iterate`、`adapter`),移动会改产品面 |
| `bench/`、`examples/`、`skills/`、`scripts/` | 基准程序、示例、cx 技能、信任包脚本 | 可动(`examples` 被 apps 引用) |
| `research/` | 历史研究(860 个文件) | 被 `cx/`、`exec/`、`prd.md` 引用,按需读 |
| `archive/` | 归档(旧计划、旧测试、历史参考实现);登记表 `archive/ARCHIVE.tsv` | 只读参考 |

## 生成物与本地残留(被 git 忽略,可随时清理)
`out/`(构建与测试产物)、`tmp/`、`node_modules/`、`unisacc.com`(已装产品)、`unisacc-dev`(安装回滚脚本)、`*.split-prev`。清理方法:`find out -mindepth 1 -maxdepth 1 -mmin +1440 -exec rm -rf {} +`(保留最近一天)。

## 我想做 X,去哪里
| 想做 | 去 |
|---|---|
| 看现在做到哪、有什么阻塞 | `prd.md` §9、`plans/v0.0.47-status.md`、`plans/v0.0.48-unisacc.md` |
| 找一个工具 | `prd.md` 能力速查,或 `ls tools | grep 关键字` |
| 改一张解析/约束表 | 改 `exec/parse2/*.tsv` → `tools/a12-cycle.cx`(闭环)→ `tools/affected.cx --files 表` 看影响批次 |
| 跑测试 | `sh ./unisacc.com tests/run.cx -- --list` / `tools/ratchets.cx` / `tests/run-batch.cx ROOT out/… 批次名` |
| 发布 | `release/README.md`、`release/candidate.tsv`、`plans/v0.0.46-unisacc.md` 的发布手册 |
