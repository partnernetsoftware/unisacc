# 回执：0.0.40 seal + Draft 产品资产（1645+；禁 Latest/公开）

墙钟：2026-10-11 ~16:45–16:52 CST。授源：政委自主；代裁 cc 有序补产品缺口（candidate 对齐 0.0.40、seal 资产）；禁切 Latest/禁公开；tip CI 旁观。

## 补了什么

| 项 | 结果 |
|---|---|
| `release/candidate.json` | **0.0.40**（原 0.0.39） |
| GHCR seal | `ghcr.io/partnernetsoftware/unisacc-candidate:0.0.40` @ `sha256:8798d9c880c9fddb5e8ae09c9e8fc0e6d22ec5247814811e8ba2e70900215386` |
| `unisacc_com_sha256` | `50ab4c213feedb2f1a4f1f39c8bfca7d8d63c74a1d6b50915db8ceb233f5cf58`（2243662 B） |
| `sources_sha256` | `739f197c1dfe5931354815cc9d26885cfe7c051ce0ee6983cafbccf79b4ab3a7`（tip freshness verified） |
| `sealed_from_commit` | `b12901c749475fa53df3e5cbdffd4dc53bf432fc` |
| 仓提交 | `1db30835` `0.0.40: seal candidate (Draft assets; cand 50ab4c21)` |
| `rc/v0.0.40` | → `1db3083506e7af178ed604052532d4226511ad01`（已推 origin） |
| Draft 资产（新增） | `unisacc.com`（unsigned）、`unisacc.com.build.json`、`unisacc-unsigned.zip`、`candidate.json`、`unsigned-receipt.json`、`SHA256SUMS-unsigned.txt` |
| Draft 正文 | 已改 zip→tar.gz 口径；列 seal 完成与仍缺项 |
| Draft target | 自 `d70a8038` **对齐**到 seal/rc `1db30835` |
| Latest | **仍 v0.0.39**（未切） |
| draft | **true**（未 `--draft=false`） |

## 材料链（未冒充 0.0.39 字节）

- B.2：`/tmp/cc40-draft-prep/b2-0d30/cand` @ tip `0d30c67e`；候选 `50ab4c21…`
- B.3：fixedpoint @ `e6dead1f`（stage2=stage3=`50ab4c21…`）
- `0d30c67e..b12901c7` 产品闭包（exec/unisa/src/kernel/include/weights/unisacc.c/seed/tests/Makefile）**空 diff** → 闭包等价，重用候选；`provenance.py check` / seedbin ident：**product freshness verified**
- 演示：`comdemo` status=passed；`fb12multi` pass 10 / wrong 0（rc0）

## Draft 是否对齐 tip

| 轴 | 值 |
|---|---|
| origin/main tip | `1db30835`（seal 提交） |
| Draft `target_commitish` | `1db30835`（已对齐 seal/rc） |
| Draft URL | https://github.com/partnernetsoftware/unisacc/releases/tag/untagged-bf5ebb399f4c1ef67337 |
| git annotated `v0.0.40` tag | **仍无**（Draft 仍为 untagged 存根；`rc/v0.0.40` 已有） |
| 相对开窗时 tip `d70a8038` | 已前进（seal + 中间 research #136） |

## tip CI 旁观

- `38124099823` @ `8ef1dbd6`：采样时仍 `in_progress`；**不挡**本 seal；不写成产品 PASS/FAIL。
- tip `1db30835` release-check 由 push 触发（旁观）。

## 卡点（仍挡公开 / 资产全齐）

1. **dmg / Apple 签名与公证**：m4pro.local **offline**；本窗无 Developer ID / notary 链。
2. **Windows company 签名**：未跑 qualification/company（需草稿 unsigned + 审批）。
3. **全量 release queue**：`tests/release.sh --com` 对封存字节 **未跑**（P4 重活；本刀未授满窗）。
4. **公开边界 / Latest**：仍另授；禁 `publish.sh` / `--draft=false`。
5. Draft 仍 untagged 存根（GitHub draft 常态）；正式 git `v0.0.40` 标签通常随公开。

## 禁项自检

- [x] 未切 Latest；[x] 未 publish / `--draft=false`；[x] 未用 0.0.39 字节冒充 0.0.40；[x] tip CI 仅旁观。
