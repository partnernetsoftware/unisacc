English | [中文](README_zh.md)

<!--
maintenance: keep README.md and README_zh.md in lockstep: same section ids in the same order, same link targets, same table rows; edit both in the same commit.
section correspondence (id -> English heading -> Chinese heading):
  what     -> unisacc            -> unisacc
  builtin  -> Built-in capabilities -> 内置能力
  download -> Download and verify -> 下载与校验
  commands -> Common commands    -> 常用命令
  index    -> Index              -> 索引
  targets  -> Targets            -> 目标
-->

<a id="what"></a>
# unisacc

A C99 compiler for six targets, {Linux, macOS, Windows} x {x86-64, arm64}: its own encoder writes ELF, Mach-O (ad-hoc signed) and PE directly, it is self-hosting, and the whole product is a single file, `unisacc.com`. The default product is model-driven: fact tables -> generators -> transition/action programs executed on a generic VM; nothing is trained. The current version, its qualification status and known gaps are not on this page; see the prd documents in the table below and the releases page.

<a id="builtin"></a>
## Built-in capabilities

Measured on the 0.0.83 public build (`unisacc 0.0.83`, APE 7399573 bytes, Linux x86-64 host; the POSIX and asm figures are 0.0.82 measurements, for the 0.0.83 fixes see [release/notes-0.0.83.md](release/notes-0.0.83.md)). Details: [prd.md 1](prd.md#1-unisacccom-是什么), [docs/readme-detail.md 二](docs/readme-detail.md#二使用手册命令选项头文件限制).

| Capability | What it is | Measured state and limits |
|---|---|---|
| (1) Built-in libc | The C99 headers and library bodies travel inside `unisacc.com`; no host libc headers are needed. `-ftrim-libc` (default) keeps only the bodies a program reaches. | Missing: `regex.h`, `strings.h`. On Linux the library calls the kernel directly. |
| (2) Built-in POSIX subset | On Linux and macOS: `unistd.h`, `sys/stat.h`, `fcntl.h`, `poll.h`, `termios.h`, `dirent.h`, processes and `sys/wait.h`, IPv4 sockets, `netdb.h` without DNS, time functions. Windows refuses these headers by name. | POSIX survey (46 probes byte-compared with gcc): 17/29 fully pass, 2 behavior defects (`sigprocmask(SIG_BLOCK)` does not block; `extern char **environ` is NULL), 10 refused or missing. Threads, sync, atomics and TLS are not implemented. Inline asm: 4/17. These are 0.0.82 measurements; 0.0.83 fixed sigprocmask masking, environ and the wchar_t guard, and basic `__asm__` plus non-whitelisted attributes are now named refusals (previously silently ignored); the POSIX and asm surfaces were not fully re-measured after the fixes. |
| (3) cx | A C extension: `#pragma unisa module NAME` and `#pragma unisa use MOD[::sym [as alias]]` instead of `#include`; layered library `sy_*`, `mm_*`, `tx_*`; runtime (not compile-time) ownership and bounds checks. | Spec: [prd_cx_spec.md](prd_cx_spec.md), [prd_cx.md](prd_cx.md). The module name must equal the file stem. |
| (4) Script mode | `-run` is the default: a `.c` or `.cx` file is compiled in memory and run, nothing is written to disk. Arguments for the program go after `--`. | `-o` writes a file only when asked. |

Also: one APE file runs on Linux and macOS (x86-64, arm64); it writes ELF, Mach-O and PE for six targets; `-c` and `-S` give objects and assembly.

Real runs (0.0.83):

```text
$ sh ./unisacc.com --version
unisacc 0.0.83
$ sh ./unisacc.com h.c a b          # h.c prints argc
hello c argc=3
$ sh ./unisacc.com hello.cx -- a b  # #pragma unisa use text.text::tx_len
hello cx argc=3 len=5
$ sh ./unisacc.com h.c -o h && ./h x
hello c argc=2
$ sh ./unisacc.com h.c -b win/x86_64 -o h.exe   # file: PE32+ executable (console) x86-64
```

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

Reading order for newcomers (by role; every other file is on demand, not a front page):

- Users: this page -> clauses and limits in [prd_spec.md](prd_spec.md), [prd_release.md](prd_release.md).
- Contributors: this page -> [prd_arch.md](prd_arch.md) -> [prd.md 2 capability quick reference](prd.md#2-有什么功能已实现能力速查) -> for `.cx` read [prd_cx_spec.md](prd_cx_spec.md), structure questions in [prd_cx.md](prd_cx.md) -> terms in [docs/glossary.md](docs/glossary.md) -> current version in [plans/INDEX.md](plans/INDEX.md).
- Agents: [AGENTS.md](AGENTS.md) -> [prd_mempalace.md](prd_mempalace.md) -> [prd_cx_spec.md](prd_cx_spec.md) -> [plans/devsvc.md](plans/devsvc.md); numbers in [prd_hist.md](prd_hist.md) and [docs/readme-detail.md](docs/readme-detail.md) are frozen, not current status.
- Releasers: [prd_mempalace.md](prd_mempalace.md) -> [prd_release.md](prd_release.md) -> release tool rows in prd.md 2 -> current version plan in [plans/INDEX.md](plans/INDEX.md).

One hop to each destination. Link targets are Chinese documents; the anchors are unchanged.

| Topic | Where |
|---|---|
| Current status, thinking tree (read first every round) | [prd.md 9.1 thinking tree](prd.md#work-tree) |
| Memory palace (the single index of all entry points) | [prd.md 9.2 memory palace](prd.md) |
| Build and chain tools, capability quick reference | [prd.md 2 capability quick reference](prd.md#2-有什么功能已实现能力速查) |
| Tests, ratchets (13), batches | [prd.md 2.2 tests](prd.md#22-测试棘轮门禁批次)、[prd.md 7 test suite design](prd.md#7-测试套件设计)、[tests/README.md](tests/README.md) |
| Structure: five layers, six slices, seven streams, eighteen nets, executor | [prd.md 3 structure](prd.md#3-结构组成unisacccom-的内部) |
| Architecture and how files relate | [prd_arch.md](prd_arch.md) |
| Repository layout and "I want to do X, where do I go" | [prd.md §8.1](prd.md#repo-map) |
| The cx language and cx-side structure | [prd_cx.md 0 cx core PRD](prd_cx.md#0-cx-核心-prd定稿-2026-11-12)、[cx/spec.md](cx/spec.md)、[docs/cx/prd.md](docs/cx/prd.md) |
| cx design documents | [design-core](docs/cx/design-core.md)、[stability](docs/cx/stability.md)、[ref-cpython](docs/cx/ref-cpython.md)、[ref-node](docs/cx/ref-node.md)、[ref-quickjs](docs/cx/ref-quickjs.md)、[ref-rust](docs/cx/ref-rust.md) |
| Release process and CI handbook | [prd_release.md 1 release chain](prd_release.md#1-发布链顺序即依赖)、[release/README.md](release/README.md) |
| Past releases and history | [prd.md 9.3 past releases](prd.md)、[prd_hist.md](prd_hist.md) |
| Toolchain internals (object files, assembler, linker) | [docs/toolchain.md](docs/toolchain.md)、[tools/kernels.md](tools/kernels.md)、[tools/packages.md](tools/packages.md) |
| Formats and interoperability | [docs/tapebin-v1.md](docs/tapebin-v1.md)、[docs/host-interop.md](docs/host-interop.md)、[docs/glossary.md](docs/glossary.md) |
| Task board, ledger, workflow | [prd_mempalace.md](prd_mempalace.md) (the only coordination entry; `archive/agentwork-retire-2026-11/` is retired, history only) |
| Agent working rules | [AGENTS.md](AGENTS.md#unisacc-工作规则) (`CLAUDE.md` is a symlink to it) |
| Current version plan and release notes | [plans/INDEX.md](plans/INDEX.md) (current version plan row)、[release/README.md](release/README.md) |
| Prior-work survey | [research/prior-art.md](research/prior-art.md) |
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
