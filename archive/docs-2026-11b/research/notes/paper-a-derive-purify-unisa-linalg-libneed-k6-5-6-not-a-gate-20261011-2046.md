# Paper A 衍生净化：tip c41 / unisa.linalg（K6-5）+ unisa.libneed（K6-6）C↔Python 点采样战役不得搭乘 A

- **拍点：** 2026-10-11 ~20:46 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `92e2c089f021610f1658f0f476b88d3f55d94a06`（= K6-6 unisa.libneed 产品钉；祖先含 K6-5 `d3f91b5c`、C #150／`708a717c`、E #149 ZData；Paper A 四 blob 本拍不改）
- **父节点 tip 链（相关）：** K6-5 `d3f91b5c022d2908e1e7c1217fb5abb5e2141cb2`（research(c41)+seed+tests: K6-5 unisa.linalg gemv/relu/argmax/margin C reference vs linalg.py (point sample; same 3, neg 2/2)）；K6-6 `92e2c089f021610f1658f0f476b88d3f55d94a06`（K6-6 unisa.libneed consts/guard/bodies/table/roots C reference vs libneed.py (scratch micro incdir; same 6, neg 3/3)；apply_guards/check_guards 另排 K6-6b）；其父链含 Merge #150 platform-matrix C、#149 ZData E、#148 padded E、#147 peversion E、#146 hostabi E、#145 same7 E
- **Latest：** **v0.0.40**（公开；target `1db3083506e7af178ed604052532d4226511ad01`）；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改该主张；禁 bump/Latest）
- **父节点：** **Paper A**（基于神经网络的编译器；TSV 表构造网络与权重，非训练；T1＝网络≡表（声明域枚举）；unisacc 为 POSIX C99 子集跨架构实证载体）→ 产品／unisa **线性代数原语** 与 **libneed 常量／guard／bodies／table／roots** 参考对 hygiene 旁支（**unisa.linalg / linalg.py**：K6-5 gemv/relu/argmax/margin 点采样 same 3、neg 2/2；**unisa.libneed / libneed.py**：K6-6 consts/guard/bodies/table/roots 点采样 same 6、neg 3/3；`seed/unisa_linalg.c`／`seed/unisa_libneed.c` 不进 release 候选身份；无 gemv_bwd/softmax_ce/Adam；apply_guards/check_guards＝K6-6b 另排；穷举另排）→ 仍回 **A**
- **状态：** **note only**（登记「须先把 unisa.linalg（K6-5 same3 neg2/2）与 unisa.libneed（K6-6 same6 neg3/3；及另排 apply_guards/check_guards／穷举）跑绿／完稿，才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合／缺之则实证封口不完整」升格 ≠ 理论／根主张／投稿闸门；**不起稿**独立 linalg／libneed 方法正文／不起与 A2 同级新预注册；**不改** A 根主张、键数身份数字、CN/EN/TeX/abstract、product/kernel/weights/facts、投稿方向；**不代裁** Ο1/Ο2（仍荐 Ο1，无新信息叠卡）；**不降级** §8.1#3／8509-8769／Ο1/Ο2；**不重钉** [`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-latest40-c41-20261011-1845.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-latest40-c41-20261011-1845.md)／[`paper-a-gap-nail-platform-matrix-tip-refresh-latest40-c41-20261011-2023.md`](paper-a-gap-nail-platform-matrix-tip-refresh-latest40-c41-20261011-2023.md)；**不重做** [`paper-a-derive-purify-unisa-zdata-k6-4bc-not-a-gate-20261011-2016.md`](paper-a-derive-purify-unisa-zdata-k6-4bc-not-a-gate-20261011-2016.md)／[`paper-a-derive-purify-unisa-padded-point-sample-not-a-gate-20261011-1943.md`](paper-a-derive-purify-unisa-padded-point-sample-not-a-gate-20261011-1943.md)／[`paper-a-derive-purify-unisa-peversion-point-sample-not-a-gate-20261011-1930.md`](paper-a-derive-purify-unisa-peversion-point-sample-not-a-gate-20261011-1930.md)／[`paper-a-derive-purify-unisa-hostabi-point-sample-not-a-gate-20261011-1908.md`](paper-a-derive-purify-unisa-hostabi-point-sample-not-a-gate-20261011-1908.md)／[`paper-a-derive-purify-unisabits-same7-point-sample-not-a-gate-20261011-1859.md`](paper-a-derive-purify-unisabits-same7-point-sample-not-a-gate-20261011-1859.md)；**主对象仅** **linalg／K6-5 + libneed／K6-6（及须先 apply_guards/check_guards／穷举完稿）被升读成「投 A／构造+T1／测量身份闭合／Softguess／第四 seal-before」**——**DISTINCT** vs ZData-2016／padded-1943／peversion-1930／hostabi-1908／same7-1859（对象是 **linalg 原语** 与 **libneed consts/guard/bodies/table/roots**，不是 ZData/padded/peversion/hostabi/bits）
- **Softguess：** tip 四文件 **NONE×4**（本拍机扫 Softguess 升格词：`Softguess`／`softguess`／`权重就是`／`weights are the logic`／`逻辑本身`／`权重即逻辑` 均 0 hit；CN `c37b9291a6cc5fcd24beda00299fdf630da0c02c` · EN `fad7b3e0b51731f3615de9753aea0144dbee79b2` · TeX `5c880b0bafd86cbd10691a149212048a7b7d7279` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`）
- **测量身份：** 8509/8769 **仍开** — 仅引用 1845／1745／1514；本拍**不重钉、不叠**；**不叠** Ο1/Ο2（仍荐 Ο1）
- **Paper A 正文漂移：** tip 四 blob **SAME**（相对本拍 tip before；本拍零正文 diff）
- **明确主张：** **linalg ≠ Softguess／≠ 第四 seal-before／≠ 8509/8769 闭合／≠ ZData／≠ padded／≠ peversion／≠ hostabi／≠ same7**；**libneed 同**（本拍钉）——linalg／libneed gate 绿／K6-5+K6-6 hygiene／另排 apply_guards／穷举 **不得**互借升格成理论或投稿闸门；与 ZData-2016／padded-1943 已切的升格风险分立（本拍钉 **已落地的 linalg＋libneed 点采样战役**）。
- **为何本拍选此点（相对优先候选／相对 2016／1943／1930／1908／1859／2023／1845）：** 2016（E）已切 **ZData K6-4b/c**；2023（C）已 tip-refresh §8.1#3 矩阵仍开；此后 tip 已落地 K6-5 `d3f91b5c`（same 3、neg 2/2）与 K6-6 `92e2c089`（same 6、neg 3/3）。新主张：把「须先 linalg/K6-5＋libneed/K6-6（及另排 apply_guards/check_guards／穷举）绿／完稿才许投 A／gate 绿＝测量身份闭合」升成闸门——**不得**搭乘 A。与 2016 **DISTINCT**（对象是 **linalg 原语／libneed 常量与图**，不是 ZData 段式数据类）；与 1943 **DISTINCT**（不是 padded）；与 1930 **DISTINCT**（不是 peversion）；与 1908 **DISTINCT**（不是 hostabi）；与 1859 **DISTINCT**（不是 unisabits same7）；与 2023／1845 **DISTINCT**（本拍不重钉矩阵／键数）。
- **搭乘风险源（本拍钉）：** tip c41／`research/c41-k6-5-linalg.md`／`research/c41-k6-6-libneed.md`／`tests/unisalinalgcheck.sh`／`tests/unisalibneedcheck.sh`／`unisalinalg same 3 diff 0 neg 2/2`／`unisalibneed same 6 diff 0 neg 3/3` 读数；「须先 linalg／libneed／K6-5+6／apply_guards／穷举完稿才许封 A」话语；「linalg／libneed gate 绿＝8509/8769 已闭／构造+T1 才成立」互借；「仅点采样／无 bwd／无 Adam／无 apply_guards／不进 release ⇒ 构造空／身份未闭／该升 Softguess」；与 ZData／padded／same7／hostabi／peversion 闸门混读；与发布验收混读。——**诚实产品／unisa 线性代数原语与 libneed 参考对 hygiene** ≠ 「须先 linalg+libneed 绿才许投 A，或主张缺之则构造+T1／实证封口不成立／须升 Softguess／＝测量身份闭合」闸门。
- **DISTINCT vs 相邻已切：**
  - ZData-2016＝**unisa.padded.ZData / padded.py K6-4b+K6-4c**；本拍＝**linalg K6-5 + libneed K6-6**
  - padded-1943＝**unisa.padded / padded.py Padded 可观测值点采样**；本拍＝**linalg／libneed**
  - peversion-1930＝**unisa.peversion / peversion.py PE VERSIONINFO 点采样**；本拍＝**linalg／libneed**
  - hostabi-1908＝**unisa.hostabi / hostabi.py TSV 六常量**；本拍＝**linalg／libneed**
  - same7-1859＝**unisa.bits / unisabitscheck same7**；本拍＝**linalg／libneed**
  - C nail 2023＝§8.1#3 矩阵在 Latest-40+c41 后**仍开**；C nail 1845＝8509/8769 **仍开**；本拍（**E**）**不**假装已闭、不重钉；且明确 **linalg ≠ Softguess／≠ 第四 seal-before／≠ 8509/8769 闭合／≠ ZData／padded／peversion／hostabi／same7**；**libneed 同**

---

## 主张一句（本页唯一）

**把「须先把 unisa.linalg（K6-5 same3 neg2/2）与 unisa.libneed（K6-6 same6 neg3/3；及另排 apply_guards/check_guards／穷举）跑绿／完稿，才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合／缺之则实证封口不完整」——不得搭乘 A。linalg／libneed 是产品／unisa 参考对 hygiene；A seal-before 仍只是 §8.1#3、8509/8769、Ο1/Ο2（仍荐 Ο1）。linalg ≠ Softguess／≠ 第四 seal-before／≠ 8509/8769 闭合／≠ ZData／padded／peversion／hostabi／same7；libneed 同。**

---

## 切什么（从 A 的主张负担里切出）

| 位点 | 摘句／现状要点 | 分类 |
| --- | --- | --- |
| K6-5 linalg 点采样 | gemv/relu/argmax/margin；C↔Python same 3、neg 2/2（`d3f91b5c`／c41-k6-5-linalg）；无 gemv_bwd/softmax_ce/Adam | **keep 产品／linalg 参考对 hygiene**；切出「linalg 绿＝投 A／T1 成立／身份闭合／Softguess」 |
| K6-6 libneed 点采样 | consts/guard/bodies/table/roots；scratch 微 incdir；same 6、neg 3/3（`92e2c089`／c41-k6-6-libneed）；apply_guards/check_guards 另排 K6-6b | **keep 产品／libneed 参考对 hygiene**；切出「须先 libneed／K6-6b 完稿才许投 A／缺之则实证封口不完整」 |
| 穷举／更大随机对拍另排 | 仍另排；非本窗完稿条件；K6-5/6 明示不证任意输入穷举 | **keep 另排工程债**；切出「须先穷举完稿才许投 A／缺之则实证封口不完整」 |
| `seed/unisa_linalg.c`／`seed/unisa_libneed.c` 不进 release 候选身份 | 参考／夹具域；不触发产品重封；linalg.py／libneed.py 只读未删 | **keep 闭包边界诚实披露**；切出「C 参考＝产品闭包／投稿前提／构造空」 |
| linalg gate `same 3 / diff 0 / neg 2/2`；libneed gate `same 6 / diff 0 / neg 3/3` | 夹具读数；原语／常量图 hygiene | **keep 读数事实**；切出「gate 绿＝8509/8769 闭合／第四 seal-before」 |
| 与 ZData／padded／same7／hostabi／peversion 混读 | 2016＝ZData；1943＝padded；1859＝bits；1908＝hostabi；1930＝peversion；本拍＝linalg＋libneed | **keep 旁支分立**；切出「linalg／libneed＝ZData／padded／same7／hostabi／peversion 闸门」 |
| §8.1#3／8509-8769／Ο1/Ο2 | 仍开硬缺口与决策卡（荐 Ο1） | **keep seal-blocking／待批**；本拍不重钉、不叠、不代裁 |
| A 根主张／T1 | 构造网络；声明域逐键全等 | **keep in A** |

**不切：** 根主张、键数 8509/8769/9174 数字本身、§8.1#3 同身份矩阵硬缺口、Ο1/Ο2、1845／1745／1514／2023 gap-nail 正文、2016 ZData／1943 padded／1859 same7／1908 hostabi／1930 peversion／1628／1759／1715／0911 已切项、product/kernel/weights/facts、CN/EN/TeX/abstract 正文、投稿方向、产品侧 linalg／libneed／unisalinalgcheck／unisalibneedcheck／K6-5+6 工程本身（只切其升格读法）。

---

## 为何不挡 A 投稿（切出项本身）

1. **A 根主张是构造＋T1，不是「unisalinalgcheck same 3 已绿／unisalibneedcheck same 6 已绿／K6-5+6 已完／apply_guards／穷举已完」。** linalg／libneed 是 unisa 参考对 **hygiene**（点采样可观测值相等 + 负例同拒）；不是构造 falsifier，也不是第二套编译器理论，更不是测量身份选定。
2. **linalg／libneed gate 绿 ≠ 理论／投稿闸门／Softguess／8509/8769 闭合／ZData／padded／same7／hostabi／peversion。** 诚实跑 K6-5+6 与另排 apply_guards／穷举是工程卫生；不能反向读成「必须先绿／先穷举才许投 A」或「gate 绿 ⇒ 测量身份已闭／须升 Softguess／＝ZData／padded／bits same7／hostabi／peversion 已闭」。
3. **与 §8.1／8509-8769／Ο1/Ο2／先验 E（含 2016 ZData／1943 padded／1859 same7／1908 hostabi／1930 peversion）正交。** 1845／2023 已钉 Latest-40+c41 ≠ 身份／矩阵闭合；2016 已切 ZData——本拍承接 **已落地** 的 linalg＋libneed，不重切 2016；§8.1#3／8509-8769／Ο1/Ο2 仍开另钉；本拍**不**把 linalg／libneed／穷举加为第四 seal-before，**不**叠 Ο1/Ο2，**不**重做 2016／1943／1859／1908／1930／2023。
4. **切后 A 更纯、更难被随便挑战：** 攻击面从「linalg 仅点采样／无 bwd／无 Adam／libneed 无 apply_guards／不进 release ⇒ 构造+T1 空／实证封口不完整／该升 Softguess／身份未闭？」缩回「A = 构造 + T1 + 诚实测量身份＋unisacc 实证；linalg／libneed／K6-5+6 另挂产品／unisa 参考对 hygiene；硬缺口仍开另钉」。
5. **本拍零 Paper A 正文 diff。** 只 notes + registry；四 blob 不改；**不**把 §8.1#3／8509-8769／Ο1/Ο2 从挡粘清单移除。

---

## 父节点如何回 A

- **产品／unisa linalg＋libneed 参考对 hygiene 回 A：** 任何未来 unisa.linalg／linalg.py／unisalinalgcheck／K6-5／unisa.libneed／libneed.py／unisalibneedcheck／K6-6／apply_guards／穷举短文，仍服务 A 已声明的边界——unisacc／unisa 工具链是实证载体周边；linalg／libneed 是参考对 hygiene，不是构造+T1 的完备条件，也不能 falsify 构造，更不能替代 8509/8769 选定，也不能替代 ZData／padded／same7／hostabi／peversion 旁支。
- **旁支回 A：** 父边 **A（construction + T1 + 测量身份 + unisacc 实证）→ unisa.linalg／linalg.py（K6-5）＋ unisa.libneed／libneed.py（K6-6；及另排 apply_guards／穷举）产品／参考对 hygiene 旁支**；与 2016／1943／1859／1908／1930 同旁支层、对象不同（ZData／padded 点采样／same7 bits／hostabi／peversion vs **linalg＋libneed**）；结果可回来收紧「载体／参考对 hygiene 如何披露」措辞，不能长成与 A 并列的第二套编译器理论，也不能把「linalg／libneed 未穷举／gate」回写成理论投稿前提、Softguess 触发器或测量身份闭合证明。
- **DAG：** A 在主张与测量债（含 8509/8769 选定／§8.1／Ο1/Ο2）收口、政委点头后可独立投／公开；linalg／libneed／穷举 hygiene 可并行；**切出的是升格闸门，不是删掉 linalg／libneed 工程债或 A 硬缺口**。

---

## 切后 A 哪一句更硬

切出后，A 可把 unisa.linalg／libneed 边界收成一句硬边界（**本拍不改正文；仅登记主张**）：

> **Paper A 只主张基于神经网络（构造网络与权重）的编译器与 T1，并以 unisacc 为实证；tip c41／unisa.linalg C↔linalg.py（K6-5 same 3 neg 2/2；无 gemv_bwd/softmax_ce/Adam）与 unisa.libneed C↔libneed.py（K6-6 same 6 neg 3/3；apply_guards/check_guards 另排；及另排穷举）是产品／unisa 参考对 hygiene，不是「须先跑绿／完稿才许投 A／构造+T1 才成立／gate 绿＝测量身份闭合」的理论闸门，也不是 Softguess 触发器、第四 seal-before、ZData 闸门、padded 点采样闸门、same7 bits 闸门、hostabi 闸门或 peversion 闸门——该卫生议题挂回 A，不起稿挡粘。§8.1#3／8509-8769／Ο1/Ο2 仍开另钉（仍荐 Ο1）。**

---

## 衍生课题提案（默认只登记）

| 项 | 内容 |
| --- | --- |
| 题名意向 | unisa.linalg K6-5 + unisa.libneed K6-6 (and deferred apply_guards/check_guards／exhaustive/random paired campaign) hygiene for constructed-table compiler empirics (product/ref-pair side-branch; hangs on A) |
| 一句话范围 | 如何组织 linalg／libneed C↔Python 点采样口径、release 候选边界、apply_guards／穷举另排披露，使线性代数原语与 libneed 参考对 hygiene 不混入 A 理论闸门／不触发 Softguess／不伪闭合 8509/8769／不与 ZData／padded／same7／hostabi／peversion 闸门混读；不含改产品码／不含代裁 Ο1/Ο2／不含闭合 §8.1#3／8509-8769 |
| 父节点 | A（经 construction + T1 + unisacc 实证载体健康） |
| 与 A 关系 | 可回来收紧「载体／linalg＋libneed 参考对 hygiene vs 理论完备／测量身份」披露；不能另起第二根；不能把「linalg／libneed／穷举」写成理论闸门或 Softguess；**不**替代 8509/8769／§8.1／Ο1/Ο2 债闭合；与 2016 ZData E／1943 padded E／1859 same7 E／1908 hostabi E／1930 peversion E／1845／2023 C 正交互补 |
| 本拍动作 | **只登记**；**不起**与 A2 同级新预注册；不改正文；不叠 Ο1/Ο2；不重钉 gap-nail；不重做 2016／1943／1859／1908／1930／1628／1759／1715／1845／2023 |
| 建议下一动作 | 等政委批是否起「草案待批」预注册页；此前维持 note-only |

---

## tip / blob 表（本拍核对）

| 项 | 值 | 相对先验 |
| --- | --- | --- |
| tip before | `92e2c089f021610f1658f0f476b88d3f55d94a06` | = K6-6 产品钉；含 K6-5 祖先 |
| K6-5 钉 | `d3f91b5c022d2908e1e7c1217fb5abb5e2141cb2` | linalg same 3、neg 2/2 |
| K6-6 钉 | `92e2c089f021610f1658f0f476b88d3f55d94a06` | libneed same 6、neg 3/3 |
| ZData E（已切，不重做） | `research/notes/…-unisa-zdata-…-2016.md`（#149） | **DISTINCT** 对象 |
| padded 点采样 E（已切，不重做） | `research/notes/…-unisa-padded-…-1943.md`（#148） | **DISTINCT** |
| peversion E（已切，不重做） | `research/notes/…-unisa-peversion-…-1930.md`（#147） | **DISTINCT** |
| hostabi E（已切，不重做） | `research/notes/…-unisa-hostabi-…-1908.md`（#146） | **DISTINCT** |
| same7 E（已切，不重做） | `research/notes/…-unisabits-same7-…-1859.md`（#145） | **DISTINCT** |
| Latest | `v0.0.40`（target `1db30835…`） | 冻结重测仍 `v0.0.19` |
| CN `research/unisacc-paper.md` | `c37b9291a6cc5fcd24beda00299fdf630da0c02c` | **SAME** |
| EN `research/unisacc-paper.en.md` | `fad7b3e0b51731f3615de9753aea0144dbee79b2` | **SAME** |
| TeX `research/arxiv-paper-a/main.tex` | `5c880b0bafd86cbd10691a149212048a7b7d7279` | **SAME** |
| abs `research/arxiv-paper-a/abstract.txt` | `43d7ea7e7297ed8be9a96e3950e014e359467973` | **SAME** |
| Softguess | **NONE×4** | 不粘正文；linalg／libneed ≠ Softguess |

---

## OUT（本拍明确不做）

- 改正文 CN/EN/TeX/abstract；粘 Softguess／近似推断进正文
- 代裁／叠卡 Ο1/Ο2（仍荐 Ο1；无新决策卡）
- 重钉或降级 8509/8769、§8.1#3
- 把 Draft／Latest 发版／公开／arXiv 方向代裁；bump Latest
- 改 product/kernel/weights/facts／键数身份数字／投稿方向
- 重做 2016 ZData、1943 padded、1859 same7、1908 hostabi、1930 peversion、1628 efficiency、1759 Ctape、1715 Draft seal、1845／1745／1514／2023 gap-nail
- 起 A2 同级新预注册；发明 linalg／libneed／穷举完稿数字；二次 rerun linalg／libneed／改 c41-k6-5／6 裁决措辞
- 把 linalg／libneed／穷举／gate 绿加为第四 seal-before／Softguess 触发／8509/8769 闭合证明／ZData／padded／same7／hostabi／peversion 闸门替身

---

## 明确非目标（同 OUT，登记口径）

- 不解决、不重钉、不降级 §8.1#3／8509-8769／Ο1/Ο2；不代裁公开／arXiv／Ο1 vs Ο2／Latest 发版
- 不改正文键数、表 1/4/5、摘要主张方向；不改 product/kernel/weights/facts；不改投稿方向；不改 linalg／libneed／unisalinalgcheck／unisalibneedcheck／K6-5+6 产品卫生本身
- 不重做 2016／1943／1859／1908／1930／1628／1759／1715／1845／1745／1514／2023
- 不起 A2 同级新预注册；不跑大实验；不二次 rerun gate
- 不把 Softguess／近似推断升成产品路径；本拍确认 tip **NONE×4**；**linalg ≠ Softguess／≠ 第四 seal-before／≠ 8509/8769 闭合／≠ ZData／padded／peversion／hostabi／same7**；**libneed 同**

---

## 证据索引

- 本笔记：`research/notes/paper-a-derive-purify-unisa-linalg-libneed-k6-5-6-not-a-gate-20261011-2046.md`
- 心跳短报：`research/notes/paper-a-heartbeat-report-20261011-2046.md`
- tip before：`92e2c089f021610f1658f0f476b88d3f55d94a06`
- K6-5 钉：`d3f91b5c022d2908e1e7c1217fb5abb5e2141cb2`；材料 `research/c41-k6-5-linalg.md`、`research/c41-k6-5-linalg-run.log`、`tests/unisalinalgcheck.sh`、`seed/unisa_linalg.c`、`unisa/linalg.py`（只读）；gate 读数 `unisalinalg same 3 diff 0 neg 2/2`；无 gemv_bwd/softmax_ce/Adam；seed 不进 release 候选
- K6-6 钉：`92e2c089f021610f1658f0f476b88d3f55d94a06`；材料 `research/c41-k6-6-libneed.md`、`research/c41-k6-6-libneed-run.log`、`tests/unisalibneedcheck.sh`、`seed/unisa_libneed.c`、`unisa/libneed.py`（只读）；gate 读数 `unisalibneed same 6 diff 0 neg 3/3`；无 apply_guards/check_guards（deferred K6-6b）；scratch 微 incdir only；seed 不进 release 候选
- Softguess NONE×4：CN `c37b9291a6cc5fcd24beda00299fdf630da0c02c` · EN `fad7b3e0b51731f3615de9753aea0144dbee79b2` · TeX `5c880b0bafd86cbd10691a149212048a7b7d7279` · abs `43d7ea7e7297ed8be9a96e3950e014e359467973`
- 先验 ZData E（不重做）：[`paper-a-derive-purify-unisa-zdata-k6-4bc-not-a-gate-20261011-2016.md`](paper-a-derive-purify-unisa-zdata-k6-4bc-not-a-gate-20261011-2016.md)
- 先验 padded 点采样 E（不重做）：[`paper-a-derive-purify-unisa-padded-point-sample-not-a-gate-20261011-1943.md`](paper-a-derive-purify-unisa-padded-point-sample-not-a-gate-20261011-1943.md)
- 先验 peversion E（不重做）：[`paper-a-derive-purify-unisa-peversion-point-sample-not-a-gate-20261011-1930.md`](paper-a-derive-purify-unisa-peversion-point-sample-not-a-gate-20261011-1930.md)
- 先验 hostabi E（不重做）：[`paper-a-derive-purify-unisa-hostabi-point-sample-not-a-gate-20261011-1908.md`](paper-a-derive-purify-unisa-hostabi-point-sample-not-a-gate-20261011-1908.md)
- 先验 same7 E（不重做）：[`paper-a-derive-purify-unisabits-same7-point-sample-not-a-gate-20261011-1859.md`](paper-a-derive-purify-unisabits-same7-point-sample-not-a-gate-20261011-1859.md)
- 仍开 C（Latest-40+c41 8509）：[`paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-latest40-c41-20261011-1845.md`](paper-a-gap-nail-measurement-identity-8509-8769-tip-refresh-latest40-c41-20261011-1845.md)
- 仍开 C（§8.1#3 矩阵 tip-refresh）：[`paper-a-gap-nail-platform-matrix-tip-refresh-latest40-c41-20261011-2023.md`](paper-a-gap-nail-platform-matrix-tip-refresh-latest40-c41-20261011-2023.md)
