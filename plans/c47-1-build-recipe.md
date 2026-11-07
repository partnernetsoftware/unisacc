# c47-1 构造配方（预演 2026-11-08，未整链）

范围：`plans/v0.0.47-unisacc.md` 末节 A1–A4/A6–A8/L0'/M12/M3 两项；TAPE-END、A5 推迟。基线 rev `44182b4059`（c46-3）→ 冻结 HEAD。

## 预演实测

- `git clone --no-hardlinks` 当前 HEAD → `/tmp/grk-c47-prev` **748M**。
- `chain-precheck init --range 44182b4059..HEAD`：**REFUSE jobs directory**。工具要已有 `CHAIN_OUT/ctx/jobs/gen-*`（c46-3 构造现场）才能按 `command.reads.tsv` 映射。本机无该 ctx，**不拷 >1G 链现场**。恰选 job **未产出**。
- 未跑 host/list/final/wave。用完已删预演根。

## 冻结时命令（有 c46-3 CHAIN_OUT 时）

私有根 `P`，链现场 `C`（含 `ctx/jobs`），新输出 `N`，已装产品 `P/unisacc.com`。

```
# 0. 私有根
git clone --no-hardlinks --single-branch <main> $P
cp <installed-unisacc.com> $P/unisacc.com

# 1. 预检（三分类 mapped/non-product/unmapped）
mkdir -p $C $N
sh $P/unisacc.com $P/tools/chain-precheck.cx -- $P $C $N init --range 44182b4059..HEAD
# 读 $N/selected.tsv = 恰选 gen-* job

# 2. 五片并行（osx/win slice 单作业 bound 120s；其余 60s）
#    复用 c46-3 argv 边界：~/.unisacc-baselines/c46-3-evidence 与 plans/ 中 release-build 记录
sh $P/unisacc.com $P/tools/chain-step.cx -- host $P $C/ctx $C/sl-lnx &
sh $P/unisacc.com $P/tools/chain-step.cx -- list $P $C/ctx $C/sl-lnx &
# wave PHASE FIRST COUNT ×5 片
# slice KEY / slice-link / pack / smoke

# 3. 私有 APE
sh $P/unisacc.com $P/tools/release-ape.cx -- smoke $N/unisacc-next.com

# 4. 正控
sh $P/unisacc.com tools/c-negative.cx -- $P $N/cneg /usr/bin/gcc
# comm 正控：check-run-comm + run-comm-new/old 材料（c46-3 配对）
```

unmapped 变更不得静默漏 gen。不造伪 ref。
