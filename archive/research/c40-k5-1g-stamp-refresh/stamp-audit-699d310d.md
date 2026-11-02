# exec 审核戳具名真审材料（基线 699d310d；cc，机房主任 04:38）
**只出材料。不是审核结论，不勾“已审”，不刷戳，不跑门，不改 stamp/gatedeps。** 材料 ≠ 真审 ≠ 刷戳。

## 0. 基线与方法
- 旧审基线 **fbb0d350f6848b398159da3813eb4e22222faf95**（“reviewed_trees.exec 9ae3c358 → 2d52b9bc after true review”，机房主任 02:24 处置 a）。对比提交 **699d310d7b983123370142d90080c57c5b457c8f**（当前 tip）。699d310d 之后若 tip 再有 exec 变动，另列，不混入本材料。
- 两个只读 worktree：/tmp/cc40-prep/sa/wt-fbb0d350、/tmp/cc40-prep/sa/wt-699d310d（HEAD 与上面一致，status 均 0 行）。
- 复算脚本沿用 /tmp/cc40-prep/gi1/inv.py（sha ff730eea…；逐行照抄 tests/gatequeue.py 第 303–315 行的清单逻辑、第 126–139 行的 digest、第 206 行的 stamp），以 `env -u UA -u MODEL_COM -u UA_RUN` 运行。`git diff fbb0d350 699d310d -- tests/gatequeue.py tests/gatedeps.json` 为空，所以两端用的算法和声明相同。复算命令的 rc 没有单独落文件（输出正常打印六行），记 UNKNOWN。
- 结果文件：inv-fbb0d350.json（sha e6bbafa6…）、inv-699d310d.json（sha 98cb63f8…），都在 /tmp/cc40-prep/sa/。stamp 是对过滤后的“路径 → [mode, sha256]”映射计算的，不是 git 树 OID。

## 1. 戳读数
| 目录 | fbb0d350 成员 / stamp / 与记录 | 699d310d 成员 / stamp / 与记录 |
|---|---|---|
| exec | 1410 / 2d52b9bc856c… / **相等** | 1411 / **e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34** / **不等** |
| include、kernel、src、unisa、weights | 64、41、17、48、21 / 全相等 | 同左 / 全相等 |
fbb0d350 上的结果等于 gatedeps 的记录值，这同时验证了复算方法。

## 2. 成员差集（完整，来自两份 JSON；mode 33261=100755，33188=100644）
| 路径 | 变化 | mode | fbb0d350 sha256 | 699d310d sha256 |
|---|---|---|---|---|
| exec/c/chain.sh | M | 100755 | ae17a951…54587 | a46974e4…61d8c |
| exec/pp/gen-delta.sh | M | 100755 | a497127d…adae0 | d84b4905…e7cc02 |
| exec/pp/run.sh | M | 100755 | 22660ed9…d39d1 | 95581230…4070b |
| exec/pipeline/prepare.sh | M | 100644 | 48d34abb…a8558 | 282322c6…72fbcf |
| exec/prune/gen-delta.sh | **A（单列）** | 100755 | — | 3a5cd26b…3e995 |
没有删除；mode 没有变化。与 `git diff --name-status fbb0d350 699d310d -- exec` 一致。完整 diff：/tmp/cc40-prep/sa/exec-delta.diff（140 行，sha cf3c7dd6…）。

## 3. 组一：已审基线上的四个修改文件
### 3.1 exec/c/chain.sh（1bd685b0，K5-1d；+3 −1）
- 内容：第 49 行 `b 60 python3 exec/build/gen.py pp "$T/e2.json"` 改为 `b 60 env SEED_GEN_DIR="$T/.seed-gen-cache" sh "$R/exec/pp/gen-delta.sh" "$T/e2.json"`，stdout/stderr 分别落到 e2-gen.out / e2-gen.err；另加两行注释。失败时提示文字不变。
- 执行路径：compilercheck.sh、elf.sh、compilercheck.py、models.py 里静态 grep 都找不到 chain.sh；调用它的是 tests/gate.sh、tests/headerchain.sh、tests/seedppchaincheck.sh。**静态判断：不在 compilercheck 执行路径上。**
### 3.2 exec/pp/gen-delta.sh（1bd685b0；+3 −2）
- 内容：只改头注释（列出消费者），**没有逻辑变化**。
- 执行路径：经 prepare.sh 调用，所以只在冷准备时进入 compilercheck 路径（见 3.4）。
### 3.3 exec/pp/run.sh（56f22fb2，K5-1e；+12 −4）
- 内容：ready() 改成每一步各自 `|| return 1`；d.json 的生成从 gen.py pp 改为在一次性 mktemp 目录里跑 `sh exec/pp/gen-delta.sh`；fresh 的命令文本带上 SEED_GEN、SEED_GEN_BIN、SEED_GEN_CC 和解析出的 cc；输入里加了 exec/pp/gen-delta.sh、seed/*.c、seed/*.h；`gen)` 分支改为 `ready || exit 1`。
- 执行路径：静态 grep，compilercheck 链上没有调用它（调用者是 exec/pp/shards.sh、tests/gate.sh、tests/seedpprun.sh、tests/rowcov.py）。**静态判断：不在 compilercheck 执行路径上。**
### 3.4 exec/pipeline/prepare.sh（67b9e777 与 699d310d 两段不同的改动，不能当一笔；累计 +6 −2）
- 67b9e777（K5-1f）：opt 步骤 `gen.py opt … --o2` 改为 `env SEED_GEN_DIR="$OUT/.seed-gen-cache" sh "$R/exec/opt/gen-delta.sh" "$OUT/e4.json" --o2`，加两行注释。
- 699d310d（K5-1g）：prune 步骤 `gen.py prune` 改为 `env SEED_GEN_DIR="$OUT/.seed-gen-cache" sh "$R/exec/prune/gen-delta.sh" "$OUT/prune.json"`，加两行注释（接线在第 18 行）。
- 执行路径：compilercheck.sh 第 13 行调用 elf.sh；elf.sh 第 17 行调用 models.py；models.py 的 prepare() 在第 83 行调用 prepare.sh。**只在冷准备（模型缓存未命中）时执行**；缓存命中时不调用 prepare。
- 冷/命中条件：models.py 的 key 是 closure() 的哈希，exec 下的 .sh 都计入（*check 除外），所以这几处改动本身会让旧模型键失效，第一次 compilercheck 会走冷准备。

## 4. 组二（单列）：新增 helper exec/prune/gen-delta.sh（699d310d；56 行，100755）
- 内容：照 exec/opt/gen-delta.sh 的形态，只收一个参数 OUT（`[ $# -eq 1 ] || usage`），阶段为 prune。SEED_GEN 只认 0/1；显式指定的 SEED_GEN_BIN 缺失或失败时不回退 Python；默认按 key（编译器 realpath、编译器字节、--version、flags、seed/*.[ch]）构建，缓存只在 sha 与 sidecar 一致时复用。
- 执行路径：经 prepare.sh 第 18 行调用，在冷准备时进入 compilercheck 路径。
- 已有证据（不等于审核）：K5-1g 受控 run1 结果 20/0；cold1 中 prune.json 与 SEED_GEN=0 的参考逐字节相同（068a6a14…）；回执见 /tmp/cc40-prep/k5-1g/receipt.md。

## 5. 审核勾选清单（供审核人逐项勾，本材料一项都不勾）
- [ ] 差集恰好是第 2 节的 5 个路径（4 个 M + 1 个 A），与 exec-delta.diff 一致，没有删除，mode 没有变化。
- [ ] chain.sh：pp 改为经 helper，用私有缓存，stderr 落文件——语义可接受；确认不在 compilercheck 执行路径上。
- [ ] pp/gen-delta.sh：只改注释，没有逻辑变化。
- [ ] pp/run.sh：ready 的失败传播、d.json 改走 helper、fresh 的命令文本与输入扩展，语义可接受；确认不在 compilercheck 执行路径上。
- [ ] prepare.sh：opt 段（67b9e777）与 prune 段（699d310d）分别确认；共享私有 SEED_GEN_DIR 的顺序是 opt 冷建，prune 与 pp 复用。
- [ ] **组二**：exec/prune/gen-delta.sh 的参数检查、fail-closed、不回退 Python、key 与 sha 校验，作为新文件单独审核。
- [ ] 已知限定可接受：并发未测；closure 成员没有打印；冷跑缓存成员数为 1 不单独证明命中；不称提速。
- [ ] 若审核通过，按同一算法在审核时的提交上重算 exec 戳（699d310d 上应为 e9d2314a…），记录该提交与戳——**这一步是刷戳（处置 a），需另授**。

## 6. 证据位置
/tmp/cc40-prep/sa/：两个 worktree、inv-*.json、exec-delta.diff；算法脚本 /tmp/cc40-prep/gi1/inv.py。

## 更正（cc 自查；只追加）
第 2 节表里有两处 sha 末段是我手抄写错的，以 inv-699d310d.json 中的全值为准：
- exec/pp/gen-delta.sh（699d310d）：d84b4905da4d08540c70ef48401204d8e5ed22f404186e05573125d026cc7e02（不是“…e7cc02”）
- exec/prune/gen-delta.sh（699d310d）：3a5cd26bf67646835d2c3a47ef03a56b6b29aa96fcb838adf566d7999e113995（不是“…3e995”）
其余各值的首尾段已与 JSON 对过。

## 收窄（cdx2；只追加）
- §5 末项：只读复算得到的 e9d2314a… 只是**候选戳**；把它写进 gatedeps 才算刷戳（处置 a），需要另授。复算本身不等于刷戳。
- §3.4“这几处改动会让旧模型键失效，第一次 compilercheck 会走冷准备”**收窄为**：内容键会变；但新键对应的模型可能已经在缓存里（例如 cold1 或其他运行留下的），所以不保证第一次一定走冷准备。
- §3.1、§3.3“不在 compilercheck 执行路径上”**收窄为**：静态 grep 没有找到直接调用，不能证明不存在间接路径（变量拼出的路径、经其他脚本转调等）；完整 diff 与间接调用还没有核。

## 再收窄（cdx；只追加）
- 冷准备条件：elf.sh 有 STAGE_MODELS_READY=1 旁路，设了就不调用 models.py。所以只有在**实际调用了 models.py**、并且**新键没有有效缓存**时，才会走 prepare.sh（冷准备）。
- §5 末项拆成两步：(1) 审核时只读重算，记录候选戳（材料，不授权）；(2) 把候选值写进 gatedeps 的 reviewed_trees.exec（刷戳，**另授**）。
