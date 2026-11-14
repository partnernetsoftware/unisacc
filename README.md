English | [中文](README_zh.md)

<!--
maintenance: keep README.md and README_zh.md in lockstep: same section ids in the same order, same link targets, same table rows; edit both in the same commit.
section correspondence (id -> English heading -> Chinese heading):
  what     -> unisacc            -> unisacc
  download -> Download and verify -> 下载与校验
  commands -> Common commands    -> 常用命令
  index    -> Index              -> 索引
  targets  -> Targets            -> 目标
-->

<a id="what"></a>
# unisacc

A C99 compiler for six targets, {Linux, macOS, Windows} x {x86-64, arm64}: its own encoder writes ELF, Mach-O (ad-hoc signed) and PE directly, it is self-hosting, and the whole product is a single file, `unisacc.com`. The default product is model-driven: fact tables -> generators -> transition/action programs executed on a generic VM; nothing is trained. The current version, its qualification status and known gaps are not on this page; see the prd documents in the table below and the releases page.

<a id="download"></a>
## Download and verify

Releases page: <https://github.com/partnernetsoftware/unisacc/releases>. Download `unisacc.com` and the `SHA256SUMS` file from the same page, verify, then use:

```sh
shasum -a 256 -c SHA256SUMS --ignore-missing   # only checks the files you downloaded
```

To uninstall, delete the downloaded files; to roll back, restore the files of the previous release. How to launch on macOS and Windows, and what has been tested there, is written in each release's notes.

<a id="commands"></a>
## Common commands

```sh
sh ./unisacc.com hello.c [args]               # default -run: compile in memory and run
sh ./unisacc.com hello.c -o hello             # write a native executable
sh ./unisacc.com hello.c -b osx/arm64 -o hello # choose a target
sh ./unisacc.com -E file.c                    # preprocess only; for -c/-S objects and tape behavior see the manual (docs/readme-detail.md)
sh ./unisacc.com tools/FILE.cx -- ARGS        # tools and scripts are .cx, also -run by default
sh ./unisacc.com tests/run.cx -- --smoke      # test entry; also --all, --list
```

<a id="index"></a>
## Index

One hop to each destination. Link targets are Chinese documents; the anchors are unchanged.

| Topic | Where |
|---|---|
| Current status, thinking tree (read first every round) | [prd.md 9.1 thinking tree](prd.md#91-思维树当前节点每轮先读事实阻塞下一动作原位更新) |
| Memory palace (the single index of all entry points) | [prd.md 9.2 memory palace](prd.md) |
| Build and chain tools, capability quick reference | [prd.md 2 capability quick reference](prd.md#2-有什么功能已实现能力速查) |
| Tests, ratchets (13), batches | [prd.md 2.2 tests](prd.md#22-测试棘轮门禁批次)、[prd.md 7 test suite design](prd.md#7-测试套件设计)、[tests/README.md](tests/README.md) |
| Structure: five layers, six slices, seven streams, eighteen nets, executor | [prd.md 3 structure](prd.md#3-结构组成unisacccom-的内部) |
| Architecture and how files relate | [ARCHITECTURE.md](ARCHITECTURE.md) |
| Repository layout and "I want to do X, where do I go" | [prd.md §8.1](prd.md#repo-map) |
| The cx language and cx-side structure | [prd_cx.md 0 cx core PRD](prd_cx.md#0-cx-核心-prd定稿-2026-11-12)、[cx/spec.md](cx/spec.md)、[docs/cx/prd.md](docs/cx/prd.md) |
| cx design documents | [design-core](docs/cx/design-core.md)、[stability](docs/cx/stability.md)、[ref-cpython](docs/cx/ref-cpython.md)、[ref-node](docs/cx/ref-node.md)、[ref-quickjs](docs/cx/ref-quickjs.md)、[ref-rust](docs/cx/ref-rust.md) |
| Release process and CI handbook | [prd_release.md 1 release chain](prd_release.md#1-发布链顺序即依赖)、[release/README.md](release/README.md) |
| Past releases and history | [prd.md 9.3 past releases](prd.md)、[prd_hist.md](prd_hist.md) |
| Toolchain internals (object files, assembler, linker) | [docs/toolchain.md](docs/toolchain.md)、[tools/kernels.md](tools/kernels.md)、[tools/packages.md](tools/packages.md) |
| Formats and interoperability | [docs/tapebin-v1.md](docs/tapebin-v1.md)、[docs/host-interop.md](docs/host-interop.md)、[docs/glossary.md](docs/glossary.md) |
| Task board, ledger, workflow | [prd_mempalace.md](prd_mempalace.md) (the only coordination entry; `agentwork/` is retired, history only) |
| Agent working rules | [AGENTS.md](AGENTS.md#unisacc-工作规则) (`CLAUDE.md` is a symlink to it) |
| Current version plan and release notes | [plans/v0.0.59-unisacc.md](plans/v0.0.59-unisacc.md)、[plans/](plans/) |
| Prior-work survey | [research/prior-art.md](research/prior-art.md) |
| UJS (parallel JS/WASM product, paper B) | [ujs/README.md](ujs/README.md)、[ujs/prd.md](ujs/prd.md) |
| Detailed usage manual (Chinese) and known-limits reference: all options, headers, `-c`/`-S`/objects and archives | [docs/readme-detail.md 二](docs/readme-detail.md#二使用手册命令选项头文件限制)、[七](docs/readme-detail.md#七已知限制) |
| Frozen historical notes (Chinese): decision-net tables, old test-suite table, corpus observations, C99 coverage table | [docs/readme-detail.md](docs/readme-detail.md) |

<a id="targets"></a>
## Targets

| System | x86-64 | arm64 |
|---|---|---|
| Linux | yes | yes |
| macOS | yes | yes |
| Windows | yes | yes (via `slice-win-arm64` and `pkg.bin`, see the release notes) |

Qualification and untested scope of each target are defined only by the releases page and prd_release.md; this page does not claim native Windows self-hosting.
