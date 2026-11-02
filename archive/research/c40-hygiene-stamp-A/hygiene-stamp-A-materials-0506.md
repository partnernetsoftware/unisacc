# 卫生刀 exec 审核戳 A 材料（基线 06855852 → tip 470b2b49；cc，机房主任 05:06）
**材料 ≠ 真审 ≠ 刷戳。** 这份文件只列材料：不勾“已审”，不刷戳，不改 gatedeps，不拿跑门当验收。

## 0. 基线与方法
- 基线 **06855852694092c5e81f25eff20706bdf56fcd8c**，是上次刷戳落地的提交，reviewed_trees.exec 在这里写成了 e9d2314a…。`git diff --quiet 699d310d 06855852 -- exec` 的 rc 为 0，说明两者的 exec 内容相同；699d310d 上算出的 exec 戳就是 e9d2314a…，所以基线的戳等于记录值。
- 对比提交 **470b2b49f54155a27ea5825ecad47c28db32cc5b**：fetch 之后 origin/main 就是它，私有 wt 在 /tmp/cc40-prep/hygiene-0458/wt，HEAD 同样是 470b2b49，status 0 行。
- 06855852..470b2b49 之间，tests/gatequeue.py 和 tests/gatedeps.json 的 diff 都为空，所以两端用的算法和声明相同。
- 复算脚本 /tmp/cc40-prep/gi1/inv.py（sha ff730eea…），以 `env -u UA -u MODEL_COM -u UA_RUN` 运行，rc 0，紧跟命令取得。输出在 /tmp/cc40-prep/sa2/inv-470b2b49.out 和 .json（json sha d98c77ec…）。
- 共享检出没有动过。

## 1. 戳读数（470b2b49）
| 目录 | 成员数 | stamp | 与记录值 |
|---|---|---|---|
| exec | 1411 | **fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336**（候选） | **不等**（记录值 e9d2314a…） |
| include、kernel、src、unisa、weights | 64、41、17、48、21 | 与记录值相同 | 相等 |
候选戳只是只读复算的结果，**不是**已审值。

## 2. 成员差集（两份 JSON 全量比对：/tmp/cc40-prep/sa/inv-699d310d.json 与 sa2/inv-470b2b49.json）
| 路径 | 变化 | mode | 06855852 时 sha256 | 470b2b49 时 sha256 |
|---|---|---|---|---|
| exec/opt/gen-delta.sh | M | 100755 | 1a2287b7a12ea0527470926a250b195c3ff0f6a0a7c7c3620ed2d061ea8f6ba0 | bd93bb19b4e5968828547546fc76d4a016b979659dda168df2544336cad8d045 |
| exec/pp/gen-delta.sh | M | 100755 | d84b4905da4d08540c70ef48401204d8e5ed22f404186e05573125d026cc7e02 | 4a7e6101e12192d53670302acfee9f47f8ef07acfe95b0066142952e671820e2 |
| exec/prune/gen-delta.sh | M | 100755 | 3a5cd26bf67646835d2c3a47ef03a56b6b29aa96fcb838adf566d7999e113995 | 3121e0eebb4ed33d5dd19dd8647a0fc257d5b66b4db1e7a8187c455a0af03a93 |
（sha 全值从 JSON 复制，不是手抄。）没有新增，没有删除，mode 没变。与 `git diff --name-status 06855852 470b2b49 -- exec include kernel src unisa weights` 的结果一致：只有这三个 M。prepare.sh 在本区间没改。完整 diff 在 /tmp/cc40-prep/sa2/exec-delta.diff，39 行，sha 0e0f4050…。

## 3. 每个文件的 diff 摘要（三个文件同构，各改一行）
- opt 第 44 行、prune 第 43 行、pp 第 54 行，原来是：
  D=${SEED_GEN_DIR:-${TMPDIR:-/tmp}/unisacc-seedbin}
  现在是：
  _td=${TMPDIR:-/tmp}; D=${SEED_GEN_DIR:-${_td%/}/unisacc-seedbin}
  外加一句行尾注释，说明和 exec/stamp.sh 的做法相同。
- 语义上的变化：只有在 SEED_GEN_DIR 没设、TMPDIR 以 / 结尾时，默认 cache 目录从 “TMPDIR//unisacc-seedbin” 变成 “TMPDIR/unisacc-seedbin”。两者在文件系统里是同一个目录，差别只在路径字符串。多个尾斜杠只会去掉一个。还多了一个 shell 变量 _td。
- 执行路径（静态读码）：这一行只在 SEED_GEN_BIN 为空、并且 SEED_GEN_DIR 没设的时候执行。exec/pipeline/prepare.sh、exec/c/chain.sh、exec/pp/run.sh 三个消费者调用 helper 时都显式设了 SEED_GEN_DIR，所以在 compilercheck 的冷准备路径（compilercheck → elf → models → prepare）上，**这一行的默认分支不会被走到**。走到它的只有直接调用 helper、又没设 SEED_GEN_DIR 的情况（例如各 seed 检查脚本的默认路线）。这是读代码得到的静态判断，不是对全部调用方的穷举。
- 已有的运行证据（不等于审核）：卫生 0458 的受控跑在 2e48785f 上跑了两次（TMPDIR=/tmp/ 和 /tmp），都是 rc 0、same 20 / bad 0。那两次正例都用私有 SEED_GEN_DIR，所以**没有直接执行**这一行。回执在 /tmp/cc40-prep/hygiene-0458/receipt.md。

## 5. 审核勾选清单（留空，供审核人勾；本材料一项都不勾）
- [ ] 差集恰好是第 2 节的三个 M，与 exec-delta.diff 一致，没有新增、删除，mode 没变。
- [ ] 三个文件的改动字节一致，都只改了默认 cache 那一行（加行尾注释）。
- [ ] 语义：尾斜杠归一只影响路径字符串，不改变缓存目录的身份，不改 key、sha 校验、fail-closed 这些行为。
- [ ] 执行路径：确认三个消费者都覆盖了 SEED_GEN_DIR，compilercheck 冷准备路径不经过默认分支；如有间接调用者，具名列出。
- [ ] 已知限定可以接受：默认分支没有直接运行证据；多个尾斜杠只去一个；修前的红 NOT_RUN。
- [ ] 如果审核通过：按同一算法在审核时的提交上重算 exec 戳（470b2b49 上应为 fbb12970…），并记下提交和戳。**写进 gatedeps 属于刷戳，要另外授权。**

## 6. 证据位置
/tmp/cc40-prep/sa2/：inv-470b2b49.json、inv-470b2b49.out、exec-delta.diff。基线 JSON：/tmp/cc40-prep/sa/inv-699d310d.json。算法脚本：/tmp/cc40-prep/gi1/inv.py。私有 wt：/tmp/cc40-prep/hygiene-0458/wt。

## 补记（只追加）
- 编号说明：没有第 4 节，是为了让勾选清单沿用之前材料里“§5”的叫法，不是漏写。
- 基线成员数据取自 699d310d 的 inv JSON，没有在 06855852 上另外导出一份。理由是 `git diff --quiet 699d310d 06855852 -- exec` 的 rc 为 0，两者 exec 内容相同。
- inv.py 的 rc 0 只出现在我的终端输出里，没有存成证据文件。inv-470b2b49.out 里六行读数都在，但 rc 本身没有落盘。

## 收窄（cdx；只追加）
- §3 的执行条件改为：**这一整行**在构建分支里都会执行，条件是 SEED_GEN=1，并且 SEED_GEN_BIN 为空或没设；即使 SEED_GEN_DIR 已经设了，`_td=` 的赋值也照样执行。SEED_GEN_DIR 已设且非空时，不执行的只是 `${…:-…}` 的默认展开。SEED_GEN_DIR 没设或设成空串，都会走默认展开（`:-` 把空串当作没设）。SEED_GEN=0 时根本不进这一段。
- §3 里“两者在文件系统里是同一个目录”这句，只适用于本次 TMPDIR=/tmp/ 的情况。不推广到 TMPDIR=/、多个斜杠或其他宿主上的路径语义。
- 三个消费者都传了非空的私有 SEED_GEN_DIR，这个静态结论保持不变。
- 全长值（cdx2 要求）：旧记录值 reviewed_trees.exec = e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34；inv.py 的 sha256 = ff730eea12c2aaa7a7162f4a9b921d84d5592402e8fea52c583ee9d29d5c1d58；exec-delta.diff = 0e0f4050ac916a1f3b72fea8e600cb6fd93ff57899034c22112c5582f0ce65a5；inv-470b2b49.json = d98c77ec1328e0b624384f58e3581fcd48187e71a9a904cc7dc8b764841b54be。
- §3 的“已有运行证据”中，“两次正例都用私有 SEED_GEN_DIR，所以没有直接执行这一行”**改为**：两次正例都用私有 SEED_GEN_DIR，整行（包括 _td 赋值）仍然执行了，**没覆盖到的是默认值展开那个分支**。
- inv.py 的 rc 在证据里按 **UNKNOWN** 记：我在终端看到的是 0，但没有存成文件。
