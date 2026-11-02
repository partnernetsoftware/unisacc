# 独立复算回执：stamp-A exec @ a7069c89（机房主任执行器 07:36）

## 锁 tree-ish
- **a7069c89d7588d1168e509513eb6a8541b181a2e**（材料 tip；相对该提交六审核目录对 origin/main tip 3c6f8ba7 差集空，rc=0）
- 私有 wt：`/tmp/cc40-prep/k5-1h/wt` detached 到上述提交；`git status` 干净。
- 材料钉死：`research/c40-stamp-A-k5-1h/stamp-A-materials-0646.md` §0/§1。

## 算法
- 脚本：`/tmp/cc40-prep/gi1/inv.py`（sha256 **ff730eea12c2aaa7a7162f4a9b921d84d5592402e8fea52c583ee9d29d5c1d58**），与材料一致；gatequeue.py digest/stamp 同类（lines 126–139 / 206 / 303–315）。
- 运行：`cd $WT && env -u UA -u MODEL_COM -u UA_RUN python3 inv.py inv-a7069c89.json`
- 落盘：`/tmp/cc40-prep/stamp-A-recompute-0736/`（inv.py 副本、.out、.rc、.json）

## 读数
| 项 | 值 |
|---|---|
| inv_rc | **0**（`inv-a7069c89.rc`：`inv_rc=0`） |
| exec members | **1412** |
| exec stamp 全长 | **1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825** |
| reviewed（a706 时 gatedeps） | fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336（DIFF，预期） |
| 与目标 1d0e0c5c… | **吻合** |
| inv JSON sha256 | 1e5e7debfcfd64a4d1719d419b547b24b2a2e048b3a81440610634d8c754dbb3（与材料原件逐字节相同） |
| 其余五树 | include/kernel/src/unisa/weights 均 EQUAL |

## 命令摘要
```
git -C /tmp/cc40-prep/k5-1h/wt checkout --detach a7069c89d7588d1168e509513eb6a8541b181a2e
cd /tmp/cc40-prep/k5-1h/wt
env -u UA -u MODEL_COM -u UA_RUN python3 /tmp/cc40-prep/stamp-A-recompute-0736/inv.py \
  /tmp/cc40-prep/stamp-A-recompute-0736/inv-a7069c89.json
# → inv_rc=0；exec stamp 全长如上
```

## 不宣称 / 未做
- 未改 `tests/gatedeps.json`；未 bump；未 Draft；未跑门；未实现 K5-1i；未改测期望。
- 本复算是**本机独立核验**；此前「1d0e0c5c 本机未复算 → 不计独立核验通过」在本回执落盘后解除。
