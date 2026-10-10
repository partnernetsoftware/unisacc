# 外包 monotonic 采点：停止点 / 怎么停 / 无残留（设计入仓；未实现）

**状态（2026-10-10 23:3x）**：只写；机房主任 23:27 (2)、23:31；接 cdx/cdx2/grk 修正。不改 `build_candidate` / 产品路径；只观察到首个 make；不做真实构造。

## 停止点

**首个 make 被调用的那一刻**。`build_candidate` 第一个 make：`tests/bound 58 make model-com … MODEL_STEP=shared`。`tests/bound` 原生 helper `execvp` 查找 PATH 中的 `make`。

## 怎么停

1. scratch 目录放 `make` 垫片，PATH 前置；不改 `build_candidate` / `tests/bound`。
2. 垫片被调：写一行（monotonic_ns、pid/ppid/**pgid/sid**、完整 argv）到采点文件，**exit 97**，不 exec 真 make。
3. `build_candidate` 见 step shared rc=97 → 立刻 exit 97（预期截停）。
4. 外包 `setsid` 起 `build_candidate`；外层 `python3 tests/bound.py 55` 兜底。记 t0 / t_make（垫片）/ t_end。
5. preflight：外包在 `build_candidate` **之前**单独跑一次同 root 的 `h1source.py` 记 monotonic；内部那次仍「只有顺序」。

## 无残留证明

- 退出后对**全体**进程采 PID/PPID/PGID/SID/argv（`ps -eo …` 或 `/proc`）；再按实际 PGID/SID 筛。**枚举失败 → UNKNOWN，不当「组空」。**
- **不用 `ps -g`**（Linux 不可靠）。
- PGID **读回**（`/proc/<pid>/stat`），不默认 `$!`（setsid 可能 fork）。
- 垫片采点恰 **1 行**，argv 含 `MODEL_STEP=shared`。
- 候选目录：仅 mkdir + step-shared.log（垫片输出）；`find`+sha；无 model-cache/产物。
- `tests/bound` 可能在树外冷编 helper 缓存：照记，不算构造产物。
- 前后 sha：`version.h`、`plans/v0.0.40.md` H1-INHERIT、`archive/plans/v0.0.38.md`；status 0 行。

## 已知限制（grk）

- `bound.c` 对子进程 **自己 setsid**：垫片**不在**外包给 `build_candidate` 的进程组/会话。须另按垫片记下的 sid/pgid 再查；组内无残留 ⇏ 无逃逸进程。
- `bound` 原样返回子 rc：垫片 97 → step 97 → `build_candidate` 97。
- 默认 `SEED_C=1` 要求根上有 `unisacc.com`+`build.json` 对，否则到不了 make；实跑前具名准备或另授 `SEED_C=0`。
- 仅在冻结+bump 之后执行；0.0.39 上禁止构造（裁定）。

## 状态

设计已齐；**未实现、未运行**。
