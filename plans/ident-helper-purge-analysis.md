# `~/.cache/unisacc/ident-helper/` 被删根因分析与持久化修复方案 (cdx round-1 修订版)

状态: `ANALYSIS_ONLY` (正式链运行中, 未执行任何编译、冷测、生成器、批次或代码修改; 运行验证、并发清理竞争实测、外仓 `system-cleanup.sh` 部署验证均标 `UNMEASURED / UNCOVERED`)。

## (1) 精确归因、行号与删除完成区分

- **定时器与脚本**: `~/repos/moltbaby/systemd/services/system-cleanup.service:7` 指向 `moltbaby/systemd/scripts/system-cleanup.sh`, 由每小时触发的 `system-cleanup.timer` 驱动。
- **清理条件与删除分支**:
  - `system-cleanup.sh:90-91` 调用 `purge_stale_children_2h "$HOME/.cache/unisacc"`。
  - `:68` 筛选顶层 `-amin +120 -mmin +120` 且不含 `evidence-*` 的目录; `:70` 检查 `$entry/.no-purge`; `:74` 对整棵子树执行 `find "$entry" \( -amin -120 -o -mmin -120 \) -print -quit`; `:75` 打印 `purge dir: ...`; `:77` 执行 `rm -rf "$entry" 2>/dev/null || true` (删除错误被吞)。
- **进入删除分支 vs 删除完成**:
  - `journalctl --user -u system-cleanup.service` 在 `2026-11-27 03:00:03` (以及当天 `00:00:05`) 均记录 `purge dir: ~/.cache/unisacc/ident-helper`, 证明清理脚本在 `:75` 进入了删除分支 (并非被 `.no-purge` 或 `-amin -120` 跳过)。
  - 正式链 `ctx` 阶段 `chain-run-9.stderr` 报错 `identity-helper-absolute-path-required errno=2` (由 `tools/source-package.cx:105` 对绝对路径执行解析时遇到 `ENOENT` 发出; 而 `:358` 仅拒非绝对路径参数) 与现场目录缺失共同证实 `:77` 的 `rm -rf` 删除已完成。

## (2) 钉住时机、全入口覆盖与并发窗口

- `tools/sweep-plan.cx:211` 的 `KEEP` 仅属仓内清理计划, 不能保护外部 `system-cleanup.timer`。
- **全入口覆盖**: 须同时覆盖 `tools/chain-ready.cx:2245-2266` (`--prepare-helper`, 含默认路径与 `--helper-out`/`UNISACC_IDENT_HELPER` 显式覆盖) 与 `:2299-2306` (`cmd_prepare` 构造入口)。
- **先钉后用与并发窗口合同**:
  1. 仅对受管目录 (`$HOME/.cache/unisacc/ident-helper`) 写入 `.no-purge` (`0644`), 不对任意外部显式路径写标记。
  2. **先钉后用**: 创建受管目录后、构建或使用 helper 之前原子写入 `.no-purge`; 若写标记失败, 直接判定 `FAIL`, 不进入后续构建/放行。
  3. **版本回收**: `seed/ident.c` 变更时在已钉住目录内原位覆盖旧二进制并刷新收据。
  4. **并发清理窗口**: 针对已经越过 `:70` `.no-purge` 检查的并发清理进程, 正式链可采用已钉住链目录内的有效期内私有稳定副本, 或在 `system-cleanup.sh:77` `rm -rf` 前复检 `[ ! -e "$entry/.no-purge" ]`。

## (3) `release-keepalive.cx` 版本归属、marker 机制与部署顺序

- **版本归属与机制**: `tools/release-keepalive.cx` 不在 `w-agyneon` `8ed669467` 树内, 本文审阅的是共享检出独立版本 `c37d97583` (`:9-16`, `:70-90`, `:115-186`)。该版本按版本与 `rev` 细分, 通过 `:70-90` 原子写保活 marker, 而非批量 `touch` 文件; 若在其中增加 `ident-helper` 保活, 须沿用原子 marker 与受管根约束。
- **部署顺序**:
  1. **先部署外仓豁免**: 在 `moltbaby/systemd/scripts/system-cleanup.sh:68` 增加 `! -name 'ident-helper'`, 立即覆盖已运行与未准备的链。
  2. **再部署仓内 pin 与 keepalive**: 在 `tools/chain-ready.cx` 两入口落地先钉后用, 并按需在 `release-keepalive.cx` 补原子 marker。
  3. **长期可选迁移**: 将默认目录迁至 `~/.local/share/unisacc/ident-helper` 留作后续兼容期子集 (保留 `UNISACC_IDENT_HELPER` 显式覆盖)。
