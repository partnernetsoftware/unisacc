<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: ext.procx

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| np_result | typedef struct np_result { int rc; char *out; size_t outn; char *err; size_t errn; int residue; } np_result; | Rust process::Output | none | impl | impl | unverified | exists | 现在 ext.sysx | incubating | 0.1.0 | none | - |
| np_run | int np_run(char *const argv[], const char *cwd, char *const envadd[], int secs, np_result *res, char *why, size_t wl); | Rust Command::output; Node execFile | -errno;ret:why | impl | impl | unverified | exists | 现在 ext.sysx;由 np_exec 取代 | incubating | 0.1.0 | none | - |
| np_free | void np_free(np_result *r); | Rust drop(Output) | none | impl | impl | unverified | exists | 现在 ext.sysx | incubating | 0.1.0 | none | - |
| np_exec | int np_exec(const sy_spawn_t *o, mg_buf *out, mg_buf *err, int64_t cap, sy_exit_t *st); | Rust Command::output; Node spawnSync 带 maxBuffer | -errno;-EMSGSIZE | impl | impl | impl:两路各一读线程不可用,改用 PeekNamedPipe 轮询 | new | 带界捕获;err 为 0 时并入 out;任一路超过 cap 记 -EMSGSIZE 并杀子进程 | incubating | 0.1.0 | none | - |
| sy_kill_tree | int sy_kill_tree(int root, int deadline_ms, const int *keep, int nkeep, char *rep, int cap); | 无(Node tree-kill 近似) | -errno | impl:/proc 遍历 | refuse:ENOSYS cap=proc-tree | refuse:ENOSYS cap=proc-tree | exists | 现在 ext.sysx | incubating | 0.1.0 | none | - |
| rp_kill_tree | int rp_kill_tree(int root, int deadline_ms, const int *keep, int nkeep, char *rep, int cap); | 同 sy_kill_tree | -errno | impl | refuse:ENOSYS cap=proc-tree | refuse:ENOSYS cap=proc-tree | alias-of:sy_kill_tree | sysx.reaper 的实现名 | incubating | 0.1.0 | none | - |
| sy_child_subreaper | int sy_child_subreaper(void); | Linux prctl PR_SET_CHILD_SUBREAPER | -errno | impl | unverified | unverified | exists | 现在 ext.sysx | incubating | 0.1.0 | none | - |
| rp_subreaper | int rp_subreaper(void); | 同 sy_child_subreaper | -errno | impl | unverified | unverified | alias-of:sy_child_subreaper | sysx.reaper 的实现名 | incubating | 0.1.0 | none | - |
| sy_child_depth | int sy_child_depth(void); | 无 | none | impl | impl | unverified | exists | 现在 ext.sysx | incubating | 0.1.0 | none | - |
| sy_child_guard | void sy_child_guard(int argc, char **argv); | 无 | noreturn | impl | impl | unverified | exists | 现在 ext.sysx | incubating | 0.1.0 | none | - |
