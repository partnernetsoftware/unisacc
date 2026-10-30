# plans/ 索引

这里只放还在用的计划和设计。完成了、或者被取代了的，移到 `archive/plans/`，当天归档的在 `archive/plans/2026-10-30/`。找归档候选用 `tools/stale.cx`，移完用 `tools/doc-links.cx --fix-moved` 修链接。

## 路线图与版本

- [v0.0.43.md](v0.0.43.md)：本版主计划。
  - [v0.0.43-units.md](v0.0.43-units.md)：单元化重构，CX0 → E2 边界。
  - [v0.0.43-finish-line.md](v0.0.43-finish-line.md)：历史终点线的入口。
- [v0.0.44.md](v0.0.44.md)、[v0.0.45.md](v0.0.45.md)：下两版的需求与验收方向。
- [dev-methodology-2026-10-28.md](dev-methodology-2026-10-28.md)：快车道和慢车道，AGENTS.md 引用。
- [candidate-to-product-2026-10-30.md](candidate-to-product-2026-10-30.md)：把候选换成产品的慢车道步骤。
- [HANDOFF.md](HANDOFF.md)：交接入口。

## 架构与语言设计

- [cx-modules.md](cx-modules.md)：`.cx` 模块化总设计。
- [cx-layers.md](cx-layers.md)：L0/L1/L2/L3 分层，只有底层对接 libc/POSIX（bdy 维护）。
- [cx-pack.md](cx-pack.md)、[cxif-v0.md](cxif-v0.md)：包清单与 CXIF 接口文本。
- [cx-include-lint.md](cx-include-lint.md)、[cxlibc-2026-10-30.md](cxlibc-2026-10-30.md)：上层禁用 `#include` 和 libc/POSIX 的两个棘轮。
- [m4-flownet-cx-module.md](m4-flownet-cx-module.md)：m4 的"流×网"范式做成 `.cx` 模块。

## L0 接口与合同

- [cx-l0-fmt-2026-10-30.md](cx-l0-fmt-2026-10-30.md)：text.fmt、text.num、sys.io。
- [cx-l0-sig-2026-10-30.md](cx-l0-sig-2026-10-30.md)：sys.sig、sy_symlink、sy_setpgid。
- [errno-per-unit-2026-10-30.md](errno-per-unit-2026-10-30.md)：错误码跨模块只走返回值。

## 当前阻塞与缺陷（等修或等装）

- [closure-once-2026-10-30.md](closure-once-2026-10-30.md)：同一份 L0 不能编进两只产品对象，产品层迁移因此卡住（grk）。
- [indirect-proto-2026-10-30.md](indirect-proto-2026-10-30.md)：间接可见的函数没有原型。候选已修，等安装。
- [unit-cache-2026-10-30.md](unit-cache-2026-10-30.md)：按内容缓存单元，解决冷编译慢的问题（排队中）。
- [defects-c43.md](defects-c43.md)：本版的缺陷清单。
- [osx-win-whole-o-2026-10-30.md](osx-win-whole-o-2026-10-30.md)：osx/win 整程序 `-o` 的进展（grk）。
- [perf-bottlenecks-2026-10-30.md](perf-bottlenecks-2026-10-30.md)：编译链的性能瓶颈。
- [peep-tighten-proposal.md](peep-tighten-proposal.md)：peep 表收紧提案，等表主人裁定。
- [foreign-host-runbook-cc.md](foreign-host-runbook-cc.md)：外宿主测试计划。
- [patches/](patches/README.md)：待合入的补丁。

## 工具与运维

- [dev-tools-2026-10-30.md](dev-tools-2026-10-30.md)：开发工具清单与待建项。每个工具的用法见 prd.md 能力速查。
- [refactor-tool-2026-10-30.md](refactor-tool-2026-10-30.md)、[symindex-design-2026-10-30.md](symindex-design-2026-10-30.md)：两大批量工具的设计和实测记录。
- [ci-bootstrap-2026-10-30.md](ci-bootstrap-2026-10-30.md)：CI 取已发布的 unisacc.com。0.0.43 发布后要把它钉进去。
- [cron-cc.md](cron-cc.md)：cc 每 5 分钟巡检的执行清单。
