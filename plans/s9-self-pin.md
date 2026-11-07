# s9 init：同一候选 P3 成员 sha 现算现钉

空跑（c46-3 pkg.bin 替身，2026-11-08）：

```
sh ./unisacc.com tools/s9-p3-pins.cx -- ~/.unisacc-baselines/c46-3/pkg.bin MAP
# PINS file=29e13aeb… p3=14a69316e4ec658063c83237f3b05a3eef22f1d68a0dad4c225d55805b227bad
# 与 stream-model-identity 的 p3= 一致；两次 cmp 相同。
```

APE 一出：把 PKG 换成新 APE/`pkg.bin`，MAP 进 s8c-bind。c47-1 尚未出六片 APE（status：后续阶段未运行）。

合法自指钉：钉值 = 本会话 candidate 的 P3 成员/profile 收据，禁止旧候选拷贝。

APE 一出：`CAND=$P/unisacc.com`（c47-1 APE）。

```
# 0. 私有根 P（git clone --shared 冻结 SHA），CAND=APE
# 1. init 空 MAP 得 session
sh $P/unisacc.com tools/s8c-bind.cx -- $P $P/out/s9 $CAND $P/out/s9.map

# 2. 现算 P3 成员 sha（同一文件 footer）
#    工具：stream-model-identity 已打 IDENTITY p3=…；或 unpack footer
sh $P/unisacc.com tools/stream-model-identity.cx -- $STREAMS $CAND all check
#    收据行 SAME 的 compressed_model_sha256 / com_sha256 写入 MAP：
#    stream-e2-sha	<hex>
#    stream-e1-sha	<hex>
#    … e3 e4 prune lower elf
#    candidate-profile-sha	<profile 收据 sha>
# 3. 再 bind 带钉的 MAP（registered=yes）
```

`make ref` 2026-11-08 私有克隆：`sourceflat: cannot read unisacc.c`（3.3s）。无 `unisacc.c` 不能编 REF。22 批暂 KNOWN-GAP，有 `unisacc.c` 再固化。

tapeprune.c、adopted-package：继续 KNOWN-GAP。
