# Gate diagnostic layers

`gatequeue.py` keeps the full suite list and its measured-duration scheduling.
Layers describe **the breadth of the claim**, not an expected runtime. A stage
test may be slow; the queue's elapsed-time history remains its scheduling input.

| Layer | First question answered | Examples |
|---|---|---|
| `contract` | Are declarations, formats, inventories and runner rules internally sound? | `dsl-ops`, `manifest-entries-pp`, `gate-infra` |
| `stage` | Does one compiler stage or subsystem agree with its reference? | `exec-pp-directives`, `exec-lower`, `seed-construct` |
| `pipeline` | Does a complete compile or product route produce the expected result? | `exec-chain-1`, `difftest-1`, `com-difftest-1` |
| `platform` | Does a target image, ABI or native runtime work? | `bigclosure-win-arm64`, `winposix`, `ccinterop` |

For a first diagnosis, list one layer and run selected names with the existing
bounded queue. Escalate from the relevant stage to its full pipeline and then
the affected platform. `--through-layer` includes every narrower layer; it is
useful for broad preflight, while `--layer` runs exactly one layer.

```sh
python3 tests/gatequeue.py --list-selection --layer stage
python3 tests/gatequeue.py --state /tmp/unisacc-pp-check --layer stage --suite exec-pp-directives --jobs 1
python3 tests/gatequeue.py --state /tmp/unisacc-preflight --through-layer stage --jobs 2
python3 tests/gatelayers.py --check
```

Use the usual `tests/term.sh env ...` wrapper and environment for real macOS
runs. An unfiltered `tests/release.sh --com` remains the only local full release
gate. A layer result is diagnostic evidence and never substitutes for it.

`gatelayers.py` classifies every suite returned by `gate.sh --list --com`.
The `gate-layers` job checks completeness and that the union of exact layers is
the original gate list. Adding a suite without a reviewed classification fails
that job. Layer selection preserves the original job order and the existing
per-suite time and input fingerprints.

## 开发者如何只测相关部分

先准备与当前产品源一致的 `UA`、`MODEL_COM`（需要时还有 `SEED_DIR`），沿用已有
prepare/require-cached 约定；本入口不构建产品或伪造缺件通过。dev/merge 的 `--run`
把 **完整计划选集**交给既有 gatequeue，每次一个 50 秒窗口，外层既有 bound 55 秒。
默认含工作区变更；共享检出用 `--path` 指定自己的输入域，不能把改动文件漏出该域。
state 和 PLAN 放仓外，首次 state 必须新建或为空，PLAN 必须是新文件。

```sh
# 改一个 exec 表：保留声明命中的闭包、冒烟和前驱（广依赖可能使推荐集很大）。
python3 tests/gateselect.py --com --tier dev --base HEAD --head HEAD --path exec/parse2/librarycallbacke3-overlay-result.tsv --run --state /tmp/dev-exec-1 --out /tmp/dev-exec-1.plan.json
# 改一个测试脚本：merge 另加受影响套件所在层与 com-cli。
python3 tests/gateselect.py --com --tier merge --base HEAD --head HEAD --path tests/gateselectcheck.py --run --state /tmp/merge-test-1 --out /tmp/merge-test-1.plan.json
# 干净工作树，只追加 E3 阶段（仍保留冒烟、also 和传递前驱）。
python3 tests/gateselect.py --com --tier dev --base HEAD --head HEAD --stage E3 --run --state /tmp/dev-stage-1 --out /tmp/dev-stage-1.plan.json
```

不加 `--run` 只产 `PLAN_ONLY`。七段别名及独立 `--ask` 仍读 `gateselect.tsv`，
套件的主/also 阶段只读 `stagemap.c`；`enc` 展开 E5∪E6，未登记 ask 消费者具名拒。
未认领输入或门元数据变更保守升级全清单，预算只报告，不能删除 A/冒烟/前驱。
超预算提示拆分工作或准备；未完成返回 75，不能当 PASS。

75 时打印的 `--continue-plan FILE --plan-sha SHA --state SAME` 命令先校验原 PLAN
字节 SHA，再通过同一 selector 重算选集和源身份，匹配后仅委托原 gatequeue 一个窗口。
它不导入结果、不改 env-pin、不另建 owner/cache；选集或输入变更具名拒，须新 state。
返回码沿用执行器：0 仅该开发选集完成，1 失败，4 未验证，75 待办；超时等也原样保留。
**candidate/release 仍全清单，其执行只走 `tests/release.sh --com`，本入口拒绝 `--run`。**

`commitgate.sh` 的 dev 推荐仍是纯影子报告，不删其既有套件或覆盖/派生要求；报告
最多 5 秒，拒绝/超时记 `UNAVAILABLE`，部分实跑观察记 `UNKNOWN`，不变造原 rc。
`MODEL_COM=/absolute/product sh tests/stagerun.sh --plan-only E3` 只对照新推荐与旧 C
mapper 集；普通 stagerun 仍执行旧集。开发选集绿不等于 state6、新树全闸或发布验收绿。
