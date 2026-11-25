# tl_dce 标活复杂度与等价证据

## 结论

(1) 不能把 16 到 64 对象的高增长直接判成标活算法平方。清单按对象大小排序，数量增 4 倍时，进入 DCE 的 tape 文本增 9.19 倍；原生标活语义计数中的标签查询增 9.31 倍、扫描候选字节增 9.10 倍。该子集的缺失外链数固定为 1。首轮 64 tape 标活只得截断下界；提高步数预算后补齐，原版完整标活 593148863 步。速度仍为 UNMEASURED，T1.6 不启动。

(2) 全量标活确有额外算法成本。完整外链下，原生工作表阶段核到 4215678 次缺失外链比较，压紧阶段另有 2208147 次定义或外链比较。这两个内层循环在原生和 tape 上都执行；只优化解码不消除它们。全量尚无 tape 等价结果，也未给全量阶段增长拟合指数。

原始原生计数与 tape 阶段记录已封存在同页工件目录的 `ops-*.mark` 与 `tape-*.mark`。从两份干净归档复跑 16 对象，启动 load1=4.20，三方对拍在单个 `tools/bound 55` 内退出 0；整轮含归档、构造和输出校验为 15.98 秒，不作为速度倍率。

## 一致输入上的计数

原生是在私有归档中的同一 `tl_dce` 加手工计数器，未修改共享仓 `cx/compiler/tapelink.cx`、未改 SID。标活列合计根扫描和可达组工作表；候选字节是送给 `tl_dscan` 的行长之和，可能含早退出，不当作实际读取字节。标签查询包含命中和未命中，探测只数已占槽迭代。tape 列用没有这些增量计数器、仅打印阶段标记的容器，记 `mrk` 到 `cpy` 的解释步数。

| 对象数 | 裁剪前文本 B | 缺失外链 M | 原生标活候选 B | 原生标签查询 | 原生已占槽探测 | 原生缺失比较 | 原生活组出栈 | tape 标活步数 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 | 136982 | 1 | 131240 | 10848 | 22033 | 1335 | 18 | 39108223 |
| 16 | 317638 | 1 | 306533 | 25273 | 2497 | 3427 | 43 | 64998445 |
| 32 | 866823 | 1 | 803557 | 66704 | 5537 | 6651 | 79 | 168631187 |
| 64 | 2920350 | 1 | 2790814 | 235171 | 23943 | 28728 | 188 | 至少 299612473，预算耗尽 |
| 102（全量） | 11918958 | 6 | 18113860 | 1604840 | 193305 | 4246326 | 2316 | UNMEASURED |

8/16/32 的 tape 与原生计数版输出逐字节一致；DCE 裁剪分别为 89/199/495 组。64 原生计数版和未计数版输出一致，裁剪 1270 组，tape 停在标活。全量原生计数版与未计数版输出一致（9540112 B），裁剪 1479 组。子集不登记真实对象外链，仅登记合成死组的缺失外链，不能当作完整链接或直接外推全量。

## 源码模型与改法

(1) `tl_dslot`（`cx/compiler/tapelink.cx:453-464`）用 djb2 截断至 32 位、开放寻址线性探测；标签已索引。容量选至少约 2 倍标签数（:523），但碰撞对样本敏感，8 对象较 16 对象探测反而多。`tl_dlive`（:465-468）首次标活就入栈，:568-578 每组最多弹一次，已有工作表；换成队列不会改变复杂度。

(2) `tl_dscan`（:472-506）扫描每个名字，先查标签；标签未命中且允许登记缺失时，:495-499 线性扫 `tl_dum[0..M)`。设标活扫描候选字节为 B、进入该分支的词数为 U、实际哈希探测成本为 H，标活成本约为 O(B + H + U*M*L)，L 为名字比较成本。这一项在 U 与 M 同时增长时可能近平方。不能从 M=1 的阶梯得出已平方的结论。

(3) 另有独立成本：`tl_miss`（:395-397）在 DCE 建表前逐 extern 线性查定义表；:589-599 每个被删函数又扫整个定义和外链表。它们分别可达 O(E*D) 与 O(K*(D+E))，不应并入标活曲线。102 全量的压紧计数实证了后一项。

(4) 建议 v0.1.4 先做最小对拍：在缺失列表冻结后一次建名字到缺失槽的索引（长度和字节完全确认，保留重名的所有槽），用它替换 :495-499；上界按 M 做减法和容量检查，继续用原标活栈和扫描规则。缺失查找期望由 O(U*M) 降到 O(U+M)，不改变 `tl_dus` 的语义。然后单独处理定义和外链的失效索引，不把两项收益混记。至少对拍无缺失直接原样、死组引用缺失、活组引用缺失仍拒、函数地址根、相邻跌落根、哈希碰撞与内存失败回退。再用固定对象清单和受控缺失规模验增长曲线，不开 T1.6。

## T1.4b-lite 最小批回放

持久工件在 `~/.local/share/unisacc/research/ompgpt/tl-dce-mark-artifacts/`。基线 `tape/m0` 的 `4852f20a1a0a6d2e651cf56fd24c7fb69b3f2812`，实际编译器 SHA-256 `934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1`；内置容器 SHA-256 `909cacfb03d1be04c1c93ada4f69aebad5731652f0d96eacfaec5e726113cf29`。固定对象来自 v0.1.0 链 `a11d28fc0a61939ee71345d732a7dcf205b261f1`。在仓根执行；只写临时目录：

```sh
set -eu
root=$PWD
a=$HOME/.local/share/unisacc/research/ompgpt/tl-dce-mark-artifacts
g=$HOME/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1/fixedpoint/g2/lnx-x86_64
d=$(mktemp -d /tmp/t14-replay.XXXXXX)
mkdir "$d/plain" "$d/count"
for mode in plain count; do
    git -C "$root/.claude/worktrees/tape-m0" archive 4852f20a1a | tar -x -C "$d/$mode"
    ln -s "$root/unisacc.com" "$d/$mode/unisacc.com"
    cp "$a/driver.cx" "$d/$mode/tools/t14-tldce-objects.cx"
done
(cd "$d/count"; git apply "$a/tapelink-native-count.patch")
cp "$a/phase-count.cx" "$d/plain/tools/t14-phase-count.cx"
(cd "$g"; sha256sum --check --quiet "$a/lite16-sha256.txt")
while IFS= read -r f; do printf '%s/%s\n' "$g" "$f"; done < "$a/lite16-order.txt" > "$d/16.list"
for mode in plain count; do
    (cd "$d/$mode"; "$root/tools/bound" 55 nice -n 19 taskset -c 10-11 sh ./unisacc.com -b lnx/x86_64 tools/t14-tldce-objects.cx -o "$d/$mode.exe")
done
"$root/tools/bound" 55 cc -shared -fPIC -o "$d/phase.so" "$a/phase.S" "$d/plain/tools/tape/asm/hoststub_lnx_x86_64.S"
(cd "$d/plain"; "$root/tools/bound" 55 nice -n 19 taskset -c 10-11 sh -c '
    set -e
    "$1/plain.exe" "$1/16.list" > "$1/plain.out" 2> "$1/plain.mark"
    "$1/count.exe" "$1/16.list" > "$1/count.out" 2> "$1/count.mark"
    sh ./unisacc.com tools/tapemode-run.cx -- --quiet --so "$1/phase.so" --budget 700000000 "$2/internal.tapeslice" "$1/16.list" > "$1/tape.out" 2> "$1/tape.mark"
    cmp "$1/plain.out" "$1/count.out"
    cmp "$1/plain.out" "$1/tape.out"
    printf "%s  %s\n" 93b5e2180cbbd49c330f17d49b1b31c7e9ba82b01eac1007e1993cfb3ede7439 "$1/tape.out" | sha256sum --check --quiet
    set -- $(od -An -tu8 -j24 -N8 "$1/tape.out")
    test "$#" -eq 1 && test "$1" -eq 199
' sh "$d" "$a")
(cd "$d/plain"; "$root/tools/bound" 55 sh ./unisacc.com tools/t14-phase-count.cx -- "$d/tape.mark" | tail -6)
sha256sum "$d/plain.out" "$d/count.out" "$d/tape.out"
od -An -tu8 -j24 -N8 "$d/tape.out"
```

预期：三个 SHA-256 均为 `93b5e2180cbbd49c330f17d49b1b31c7e9ba82b01eac1007e1993cfb3ede7439`，cut=199，`mrk -> cpy` 为 64998445 步。最小批计时只含有界执行和对拍，不含构造；正式登记先把清单绑定链产物身份、把候选程序和 `.so` 作为准备材料，再在运行阶段从真实产物重跑，不能缓存 PASS。对照旧运行时快照可另外核同一 SHA。重建容器时，在干净归档应用 `tapelink-phases.patch`、复制同一驱动，`-S -b lnx/x86_64`，用 SHA-256 `816bc690fd41534489d7487373ce3b8231918eda752624439e25816f6c1a4549` 的 `tools/tapepack.cx` 打包并核容器 SHA。不要把 `tapelink-native-count.patch` 用于 tape 步数基线。

## 私有缺失外链索引原型

只在归档树改 `tapelink.cx`，未改共享树和 SID 输入。扩展既有标签表：正槽值指标签，负槽值指缺失外链，0 为空；先登记标签再登记缺失外链，保留标签优先，同一词只散列一次。容量纳入缺失项计数，溢界沿用原样返回。缺失表来自已按完整名字去重的 extern 名称表。没有改压紧阶段。补丁 `tl-dce-mark-artifacts/tapelink-missing-index.patch` 基于 `tapelink-phases.patch`，SHA-256 `84835d76e99bed3d19a69a7ea6985fdf7b630b0967cd750f3d9f724a5e888874`；容器 SHA-256 `e935529f159a64e0b6e05f11488d2aac6b21eaca0a227eaae97023248881599e`。

| 对象数 | 原生基线 s | 原生索引 s | 原生输出 SHA-256，两端 cmp=0 | tape 索引总步数 | tape 索引标活步数 |
|---:|---:|---:|---|---:|---:|
| 16 | 0.109 | 0.116 | `93b5e2180cbbd49c330f17d49b1b31c7e9ba82b01eac1007e1993cfb3ede7439` | 462243199 | 64756464 |
| 64 | 0.522 | 0.467 | `e46d0cc16ad97062bbbd9679251c5d188fe63d99835865b67d0ef47f56feb42c` | 3235309258 | 591258560 |
| 102 | 2.035 | 1.926 | `8972c2cfc069ba3d221a659d1c96100b0687dbe29b4e9c4c95a27f7507c2e76a` | UNMEASURED | UNMEASURED |

原生按基线、索引交替各一次，启动 load1=4.48，不作速度结论。16 与 64 的索引 tape 均 `status=0 exit_code=0`，与索引原生 stdout 逐字节一致，cut 分别为 199、1270；启动 load1 分别为 3.97、5.80。16 相对原 tape 总步数减少 227268（0.049%），标活减少 241981（0.372%），建表反增 14713 步。两子集都只有一个缺失外链，不代表全量收益；在全量 tape 完成前不得把原生单次差值当成优化收益或 15 倍门结论。

64 原版补测用同一 45 亿步预算、55 秒外限，`load1=5.10`，`status=0 exit_code=0 steps=3237008332`，标活 593148863 步；与索引 tape、两份原生均 `cmp=0`。索引总步数减少 1699074（0.052%），标活减少 1890303（0.319%），建表增加 191229 步。没有测缺失外链规模扩大时的完整 tape 收益。建议把外链索引单列，再测压紧热点；不以当前样本支持 v0.1.4 直接上产品。
