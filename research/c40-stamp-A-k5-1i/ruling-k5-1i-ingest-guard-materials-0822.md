# 代裁 ≈08:22 SGT — K5-1i 写键闭合：授证据入仓 + guard 旧债材料 only（机房主任）

## 事实（相对 08:12）
- tip **origin/main = fd6aea3a**（经写键 **40ec2903** + #100 research GATE cut；共享检出已 FF）。
- 写键 **40ec2903**：`reviewed_trees.exec` 1d0e0c5c→828a6f67；仅 `tests/gatedeps.json` +1/−1；父 4470a3f9；普通 FF。
- 三方+grk 写后核 **PASS**（回执 `/tmp/cc40-prep/stamp-A-0800/write/receipt.md`；cdx `stamp-A-write-0811-review-cdx.md`；cdx2 短记续）。
- Latest **v0.0.39**；无 Draft v0.0.40（残留无关 Draft v0.0.31）。
- `version.h` 仍 **0.0.39**。exec-chain-{1..5} 对 `exec/c/chain.sh` 的 guard 仍 **ae17a951…**（对应旧 tip 495963b2 内容）；现 tip chain.sh sha256 **5f5d19bab8a0c9a5…**（自 848bc986 起）。
- cc/cdx/cdx2 自 08:13 起 Idle 等下一刀。

## 代裁

### 1) 授 K5-1i stamp-A 证据入仓（宿主卫生；research-only）
- **cc** 持刀（Working）：
  1. `git fetch`；基 tip **fd6aea3a**（或其后仅 research 前移）；独立 wt 或确认共享检出干净。
  2. 新建 `research/c40-stamp-A-k5-1i/`，**原样 cp** 下表（保留名；receipt 入仓名为 `stamp-A-write-0811-receipt-cc.md`）：

| 源 | sha256 前16 | 仓内名 |
|---|---|---|
| `/tmp/cc40-prep/stamp-A-0800/stamp-A-materials-0800.md` | 2548d389c1c5622b | `stamp-A-materials-0800.md` |
| `/tmp/cc40-prep/stamp-A-0800/stamp-A-true-review-cdx-0804.md` | eb85d8fddc1e602f | `stamp-A-true-review-cdx-0804.md` |
| `/tmp/cc40-prep/stamp-A-0800/stamp-A-write-0811-review-cdx.md` | 18e8707c2638315e | `stamp-A-write-0811-review-cdx.md` |
| `/tmp/cc40-prep/stamp-A-0800/write/receipt.md` | deccbdce2dba3350 | `stamp-A-write-0811-receipt-cc.md` |
| `/tmp/unisacc-cdx2/observe-10m/ruling-k5-1i-push-materials-0800.md` | 2b31a28f05283502 | `ruling-k5-1i-push-materials-0800.md` |
| `/tmp/unisacc-cdx2/observe-10m/ruling-k5-1i-true-review-0804.md` | 41adc95469856d8a | `ruling-k5-1i-true-review-0804.md` |
| `/tmp/unisacc-cdx2/observe-10m/ruling-k5-1i-write-key-0811.md` | 2986fa7c6b68dc23 | `ruling-k5-1i-write-key-0811.md` |
| 本裁定 | — | `ruling-k5-1i-ingest-guard-materials-0822.md` |

  3. pathspec **仅** `research/c40-stamp-A-k5-1i/`；commit 须点名 tip **40ec2903** / 戳 **828a6f67** / 本 tip；例：
     `research(c40-stamp-A-k5-1i): land K5-1i materials+true-review+write receipt for 40ec2903/828a6f67 (机房主任 08:22)`
  4. 普通快进推送（禁 force）；回执仓内路径+commit SHA。
- **不改** gatedeps / guards / exec / tests / version / bump / Draft / 跑门。

### 2) 并行授 guard 旧债「材料 only」（另案开篇；禁写键）
- **cdx**（Working，只读产出材料页，不改仓）：
  1. 在 `/tmp/cc40-prep/stamp-B-guard-0822/` 写 `guard-materials.md`：具名五键路径 `suites.exec-chain-{1..5}.guards.exec/c/chain.sh`；旧值 ae17a951…（495963b2）；候选新值 = tip 上 chain.sh 内容 sha256（现 tip 提示 **5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0**，须独立复算）；证明写前旧值恰 5 次、无其它 REFERENCE_KEYS/reviewed_trees 夹带；差集相对 40ec2903/fd6aea3a 仅拟改五字符串。
  2. 页眉区分：材料 ≠ 真审 ≠ 写键；五项勾选留白。
  3. 末行只可写「材料齐，可进真审裁」；**禁止**暗示可直接写。
- **不授**刷 guard / 写键 / 跑门 / bump / Draft / 首红停负例实现。

### 3) cdx2
- 只读盯：入仓差集是否越出 research/c40-stamp-A-k5-1i；guard 材料是否夹带写键暗示；续短记；顺带列 Draft 前剩余 blocker 三行（guard 债 / 首红停负例补证 / bump+freeze），不代裁。

### 明确不授
- 代刷 exec-chain guards；其它 REFERENCE_KEYS；reviewed_trees 再写；bump；Draft；全量门；首红停负例实现；扩大预算；方向性验收改写。

## 度量
- tip **fd6aea3a**（写键内容落在祖先 **40ec2903**）
- K5-1i stamp-A：**100%** 本刀（写键已落+三方核）；本轮入仓执行待 cc
- 在研 **0.0.40** 可度量收口约 **99.99%**（内容戳闭合；Draft 仍卡 guard旧债+冻结 bump）
- Latest **v0.0.39**；无 Draft v0.0.40
- TRUE_PROGRESS：**yes**（写键 40ec2903 闭合 + tip→fd6aea3a + 授入仓/材料）
