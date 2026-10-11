# 独立复算回执：stamp-A exec @ 4470a3f9（机房主任执行器 08:29）

## 锁 tree-ish
- **4470a3f97a99c17493f8d9db117deaab83260313**（写键父提交；材料钉死的被刷戳 tip；审核目录与 848bc986 内容相等，`git diff --quiet 848bc986 4470a3f9 -- exec include kernel src unisa weights` rc=0）。
- 私有 wt：`/tmp/cc40-prep/k5-1i/wt` detached 到上述提交；`git status` 干净。
- 材料钉死：`research/c40-stamp-A-k5-1i/stamp-A-materials-0800.md` §1/§2；写键回执 `stamp-A-write-0811-receipt-cc.md`。

## 算法
- 脚本：`/tmp/cc40-prep/gi1/inv.py`（sha256 **ff730eea12c2aaa7a7162f4a9b921d84d5592402e8fea52c583ee9d29d5c1d58**），副本落 `/tmp/cc40-prep/stamp-A-recompute-0829/inv.py`；与材料/前次 0736 复算一致；gatequeue digest/stamp 同类。
- 运行：`cd $WT && env -u UA -u MODEL_COM -u UA_RUN python3 inv.py inv-4470a3f9.json`
- 落盘：`/tmp/cc40-prep/stamp-A-recompute-0829/`（inv.py、.out、.rc、.json、summary.txt）

## 读数
| 项 | 值 |
|---|---|
| inv_rc | **0**（`inv-4470a3f9.rc`：`inv_rc=0`） |
| exec members | **1412** |
| exec stamp 全长 | **828a6f67417de1998627ef73d1346ff9709ef3cccd1cb7f2ebd904baf09a4775** |
| reviewed（4470 时 gatedeps） | 1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825（DIFF，预期） |
| 与目标 828a6f67… | **吻合** |
| inv JSON sha256 | 365b55de5324553b5f28a9ffd7f9c5efdedd602d2f8f8c079eb54a839a3b5813（与材料 inv-848bc986.json **逐字节相同**） |
| 其余五树 | include/kernel/src/unisa/weights 均 EQUAL |

## 命令摘要
```
git -C /tmp/cc40-prep/k5-1i/wt checkout --detach 4470a3f97a99c17493f8d9db117deaab83260313
cd /tmp/cc40-prep/k5-1i/wt
env -u UA -u MODEL_COM -u UA_RUN python3 /tmp/cc40-prep/stamp-A-recompute-0829/inv.py \
  /tmp/cc40-prep/stamp-A-recompute-0829/inv-4470a3f9.json
# → inv_rc=0；exec stamp 全长如上
```

## 不宣称 / 未做
- 未改 `tests/gatedeps.json`；未 bump；未 Draft；未跑门；未刷 guard；未实现 K5 新代码；未改测期望。
- 本复算是**本机独立核验**；此前「828a6f67 本机未复算 → 不计写入核验通过」在本回执落盘后解除。
