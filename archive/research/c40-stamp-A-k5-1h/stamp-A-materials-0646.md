# 审核戳 A 材料（基线 e418bdcf → tip a7069c89；cc，机房主任 06:46）
**材料 ≠ 真审 ≠ 刷戳。**本页只是材料：不勾“已审”，不刷戳，不改 gatedeps 或哈希。写键 fbb12970 已 CLOSED，不重开。

## 0. 基线与方法
- **基线 e418bdcfc5eee3bb45f664065341937365b5ddbd**：gatedeps 里 compilercheck.reviewed_trees.exec 的记录值为 fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336（`git show e418bdcf:tests/gatedeps.json` 读出）。`git diff --quiet dbb4b10d e418bdcf -- exec include kernel src unisa weights` 的 rc 为 0，因此基线成员表直接沿用 /tmp/cc40-prep/sa3/inv-dbb4b10d.json（sha d98c77ec…）。
- **对比提交 a7069c89d7588d1168e509513eb6a8541b181a2e**：私有 wt /tmp/cc40-prep/k5-1h/wt，HEAD 即此提交，status 为 0 行。fetch 后 origin/main 已前进到 98e0f4db，但 a7069c89..98e0f4db 在 6 个审核目录及 gatequeue/gatedeps 下都是 0 个文件变化（新增提交只有 paper-a 的 research），所以本材料对 98e0f4db 同样成立。
- e418bdcf..a7069c89 之间，tests/gatequeue.py 和 tests/gatedeps.json 都没有改动，即算法和声明相同。
- 复算脚本 /tmp/cc40-prep/gi1/inv.py（sha256 ff730eea12c2aaa7a7162f4a9b921d84d5592402e8fea52c583ee9d29d5c1d58），运行时 `env -u UA -u MODEL_COM -u UA_RUN`。**rc 已落盘**：inv-a7069c89.rc 内容为 inv_rc=0。输出为 inv-a7069c89.out 和 inv-a7069c89.json（sha 1e5e7debfcfd64a4d1719d419b547b24b2a2e048b3a81440610634d8c754dbb3）。

## 1. 戳读数（a7069c89）
| 目录 | 成员数 | stamp | 与记录值 |
|---|---|---|---|
| exec | 1412 | **1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825**（候选） | **不等**（记录值 fbb12970…） |
| include、kernel、src、unisa、weights | 64、41、17、48、21 | 与基线相同 | 相等 |
候选戳只是只读复算的结果，不等于已审值。

## 2. 成员差集（两份 JSON 全量比对，sha 全值从 JSON 复制）
| 路径 | 变化 | mode | e418bdcf 时 sha256 | a7069c89 时 sha256 |
|---|---|---|---|---|
| exec/parse2gen/gen-delta.sh | **A（单列一组）** | 100755 | — | cacd2fbbefa84e939be565f97104a671c4527d1e3ddfa8a10852f27de40ca8d8 |
| exec/pipeline/prepare.sh | M | 100644 | 282322c6b7aadc0d7105d72e6ef0c590af6f8ff688136ab17dc548f79a72fbcf | 63474552642c3be637d6627c0d7068e812ec18951d2303791b08ea426e31724d |
没有删除，mode 没有变化。与 `git diff --name-status e418bdcf a7069c89` 的结果一致。完整 diff 存于 exec-delta.diff（84 行，sha d12a3f86…）。

## 3. 每个文件的 diff 摘要与执行路径
### 3.1 exec/pipeline/prepare.sh（K5-1h，原 55df08ba，rebase 后为 fc397468；+5 −4）
- 第 12 行 `b python3 exec/build/gen.py parse2 "$OUT/e3.json"` 改为：一行注释，加上 `b env SEED_GEN_DIR="$OUT/.seed-gen-cache" sh "$R/exec/parse2gen/gen-delta.sh" "$OUT/e3.json"`。
- opt 段的两行注释改写为“parse2 先冷建，opt/prune/pp 复用”；prune 段一行注释从 “opt step” 改为 “parse2 step”。**除了 parse2 那一行，其余都只是注释改动。**
- 执行路径：compilercheck.sh 第 13 行调 elf.sh，elf.sh 第 17 行调 models.py，models.py 的 prepare() 在第 83 行调 prepare.sh。只有在**确实调用了 models.py、并且模型键没有有效缓存**时才会执行 prepare.sh；elf.sh 若设了 STAGE_MODELS_READY=1 就不会调用 models.py。
### 3.2 exec/parse2gen/gen-delta.sh（新增，组二单列；56 行，100755）
- 内容：照 exec/prune/gen-delta.sh 复制，只有头注释、诊断里的路径和 stage 名（`parse2`）不同；只收一个 OUT 参数，带任何 flag 都 rc 2；SEED_GEN 只认 0 或 1；显式给了 SEED_GEN_BIN 但失败时不回退 Python；key 计算段与 opt/prune/pp 三个 helper 逐字相同；默认 cache 路径用 `${_td%/}` 去掉尾斜杠。
- 执行路径：由 prepare.sh 第 13 行调用（冷准备时），传入私有的 SEED_GEN_DIR。它放在 exec/parse2gen/ 下，不在 seedmemory 的 parse2 glob（exec/parse2/*）范围内，键 A 仍为 3d19910f（见 0637 核验）。
- exec/c/chain.sh 的 parse2 仍直接调用 gen.py（第 53 行），**不经过**这个 helper。
- 已有运行证据（不等于审核）：K5-1h run2 九片全绿；cold1 的 models_rc 为 0，用时 22.58 s，e3/e4/prune 三个 JSON 都与 SEED_GEN=0 的参考逐字节相同。回执见 /tmp/cc40-prep/k5-1h/receipt-run2.md。

## 4. 条件式单键（提案；不能代替真审）
若真审通过，可以授权一次“条件式单键写入”：fetch 后，只有同时满足以下全部条件才执行——(1) origin/main 上 e418bdcf 之后 exec 等 6 个审核目录及 gatequeue/gatedeps 的差集仍恰好是本页第 2 节那两项；(2) 写前复算的 exec 戳恰好等于 1d0e0c5c…（全长）；(3) gatedeps 里旧值 fbb12970 只出现 1 次。执行时只写 reviewed_trees.exec 这一个键，并用 JSON 还原比对确认没有改到其他键，然后做写后复算、commit、FF 推送。任何一条不满足就停手回报。**这个提案需要另外授权。**

## 5. 审核勾选清单（留白，供审核人勾；本页一项都不勾）
- [ ] 差集恰为第 2 节两项（1 A + 1 M），与 exec-delta.diff 一致，没有删除，mode 不变。
- [ ] prepare.sh：只有 parse2 那一行有语义改动，其余是注释；冷准备的执行条件（models.py 实际被调用、且新键没有有效缓存）可以接受。
- [ ] parse2gen/gen-delta.sh：作为新文件单独审——参数检查、fail-closed、不回退 Python、key 与 sha 校验和 opt/prune/pp 同构，stage 名为 parse2。
- [ ] 放置位置：exec/parse2gen/ 不进入 seedmemory 的 parse2 键（键 A 仍为 3d19910f）；chain.sh 没有迁移。
- [ ] 已知限定可以接受：并发未测；进程组读数不能证明已清空；run1 的超时原因 UNKNOWN；RSS 为 UNKNOWN；默认 cache 分支没有直接运行证据；没有跑过 seedparse2 的准入。
- [ ] 若审核通过：按同一算法在审核时的提交上重算，exec 戳应得到 1d0e0c5c…。**写入 gatedeps 属于刷戳，需另授**（可以考虑第 4 节的条件式单键提案）。

## 6. 证据
/tmp/cc40-prep/stamp-A-0646/：inv-a7069c89.json、.out、.rc，以及 exec-delta.diff。基线成员表：/tmp/cc40-prep/sa3/inv-dbb4b10d.json。算法脚本：/tmp/cc40-prep/gi1/inv.py。

## DONE / 待真审
材料齐。待真审：第 5 节第 1–5 项（第 6 项属于刷戳，另授）。我停在这里。

## 口径更正（cdx；只追加）
第 5 节第 6 项要拆成两步，不能整项都算作刷戳：(1) 审核时在被审的提交上只读重算，记录候选戳——这一步属于材料或真审；(2) 把候选值写进 gatedeps——**只有这一步**才是刷戳，需要另外授权。DONE 段里写的“第 6 项属于刷戳”据此收窄为“第 6 项的第 (2) 步属于刷戳”。
- 口径更正二（cdx2）：第 2 节写的“与 `git diff --name-status e418bdcf a7069c89` 一致”**限定为** `-- exec`（或 6 个审核目录）；全仓的 name-status 还包含 tests/ 下的夹具和 gate.sh 改动，它们不在 compilercheck 的 reviewed_trees 里。exec-delta.diff 的全长 sha256 是 d12a3f86c7572e6ed5df32e23be50055bd1d454e9b31d516f9e867d83f7284bc。第 6 项的界线同上一条更正。
