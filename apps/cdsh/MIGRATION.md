# csih 迁到 unisacc/apps/cdsh

产品名是 csih。这个目录名按 PRD 仍叫 `cdsh`，不放进 `examples/apps/`。

这一份是从 moltbaby `skills/llm/dsh/cdsh` 在 2026-10-06 拷过来的起点。正在用的源还是 moltbaby 那份。`bin/llm`、`bin/rawcdsh`、tmux 窗口 `csih-tui` 都还指着那边。这边 selftest 绿、引用改完之前，不删 moltbaby 里的那份。

unisacc 的 main 当时已有别的未提交改动，并且比 origin 超前，所以这次没有把 `apps/cdsh` 提交或推送。

## 还没改的引用

- `moltbaby/bin/rawcdsh` 的 `CDSH_DIR` 仍是 `skills/llm/dsh/cdsh`
- `moltbaby/bin/llm` 的工具名仍是 `cdsh`，包装仍是 `bin/rawcdsh`
- `suite.c`、`agent.c` 里写死的 `/dsh/cdsh`
- 仓外 `~/.csih/run.c` 仍执行 moltbaby 的 `cdsh.sh`
- 入口文件名仍是 `cdsh.sh`、`cdsh.c`，环境变量仍是 `CDSH_*`
- 信封窗口 `0:grkwjcgmcdsh` 不改名

## 完成标准

1. 在这个目录跑 `./cdsh.sh selftest`。2026-10-06 已绿。
2. `CDSH_DIR` 改到这里，`~/.csih/run.c` 改到这里的 `cdsh.sh`。还没做。
3. 再删 moltbaby 里的旧树，或把它收成指向这里的说明。还没做。
