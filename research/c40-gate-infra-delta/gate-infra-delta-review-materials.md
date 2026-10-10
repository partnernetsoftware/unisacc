# gate-infra：exec 三处改动的 delta 审材料（cc，2026-10-11 ~02:1x；机房主任 02:02）
**只出材料。不是审核结论，不勾“已审”，不刷戳，不改仓，不选处置 (a)(b)(c)。** 因果口径保持“已识别充分静态原因，并存原因未排除；tools 与 fixture 内 stamp UNKNOWN”，inventory 措辞不改（交政委）。

## 0. 范围与基准
- 只含三文件：exec/opt/gen-delta.sh、exec/pp/gen-delta.sh、exec/pipeline/prepare.sh。
- 基准：b8ea2faa（exec 审核戳 9ae3c358… 与重算一致的最后一处）；对比：a8a114a0（当前 tip）。
- 完整 diff：/tmp/cc40-prep/next/gate-infra-delta.diff（150 行，sha256 06761f9b…，`git diff b8ea2faa a8a114a0 -- 三文件`）。统计：3 文件，+126 −1。

## 1. 各文件 diff 摘要
| 文件 | 相对 b8ea2faa | 当前 blob / 模式 / 行数 | 引入与修订提交 | 内容要点 |
|---|---|---|---|---|
| exec/opt/gen-delta.sh | 新增 | 1d32a5f6 / 100755 / 57 行（sha256 1a2287b7…） | ab4f75b8 → 418f7743 → 70ed1b2a | opt δ helper：SEED_GEN 只认 0/1；最多一个 flag（--o2）；SEED_GEN_BIN 显式时缺失或失败不回退 Python；默认在 SEED_GEN_DIR（默认 /tmp/unisacc-seedbin）按 key 构建 seed-gen，key = 编译器 realpath/字节/--version/flags + seed/*.[ch]，缓存只在 sha 与 sidecar 一致时复用 |
| exec/pp/gen-delta.sh | 新增 | bdd6b192 / 100755 / 66 行（sha256 a497127d…） | 12b95470 → 1c70d9e8 | pp δ helper：同 opt 形态；flag 只放行 manifest 的 6 个，各至多一次，--osx 与 --win 分别检查成员、互斥 rc2 |
| exec/pipeline/prepare.sh | 变更（+3 −1） | 35 行（sha256 48d34abb…） | 839f1951 | 第 17 行：`b python3 exec/build/gen.py pp …` → `b env SEED_GEN_DIR="$OUT/.seed-gen-cache" sh "$R/exec/pp/gen-delta.sh" "$OUT/e2.json" $OSFLAG $ARCHFLAG`，加两行注释 |

## 2. 为什么进入 compilercheck 的 exec 审核清单（依据）
- gatedeps.json `families.compilercheck`：reviewed_trees 含 exec；all_files_trees 只有 include；inventory_suffixes = .py .c .h .inc .tsv .json .sh .S；excluded_dirs 为空。
- gatequeue.py 第 303–315 行：对 exec 取 git 可见文件，按后缀过滤。三个文件都是 git 跟踪、后缀 .sh、不在排除目录 → **都进入清单**（第 1 步成员差集已列：b8ea→ab4f 新增 opt/gen-delta.sh；→a8a1 再新增 pp/gen-delta.sh、变更 opt/gen-delta.sh 与 pipeline/prepare.sh）。

## 3. 对 audited / fallback 的影响（引第 1 步回执 /tmp/cc40-prep/next/gate-infra-step1-readonly-receipt.md）
- 清单变 → exec stamp 变（ab4f75b8 起不等于 9ae3c358…）→ gatequeue 第 315 行 audited 为假 → 第 363 行 else 分支用 global_inputs（整树）→ 7 个 exec-driver-* 在 README 变动时失效 → queuecheck 第 369 行测例红。
- 只有在审核人确认这三处改动后，把审核戳更新到当时重算的值，这 7 个套件才会恢复精确闭包；那属于处置 (a)，**本材料不做**。

## 4. 与 compilercheck 实际执行的关系（审阅时需判断的事实）
- exec/c/compilercheck.sh 第 13 行调用 `./exec/pipeline/elf.sh`；elf.sh 第 17 行调用 `exec/pipeline/models.py`；models.py 冷准备时调用 `exec/pipeline/prepare.sh`。所以 **prepare.sh 的改动在 compilercheck 的执行路径上**：冷模型准备时，pp δ 现在走 exec/pp/gen-delta.sh（seed-gen 优先，私有 SEED_GEN_DIR）。
- exec/pp/gen-delta.sh 经由 prepare.sh 被调用，**也在执行路径上**。
- exec/opt/gen-delta.sh：tip 上只有 tests/seedoptcheck.sh 调用它，**不在** compilercheck 执行路径上；它进入清单只是因为后缀 .sh。
- 以上调用链是读脚本得到的静态事实，未运行 compilercheck。

## 5. 审核勾选清单（供审核人逐项勾，本材料一项都不勾）
- [ ] 三文件的 diff 与 /tmp/cc40-prep/next/gate-infra-delta.diff 一致，且仅此三文件。
- [ ] exec/opt/gen-delta.sh：参数检查（SEED_GEN 0/1、单 flag）、fail-closed、不回退 Python、缓存键与 sha 校验的语义可接受。
- [ ] exec/pp/gen-delta.sh：同上，外加 6 个 flag 的放行、重复拒绝、--osx/--win 互斥（1c70d9e8 修正后的成员检查）。
- [ ] exec/pipeline/prepare.sh:17：接线与 K5-1c 授权一致；私有 SEED_GEN_DIR 的三项代价（每次冷准备冷编、子目录留在模型缓存、覆盖继承 DIR）已明批（机房主任 01:23）。
- [ ] 已知限定可接受：并发未测；seed-gen 键与模型键当前同为 11 个 seed 文件、不对称是潜在的；gen-delta 头注释过时（延后）；opt 的消费者未接。
- [ ] 这三处改动对 compilercheck 执行结果的影响已知或可接受（prepare 的 pp 步骤由 Python 改为 seed-gen；K5-1c 冷路线 lnx/x86_64 e2 与 Python 逐字节相同，其余目标只有受控证据）。
- [ ] 若审核通过：按 gatequeue 同一算法在审核时的提交上重算 exec 戳，记录该提交与戳——**这一步是处置 (a)，需另授**。

## 6. 相关证据位置
- 第 1 步回执与复算：/tmp/cc40-prep/next/gate-infra-step1-readonly-receipt.md；/tmp/cc40-prep/gi1/inv.py、inv-<提交>.json、matrix.md。
- K5-1 / K5-1b / K5-1c 回执：/tmp/cc40-prep/k5-1/、/tmp/cc40-prep/k5-1b/、/tmp/cc40-prep/k5-1c/。

## 补：两端完整 SHA
- 基准 b8ea2faa543d6f95c43dce0f4ddd477e878ab9b4
- 对比 a8a114a07699efda2101dc442d2dc1fe64f575f4

## 收窄（cdx 02:2x）
- §3 末句“更新审核戳后这 7 个套件才会恢复精确闭包”**收窄为**：更新审核戳只消除“exec 审核戳不等”这一个失败谓词；tools、其他资格条件、K2b 白名单仍须各自满足，**不保证恢复**精确闭包。
- §5 第六项补充：K5-1c 冷路线里独立 Python 参考的真实 rc 是 UNKNOWN；两份 e2 字节相同，不等于“独立 Python 成功执行”已证。

## 补正（cdx2 02:2x）
- §1 表里 exec/pipeline/prepare.sh 补 blob 74c148c1、模式 100644（另两个 gen-delta.sh 是 100755）。
- §3 收窄同 cdx：更新戳只消除 exec 失配这一条件；其他谓词和 K2b 仍须满足——仓库里的正式 K2b 白名单是空表，queuecheck 的 fixture 是自己写的白名单，两者不能混为“已准入”。
- §4 收窄：调用链 compilercheck → elf.sh → models.py → prepare.sh 只在**冷准备、且没有跳过条件**时才走到 prepare.sh；模型缓存命中时不调用 prepare，所以不是每次 compilercheck 都会执行这三处改动。

## 行号说明（grk 02:2x）
材料里说的“prepare.sh 第 17 行”是**替换点**（b8ea2faa 时 gen.py pp 所在的行）。当前 a8a114a0 上，第 17–18 行是新加的两行注释，接线命令在**第 19 行**。
