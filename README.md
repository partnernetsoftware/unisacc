# unisacc

C99 编译器,六个目标 {Linux, macOS, Windows} x {x86-64, arm64}:自己的编码器直接写出 ELF、Mach-O(ad-hoc 签名)、PE,能自举,整个产品是一个文件 `unisacc.com`。默认产品是模型驱动的:事实表 -> 生成器 -> 通用 VM 上执行的转移/动作程序,不是训练所得。当前版本、资格与已知缺口不在本页,见下表的 prd 系列文档与发布页。

## 下载与校验

发布页:<https://github.com/partnernetsoftware/unisacc/releases>。下载 `unisacc.com` 与同页的 `SHA256SUMS`,校验后使用:

```sh
shasum -a 256 -c SHA256SUMS --ignore-missing   # 只核对已下载的文件
```

卸载即删除下载的文件;回退即恢复上一版 Release 的文件。macOS 与 Windows 的启动方式和已测范围写在各版发布说明里。

## 常用命令

```sh
sh ./unisacc.com hello.c [args]               # 默认 -run:内存中编译并运行
sh ./unisacc.com hello.c -o hello             # 写本机可执行文件
sh ./unisacc.com hello.c -b osx/arm64 -o hello # 指定目标
sh ./unisacc.com -E file.c                    # 只预处理;-c/-S 的目标文件与 tape 行为见使用手册
sh ./unisacc.com tools/FILE.cx -- ARGS        # 工具与脚本都是 .cx,同样默认 -run
sh ./unisacc.com tests/run.cx -- --smoke      # 测试入口;另有 --all、--list
```

## 索引

每行一跳到达。

| 主题 | 去哪里 |
|---|---|
| 当前状态、思维树(每轮先读) | [prd.md 9.1 思维树](prd.md#91-思维树当前节点每轮先读事实阻塞下一动作原位更新) |
| 记忆宫殿(全部入口的唯一索引) | [prd.md 9.2 记忆宫殿](prd.md#92-记忆宫殿索引此处是唯一索引正文各节只指向本表) |
| 构造与链工具、能力速查 | [prd.md 2 能力速查](prd.md#2-有什么功能已实现能力速查) |
| 测试、棘轮(13 项)、批次 | [prd.md 2.2 测试](prd.md#22-测试棘轮门禁批次)、[prd.md 7 测试套件设计](prd.md#7-测试套件设计)、[tests/README.md](tests/README.md) |
| 结构五层、六片、七流、十八网、执行器 | [prd.md 3 结构组成](prd.md#3-结构组成unisacccom-的内部) |
| 架构与文件关系 | [ARCHITECTURE.md](ARCHITECTURE.md) |
| 仓库目录与"我想做 X 去哪里" | [docs/REPO-MAP.md](docs/REPO-MAP.md#我想做-x去哪里) |
| cx 语言与 cx 侧结构 | [prd_cx.md 0 cx 核心 PRD](prd_cx.md#0-cx-核心-prd定稿-2026-11-12)、[cx/spec.md](cx/spec.md)、[docs/cx/prd.md](docs/cx/prd.md) |
| cx 设计文档 | [design-core](docs/cx/design-core.md)、[stability](docs/cx/stability.md)、[ref-cpython](docs/cx/ref-cpython.md)、[ref-node](docs/cx/ref-node.md)、[ref-quickjs](docs/cx/ref-quickjs.md)、[ref-rust](docs/cx/ref-rust.md) |
| 发布流程与 CI 手册 | [prd_release.md 1 发布链](prd_release.md#1-发布链顺序即依赖)、[release/README.md](release/README.md) |
| 往期发布与历史记录 | [prd.md 9.3 往期发布](prd.md#93-往期发布与历史索引)、[prd_hist.md](prd_hist.md) |
| 工具链内部(目标文件、汇编、链接) | [docs/toolchain.md](docs/toolchain.md)、[tools/kernels.md](tools/kernels.md)、[tools/packages.md](tools/packages.md) |
| 格式与互操作 | [docs/tapebin-v1.md](docs/tapebin-v1.md)、[docs/host-interop.md](docs/host-interop.md)、[docs/glossary.md](docs/glossary.md) |
| 任务看板、账本、工作流 | [agentwork/WORKFLOW.md](agentwork/WORKFLOW.md)、`agentwork/items.tsv`、`agentwork/ledger.tsv` |
| agent 工作规则 | [AGENTS.md](AGENTS.md#unisacc-工作规则)(`CLAUDE.md` 是其软链) |
| 当前版本计划与发布说明 | [plans/v0.0.59-unisacc.md](plans/v0.0.59-unisacc.md)、[plans/](plans/) |
| 先验工作综述 | [research/prior-art.md](research/prior-art.md) |
| UJS(平行 JS/WASM 产品,论文 B) | [ujs/README.md](ujs/README.md)、[ujs/prd.md](ujs/prd.md) |
| 使用手册:全部选项、头文件、`-c`/`-S`/对象与归档、已知限制 | [docs/readme-detail.md 二](docs/readme-detail.md#二使用手册命令选项头文件限制)、[七](docs/readme-detail.md#七已知限制) |
| 冻结的历史说明:决策网表、旧测试套件表、语料观测、C99 覆盖表 | [docs/readme-detail.md](docs/readme-detail.md) |

## 目标

| 系统 | x86-64 | arm64 |
|---|---|---|
| Linux | 是 | 是 |
| macOS | 是 | 是 |
| Windows | 是 | 是(经 `slice-win-arm64` 与 `pkg.bin`,见发布说明) |

各目标的资格与未测范围只以发布页与 prd_release.md 为准;本页不声称 Windows 原生自举。
