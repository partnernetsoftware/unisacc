# 可复跑的 typed 模型观测收据

这些 JSON 是各自提交时的历史冻结观测，不是当前源码的 PASS。重采生成新目录中的 `steps.tsv`、逐步 JSON 和 cache 清单；不回填历史 JSON，不据此改 admission/reference key。

统一入口是 `release/tools/model-reseal.py`（观察层）。它只调用既有 `check.sh`，每条子命令经既有 `tests/bound.py 55`；计时用 monotonic。缓存创建、摘要命中与准备依赖仍归原检查脚本。`--product` 仅是 `-run` 检查的观察工具身份，不能据此声称 fresh 产品验收。

在干净源码导出上设 `ROOT`、观察产品 `PRODUCT`、仓外新目录 `OUT`。若要冷准备，由调用方预先提供空的 `TMPDIR`；入口不清缓存、不改变缓存生命周期。每次只运行下列一个 step，不把整张列表塞进一个 60 秒作业：

```sh
python3 "$ROOT/release/tools/model-reseal.py" --root "$ROOT" --model lower --list
python3 "$ROOT/release/tools/model-reseal.py" --root "$ROOT" --model lower --step STEP --product "$PRODUCT" --out "$OUT"
```

统一 TSV 列：`step, argv, rc, monotonic_s, product_sha, source_hash, artifact_sha`。`artifact_sha` 绑定同一步 cache 清单中相对路径、长度与 SHA256；源码、产品与工具身份属于观测层，不混入核心阶段身份。逐步 JSON 另记 producer SHA、cache 路径。超时/未知/陈旧来源不得记为 PASS；非零结果保留原 rc。源/产品在调用期间变化会报 STALE。

## Lower 分步配方

`MODEL=lower`。先单独运行 `STEP=tools`；随后每个变体单独一个 step：

`lnx-x, lnx-a, osx-x, osx-a, win-x, win-a, obj-lnx-x, obj-lnx-a, obj-osx-x, obj-osx-a, obj-win-x, obj-win-a`。

最后 `STEP=1`、`2`、`3`、`4` 各自单跑。它们直接对应 `exec/lower/librarytypedcheck.sh --prepare VARIANT` 和四个模型作业；原脚本负责 C/P、base/off/on 构造、tbl/net 与动态 C 参考对拍。Windows 按现行拒绝合同记录，不等于 Windows 运行时已验证。


## 种子读取集与 RSS

历史 `seed-memory.json` 不是上述构造/检查作业的 RSS。种子重采使用同一仓内观察入口 `tests/seedmemoryevidence.py`，每次一个路线/后端：

```sh
python3 "$ROOT/tests/seedmemoryevidence.py" --root "$ROOT" --out "$OUT/seed" --name e2 --family gen --kind trace --backend c --binary "$GEN" -- pp --shared-predefines
python3 "$ROOT/tests/seedmemoryevidence.py" --root "$ROOT" --out "$OUT/seed" --name e2 --family gen --kind rss --backend c --binary "$GEN" -- pp --shared-predefines
python3 "$ROOT/tests/seedmemoryevidence.py" --root "$ROOT" --out "$OUT/seed" --name e2 --family gen --kind trace --backend python -- pp --shared-predefines
```

`GEN` 由同一最终源树构建；trace 复用 strace/openat，RSS worker 直接 wait4，执行/清理由既有 bound 负责，不新增看门狗。替换命令尾部及 name 为对应 gen/parse2 逐路线参数；完整路线名单以 `tests/seedgencheck.sh`、`tests/seedparse2check.sh` 为准。读集按实际 trace 收，不预断只有 manifest 变化；RSS、读集、二进制摘要与参考键须在同一最终源快照上核对，经独立审查再更新 admission 数据。Python/其他宿主/未实跑 flag 不因 C 路线结果变绿。

原件中的绝对路径只标历史观测位置；复跑以本 README、仓内入口、实际 argv/摘要为准，不依赖这些历史目录仍存在。
