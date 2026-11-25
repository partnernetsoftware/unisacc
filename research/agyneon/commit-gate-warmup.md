# `affected-canary` 提交门预热做法

- 维护者: `agyneon` | 用途: 供 `release-train` 技能吸收

## 1. 适用条件

凡提交涉及 `cx/`、`ext/`、`tools/cxlib/` 或 `tools/affected.cx` 本身时, 均落在 `affected.cx` 的编译读集内, 会使 `run-cache` 失效。`precommit` 的 `affected-canary` 门预算上限为 10 秒 (`bound_s=10`), 冷编译耗时 25-45 秒, 直接提交必超时被拒 (`GATE_TIMEOUT gate=affected-canary cmd=sh bound_s=10`)。

## 2. 预热命令 (全核正常优先级, 不用 `taskset`/`nice`)

```sh
cd <工作树> && sh ./unisacc.com tools/affected.cx -- --registry-check
time sh ./unisacc.com tools/affected.cx -- --registry-check
```

1. 第一次运行完成冷编译并落盘 `run-cache`;
2. 第二次运行确认热跑 `< 3s` (实测 `1.087s`, `AFFECTED_REGISTRY_CHECK batches=2018`);
3. 随后立即执行 `sh ./unisacc.com tools/devsvc.cx -- git commit ...` (中间不隔其它编译)。

## 3. 实测对比 (`v0.1.3` `cx/sys/proc.cx`)

- 未预热 canary (`load1=5.75`): `precommit: GATE_TIMEOUT gate=affected-canary cmd=sh bound_s=10`, `GIT_COMMIT rc=1 reason=blocked`。
- 预热 canary 后 (热跑 `1.087s`): `gates_ms=8236`, `GIT_COMMIT rc=0 commit=5e8c0e5278 ms=23196`。
