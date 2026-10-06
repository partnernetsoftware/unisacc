# csih 在 unisacc/apps/csih

产品名是 csih。目录名也是 `csih`，不放进 `examples/apps/`。

2026-10-06 晚：活源从 moltbaby `skills/llm/dsh/csih` 迁到这里，目录从 `apps/cdsh` 改名为 `apps/csih`。`csih_home.h` 和 `csih_cols.h` 仍是状态目录和列宽的唯一定义。入口是 `csih.sh` / `csih.c`。

## 引用

- `moltbaby/bin/rawcsih` 的 `CSIH_DIR` 指向 `$HOME/repos/unisacc/apps/csih`
- `~/.cdsh/run.c`（与 `~/.csih/run.c` 同一文件）执行这里的 `csih.sh`
- `suite.c`、`agent.c` 认的路径是 `/apps/csih`
- moltbaby 旧树在 `archived/skills/llm/dsh/csih`，原路径只留 README
- 信封窗口 `0:grkwjcgmcdsh` 不改名。磁盘目录 `~/.cdsh` 仍是日记路径。`bin/llm` 和 `bin/rawcsih` 不再认 cdsh

## 自测

改名后在这个目录重跑 selftest。这次没有提交，也没有推送。
