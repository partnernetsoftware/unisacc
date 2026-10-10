# 卫生 stamp A 独立复算回执（机房主任执行器 ≈06:19 SGT）

**只读复算；不改 gatedeps；不授 K5-1h；不跑满门。**

## 0. 锁定 tree-ish
- 复算 tree-ish：**b0548cd155b07848a256eec877ac9c78d4d70a1d**（e418bdcf 父；材料所称 470b2b49/dbb4b10d/ac39786a exec 等价点）
- `git diff --quiet b0548cd1 470b2b49 -- exec` rc=0；同理 vs ac39786a / dbb4b10d / tip 1f8de87a 均为 0
- 私有 wt：`/tmp/cc40-prep/hygiene-recompute-0619/wt`（detached b0548cd1；六审核目录 clean）
- 共享检出未动写；K5-1h 私有 wt `/tmp/cc40-prep/k5-1h/wt` 未触碰

## 1. 命令与 rc
```
cd /tmp/cc40-prep/hygiene-recompute-0619/wt
env -u UA -u MODEL_COM -u UA_RUN python3 /tmp/cc40-prep/hygiene-recompute-0619/inv.py \
  /tmp/cc40-prep/hygiene-recompute-0619/inv-b0548cd1.json
```
- **inv_rc=0**（落盘 `inv-b0548cd1.rc`：`inv_rc=0`）
- inv.py sha256 = `ff730eea12c2aaa7a7162f4a9b921d84d5592402e8fea52c583ee9d29d5c1d58`（与材料 ff730eea… 同）
- 墙钟：started 2026-10-11T06:20:51+08:00 → finished 2026-10-11T06:20:51+08:00（Asia/Shanghai）

## 2. 复算结果（exec）
| 项 | 值 |
|---|---|
| members | 1411 |
| stamp（全长） | **fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336** |
| 目标 fbb12970…（全长） | fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336 |
| 吻合 | **是**（逐字相等） |
| 当时 recorded（b0548 gatedeps） | e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34（DIFF 预期：写键前） |

其余五树相对 b0548 recorded：**EQUAL**（include/kernel/src/unisa/weights）。

## 3. 证据文件 sha256
| 文件 | sha256 |
|---|---|
| inv-b0548cd1.json | d98c77ec1328e0b624384f58e3581fcd48187e71a9a904cc7dc8b764841b54be（与 0506/0537 的 d98c77ec… 逐字节同） |
| inv-b0548cd1.out | 9cbfd7cfd6f64868785d27d5531f19b17e2ed73be7e2c00984c859c7e0181fb8 |
| inv.py | ff730eea12c2aaa7a7162f4a9b921d84d5592402e8fea52c583ee9d29d5c1d58 |

## 4. 裁定依据摘要
- 入仓真审 PASS：`research/c40-hygiene-stamp-A/hygiene-stamp-A-true-review-cdx.md`（差集恰三 M TMPDIR 归一）
- 本独立复算 stamp == fbb12970… 全长
- → **认 e418bdcf 写键有效**；**不改** gatedeps（tip 已是目标值）

## 5. 明确未做
- 未改 tests/gatedeps.json / reviewed_trees
- 未停/未改 K5-1h 实现域
- 未 bump / Draft / 跑满门当验收 / 改测试期望
