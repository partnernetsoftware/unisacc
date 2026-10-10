# R1 exec/COV1 具名语义重审

审阅人：cdx-unisacc；日期：2026-10-10。审阅基线：unisacc-cc HEAD 38560e021668bb86a8d7c70aa1e0725a4c25c47c。对象：8a433c5d 的 exec 差分及 f44992b4 的 reviewed_trees/exec 刷新。cdx 是该修片原作者，本记录为具名作者复审，不冒称独立第二审；与 cdx2 对旧区间的记录范围不同。本记录只审不裁，不结清 R1，不修改 gatedeps、rulings 或验收措辞。

## 两树身份与完整差集

读取 tests/refresh_gatedeps.py 的算法，分别以 git archive、tar.umask=022 提取三个历史提交的 exec；按 compilercheck.inventory_suffixes、路径排序、stat mode 与内容 SHA256 重算，不运行会写入 gatedeps 的刷新命令。

| 快照 | 纳入文件数 | exec stamp |
|---|---:|---|
| 8a433c5d 的父提交 | 1408 | 476d03bd76172cea775fcbed87e8b2e99c9270b74c90b974a810ff723ee3b77d |
| 8a433c5d | 1408 | 9ae3c35897c7185c7b2ef2c11f6985d5281aafb424fe57b9668c2eabd3d34036 |
| 本次 HEAD 38560e02 | 1408 | 9ae3c35897c7185c7b2ef2c11f6985d5281aafb424fe57b9668c2eabd3d34036 |

完整 exec 差集仅三路径：x86-procs-result.tsv 修改；neg-callundef.txt→x86-callundef.txt 为 R100 内容不变重命名；新增 x86-undefined-call.txt。两个 .txt 夹具不属于 compilercheck 的 reviewed 后缀集合，故 stamp 的实际内容变化仅 TSV；它们仍逐项纳入本次语义审阅。当前 exec 未有后继内容变化。f44992b4 明写 hash refresh、不是重审；当时 stamp 的存在不能用作当时审核已经完成的证据。

## 状态机语义审阅

1. 原 LADDR 从 TGT 查 LABD；LABD=0 是目标未定义。定义目标仍走 LA.1，偏移选择与 endo 处理完全未改。本次只改原本直接 DEAD.undef 的分支。
2. x86-line-byte.tsv 的 BR.c 明确写 KND=3；jump 写 1，jumpz 为另一分支。新 LA.undefined 读取 KND，仅 KND=3 置 la=0 并 RET；其他值仍到原 DEAD.undef→undefined 拒绝。不把未定义 jump/jumpz 放行。
3. 新分支只读 KND、使用既有临时寄存器 k，并为返回值 la 赋 0；不写 LABD/TGT/尺寸/偏移表、不消费输入、不新增 PUSH。RET 使用原 LADDR 调用点既有返回栈；WR.c 随后计算 d=la-(OFF+5)，输出 E8 与四字节位移，因此 OFF=0 的 call absent 正好是 e8 fb ff ff ff。新固定状态名在现有表中无其他定义冲突。
4. src/back_encode.c 的 bk_label 在未定义 t<0 时返回 0；x86 call 同样编码 E8、bk_label-(off+5)。这条补覆盖保留参考的完整 tape 与偏移，不用 prune 删除块，不改变后续已定义标签布局。与 ARM 产品已有未定义 call→偏移0的规则相符。
5. 编码器本身不证明调用不可达：本修片在 raw tape 上接受任何未定义 call。源级可达性/未定义函数诊断归前端，src/front_parse.c 的 undef_reach 及检查跳过不可达块说明参考为何允许本例。此审核不声称重新审完产品前端、所有可达性情形、所有 ABI 或原生运行平台；不将 enc 放行当作源级合法性的证明。
6. object 模式有独立未定义符号处理，不能以普通 enc 的分支推定全部对象语义已审完。已核 §24 object 构造输出的 C/Python/ASan 对拍一致，作为构造一致性佐证；没有本窗新增对象运行证据。

## 夹具与既有实测佐证

原 call Nowhere 夹具内容保持，通过正向 x86-*.txt 循环与参考编码字节对拍；这是 R8 补覆盖后的预期更新，不删测。新增夹具 jump live 跳过 unused 段，段内两个未定义 call 后有 ret，live 也有 ret，用于核多处偏移及不可达块的完整输出。原 neg-undef.txt 的 jump Nowhere 继续在拒绝循环中。

已存在的佐证：§24 三十个实际调用位置最终 C O2、Python 参考、ASan 输出逐字节一致；warnparse 首次 3GiB ASan OOM 原记录保留，按已裁独立上限续跑通过，detect_leaks=0，仅 ASan 诊断，不声称 LSan。exec/enc/check.sh 修正夹具后 rc0，54项/0坏，FP 550/0，hello/fib完整参考字节对拍及执行正确。这些绿不能替代上述逐条语义重审，也不能追认 hash refresh 为审核。

cc 干净候选 SHA256 92e6d4a76f30c96d1f8e02c1650ac88a7b66686737d04bc24d67f94bfddc336a 的 com 1/4 原账触发 REVIVED 1。原调用日志 diffo=0 为误取 tail 状态，保留；补录真实 checkrun rc1。删 fb12-31 两账行后 agree195/wrong0/refuse0/revived0/named0。只将补录原 rc 用作 REVIVED 拒绝证据。

结论：对指定 476d03bd→9ae3c358 的完整 exec 差分，未发现超出 R8 授权补覆盖的语义改动或阻塞问题；x86 未定义 call 与参考偏移0编码一致，其余未定义分支拒绝不变。可将本记录提交给机房主任作为真重审材料。是否据此通过审核及结清 R1，仍由机房主任裁定；NEEDS_RULING 不由本记录自行改成 PASS。本窗未复跑套件、未构造候选、未刷新/回退任何审核哈希、未动共享 examples 删除或 gatedeps 其他条目。
