# 代裁 ≈08:11 SGT — K5-1i A 真审 PASS：授条件式单键写键（机房主任）

## 事实（相对 08:04）
- tip **origin/main = 4470a3f9**（08:05 合入 #99 paper-a construction-combo；仅 research/，未动 exec/gatedeps/gatequeue）。本地曾停 848bc986；写键前须 fetch+FF。
- 审核目录相对 1b81c2e4→origin/main 差集仍恰 **两 M**：`exec/c/chain.sh`、`exec/parse2gen/gen-delta.sh`；与 848bc986 相对无 exec 内容差。
- Latest **v0.0.39**；无 Draft v0.0.40（仅残留无关 Draft v0.0.31）。
- cdx 真审回执 **PASS**：`/tmp/cc40-prep/stamp-A-0800/stamp-A-true-review-cdx-0804.md`（五项勾选+依据；候选全长 828a6f67…；guard 债单列）。
- cc Standby 等写键授权；cdx Ask（真审毕）；cdx2 08:04 短记时未见回执，本轮补推。

## 代裁
### 1) 授条件式单键写键（宿主/顺延；真审已 PASS）
- **cc** 执行（Working）：
  1. `git fetch`；确认 `origin/main` 为 4470a3f9（或其后仅 research 前移且审核目录仍两 M）；本地 FF 到 tip。
  2. 写前三条**全满足**才写，否则停手回报：
     - (1) `1b81c2e4` 之后审核目录 + gatequeue + gatedeps 差集仍恰材料 §3 两项 M；
     - (2) 写前复算 exec 戳全长 = `828a6f67417de1998627ef73d1346ff9709ef3cccd1cb7f2ebd904baf09a4775`；
     - (3) 旧值 `1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825` 在 gatedeps 只出现 **1** 次。
  3. **只改** `reviewed_trees.exec` 这一键 → JSON 还原比对 → 写后复算 → commit → **普通快进推送**（禁 force）。
  4. 回执落到 `/tmp/cc40-prep/stamp-A-0800/`（写键 ack + commit SHA）；材料/真审页不改写。
- **cdx**：只读哨写键过程与回执；发现扩差/多键/刷 guard/跑门立刻喊停。
- **cdx2**：只读盯越权与漏证；续短记；墙钟/出口若写键后有新 tip 记一笔。

### 2) 明确不授
- 代刷 exec-chain-* guards / 其他 REFERENCE_KEYS；bump；Draft；全量门；首红停负例；扩大预算；方向性验收措辞改写。
- guard 旧债（ae17a951）仍单列，另案。

## 度量
- tip **4470a3f9**（K5-1i 内容仍落在祖先 848bc986；写键 commit 将再前移）
- K5-1i：真审 PASS + 本轮授写键 ≈ **98%**；写键落地后 ≈100% 本刀
- 在研 **0.0.40** 可度量收口约 **99.98%**
- Latest **v0.0.39**；无 Draft v0.0.40
- TRUE_PROGRESS：**yes**（真审 PASS 卡点解开 + 授写键；origin tip 因 #99 亦前移）
