# 候选 19 降 P1 留档 (0.1.1 车后重做)

日期: 2026-11-25
责任窗口: `agy-h` | 复核窗口: `cdx` | 裁定: `cc`
状态: 降 P1 (`0.1.1` 车后重做), 暂不落库; 共享检出 `tools/guard-hook.cx` 已由 `cc` 在 `a84b2075fd` 还原原版 (`P0` 出口不等候选 19)。

---

## 1. 既有版本与复核结论

1. 历史经过:
   - 本地首落 `7e94f02524` 因全命令级变量+证据词拦截即时在共享检出触发误拒, `cc` 已 `git revert` 为 `a84b2075fd` (本地未推)。
   - 私有隔离树 (`/tmp/cand19-priv`, 基于 `a84b2075fd`) 收窄补丁 `/tmp/cand19-guard-hook.patch` (`sha256: 2de1ef3c31ebfec1d6dd0ff8a418511a328c6fac741fc8ed00f5fdac0ffe2e91`):
     - `CHECK_GUARD_HOOK cases=217 failed=0` (含 7 类常见含变量命令 JSON 探测 `CAND19_COMMON_PROBE cases=7 failed=0`), 去掉动作限定变异 `failed=7`。
2. `cdx` 补测阻断两条真实反例 (均预期 `rc=0`, 实测误拒 `rc=2`):
   - 反例 1: `echo` 写 `/tmp/prerun-note.txt` (仅临时输出文件名含 `prerun` 证据词)。
   - 反例 2: 先 `cat` 证据文件再清理无关临时文件 (例如 `cat "$PRERUN_DIR/strict-W0001.tsv" && rm -f /tmp/unrelated.tmp`): 前段变量与证据词通过整条 `raw_cmd` 串入后一段 `rm` 分段, 导致清理无关临时文件被误拒。
3. 根因:
   - 将整条未分段 `raw_cmd` 传给 `check_seg()` 的每个子分段做 `has_var_or_subst(raw_cmd) && has_prerun_kw(raw_cmd)`, 未能按动作切分后的单个分段本身做变量和实际目标识别。

---

## 2. `0.1.1` 车后 P1 重做方向与回归门

1. 按分段独立识别: 复用 `tools/guard-hook.cx` 现有分段边界 (考虑引号内空白/常见双引号变量路径不丢 `"` 内 `$VAR`), 在每段内部仅基于该分段自身的原始片段检查变量/命令替换 (`$VAR`, `${...}`, `$(...)`, 反引号) 及操作/重定向实际目标。
2. 精确目录/文件名判据:
   - 仅当实际解析后路径(或该段目标参数/重定向右值本身)满足 `prerun/<目录>/` (按路径分量匹配, 放行 `prerunner-sandbox`) 且目标文件为 `strict-` 开头、`done.` 开头或 `supervise.log` 之一, 或含未解析变量作为操作目标自身时, 才拦截 `rm`/`unlink`/`shred`/`truncate`/`mv` 与非追加重定向 `>`/`>|`。
   - `/tmp/prerun-note.txt` 等非 `prerun/<目录>/` 字面临时路径放行。
3. 必备验收夹具 (私有树验证后再经 `cdx` 复核):
   - 保留本轮全部 35 条新正反夹具(`rm`/`unlink`/`shred`/`truncate`/`>`/`>|`/`>>`, `$VAR`/`${...}`/`$(...)`/反引号, `prerunner-sandbox`, `mv` 最简 `.orig`/含 `backup` 放行与他处/选项拒, 常见 `echo`/`mux`/`cat`/`grep`/`git`/`gh`/`sed -n` 放行);
   - 必补 `cdx` 两条反例:
     (a) `echo x > /tmp/prerun-note.txt` -> 放行 (`rc=0`);
     (b) `cat "$PRERUN_DIR/strict-W0001.tsv" && rm -f /tmp/unrelated.tmp` -> 放行 (`rc=0`), 而不影响同段内变量删证据拦截。
   - 真真实 JSON 常见命令全集探测零误拒 (`CAND19_COMMON_PROBE failed=0`) + 动作限定变异转红 + 多分段串染回归用例。
