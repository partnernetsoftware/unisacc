# 代裁 ≈08:47 SGT — K5-1i guard 真审 PASS：授条件式五键写键（机房主任）

## 事实（相对 08:44）
- tip **c928581f**（= origin/main）；Latest **v0.0.39**；无 Draft v0.0.40。
- stamp-B 材料+真审已落仓内 `research/c40-stamp-B-guard-k5-1i/`：`guard-materials.md` + `guard-true-review-cdx-0844.md`（末行 **PASS 可进刷键裁**）+ `guard-watch-cdx2-0844.md`（旁观未见越权）。
- 机房主任独立复算：`exec/c/chain.sh` sha256 = **5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0**；gatedeps 旧值 **ae17a951…** 恰 5 次，均在 `suites.exec-chain-{1..5}.guards["exec/c/chain.sh"]`；`reviewed_trees.exec` 仍 **828a6f67…**。
- 三窗活：0:1=cc Idle / 0:2=cdx Idle（真审完）/ 0:3=cdx2 Idle（旁观完）。
- 工作树脏：`prd.md`（cdx2 旁观两行）+ 未跟踪 `research/c40-stamp-B-guard-k5-1i/` —— **写键 commit 不得夹带**；另案卫生入仓。

## 代裁

### 1) 授条件式五键写键（cc 持刀）
- 基 tip **c928581f**（或其后仅 research/prd 卫生前移；写键 diff 仍只能动五字符串）。
- **仅**将 `tests/gatedeps.json` 中下列五键旧值 `ae17a9515fa5aed38619486621a9e1682f29e569f14dfb45e2f2954f75954587` → 新值 `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`：
  - `suites.exec-chain-1.guards["exec/c/chain.sh"]`
  - `suites.exec-chain-2.guards["exec/c/chain.sh"]`
  - `suites.exec-chain-3.guards["exec/c/chain.sh"]`
  - `suites.exec-chain-4.guards["exec/c/chain.sh"]`
  - `suites.exec-chain-5.guards["exec/c/chain.sh"]`
- pathspec **仅** `tests/gatedeps.json`；commit 例：`chore(gatedeps): refresh exec-chain-1..5 chain.sh guards to 5f5d19ba (K5-1i stamp-B; 机房主任 08:47)`；普通快进推送（禁 force）。
- 写前确认：旧值恰 5；写后新值恰 5、旧值 0；无其它键/reviewed_trees/REFERENCE_KEYS 变动；unified diff 恰 ±5 行字符串。
- 回执：commit SHA + 五键核表；落 `/tmp/cc40-prep/stamp-B-guard-0822/write/receipt.md`（可另 cp 进 research 目录，但**本刀 commit 不含 research**）。

### 2) cdx / cdx2
- **cdx**：写后只读复核 diff/哈希/次数；短记；**不**代写。
- **cdx2**：只读盯越权；续短记；**勿**再改 `prd.md` 抢写键窗；0.0.41 合并稿仍只读。

### 3) 并行卫生（写键落地后或写键空档，另 paste；可同轮若写键已推）
- 将 `research/c40-stamp-B-guard-k5-1i/` 入仓（含 materials/true-review/watch/rulings）；`prd.md` 旁观两行可并入或丢弃（丢弃更干净：`git checkout -- prd.md` 亦可）。**不**与写键同 commit。

## 明确不授
- 刷其它 guards；改 reviewed_trees；bump；Draft；全量门；首红 NEG 当验收；force；扩大预算；方向性验收改写。

## 度量
- 在研 **0.0.40** ≈ **99.997%**（真审 PASS；写键执行中）
- Latest **v0.0.39**；无 Draft v0.0.40
- TRUE_PROGRESS：**yes**（harness 恢复 + 材料重采 + 真审 PASS 落仓 + 授写键）
- 卡点：cc 执行五键写键并推送

— 机房主任 2026-10-11 08:47 SGT
