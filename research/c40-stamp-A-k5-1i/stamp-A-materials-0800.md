# 审核戳 A 材料（K5-1i 之后；基线 1b81c2e4 / 6c4a2734 → 848bc986；cc，机房主任 08:00）
**材料 ≠ 真审 ≠ 写键。** 本页只是材料：不勾“已审”，不写键，不刷戳，不改 guard，不 bump，不开 Draft，不跑全量门。

## 0. 推送（0800 ①）
- fetch 后 origin/main 为 6c4a2734，没有前移，所以不需要 rebase。848bc986 的父提交就是 6c4a2734，patch-id（stable）为 ec8727a69037ba8e983c27e9ca5a610f38970af2。
- 推送前 ls-remote 为 6c4a2734；普通快进推送，没有 force，push_rc 0；推送后 ls-remote 为 **848bc986e527cc07fc5841d5be6695591f1097a9**。日志在 /tmp/cc40-prep/k5-1i/push.log。墙钟没有采，记 UNKNOWN。

## 1. 基线与方法
- **有效记录值**：1b81c2e4 写入的 reviewed_trees.exec = 1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825。用 `git show 6c4a2734:tests/gatedeps.json` 读出来是同一个值。
- 基线成员表直接用 /tmp/cc40-prep/stamp-A-0646/inv-a7069c89.json（sha 1e5e7deb…）。可以这样用，是因为 `git diff --quiet a7069c89 6c4a2734 -- exec include kernel src unisa weights` 的 rc 为 0，两边审核目录的内容完全相同。
- 对比提交：**848bc986**，私有 wt /tmp/cc40-prep/k5-1i/wt，HEAD 即此提交，status 为 0。6c4a2734 到 848bc986 之间，tests/gatequeue.py 和 tests/gatedeps.json 都没有改动。
- 复算：用 /tmp/cc40-prep/gi1/inv.py（sha256 ff730eea12c2aaa7a7162f4a9b921d84d5592402e8fea52c583ee9d29d5c1d58），在 `env -u UA -u MODEL_COM -u UA_RUN` 下运行。**rc 已落盘**：inv-848bc986.rc 内容为 inv_rc=0。输出为 .out 和 .json，json 的 sha256 是 365b55de5324553b5f28a9ffd7f9c5efdedd602d2f8f8c079eb54a839a3b5813。

## 2. 戳读数（848bc986）
| 目录 | 成员数 | stamp | 与记录值 |
|---|---|---|---|
| exec | 1412 | **828a6f67417de1998627ef73d1346ff9709ef3cccd1cb7f2ebd904baf09a4775**（候选） | **不等**（记录值 1d0e0c5c…） |
| include、kernel、src、unisa、weights | 64、41、17、48、21 | 成员和 stamp 都与基线相同 | 相等 |

## 3. 成员差集（两份 JSON 全量比对；sha 全值从 JSON 原样复制）
| 路径 | 变化 | mode | 6c4a2734 时 sha256 | 848bc986 时 sha256 |
|---|---|---|---|---|
| exec/c/chain.sh | M | 100755 | a46974e4c01c864070b9039f7f586b5f7c41d6f94959946d0aedafe093361d8c | 5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0 |
| exec/parse2gen/gen-delta.sh | M（只改注释） | 100755 | cacd2fbbefa84e939be565f97104a671c4527d1e3ddfa8a10852f27de40ca8d8 | 647c8ada6a71e63036895bdec890c5b18ad476a6f85b86e6da712376be3927fc |
没有新增、删除，mode 也没有变化。与 `git diff --name-status 6c4a2734 848bc986 -- exec`（只看审核目录）的结果一致。完整 diff 在 exec-delta.diff，28 行，sha256 760012cdfba9c362473245c2d58c58d60780c4015243ab19f3f90059db5f62b5。

## 4. diff 摘要与执行路径
### 4.1 exec/c/chain.sh（+3 −1）
- 第 53 行原为 `b 60 python3 exec/build/gen.py parse2 "$T/e3.json" >/dev/null 2>&1 || …`，现在是两行注释，加上 `b 60 env SEED_GEN_DIR="$T/.seed-gen-cache" sh "$R/exec/parse2gen/gen-delta.sh" "$T/e3.json" >"$T/e3-gen.out" 2>"$T/e3-gen.err" || { echo "chain: E3 gen failed"; exit 1; }`。失败时的提示和退出码都不变。cache 与上一行的 pp 共用：pp 先冷建，parse2 复用。
- 执行路径：compilercheck.sh、elf.sh、models.py、prepare.sh 都**不调用** chain.sh。调用方是 gate 的 exec-chain-1..5、tests/seedppchaincheck.sh 和 tests/headerchain.sh。这是静态读码得出的结论，不是对所有调用方的穷举。
### 4.2 exec/parse2gen/gen-delta.sh（+2 −1）
- 只改头注释：Consumers 改为 prepare.sh（K5-1h）和 chain.sh（K5-1i）。**没有逻辑改动**。
### 4.3 已有运行证据（不等于审核）
K5-1i run1 三片都绿（回执 /tmp/cc40-prep/k5-1i/receipt-run1.md）：pp→parse2 两个 helper，编译 1 次，key 步骤 2 次，两个快照相同；跨片 e2 为 ee8ac744，e3 为 2187cf66。

## 5. 单列：既有债，不在本材料处置范围
- **exec-chain-1..5 的 guard**：tests/gatedeps.json 里这五个 suite 记录的 chain.sh sha 都是 ae17a951（K5-1d 之前的值）。6c4a2734 时文件已是 a46974e4，现在是 5f5d19ba，从 K5-1d 起就一直失配。本材料**不处理、不代刷** guard，怎么处置需另外裁定。
- exec-chain 门的预算（每个分片会多一次 C parse2）没有测过，记 UNKNOWN。

## 6. 条件式单键（提案；需要另外授权，不能代替真审）
真审通过、并且另外授权写键之后，才可以执行。执行前 fetch 一次，以下三条**全部**满足才写：(1) 1b81c2e4 之后，审核目录、gatequeue、gatedeps 的差集仍然恰好是第 3 节那两项；(2) 写前复算得到的 exec 戳全长等于 828a6f67…；(3) 旧值 1d0e0c5c 在 gatedeps 里只出现 1 次。只改 reviewed_trees.exec 这一个键，改完用 JSON 还原比对，再做写后复算，然后 commit 并快进推送。任何一条不满足就停手回报。

## 7. 审核勾选清单（留白；本页不勾）
- [ ] 差集恰好是两个 M，与 exec-delta.diff 一致，没有增删，mode 不变。
- [ ] chain.sh：只改了 parse2 那一行，外加注释；与 pp 共用 cache；失败语义不变；不在 compilercheck 的执行路径上（只读确认，必要时具名列出间接调用方）。
- [ ] parse2gen/gen-delta.sh：只改头注释，没有逻辑改动。
- [ ] 已知限定可以接受：exec-chain 门的预算未测；guard 债单列；UA 不过 shim；首红后零启动没有演示；进程组只证到 -g 选择器层面。
- [ ] 审核时只读复算一次（应得 828a6f67…）。**写进 gatedeps 要另外授权**。

## 收窄（cdx2；只追加）
- §4.1 原文“失败语义不变”收窄为：chain 这一层仍把失败统一映射成 `exit 1` 并输出 "chain: E3 gen failed"，这一点没变。但底下 parse2 的失败来源变了：以前是 gen.py，现在是 helper，helper 自己的 rc 可能是 2 或 3 等。
- §4.1 原文“pp 先冷建，parse2 复用”只在**常规默认路线**成立，即 SEED_GEN=1、没有显式 SEED_GEN_BIN、两个 helper 计算出同一个键。SEED_GEN=0 时两个 helper 都走 Python，不建缓存；给了显式 SEED_GEN_BIN 时直接用那个二进制，也不经过缓存。
