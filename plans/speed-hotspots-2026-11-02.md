# 提速热点表(只读,2026-11-02)

数据来源:`out/c44-five-lnx/entry.elf.rr`(唯一一份可用 receipt,单对象 `entry.elf`)。
**67 对象逐项耗时:UNMEASURED** —— 现有 out/ 无逐对象 receipt。
补测命令(纪元后跑):`for o in <67 entries>; do ... -o ... --time-receipt out/speed/$o.rr; done`
再用 `awk -F'\t' '$1=="RR_TIME"&&$2=="inference"{split($3,a,"=");t[a[2]]+=...}'` 汇总。

## 单对象三相占比(实测,总 ≈ 6.92 s)

| 相 | ns | 占比 |
|---|---|---|
| lower | 3,497,207,341 | **50.5%** |
| elf | 2,359,096,594 | **34.1%** |
| tapecheck | 753,788,385 | 10.9% |
| prune | 246,544,617 | 3.6% |
| construction | 45,683,629 | 0.7% |
| output | 1,215,878 | 0.02% |

## 与目标差距

- 目标:整程序 `-o` 五片 ≤ 40 s;参考 `drive.cx -c -funit lnx/x86_64`:0.0.43 = 8.14 s,79fb84e6 = 6.5 s。
- 单对象 6.92 s 里 lower+elf = 5.86 s(84.6%);**任何"整段重放"都只会放大这两相**,所以先砍重放。

## 每个热点的一刀

1. **lower 3.50 s**:消掉第二遍整段重放(一次性把可见集算完再 lower),[推断] 预期砍到一半以下;改动点在 lower 阶段驱动,不在各 arch 后端。
2. **elf 2.36 s**:`out/c44-five-lnx/` 里 67 个 `.o` 已先例化,优先查"是否重复写/重复定位段";[推断] 预期 20–30%。
3. **tapecheck 0.75 s**:稀疏表,可缓存每 unit 的 tape 指纹跨对象复用;[推断] 预期 40%。
4. 其余(prune/construction/output)合计 7%,不动。

## 2026-11-13 补测(SPEED-1,候选 unisacc.com sha e43ef36832939c7f,全表见 `build/speed-baseline.tsv`)

`UNISACC_RR_TIME=1 sh ./unisacc.com tools/ratchets.cx -o IMAGE`(10 单元,单元缓存命中,2 次,最小值):

| 相 | ms | 占五相合计 |
|---|---|---|
| e3 | 2460.4 | 55.2% |
| lower | 798.8 | 17.9% |
| elf | 464.7 | 10.4% |
| units | 368.7 | 8.3% |
| prune | 334.9 | 7.5% |

- 与上表不同:工具整程序 `-o` 路由下最大相是 e3,不是 lower;上面"每个热点的一刀"的排序只对单对象 `entry.elf` 成立。
- 冷 e2/e1、tapecheck:UNMEASURED。67 对象逐项:仍 UNMEASURED。
- `tools/speed-probe.cx` 对 3 个 `-o` 作业三次均 rc=0 但四列全 0、`per-object.tsv` 未落盘;同命令直接跑有回执。取数缺陷未修。
- 优化补丁:未写。前后对比:无。

### SPEED-1B 冷分相与 e3 定位(2026-11-13,同候选,全新 TMPDIR,`-o` 只编不跑)

各相是什么(`cx/compiler/streamname.cx`):e2 = preprocess,e1 = lex,e3 = parse,prune = prune,lower = lowering;
units 与 elf 是整程序 `-o` 路由的合并与成像步。e2/e1 每单元各跑一次,units 之后的各相整程序各跑一次。
回执里 `inference stage=X` 的区间是 `src/stages.cx` 中 `ex_ri_exec` 一次调用(`cx/runtime/runexec.cx` ->
`src/executor.cx` 的 `core_run`,汇编内核);`model stage=X source=inflate` 是该相网的解压装载。
`-o` 整程序路由的回执没有 tapecheck 相(101 行全文核过)。

`tools/ratchets.cx -o`(10 单元,单元缓存 10 次全未命中,4 次,四次镜像 sha256 相同):

| 相 | 最小 ms | 中位 ms | 占七相合计(最小) |
|---|---|---|---|
| e2(10 单元合计) | 2391.7 | 2434.9 | 33.4% |
| e3 | 2368.7 | 2442.8 | 33.1% |
| lower | 836.7 | 851.6 | 11.7% |
| elf | 464.5 | 476.9 | 6.5% |
| units | 358.2 | 362.6 | 5.0% |
| prune | 338.9 | 342.5 | 4.7% |
| e1(10 单元合计) | 324.3 | 325.5 | 4.5% |
| 合计 | 7163.3 | 7256.8 | 100% |

模型解压另计:合计最小 249.9 ms(e2 95.2、e1 85.6、e3 37.7)。

对照(各 3 次,最小/中位 ms),用来把 e3 拆成"前导"与"用户代码"两段:

| 输入 | e2 | e1 | e3 | 七相合计 |
|---|---|---|---|---|
| 最小 `.c`(`int main` 一行,无前导) | 2.0/2.1 | 0.2/0.2 | 2.7/2.8 | 10.8/11.1 |
| 最小 `.cx`(同一行,只多隐式前导 `include/cx-flat.h`,9165 行) | 108.5/108.6 | 25.8/26.1 | 88.1/90.0 | 253.2/258.7 |

- 实测:只多一份前导,单单元 e3 多约 85 ms、e2 多约 106 ms、e1 多约 26 ms,合计约 242 ms/单元。
- [推断,未实测] ratchets 的 10 个单元各带一份前导;若 e3 对前导近似线性,前导约占 e3 的 880/2369 ms、
  占 e2 的 1085/2392 ms。e3 对输入规模是否线性:UNMEASURED。
- e3 内部更细分段:UNMEASURED。`cx/runtime/profile.cx` 写明 cx 层能看到的最细粒度是一相的墙钟(本路径
  `prof_stage` 的步数传 -1),e3 区间内没有任何 `cx/compiler/**` 代码在途。
- 结论:冷路径上 e2 与 e3 并列最大(各约三分之一),上一节"e3 占 55.2%"只对单元缓存命中成立。
  可动的杠杆都在 SPEED-1B 写域之外:(1) 前导按单元重复预处理与解析(`include/cx-flat.h`、E2/E3 网、
  `cx/runtime` 单元缓存的粒度);(2) `core_run` 与 parse 网本身(`kernel/` 生成物)。
- SPEED-1B 优化补丁:未写。前后对比:无。不声称变快。

### SPEED-1C 前导重复成本:三条路对照与旁证(2026-11-13,已装 0.0.59,冷,全新 TMPDIR,各 3 次)

路径事实(读源码):`include/cx-flat.h` 是生成物(`tools/cx-flatten.cx --write`,源 `include/cx-nested.h`),
`.cx` 单元经 `src/stages.cx` 的 `st_cxh`("cx.h")作强制前导进 e2;e2 按 `exec/facts/pp-autoinc-gen.tsv`
对 `.cx` 自动定义 `__UNISA_FTRIM_LIBC` 与所需 `__UN_*`,因此前导的 e2 输出随单元的需求集变化。
单元缓存(`cx/runtime/unitcache.cx`)键 = UNIUC3 + 产品身份 + route + `\0cli` 状态哈希 + 单元字节哈希,
值是整单元的 e2+e1 结果;e3 是整程序一次 `core_run`,cx 层没有可恢复的中间状态。

实测一(最小 `.cx` 的 `-E` 输出):321,789 B、9093 行,其中非空行只有 200 行;被裁掉的函数体与注释
以等长空白留在输出里。

实测二(同一份 e2 后内容作 `.c` 的 `-include`,只差空白;两者都在 e3 被拒 `multiple definitions`,
没有镜像,只能当诊断,e3 是否跑完未证实):

| 前导字节 | e2 | e1 | e3 | 三相合计(最小 ms) |
|---|---|---|---|---|
| 321,789 B(带空白填充) | 93.1 | 26.1 | 82.1 | 201.2 |
| 7,095 B(去空行、并空白) | 4.8 | 3.0 | 19.8 | 27.6 |

实测三(`.c` + `-include`,此路由不触发自动裁剪,镜像 220,101 B;九次镜像 sha256 前 16 位同为
db9d0e05e3cf966b,即输出字节一致):

| 前导 | 字节 | e2 最小/中位 | e1 | e3 | 六相合计 最小/中位 |
|---|---|---|---|---|---|
| cx-flat.h 原样 | 409,158 | 314.7/328.0 | 61.7/61.9 | 1182.7/1214.1 | 2946.0/2963.7 |
| 去注释(行号不变) | 321,990 | 285.0/295.5 | 58.9/59.0 | 1118.6/1183.2 | 2784.4/2838.2 |
| 去注释再去空行并空白 | 288,822 | 277.1/279.2 | 59.0/60.0 | 1097.3/1148.0 | 2681.2/2747.9 |

第三行的变体会并掉字符串字面量内的连续空白,只作测量,不能原样入库。CLI 加
`-D__UNISA_FTRIM_LIBC=1` 复测三变体,镜像仍是 220,101 B,说明命令行宏不走自动裁剪路径;
隐式前导路径(真正的产品路径)下的紧凑前导:UNVERIFIED。

| 路 | 预期收益 | 依据 | 要改 | 风险 | 状态 |
|---|---|---|---|---|---|
| (a) 前导 e2/e1 结果按内容哈希缓存 | 上限约 130 ms/单元 | 最小 `.cx` e2 107.7、e1 25.3 | E2 网(要能从保存的宏表续跑)、`cx/runtime/unitcache.cx`、`src/stages.cx` | 高:前导输出依赖每单元需求集,键必须含需求集;宏状态跨段传递现网不支持 | UNVERIFIED,未写补丁 |
| (b) 前导 e3 结果序列化复用 | 上限约 85 ms/单元 | e3 87.3 对 2.8 | parse 网与 `kernel/`(生成物) | 最高:e3 是整程序一次推理,无中间状态接口 | UNVERIFIED,判为本版不可行 |
| (c) 前导瘦身/紧凑 | 诊断上限约 170 ms/单元(实测二);仅去注释约 5%(实测三) | 实测一至三 | `tools/cx-flatten.cx` 出紧凑文本;E2 网对跳过区不再出等长空白;`include/cx-flat.h` 重生成 | 中:诊断行号映射、六目标对象 sha 要重对拍 | 部分旁证,产品路径 UNVERIFIED |

决策:选 (c)。理由:空 `.cx` 的前导成本主要随字节数走,不随声明数走(200 行有效内容,
其余是空白);(a)(b) 都要网能续跑。下一步两刀,各自独立成候选:(1) `cx-flatten` 生成时去注释
(行号不变,零语义改动);(2) e2 对被裁区间不出填充。两刀都改产品输入,须重建后在隐式前导路径上
测前后并对拍镜像字节。本节不声称变快。测量脚本与变体在作业临时目录 `tmp/s1c/`(bench.cx、var.cx、
bench1.out、bench2.out、nc.h、ncs.h),未入库。

### SPEED-1D 第一刀补丁:cx-flatten 去注释(2026-11-13,已装 0.0.59,输入 9699fdeed9,补丁未入库)

可行性判断:两刀里第一刀最小且可出补丁;第二刀(e2 不出等长填充)要改 E2 网并重建,本窗口未做。
补丁在作业临时目录 `tmp/speed1d.patch`(216,919 B):`tools/cx-flatten.cx` 加 `fl_strip`(块注释变一个空格,
行注释删去,换行全留,指令内跨行注释改成续行,行尾空白删去,首行标记保留),重生成的
`include/cx-flat.h` 409,158 -> 318,714 B(-22.1%),行数 9165 不变,故诊断行号不动。
`git grep 'cx-flat\.h:[0-9]'` 只命中账本一条历史记录,没有断言依赖行号。

关键限制(实测):隐式前导读的是包内嵌头 `\0hdr/cx-flat.h`,私有副本只能经 `-I` 进入,而 `-I` 目录不算
系统头目录,自动裁剪不生效:最小 `.cx` 不带 `-I` 镜像 sha 635393af18a1812f,带 `-I 原样副本` 变成
db9d0e05e3cf966b(即 SPEED-1C 实测三的未裁剪镜像)。所以下面全部是未裁剪路由的数据,
产品路径(隐式前导+裁剪)的前后对比:UNVERIFIED,须重建产品。

字节等价(A=原样副本,B=补丁后副本,均经 `-I DIR/include`):

- 10 个 `tools/*.cx`(cx-flatten、ledger-add、own-diff、edit-span、register、cap-gen、speed-probe、commit-own、
  tape-gap、ratchets)`-c -o`,`UNISACC_RUN_CACHE=0`、各自全新 TMPDIR,20 次编译 rc 全 0,A/B 对象 sha256
  逐一相同(10/10)。
- `tools/cx-flatten.cx -o` 镜像 681,282 B,A、B 各 3 次 sha 同为 b8472a13d1bc0e3d。
- 独立对照:宿主 `gcc -fpreprocessed -dD -E -P` 去注释后删空白,A、B 的 sha 同为 6b8996b364eba54f。
- 补丁在 `git archive HEAD` 副本上 `git apply` 成功,应用后 `cx-flatten --check` 输出 `SAME include/cx-flat.h 41 headers`。

耗时(只有 1 个单元测完;`tools/cx-flatten.cx -o`,冷,全新 TMPDIR,各 3 次,最小/中位 ms):

| 前导 | e2 | e1 | e3 | 六相合计 |
|---|---|---|---|---|
| A 原样 409,158 B | 982.6/1015.3 | 166.5/167.4 | 4323.3/4348.8 | 9014.8/9088.5 |
| B 去注释 318,714 B | 933.0/948.1 | 166.2/167.1 | 4228.5/4517.9 | 8889.8/8999.5 |

e2 最小值降 5.0%,合计最小值降 1.4%;e3 中位数 B 反而高,在噪声内,不声称 e3 变快。其余 9 个单元的
耗时:未跑(每次约 9 至 20 秒,10 单元各 3 次超出窗口,已按真实 PID 停掉在途测量)。
`cx-flatten` 目前没有 `check-*.cx` 与批次,落地时应补(含行数不变与无注释两条断言)。

## 补测工具(已写,纪元后用)

`tools/speed-probe.cx`(批次 `speed-probe`,check 6/6 PASS):输入 `JOBS.tsv`(每行 `NAME<TAB>ARGV...`,
经 `sh ./unisacc.com <ARGV...>` 跑,`UNISACC_RR_TIME=1`,stderr 收 receipt),输出到 stdout 的
`per-object` TSV(name/total_ms/lower_ms/elf_ms/tapecheck_ms/rc)与 `top10` 排序。零 shell、零 libc,
只走 `sy_spawn_pipe`/`sy_read`/`cx_read`/`tx_*`。
67 个对象的 `JOBS.tsv` 由调用方按同一 recipe 生成(纪元后跑):
`for o in out/c44-five-lnx/*.o; do ...; done` → 再 `sh ./unisacc.com tools/speed-probe.cx -- JOBS.tsv out/speed`。

## 0.0.61 重构提速方案(2026-11-13,设计,未改产品)

候选身份沿用基线 `e43ef36832939c7f`。下面凡写 [推算] 的都不是实测。慢车道 = 改产品输入后必须重建再测。快车道 = 只动工具或测量,不重建。

### 各相为什么贵,能否从设计上拿掉

- e2:冷编译 ratchets 七相最小 7163.3 ms 里 e2 2391.7 ms(33%)。`.cx` 每单元强制前导 `include/cx-flat.h`(9165 行)。最小 `.cx` 比最小 `.c` 的 e2 多 106.5 ms。e2 对被裁行仍输出等长空白:诊断里 321789 B 只有约 200 行有字符,压到 7095 B 后 e2+e1+e3 从 201.2 ms 降到 27.6 ms(该路被拒,无镜像,不能当产品路径)。设计上能拿掉的是等长填充,不是再去注释。去注释已实测只降六相合计 1.4%(9014.8 到 8889.8),账本 SPEED-1D-DECISION 不排进候选。
- e1:324.3 ms(4.5%)。随 e2 输出字节走。填充拿掉后会跟着降,不单独立项。
- e3:2368.7 ms(33%),整程序一次 `core_run`,无中间状态(`cx/runtime/unitcache.cx` 只缓存 e2+e1)。单元缓存命中时 e3 占 55.2% 只对那条路径成立,不能拿来排冷编译。增量或分块要改 parse 网,本版不可行,列为被高估项。
- prune 338.9(4.7%)、units 358.2(5%):合并步,不是重复前导。不单独立项。
- lower 836.7(12%)、elf 464.5(6.5%):2026-11-02 单对象表里 lower+elf 占 84.6%(3.50 s+2.36 s / 6.92 s)。那组数不是本基线。本基线两者合计 1301.2 ms,占七相 18%。没有新证据说整段重放还在。不把旧 84% 当 0.0.61 收益。
- 启动 first 796.354 ms:工具自述含 watchdog、编译、运行、管道,不是解释器本身。repeat 27.273 ms。内部分相 UNMEASURED。800 ms 不能当成可单独砍掉的启动。
- 生成代码:8MB sort 中位 unisacc 3862 ms,gcc-O0 475 ms,gcc-O2 311 ms。账本 REGALLOC:严格直通只折 2.9% 负载,文本上界 47% 不是可兑现值,且原型曾写出错误 mov。这是运行时最大缺口,不是编译时缺口。
- 迭代:tlib kit 冷建约 40 s 已被内容键和单元增量打过(改一字节约 37 s 降到 7.1 s,账本 KIT-UNIT-INCR)。再改 kit 键是被高估项。affected 提交档 44 批若每批冷启动, [推算] 44*796 ms = 35 s 只花在启动;若已共享进程则这个数不成立,必须先测。

### 评估表(收益上限是推算,除非注明实测)

| 项 | 车道 | 收益上限 | 依据 | 改动面 | 风险 | 排序 |
|---|---|---|---|---|---|
| g 生成代码槽级活跃性 | 慢 | [推算] sort 从 8.1 倍 O0 降到约 4 倍,不是追平 475 ms | 3862/475;47% 只是 load 文本上界 | lower 之后的 tape 文本,默认关 | 高:REGALLOC 原型语义错误 | 1 |
| a e2 不出等长填充 | 慢 | [推算] 前导 e2 每单元从约 93 ms 降到约 5 ms 这一档的一部分;10 单元上限约 0.9 s,不是整个 2391 ms | 诊断 93.1 对 4.8;产品路径 UNVERIFIED | E2 网、重建、cx-flat 行号映射 | 中:六目标对象 sha 要重对拍 | 2 |
| h 批次共享 prep | 快 | [推算] 44 批冷启动 35 s;实测可能远小 | first 796.354 * 44;进程是否共享 UNMEASURED | tools/affected.cx 与批次准备,不新建常驻服务 | 低,先测再改 | 3 |
| b 单元级并行 | 慢(单次 -o) | [推算] 只覆盖 e2+e1 2716 ms 的墙钟,e3 2368 ms 不动;10 个相近单元时墙钟上限约少 2 s | 七相表 | src 驱动,e3 之前 fork | 中:对象字节必须稳定 | 4 |
| f 启动分相 | 快 | 上限未知,先消除误判 | 内部分相 UNMEASURED;repeat 已是 28 ms | tools/run-startup-bench.cx | 低 | 5 |
| c 前导 e2/e1 跨单元缓存 | 慢 | [推算] 约 130 ms/单元,且需求集不同就失效;a 落地后此上限下降 | SPEED-1C 表 (a) | unitcache 键加需求集,E2 要能续跑 | 高 | 不做,排在 a 之后再估 |
| d e3 增量 | 慢 | 无接口就没有可兑现上限 | e3 一次推理 | parse 网与 kernel | 最高 | 不做 |
| e lower/elf 去重放 | 慢 | 本基线合计只 1.3 s,旧 84% 不适用 | 836.7+464.5 对 7163.3 | lower 驱动 | 中 | 不做,除非新测量证明重放还在 |

被高估、不要排期:去注释(实测 1.4%);冷路径 e3 占 55%(只对缓存命中);启动 800 ms 当解释器启动;REGALLOC 47% 当可兑现加速;kit 再改键(7.1 s 已落地);lower+elf 占编译 84%(旧单对象表)。

### 前 5 项五段简报

(1) SPEED-2 生成代码,慢车道,必须等产品重建。
- SPEED-8 实测更正(2026-11-13,已装 0.0.59,探针 $CLAUDE_JOB_DIR/tmp/s8/s8.c,数据行在 build/speed-baseline.tsv):8MB sort 的 8.1 倍不是纯代码生成差。运行库 qsort 是堆排序加逐字节 swap(include/stdlib.h:769-805),gcc 档链的是 glibc 归并。
  - 拆解 [实测]:同一份堆排序源 gcc-O0 1767 ms,unisacc 3867 ms,glibc qsort(gcc-O0 比较函数)463 ms。算法倍数 3.82,代码生成倍数 2.19,乘积 8.4。
  - 原型 [实测]:内省排序(三数取中快排,12 以下插入,超深度回落堆排序,4/8 字节按字交换)用已装产品编译,8MB sort 5 次最小/中位 998/1007 ms,对照同轮运行库 4009/4213 ms,提速 4.18 倍;5 次 stdout 逐字节相同。对 gcc-O0 的差距由 9.1 倍降到 2.18 倍。
  - 对拍 [实测]:rec(16 字节结构 50 万)、str(指针加 strcmp 20 万,含重复)、odd(7 字节 30 万)、pat(6 种病态分布各 100 万)四程序,unisacc lib/hs/pq 与 gcc-O0 lib/pq 五路 stdout 哈希全部相同;耗时 rec 2120 至 740,str 589 至 151,odd 880 至 424,pat 7867 至 3228 ms。
  - 热循环构成 [实测,tape 行静态计数,未反汇编 x86]:swap 内层每字节 unisacc 75 条 op 加 20 条 .frame(load 24,store 14,其中求值栈压入/弹出各 10;imm 15;以 r6 为基的槽地址 sub64 14),gcc-O0 22 条(load 14,store 4)。前三项差异:求值栈往返 40 条(含 .frame),槽地址 imm 加 sub64 共 28 条,其余 27 对 22。微基准:8800 万次比较调用 1127 对 423 ms,4000 万次 4 字节 swap 1746 对 605 ms。
  - 未做:堆排序内 3867 ms 只测到比较 81607047 次(按微基准单价折算约 1045 ms,属推算),sift 与 swap 未分开计时;perf 被 perf_event_paranoid=4 挡住。include/stdlib.h 的补丁未出,kernel/unisa_headers.inc 需随产品重建;等值键的相对顺序会变(qsort 本不稳定);4/8 字节按字交换假定可非对齐访问,六目标未验。
  - 已落地(SPEED-10,2026-11-13):include/stdlib.h 的 qsort 改为内省排序(_unisa_qintro/_unisa_qins/_unisa_qswap,_unisa_hsort 保留堆排序作回落);检查 tools/check-qsort-intro.cx;已装产品内嵌头仍是旧堆排序,产品重建后才生效;8MB int 前后 5 次最小 4235 -> 1096 ms。
  - 结论:先换运行库 qsort(收益 4.18 倍,已实测),槽级活跃性只针对剩下的 2.19 倍,其验收基线应改为同算法对照。
- 目标:单基本块槽级活跃性,默认关;开时只删可证明的 load,保留 store 与 .frame。不重做 2.9% 直通。
- 写域:src/stages.cx;cx/compiler/**(lower 之后的 tape 文本变换)。禁区:cx/sys、cx/core、apps/csih、ujs。
- 验收:关时 8MB sort 输出与现产品逐字节相同。开时 sha 输出与基线逐字节相同,且 sort 中位低于基线 3862 ms 的 80%(即 <=3089 ms,5 跑中位)。工具 tools/check-bench-gencode.cx。基线行:tools/check-bench-gencode.cx / 8MB sort unisacc / 3862。达不到 3089 就关开关入库,不声称变快。
- 截止:设计 60 分,实现另计,单步不超过 5 分。
- 禁区:不改函数语义,不吞 .frame,不在窗口内写 rS 仍删 store。

(2) SPEED-3 e2 不出等长填充,慢车道,必须等产品重建。
- 目标:被裁区间不再输出等长空白。行号映射保留。去注释不在本项。
- 写域:E2 网事实表与生成器;include/cx-flat.h 只作生成物重出。禁区同上,且不手改 kernel/。
- 验收:重建后隐式前导路径上,最小 .cx 的 e2 最小 <= 基线 108.5 ms 的 50%(<=54 ms);ratchets 冷七相最小 <= 基线 7163.3 ms 的 85%(<=6088 ms)。镜像与重建前逐字节对拍,不一致即失败。工具:UNISACC_RR_TIME=1 回执,基线行「最小 .cx -o 推理相 e2」108.5 与「七相推理合计」7163.3。
- 截止:网改与对拍分开,各不超过 5 分墙钟的测量步;重建本身按慢车道。
- 禁区:不把诊断路 201.2 到 27.6 写成产品收益。

(3) SPEED-4 批次共享准备,快车道,现在就能做。
- 目标:先测 affected 提交档 44 批里启动与 prep 各占多少;若冷启动合计 >10 s,让同一产品身份的批共享一次准备,不新做常驻服务(FOCUS-PRODUCT:基础设施不扩张)。
- 写域:tools/affected.cx;tools/run-batch.cx。不改产品。
- SPEED-9 实测(2026-11-13,load 1.4 至 2.4,基线表同日 6 行):
  - check-cx-text-fmt 的 47.9 s 不是冷编译:编译 1.3 s,运行 47.9 s,全在 overflow-width 一例。prelude 的宽度填充逐字节走完才报溢出,约 89 MB/s。该例改为最小溢出 `%2147483647d%d`,批次 run_ms 29.5 s,PASS,18 例不变。
  - 15 s 目标在检查侧走不通:溢出路下限 23.6 s(2^31 字节填充);`%lc` 非法码点与超 int 宽度都不返回负值。下一条路在 prelude 的 vsnprintf:计数越过 INT_MAX 即返回,或填充按块计数。归 cx 车道,未做。
  - kit 命中 945/957 ms 的构成:cmd_kit 之前约 290(启动、编译装载 tlib.cx、键计算),产品摘要 56,kit_verify 8,17 包 kit_keep 循环约 590(62%)。大头是逐包复核而不是子进程启动。「进程内核键跳过子进程」的 affected 原型未做;SPEED-7 的 120 ms 探针不含 kit_keep 复核,不能当作等价替换的成本。
  - 共享检出里 tools.cxlib STALE 时不走命中路:每次 2.75 至 2.84 s,kit.tsv 每次退役重写。
  - 未做:tlib-link、tlib-base 的时间分解;cx-l0-text-fmt 预算列仍为 55(tests/batches.tsv 同列须同步,不在本次写域)。
- 验收:同一提交档墙钟比改前下降,且批结果逐条不变。判据:测完把启动合计写入 build/speed-baseline.tsv 新行;若启动合计 <10 s 则本项只交测量、不改调度。工具:affected 自身计时加 run-startup-bench 的 first 796.354 作对照,不把 35 s 当实测。
- 截止:测量 30 分。
- 禁区:不新增长驻进程,不放宽时限,不删断言。
- 先测结果(2026-11-13,实测,候选 e43ef368,只测未改):12 批各 3 次 `affected --ids X --run -j 1 --prepare-materials`,取 BATCH_TIMING。total 最小/中位 ms:awpath 828/842,surfline 813/846,std-surface 873/902,doc-pair 1140/1205,tlib-cover 3100/3400,xs-bound-width 4060/4071,unfreeze 4513/4611,tlib-prep 11572/11850,ledger-add 15303/15422,tlib-base 23549/24006,tlib-link 29286/29313,cx-l0-text-fmt 18630/18640(该批 FAIL,非本任务引入,未定因)。最小之和 113667。
  - 每批固定启动成本:prep_ms 全 0,link_ms 0 至 1,norm_ms(产品前后摘要与清理)最小 779 至 831,12 批之和 9531,占总和 8.4%;轻批 awpath/surfline/std-surface/doc-pair 的 norm 占 total 的 70% 至 95%。每次 affected 调用另付 kit_ms 约 1.0 s + prep_ms 约 0.8 s(11 次单批调用 977 至 1074 / 758 至 894)。
  - 约 800 ms 编译 first 不在 run_ms:轻批 run_ms 最小 21/39/93/309 ms 均小于 800。重批 run_ms 内编译占比 UNMEASURED(BATCH_TIMING 不分相)。norm 的 0.79 s 是产品摘要,不是编译。
  - 外推(算术,非实测):44 批各付一次 norm 约 34.8 s;44 批各自一次调用的 kit+prep 约 79 s。判据 10 s:norm 外推超出,但那是摘要不是解释器启动;是否合并为每产品身份一次摘要由后续改 run-batch 决定,本次不改。
  - -j 4 整体(11 批,不含 cx-l0-text-fmt):墙钟 44353 ms,AFFECTED_RUN elapsed 43824(kit 2764 prep 773 run 40287),pass=10 fail=1(tlib-prep 本次 FAIL,单批 3 次 PASS,输出含 `tlib: STALE tools.cxlib`,工作区有他人改动;未定因)。同 11 批 -j 1 total 最小之和 95037。
  - 测时 load 2 至 4.6,4 路并行互扰,heavy 批冷热差大(awpath 首次 4159 后 828,首跑含缓存冷)。
- SPEED-7 定因与方案(2026-11-13,实测,候选 e43ef368,APE 单文件 6856916 B,load 1.4 至 1.9;入库的 run-batch/affected/productpkg 未改,补丁待集成者核):
  - norm_ms 是什么:`tests/run-batch.cx` 第 131 行的 `product_digest(root,after)`,即 `tools/cxlib/productpkg.cx` 的 `pp_product_digest`。APE 布局下它做三件事:`pp_layout` 整文件读一遍找 UNIPKG1 尾(16 至 18 ms);`pp_file_hex` 再整读一遍(16 至 17 ms);用序曲的软件 `cx_sha256_hex` 逐字节算 6.86 MB(734 至 779 ms,约 9 MB/s)。不起子进程、不调 shell。800 ms 的原因就是软件摘要;同一块字节走 `sys.sha` 的内核 AF_ALG 是 38 至 39 ms,两者十六进制逐位相同(探针 5 轮 same=1)。
  - 分步骤(5 轮,最小/中位 ms):layout 16/17,read 16/16,soft_sha 734/735,kernel_sha 38/38,pp_product_digest 整体 771/775,lstat 千次 1/1。
  - 漏计:运行前那次 `product_digest(root,before)` 在 `t_prep` 之前,不进 BATCH_TIMING 的任何一相。所以每批真实固定成本是两次摘要约 1.55 s,不是 norm 的 0.79 s。实测:直接跑 run-batch surfline,墙钟 2044/2054 ms 而 total_ms 只有 805/810。
  - 这项摘要守什么:前后两次整产品摘要相等才判 PASS,防的是批次运行期间(含被杀进程树的孤儿,所以 after 排在收割之后)有人改写了被测产品;另把 before 作为 `candidate=` 写进 BATCH_RUN,把结论绑到确切字节。两条都要求读真实字节。
  - 候选对照:
    - (A) `pp_file_hex` 对 >=65536 B 的成员走 `sy_sha256_hex`,失败回落软件实现(与 run-batch 自带的 `digest()`、`tp_sha` 同形)。收益实测:摘要 771 -> 73 ms,norm 784 -> 74,单批墙钟 2044 -> 645 ms。安全性不变:算法、输入字节、前后比较、symlink 拒绝都没动,只换了算 SHA-256 的实现,内核路失败即回落。改动面:productpkg.cx 加 1 条 use、改 1 行;所有调用者(zero-wave、preflight、package-refresh 等)同时受益。选它。
    - (B) 同一次 affected 调用内按 (dev,inode,size,mtime_ns,ctime_ns) 复用摘要。不选:stat 相同不证明字节相同(mmap 写回、改后恢复时间戳),会把「逐字节未变」降成「元数据未变」,削弱了上面那条性质;(A) 之后剩余收益上限只有约 75 ms x 2。
    - (C) affected 算好摘要传给 run-batch、run-batch 只验长度。不选:after 必须在批跑完后重读,传入值只能替代 before,而 before 若不由 run-batch 自己读,就不能证明它起跑时产品是那份字节。
    - (D) `pp_product_digest` 里 `pp_layout` 被调两次(自身一次、`pp_member_list` 一次),各整读一遍;合成一次可再省约 17 ms x 2 次摘要(推算)。可随 (A) 之后做,未做原型。
  - prep 约 0.8 s 的原因相同:`tools/affected.cx` `prebuild()` 无条件 `file_sha(ROOT/unisacc.com)`,走软件 `cx_sha256_hex`,即使没有任何材料要建。改成 `tp_sha`(cxlib.tlibpkg 已导出,内核路加回落)一行:prep_ms 755/772 -> 59/59(各 3 次,单批 surfline)。
  - kit 约 1.0 s:命中路(rebuilt=0 reused=17 kit=unchanged)墙钟 972/987,tlib.cx 自报 ms=675/682,余约 300 ms 是 sh 加产品启动加键计算(tlib 只打用法 164 ms)。675 ms 内部是 `ensure_product_sha`、17 个 tlib 的 `kit_keep`(load_lib 验字节、fresh 重算源闭包、input_sha)与 `kit_verify`,逐步占比 UNMEASURED。进程内单算 kit 键并看 kit.tsv 在不在只要 120/122 ms(4 轮)。方案(未做原型):affected 先在进程内 `tp_kit_dir` 加 `sy_exists(kit.tsv)`,在就不起 `tlib kit` 子进程;安全性靠每批链接时 `tlib prep` 仍按原样验 kit_sha、产品 sha 与 tlib.cx 闭包(cmd_prep 注释所述 3/4 退出码),坏 kit 仍会落到 fallback 或拒绝。收益上限约 0.85 s 每次调用(推算)。须先确认 kit_keep 的 STALE 重建职责在命中键下确实冗余,再改。
  - 合计:单批 affected 调用 elapsed 3798/3807 ms;只打 prep 补丁 3113/3142(实测);再加 (A) 约 1.7 s(推算:3113 减两次摘要 1.4 s);再跳过命中的 kit 约 0.85 s(推算)。339 批提交档每批省 1.4 s 约 475 s 的 CPU 时间(算术外推,非实测;-j 4 下墙钟约四分之一)。
  - 补丁与原型(仓外,未入库):`$CLAUDE_JOB_DIR/tmp/speed7-productpkg-kernel-sha.patch`(20 行)、`speed7-affected-prep-kernel-sha.patch`(11 行),即 `/home/devali/.claude/jobs/2798e440/tmp/`;探针 `w/tools/normbench.cx`、`w/tools/kitkeybench.cx`。未验:PP_SPLIT 布局、非 lnx/x86_64(内核路直接返回非零走回落,行为同现状)、productpkg 的 check 批次(check-productpkg 等)须在补丁入库时由 affected 实跑。

(4) SPEED-5 单元级并行,慢车道(单次 -o 在产品内)。测量步现在就能做。
- 目标:e3 之前按单元并行 e2/e1。e3、lower、elf 仍整程序一次。
- 写域:src/stages.cx;cx/runtime/unitcache.cx。禁区同上。
- 验收:10 单元 ratchets 上,并行后 e2 墙钟 <= 串行 e2 2391.7 ms 的 60%(<=1435 ms),七相合计 <=6088 ms,对象 sha 与串行一致。基线行「推理相 e2(10 单元合计)」2391.7。单元时长不均导致达不到 60% 时,交分单元表并停,不硬凑并行。
- 截止:分单元测量 30 分;并行实现另计且依赖重建。
- 禁区:不并行 e3。

(5) SPEED-6 启动 first 的构成,快车道,现在就能做。
- 目标:把 first 796 ms 拆成预处理/装载/绑定/推理/输出,或明确该工具做不到并改工具。不优化未测的相。
- 写域:tools/run-startup-bench.cx。
- 验收:基线里「启动内部分相」从 UNMEASURED 变为五相都有最小与中位,且五相之和与 first 796.354 ms 的差在 15% 以内;对不上就记 UNVERIFIED,不改产品。工具即该 bench。
- 截止:30 分。
- 禁区:不把 first 降到 28 ms 当目标(那是 repeat,不含编译)。
- 先测结果(2026-11-13,实测,同形最小程序,全新 TMPDIR,`UNISACC_RR_TIME=1`,3 次,load 4.4,未改工具):first 墙钟 896/905/875 ms(最小 875,中位 896)。
  - first 分相(ms,三次):推理六相合计 739.0/735.0/722.5(e2 182.7/248.3/187.3,e1 31.4/46.8/33.5,e3 248.8/225.4/272.8,prune 84.1/82.8/89.0,lower 121.3/86.8/93.3,elf 70.7/44.8/46.7);模型解压六相合计 104.4/121.1/103.5(modelcache 全 miss);construction 4.3/4.0/3.7;lookup 0.1;output 0.1。回执合计 847.9/860.3/829.9,占墙钟约 94% 至 95%。
  - repeat:墙钟 9 至 12 ms(6 次),回执 lookup_hit=2 lookup 4.9 ms,无任何编译相。即 first-repeat 差约 865 ms 基本全是编译(推理+解压);watchdog、sh、进程、管道、运行最小程序合计 UNMEASURED,上界为 repeat 墙钟 12 ms,另有 first 墙钟减回执合计的余量 45 至 50 ms 左右。
  - 缓存装载:first 是全新 TMPDIR(缓存空),装载无命中,repeat 的装载即上述 lookup 4.9 ms。
  - 五相和对 first:回执合计与墙钟差在 15% 内。run-startup-bench 本身未跑(它按 8 s 界四次并有源身份观测);此处用 sh ./unisacc.com 直接对同形程序,不是该工具的输出。

测量总则:每项用上表点名的工具,对照 build/speed-baseline.tsv 的 min 列。新数追加行,不改旧行。未跑记 UNMEASURED。

## 运行库函数吞吐扫描(SPEED-11,2026-11-13)

条件:已装 unisacc.com sha e43ef36832939c7f,HEAD 3e34166ffe,load 3.3 至 6.9(未因负载丢数据)。同一份源 `$CLAUDE_JOB_DIR/tmp/s11/bench.c` 分别 `sh ./unisacc.com bench.c -o` 与 `gcc -O0 -fno-builtin`(链 glibc)。进程内每项连跑 5 次,clock_gettime(CLOCK_MONOTONIC) 纳秒,取最小与中位。结果文件与标准输出六组 cmp 全部逐字节一致。汇总 `sum.cx`、`psum.cx`(同目录,.cx)。入库代码未改,补丁未出(只有仓外原型 `proto.c`)。

读法:比值是 unisacc 档对 gcc-O0 加 glibc 档,同时含三样:运行库算法、代码生成(SPEED-8 实测约 2.19 倍)、glibc 的 SIMD。能靠改 include/ 拿回的部分看下面「前五项」的原型前后数(原型与现状同用 unisacc 编译,差值只来自运行库写法)。

### 全表(全部 [实测];us 为每轮总耗时,最小/中位;比值为最小比/中位比;按最小比降序)

```
函数                    规模B    次数   unisacc最小/中位us   gcc-O0最小/中位us  比值
realloc_double        4194304       5      32621/32983            1/1        20287/20486 (注1)
memchr                   4096    1024      20280/20551           34/34        591/598
memcmp                   4096    1024      26687/27381           46/46        578/592
snprintf_s               4096    1000      74857/75461          136/136       548/551
strchr                   4096    1024      25274/25302           48/48        517/517
strlen                   4096    1024      12627/12638           28/47        440/265
strncmp                  4096    1024      28583/28591           75/75        378/376
memmove_overlap_back     4096    1024      22173/22682           59/66        375/342
memcpy_misalign          4096    1024      20729/20962           60/60        344/348
strcmp                   4096    1024      22015/22557           65/65        334/342
strcat                   4096    1024      27214/27272           85/85        320/319
strcpy                   4096    1024      20820/20874           69/76        300/272
malloc_free           4194304     200       1697/1707             6/6         258/258 (注2)
strchr                4194304       1      25363/25419          164/176       155/144
snprintf_s            4194304       1      77723/78625          518/537       150/146
strrchr                  4096    1024      25560/25577          174/176       147/145
memmove_overlap_back  4194304       1      22514/23310          166/207       135/112
strrchr               4194304       1      25665/25714          190/223       135/115
memchr                4194304       1      20524/20694          161/173       127/119
memset                   4096    1024       3560/3570            35/35         99/99
printf_stdout              16   10000     148617/150420        1653/1677       90/90
strlen                4194304       1      12675/12830          162/196        78/65
strstr                4194304       1      41860/41906          633/777        66/54
strcat                4194304       1      27692/28883          442/555        63/52
strcpy                4194304       1      21123/21168          340/480        62/44
strstr                   4096    1024      41849/41921          679/686        62/61
puts_stdout                16   10000      24202/24555          446/490        54/50
putchar_stdout          40000       1      48711/49054          923/963        53/51
memcpy_misalign       4194304       1      20875/21068          396/581        53/36
calloc_free              4096   20000      67098/67559         1285/1329       52/51
memcpy                   4096    1024       2844/2849            58/58         49/49
strncmp               4194304       1      31783/31913          712/796        45/40
memcmp                4194304       1      29962/30224          700/718        43/42
calloc_free           4194304      20     121883/122791        3253/3346       37/37
realloc_double           4096   20000     121813/123554        3274/4994       37/25
strcmp                4194304       1      25055/25351          720/752        35/34
fputs_stdout               16   10000      12331/12434          382/436        32/29
memcmp                     16  262144      29211/29354          908/915        32/32
strchr                     16  262144      28003/28044          910/959        31/29
strrchr                    16  262144      28694/28714          937/954        31/30
memcpy_misalign            16  262144      27602/27744          961/982        29/28
memchr                     16  262144      22627/22816          866/883        26/26
strlen                     16  262144      16732/16753          657/665        25/25
strncmp                    16  262144      30831/30860         1239/1260       25/24
strstr                     16  262144      46259/46303         1918/1925       24/24
memmove_overlap_back       16  262144      32981/33878         1409/1414       23/24
fgets_file            4194304       1     848425/857565       36625/37097      23/23 (注3)
strcmp                     16  262144      24847/25154         1082/1162       23/22
floor                       8  200000      11974/12079          563/569        21/21
memset                4194304       1       3618/3646           173/175        21/21
pow                         8  200000     105950/106163        5396/5409       20/20
fgetc_file            4194304       1     808380/816392       46432/47853      17/17 (注3)
fputc_file            4194304       1     350764/358328       24338/26425      14/14
log                         8  200000      23500/23513         1740/1749       14/13
exp                         8  200000      27118/27421         2093/2096       13/13
strtol_x3                  16  100000      87079/87863         6728/6783       13/13
sin                         8  200000      34442/34458         3004/3023       11/11
fread_big_file        4194304       1     382391/389056       34584/35803      11/11 (注3)
fread16_file          4194304       1     426674/433718       39764/40151      11/11 (注3)
fprintf_file               16  100000     174786/176413       16417/16746      11/11
strcpy                     16  262144      27735/27785         2673/2699       10/10
memcpy                4194304       1       2961/3061           352/405       8.4/7.6
memcpy                     16  262144       7523/7605           948/962       7.9/7.9
snprintf_ld                16  200000     108749/109631       14624/14906     7.4/7.4
snprintf_mix               64  100000     172993/177272       23673/23930     7.3/7.4
snprintf_s                 16  100000      44583/45147         6683/6808      6.7/6.6
strcat                     16  262144      35073/35087         5438/5456      6.5/6.4
sqrt                        8  200000       3216/3225           563/569       5.7/5.7
fwrite16_file         4194304       1      54647/56508        12433/12613     4.4/4.5
memset                     16  262144       8315/8347          1908/1925      4.4/4.3
malloc_free                16  200000       8462/8647          2903/2918      2.9/3.0
calloc_free                16  200000      15133/15180         5264/5642      2.9/2.7
bsearch                  4096  100000      33952/34049        11948/12019     2.8/2.8
bsearch               4194304  100000      95095/96194        33990/34598     2.8/2.8
atoi_x2                    16  100000      11450/11543         4113/4198      2.8/2.8
malloc_batch1024         4096   20480       2158/2191           808/878       2.7/2.5
strtod_x3                  16  100000      59445/59675        24821/24968     2.4/2.4
malloc_free              4096  200000      15918/17134         6723/6781      2.4/2.5
malloc_batch1024           16  204800       9507/9741          4190/4289      2.3/2.3
fwrite_big_file       4194304       1       5105/6308          5275/5693      1.0/1.1
```

- 注1:glibc 对大块 realloc 走 mremap 且本例不触页,1 us 是不拷贝的结果;unisacc 每次倍增都新映射加按字拷(stdlib.h:130-146)。比值不代表典型程序,不进前五。
- 注2:大于 65536 的块每次 malloc 一次映射、free 一次解除映射(stdlib.h:61-68、98),每对约 8.5 us;glibc 有动态阈值复用。未做原型。
- 注3:这四行的读测试把造 4 MiB 输入(逐字节 fputc,约 350 ms)计进了每轮,两档同样计入,所以比值偏向 fputc 的 14 倍。纯读的数以前五项第 (2) 项的原型表为准(fgetc 现状 423 ms、fgets 473 ms)。
- 分配器结论:小块是 2 的幂分箱加单链空闲表,O(1),无逐次扫描(stdlib.h:57-90);比值 2.3 至 2.9 落在代码生成档,不是热点。calloc 4096 的 52 倍来自逐字清零(stdlib.h:124-126)加对新映射块的重复清零。
- 落在代码生成档(比值约 2 至 8,改 include/ 收益有限):malloc/free 小块、bsearch、atoi、strtod、对齐 memcpy、小块 memset、snprintf 数字。
- 已知不重测:qsort(SPEED-10 在做)、printf 大宽度零填充(SPEED-9,`%100000000d` 1127 ms,逐字节 89 MB/s,可做算术计数路径)。
- 数学:floor 21 倍(math.h:135)、pow 20 倍(math.h:793)、log/exp/sin 11 至 14 倍;glibc 档是查表加 FMA,未做原型,未定位到具体慢因。

### I/O 每字符系统调用数([实测],strace -c -e trace=read,write)

| 路径 | unisacc write 次数 | gcc-O0 加 glibc | 说明 |
|---|---|---|---|
| bench 第 5 组(标准输出 944450 字节:putchar 200000、printf 594450、puts 5 万行、fputs 5 万行) | 944452 | 611 | 逐项分解恰好相加:putchar 每字符 1 次;printf 每输出字节 1 次;puts 每行 2 次;fputs 每次调用 1 次 |
| bench 第 4 组(fopen 的文件,读写各 4 MiB 级) | write 32302,read 15385 | write 32302,read 15382 | 文件流有 4096 缓冲,系统调用数与 glibc 相同;fgetc/fgets 慢不是系统调用,是每字符的调用链 |
| proto 现状(printf 594450 字节加 putchar 200000) | 794452 | 未测 | |
| proto 原型(同样输出,4096 用户态缓冲) | 202 | 未测 | 输出 794450 字节逐字节一致 |

### 前五项(原型 `$CLAUDE_JOB_DIR/tmp/s11/proto.c`,old 走运行库、new 走原型,同用 unisacc 编译;5 次原值在 pt.old/pt.new)

一致性证明:`cmp pr.old pr.new`(校验和文件)与 `cmp po.old po.new`(标准输出 794450 字节)均一致,sha256 前缀 640eec45c1d12c65 与 41e3f79fa9d6388b;gcc 编译的 old/new 两档结果也与之一致(四方相同)。

(1) 标准流无缓冲,printf 每字节一次 write。位置:include/stdio.h:95(`_u_st_wput` 对 fd<=2 直通 `_u_st_raw`)、stdio.h:668-677(`_u_put` 每字符调一次 `_u_st_wput`)、stdio.h:148(puts 的换行单独一次 write)。原因:系统调用次数,不是算法。原型:4096 字节用户态缓冲,满或结束时一次 write。
- printf 1 万行:前 151456 151967 155238 158093 154475 us(最小 151456 中位 154475),后 8868 9061 8877 8822 8804 us(最小 8804 中位 8868),17.2 倍。
- putchar 4 万字符:前 49714 50368 50766 49685 51154 us(最小 49685),后 1207 1212 1231 1202 1209 us(最小 1202),41.3 倍。
- 落地须保留的合同(原型未覆盖):stderr 不缓冲;终端上按行冲洗;exit/abort/fork/exec 与读 stdin 之前冲洗;与直接 write(1) 混用的顺序。tools/ 下大量 .cx 用 printf 输出,受益面最大,风险也最大。

(2) fgetc/fgets 每字符走完整 fread。位置:stdio.h:357-379(fgetc 先 `_u_st_slot` 线性找槽再 `fread(&c,1,1,f)`)、stdio.h:242-277(fread 内再找一次槽、再调 `_u_st_copy` 拷 1 字节)、stdio.h:460-475(fgets 逐字符调 fgetc)。原因:每字符固定开销约 100 ns,不是系统调用。原型:fgetc 缓冲命中时直接取字节;fgets 在缓冲内用 memchr 找换行后整段拷。
- fgetc 4 MiB:前 424739 465912 435407 424911 423419 us(最小 423419),后 40673 40768 40760 45656 40778 us(最小 40673),10.4 倍。
- fgets 4 MiB(40 字节行,缓冲 256):前 491864 483720 484479 475950 473042 us(最小 473042),后 26718 26693 26771 26398 26275 us(最小 26275),18.0 倍。缓冲 17(每行截成三段):前最小 482720,后最小 54988,8.8 倍。
- 原型用独立的读结构体,未接 ungetc、eof/err 标志与读写切换;落地时这些留在慢路径。

(3) 扫描族逐字节:strlen(string.h:12-17)、strchr(70-78)、memchr(199-208)、memcmp(149-157)、strcmp(48-53);strrchr、strncmp、strstr 同形未做原型。原因:逐字节循环。原型:对齐后一次 8 字节,用减一与取反找零字节;strcmp 只在两指针同余时按字读(否则读整字可能越过终止符所在页)。4096 规模最小值前后:
- strlen 13185 到 2695 us,4.9 倍;memchr 20140 到 4241,4.7 倍;strchr 50476 到 8239,6.1 倍;memcmp 43216 到 6414,6.7 倍;strcmp 44327 到 7748,5.7 倍。
- 16 字节规模只有 1.4 至 3.0 倍(strlen 17948 到 5888;memchr 19701 到 14283)。4 MiB 规模 4.0 至 6.0 倍。

(4) 拷贝族的逐字节退路:memcpy 在源与目的不同余时全程逐字节(string.h:127-131 的条件)、memmove 反向全程逐字节(string.h:143-144)、strcpy 逐字节(string.h:21-27)、`_u_st_copy` 逐字节(sys/_exit.h:46-49,所有文件流读写都经它)。原型:目的对齐后源按非对齐字读,每轮 32 字节;反向按字;strcpy 等于 strlen 加 memcpy。
- memcpy 不同余 4096:前 16554 us,后 2220 us,7.5 倍;4 MiB:21325 到 2264,9.4 倍。
- memmove 反向重叠 4096:22122 到 3467,6.4 倍;4 MiB:26677 到 7542,3.5 倍。
- strcpy 4096:20954 到 4810,4.4 倍。对齐 memcpy 4096:3882 到 2138,1.8 倍(只是 32 字节展开)。
- 非对齐字读在 x86-64 与 arm64 上合法;六目标是否都允许未核,落地前要逐目标确认。

(5) snprintf 的 `%s` 每字符一次 `_u_put`。位置:stdio.h:1028-1033 取串与长度,输出循环经 stdio.h:668-677 逐字符判上限并写。原因:逐字节加每字节函数调用。原型:按上限截断后整段 memcpy。
- 4096 字节串 1000 次:运行库 snprintf 前 80299 80797 80828 80819 81334 us(最小 80299),原型后 4821 4852 4866 4841 4826 us(最小 4821),16.7 倍。
- 只隔离内层循环(原样抄的 `_u_put` 缓冲分支对整段拷,含截断到一半的情形):4096 为 68921 到 4849,14.2 倍;4 MiB 为 136736 到 9019,15.2 倍。
- 原型只覆盖 `[%s]` 一种格式,不是 `_u_vfmt` 的替换件;宽度、精度与流输出分支未做。

原型之后对 glibc 的剩余差距([实测] 原型档对 gcc-O0 的 old 档):fgetc 2.1 倍,fgets 4.8 至 5.2 倍,16 字节的 strchr/strcmp/memcmp/memcpy 仍有 10 至 18 倍(小规模由调用与代码生成主导),4 MiB memcmp 约 10 倍、strcmp 约 11 倍(glibc 是 SIMD)。这部分归代码生成与向量化,改 include/ 拿不到。

未做:入库补丁(三方同写 cx-flat.h,按派工只留仓外原型)、strrchr/strncmp/strstr/strcat 原型、大块 malloc 复用与 calloc 免清零原型、数学函数定因、六目标验证。上表读文件四行的口径问题见注3,未重跑改正。

## 编译相微重构(SPEED-12,2026-11-13,已装 unisacc.com sha e43ef36832939c7f,HEAD 3e34166ffe)

状态:只交实测与定因,微重构补丁未写,字节一致证明未做,不声称变快。15 分钟窗口内未做私有重建。

(1) 七相冷编译(`UNISACC_RR_TIME=1 sh ./unisacc.com tools/ratchets.cx -o`,全新 TMPDIR,4 次,load 3.1 至 3.9,
四次镜像 sha256 前 16 位同为 48487408a68bb0e2;用 `tools/cx-phase.cx --parse` 汇总):

| 相 | 调用次数 | 最小 ms | 中位 ms | 解压装载 ms |
|---|---|---|---|---|
| e2 | 10 | 2393 | 2484 | 94 至 104 |
| e3 | 1 | 2363 | 2410 | 38 至 39 |
| lower | 1 | 809 | 815 | 14 至 19 |
| elf | 1 | 480 | 486 | 7 至 9 |
| units | 1 | 351 | 362 | 4 至 5 |
| prune | 1 | 320 | 339 | 3 至 4 |
| e1 | 10 | 319 | 326 | 85 至 91 |
| 合计 | 25 | 7051 | 7303 | 249 至 266 |

(2) 函数级分解:UNMEASURED,且按现结构在 cx 层量不出来。读码事实:七相全部是同一条调用链
`src/stages.cx st_span -> cx/runtime/runexec.cx ex_ri_exec -> src/executor.cx ex_exec -> core_run`
(`exec/c/asm/run_x86_64.S` 268 行,加冻结的 `cx/core/execute.cx`),每相一次 core_run 解释该相的网;
lower/elf/units/prune 不是 cx 或 C 函数,相内没有可打点的 cx 函数。`UNISACC_PROFILE=1` 实测只出
`PROFILE 相 -1 ns`(步数传 -1),没有步数。所以「每相前三函数」这张表交不出,要得到相内分解必须在
core_run 里加按网节点或按 op 的计数(改冻结的 cx/core 与汇编内核,需重建)。

(3) 空白处理的线性度与常数(实测,`.c` 文件 = N 字节空格 + 一行 main,`UNISACC_RUN_CACHE=0`,各 3 次取最小 ms):

| N 空格字节 | e2 | e1 | e3 | prune | lower | elf |
|---|---|---|---|---|---|---|
| 0 | 2.0 | 0.18 | 2.9 | 1.56 | 2.25 | 2.10 |
| 100,000 | 32.0 | 7.6 | 18.3 | 1.55 | 2.27 | 2.20 |
| 200,000 | 67.2 | 13.9 | 30.5 | 1.54 | 2.37 | 2.03 |
| 400,000 | 126.3 | 28.3 | 59.1 | 1.58 | 2.43 | 2.32 |
| 400,000 换行 | 153.6 | 28.3 | 62.4 | 1.61 | 2.61 | 2.32 |

- 实测结论:三相对空白都是线性的,没有 O(n^2);但常数很大:每空白字节 e2 约 311 ns、e1 约 70 ns、
  e3 约 140 ns,合计约 520 ns(换行约 610 ns)。e3 也随空白字节数涨,说明空白没有在 e1 被吃掉,一路带到 parse。
- [推算,未实测] 每个 `.cx` 单元前导的 e2 输出 321,789 B 中约 98% 是等长空白,按上面常数约 160 ms/单元,
  10 单元约 1.6 s,占七相合计的两成多;与 SPEED-1C 实测二(201.2 -> 27.6 ms)方向一致。
- prune/lower/elf 不随空白变,它们的 809/480/351/320 ms 由整程序规模决定,相内原因 UNMEASURED。

(4) 候选改动(都改产品输入,须重建;均未写补丁、未验证):

| 热点 | 改哪里 | 依据 | 状态 |
|---|---|---|---|
| e2 对被裁函数体出等长空白 | E2 网的事实表(`exec/facts/pp-gen.tsv`、`exec/facts/pp-autoinc-gen.tsv`,裁剪体在 `exec/pp/ftrim-libc-body-*`)经生成器重出,被裁区只留换行 | 上表 e2 311 ns/B | 具体行未定位,UNVERIFIED |
| e1 对连续空白逐字节走网 | `exec/facts/lex-gen.tsv` 的空白类转移,连续空白一步跳过 | 上表 e1 70 ns/B、e3 140 ns/B | 未定位,UNVERIFIED |
| 相内无剖析 | core_run 加按 op 计数(冻结域) | (2) | 需项目所有者解冻 cx/core 才能做 |

落地步骤(交集成者排候选):改事实表 -> `tools/facts-gen.cx --check` -> 正规构造链重建候选 -> 隐式前导路径上对
10 个以上 `tools/*.cx` 对拍镜像 sha256(行号与诊断位置必须不变,只留换行即为此)-> 前后各 5 次冷七相。
测量材料在作业临时目录 `tmp/s12/`(r1..r4.err、lin/),未入库。

### 12b 等长空白的落点、被拒定因与预压对拍(2026-11-13,已装 e43ef36832939c7f,HEAD 8805d2f54e,load1 0.8 至 2.0)

状态:落点已定位,补丁已出(未入库),预压路 12 单元镜像字节一致,e1/e3 收益已实测;私有重建未做,
补丁后的 e2 相与隐式前导路径上的前后对比:UNVERIFIED。不声称产品已变快。

(1) 落点(读码加实测):等长空白不出自 `exec/facts/pp-gen.tsv` 或 `lex-gen.tsv`(那两张是类表与词表生成物),
出自 E2 网的声明源 `exec/pp/directive-scan-byte.tsv` 第 12 行 `P3BLANK * P3BLANK [["OUT",32],["ADV"]]`:
指令行(define/include/undef/pragma push 等,先 `JUMP LS` 回行首)与条件跳过区的每个字节出一个空格,
第 10 行遇 LF 才 `COPYT` 并 `LINES+1`。P3BLANK 只处理整行,所以被填充的行上没有任何留存记号;
删去 `["OUT",32]` 后行号(LF 计数)不变,留存记号的列号也不变(它们不与填充同行)。
实测 `-E`:`#if 0`、被跳过行、`#endif`、`#define M 1` 各变成与原行等长的空格行。

(2) SPEED-1C 实测二被拒的定因(复现):`-include min.E x.c` 被拒 `multiple definitions of this function`,
原因是 `min.E` 里已含 min.cx 的 `main`,x.c 又定义 `main`;换成不含 main 的 y.c 则 rc=0。另一种拒法
`redefinition of typedef in the same scope` 出在不带 `-nostdinc` 把 `-E` 输出当 `.c` 重编(自动头重复)。
两者都是语义重复,与空白、行号、列号无关;带空白与预压两版被拒文案相同。

(3) 预压对拍(`tools/NAME.cx` 直接编;`-E` 输出原样作 A.c;A.c 中纯空格行压成空行作 B.c;A/B 均 `-nostdinc`):

| 单元 | 直接 | A 原样 | B 预压 | A 字节 -> B 字节 |
|---|---|---|---|---|
| register-fixture | 349ef150c635f985 | 同 | 同 | 396,634 -> 39,051 |
| ci-verdict | e3a26330551c215a | 同 | 同 | 505,336 -> 57,757 |
| release-body-diff | fdfb51eadb8b4fa5 | 同 | 同 | 504,040 -> 55,691 |
| post-publish-check | fc0290fbd3ca8eb7 | 同 | 同 | 461,841 -> 60,358 |
| check-ci-verdict | 7d7edecf9ae550d7 | 同 | 同 | 505,200 -> 56,105 |
| check-post-publish-check | 3e9c40afaf15b353 | 同 | 同 | 508,560 -> 59,490 |
| check-release-body-diff | 788a958d2ff56dcb | 同 | 同 | 505,618 -> 56,518 |
| pins-reanchor | 81a45ae10e1b1ab4 | 同 | 同 | 509,364 -> 61,025 |
| check-chain-verdict | f53c27f338550389 | 同 | 同 | 504,831 -> 55,880 |
| check-pins-reanchor | f51f31ea26f21b8a | 同 | 同 | 457,881 -> 56,074 |
| chain-verdict | d6d5129f66b8a27e | 同 | 同 | 505,775 -> 72,130 |
| safedel | a86cc53a9748878c | 同 | 同 | 494,609 -> 77,615 |

12/12 三路镜像 sha256 前 16 位相同;最小 `.cx` 另一例三路同为 bc35367054850363(387,124 -> 38,312 B,
11,032 行中 9,089 行是纯空格行)。未计入:cxproc(直接编即被拒 `no main`)、cx0-extract(A/B 同样
`link: undefined reference to fchmod`,直接编 605ecc94dce3e9b6,重编路不等价于产品路)。
范围限制:只测了单单元工具(多单元 `-E` 报 `multiple preprocessing outputs not migrated`);
只测 lnx/x86_64;诊断文案的位置未对拍。

(4) 耗时(ci-verdict,A.c 对 B.c,冷,全新 TMPDIR,`UNISACC_RUN_CACHE=0`,各 5 次,镜像十次同 sha,少 447,579 个空白字节):

| 相 | A 最小/中位 ms | B 最小/中位 ms | 差(最小) |
|---|---|---|---|
| e1 | 45.7/46.8 | 11.7/12.8 | 34.0 |
| e3 | 326.6/332.1 | 209.5/213.8 | 117.1 |
| e2(重喂文本走 P3COPY,不是补丁后的 P3BLANK 路) | 121.2/122.1 | 16.9/18.3 | 仅旁证 |
| 墙钟 | 972/1008 | 730/755 | 242 |

e1+e3 实测每单元省 151 ms(约 338 ns/空白字节);e2 在补丁后少做 44.8 万次 OUT,但仍要逐字节 ADV,
省多少 UNMEASURED。[推算] 10 单元仅 e1+e3 约 1.5 s。

(5) 补丁(作业临时目录 `tmp/s12b/speed12b.patch`,`git apply --check -p2` 通过,未入库):只改上述第 12 行为 `[["ADV"]]`。
`exec/facts/pp-gen.tsv` 的 `# input` 不含该文件,故 facts-gen 的表不需重生成;E2 网权重与 `kernel/` 须经正规构造链重出。
未做及原因:私有重建要 `net-incremental-rehearsal`(改 pp 一处重建 3 网,构造约 12 s)再经 `test-loader --net e2=ABS --swap-tool`
绑定 5 个 `TEST_LOADER_*` 路径,15 分钟窗口内未搭起。下一步拆三相:(a) 演练副本内套补丁重建 e2 网;
(b) test-loader 单网替换,对上表 12 单元加 10 个多单元工具对拍镜像 sha,并对拍一批拒绝用例的诊断行列;
(c) 前后各 5 次冷七相。风险:依赖 `-E` 文本逐字节的金样(pp-differential、c99-whitespace、UNIPP2 位置封装)须同候选重对。
若 (b) 发现位置依赖,退路是 e1 对行首连续空白一步跳过(上限仅 e1 的 34 ms/单元,e3 的 117 ms 拿不到)。

### 12b-2 补丁在产品编译路径上的等价与耗时(2026-11-13,已装 e43ef36832939c7f,副本 = `git archive` 4ea4ab8fb0,load1 1.2 至 3.7)

状态:单网替换路上产物字节一致、诊断一致、七相变快,均为实测;这不是重建后的候选,六目标、正规构造链、
金样批次均未跑,不声称 QUALIFIED。补丁仍未入库。

做法(全部在作业临时目录 `tmp/s12b2/`,共享检出未动):
- 构造器按 `tools/cxlib/sourcepackage.cx job_command` 的正规 argv 自建:`gcc -std=c99 -O2 -w -I seed seed/{gen,tbl,net}.c`;
  每张网 gen 约 1.0 s、tbl 约 0.5 s、net 约 0.03 s,远低于 5 分钟。未用 `net-incremental-rehearsal`(它的 mutate 是固定场景,不收任意补丁)。
- 包内 D 行实测:编译路由的 e2 是模型 0(`pp.locations`,配方 `pp --locations --shared-predefines`),`-E` 路由是模型 3
  (`pp.text`,配方 `pp --shared-predefines`)。第一次只换了 `pp.text` 到模型 0,被拒 `malformed preprocessing locations`,属我配方选错,已改正。
- 基线组与补丁组各重建这两张网,用 `tools/lab-loader/p3-swap.cx` 按序号替换 0 与 3,补 UNIPKG1 尾,
  经 `UNISA_CONTAINER=PKG sh unisacc.com --models PKG ...` 运行(与 test-loader 的启动方式相同,切片换成整 APE)。
  每次全新 TMPDIR、`UNISACC_RUN_CACHE=0`。网 sha256 前 16 位:`pp.locations` 基线 0834448a3b393921 / 补丁 12df055ca5db837f;
  `pp.text` 基线 19864985e8a738fd / 补丁 a7a01695e8db6bc7。

(1) `-E` 输出(最小 `.cx`):基线组 387,124 B 与已装产品逐字节相同;补丁组 38,647 B;两者都是 11,032 行。

(2) 镜像对拍(真实编译,不经 `-E`):25/25 基线组与补丁组 sha256 前 16 位逐一相同。
单单元 13 个:register-fixture 349ef150c635f985、ci-verdict e3a26330551c215a、release-body-diff fdfb51eadb8b4fa5、
post-publish-check fc0290fbd3ca8eb7、check-ci-verdict 7d7edecf9ae550d7、check-post-publish-check 3e9c40afaf15b353、
check-release-body-diff 788a958d2ff56dcb、pins-reanchor 81a45ae10e1b1ab4、check-chain-verdict f53c27f338550389、
check-pins-reanchor 59e35ec1ea77f092(该文件工作区有他人未提交改动,故与 12b 表不同)、chain-verdict d6d5129f66b8a27e、
safedel a86cc53a9748878c、cx0-extract 605ecc94dce3e9b6。
多单元 12 个(括号内为 use 条数):ratchets(5) f1df15ef26618f21、ledger-add(9) 3900bdfed6198dc5、commit-own(8) f77857e1cf998f4f、
own-diff(5) 39c5782efa50b6dd、register(10) 6f847bd625206855、refactor(9) b44545561aa232f6、edit-span(3) 252cd0435db9e390、
net-digest(5) 7b0b569335f30c76、net-dump(4) 71f582fff50b4656、facts-gen(2) 6ee8c6a9a0fd96fe、cx-phase(9) a939fecc35af1ef1、
net-build-profile(5) ebab81cb99480f8b。最小 `.cx` 两组同为 bc35367054850363(也等于已装产品)。
未计入:test-loader 两组同样 rc=1(`sy_env exported by both sys.env and env`,stderr 逐字节相同,与补丁无关)。

(3) 诊断对拍:8 类拒绝用例各出 `.cx` 与 `.c` 两份共 16 例,错误行之前都放了 `#define`、`#if 0` 跳过区、同行注释加指令、`#ifdef` 跳过区。
完整 stdout+stderr+rc 在基线组、补丁组、已装产品三方逐字节相同,16/16:
语法错 `syn:10:25: error: this is not the start of an expression`;未声明 `undecl:12:15: error: unknown identifier`;
`#error` `err:11:3`;缺头 `inc:10:11: error: no such file for #include`;函数重定义、typedef 重定义、未闭合条件、未闭合字符串为不带位置的 reject。
没有发现位置依赖。

(4) 冷七相(`tools/ratchets.cx -o`,`UNISACC_RR_TIME=1`,两组交替各 5 次,十次镜像同 f1df15ef26618f21,每次 e2 推理 10 次即单元缓存未命中):

| 相 | 基线组 最小/中位 ms | 补丁组 最小/中位 ms | 最小值之差 |
|---|---|---|---|
| e2(10 次) | 2445/2457 | 1262/1270 | -1183 |
| e1(10 次) | 325/330 | 76/78 | -249 |
| units | 370/371 | 169/176 | -201 |
| e3 | 2390/2406 | 1617/1647 | -773 |
| prune | 329/347 | 333/340 | 持平 |
| lower | 803/817 | 799/842 | 持平 |
| elf | 471/475 | 471/488 | 持平 |
| 七相合计 | 7183/7205 | 4730/4859 | -2453(-34%) |
| 墙钟 | 7908/7932 | 5189/5313 | -2719(-34%) |

比 12b 的推算(仅 e1+e3 约 1.5 s)多,多出的是 e2 自身(少 OUT)与 units 相。

(5) 未做与原因:
- 金样批次 pp-differential、pp-negative(`tools/`,参数 `ROOT ABS_OUT HOST_CC`)、c99-whitespace(`tests/c99-whitespace.cx`,需 4 或 6 个参数)、
  unipp2-reference(`ROOT PP_RUN PP_NET NEW_ABS_OUT`):在副本里经包装入口起了两次,都停在 `usage` rc=2(包装把 `--models` 插在工具路径前,
  工具参数没有传到),按两次即停,未实跑,红绿未知。它们比对 `-E` 文本的,补丁后必然要重对或换金样。
- 正规构造链重建候选、`kernel/` 重出、六目标、s9:未做。只测 lnx/x86_64 的 image 路由。
- 重建的基线网与已装包内的不逐字节相同(包体 1,763,620 -> 1,764,343 B),但行为对拍相同;A/B 以同法重建的基线组为准。

回滚:补丁只有一行,还原 `exec/pp/directive-scan-byte.tsv` 第 12 行即可;本节未改任何入库产品输入。

## 架构层高 ROI 审视(SPEED-14)

条件:2026-11-13,已装 unisacc.com sha e43ef36832939c7f,HEAD 6ca621f388,load1 3.4 至 6.6(未因负载丢数据)。15 分钟窗口,只测未改;
材料在作业临时目录 `tmp/s14/`(r1..r10.err、c1..c10.err、one.strace、rat.strace、rat.out),未入库。凡未标 [实测] 的都是 [推算] 或 UNMEASURED。

### 六层质问表

| 层 | 被质问的假设 | 实测证据 | 候选 | ROI 排名 |
|---|---|---|---|---|
| (1) 缓存层级 | 「模型缓存命中后装载就免费」 | [实测] 最小 C 程序连续 10 进程(每次换源避开运行缓存,同一 TMPDIR):第 1 次 6 miss,第 2 至 10 次 6 hit/0 miss;但命中后 model 六相合计仍 46.6 至 50.1 ms(最小 46.6,中位 47.4),推理六相合计只 10.5 至 11.4 ms,墙钟 89 至 102 ms。即命中路上装载占墙钟约 52%,是推理的 4.3 倍。`UNISACC_RUN_CACHE=0` 时(回执全为 miss/inflate)model 67.6 至 71.6 ms、墙钟 108 至 114 ms,占 63%。缓存只省了 69 -> 47 ms。命中仍慢的原因 UNMEASURED:strace -r 显示无单个系统调用超过 13 ms,耗时散在匿名 mmap 之间的用户态,不是 AF_ALG 摘要也不是读盘 | A1 命中路装载定因后去掉(预解析形态直接 mmap) | 2 |
| (2) 进程模型 | 「每个检查一个进程,固定成本可忽略」 | [实测] `tools/ratchets.cx` 一轮(strace 下 31.2 s,13/0):execve 尝试 2522 次,其中 unisacc.com 494 次、成功的 git 251 次(同一条 `git ls-files -s -z -- *.cx` 重复 44 次,`git ls-files -z -- 路径` 192 次)、sh 698 次。启动下界 [实测]:运行缓存命中的重复启动墙钟 9 至 12 ms(SPEED-6);每批两次产品摘要已由 SPEED-7 降到约 73 ms(productpkg.cx:175 已入库) | B1 棘轮内同一 git 查询只做一次并经环境或文件传给子检查 | 5 |
| (3) 数据结构 | 「选批是廉价的查表」 | [实测] `affected --files F --list-only` 改一个文件:提交档 926 至 956 ms,默认档 1929 至 1970 ms(4 个文件各 1 次),与命中 0 批还是 397 批无关(tools/dag.cx 0 批也要 951/1934 ms),每次 scanned=1049 全量重算 use 闭包与字面量,无持久索引。静态:引用 tests/batches.tsv 的工具 61 个、coverage.tsv 34 个;一轮内同一 TSV 被重复打开的次数 UNMEASURED(本次只追了 openat,产品用 open) | C1 闭包索引按内容键落缓存,未变文件不重扫 | 1 |
| (4) 依赖闭包粒度 | 「提交档选出的批太多是因为闭包过粗」 | [实测] 一个文件的改动:cx/sys/sha.cx 提交档 120 批(use 67、literal-use 51、literal 1、arg-use 1),默认档 397 批(use 248、literal-use 141);tools/ledger-add.cx 8/42 批;tests/run-batch.cx 11/34 批;tools/dag.cx 0 批。提交档 56% 是真编译依赖(use),43% 是「检查起了一个用到它的工具」;没有证据说闭包过粗。kit 全局键:UNMEASURED(本次未测;单元增量 7.1 s 见 KIT-UNIT-INCR) | 不立项 | 否定 |
| (5) 代码生成 | 「每条表达式落栈是必须的」 | [实测,SPEED-8 静态计数,本次未重测] swap 内层每字节 95 条(75 op + 20 .frame)里求值栈压入/弹出各 10、连 .frame 共 40 条,槽地址 imm+sub64 28 条;gcc-O0 22 条。同算法代码生成倍数 2.19 | E1 只对求值栈槽做基本块内 store->load 转发(求值栈槽由编译器产生、不被取址) | 3 |
| (6) 运行库 | 「libc 函数慢是代码生成的错」 | [实测,SPEED-11] 标准流无缓冲 17.2 倍、扫描族 4.7 至 6.7 倍、memcpy 不同余 7.5 倍已有原型。本次读码定因:大块 malloc 258 倍 = 大于 65536 的块每次 malloc 一次 mmap、free 一次 munmap(stdlib.h:61-68、98),无任何复用;calloc 4096 的 52 倍 = 对新映射的零页再逐字清零(stdlib.h:124-126);floor 21 倍 = 一次 floor 调 trunc 再比较再判零(math.h:135-141),三层调用乘 2.19 的代码生成档,glibc 是一条 roundsd;pow 20 倍 = fdlibm 式约 30 个局部 double 全部落栈(math.h:793 起),属代码生成档不是算法。软件 SHA:`cx_sha256_hex` 仍被 300 个工具文件 678 处调用(cx-flat.h:8363),速率约 9 MB/s 对内核路 180 MB/s(SPEED-7 实测 734 对 38 ms);每轮实际摘要字节数 UNMEASURED | F1 大块缓存一档空闲表(F2 cx_sha256_hex 对 >=65536 走内核路加回落) | 4 |

### 被否定的方向(附数据)

- e3 增量:最小程序 e3 推理只 2.9 ms,10 单元工具 2363 ms 是整程序一次 core_run 且无中间状态;同一轮里可免的装载 47 ms/进程 x 494 次调用更便宜、无需改 parse 网。
- 全局寄存器分配:REGALLOC 严格直通只折 2.9% 负载且原型写出过错误 mov;求值栈往返占静态指令 40/95,局部规则先拿。
- 再细化 affected 闭包:提交档 56% 为真 use 依赖,剩余是工具链式依赖,砍它等于缩覆盖。
- 数学函数重写:floor/pow 的倍数落在代码生成档与调用层数,换算法拿不回。
- 模型缓存改键或加大预算:第 2 至 10 次已 6/6 命中,问题在命中之后的 47 ms。
- 常驻服务:重复启动下界 9 至 12 ms 已够低,固定成本在选批(0.95 至 1.95 s)与装载,不在进程创建。

### 前三项五段简报

(1) SPEED-14 affected 选批索引落缓存(快车道)
- 目标:每个根源文件的 use 闭包与路径字面量按该文件内容 sha 存入缓存目录,未变文件不重扫;选批结果与现状逐行相同。
- 写域:tools/affected.cx;tools/cxlib/ 下新增一个索引单元;tools/check-affected-index.cx;tests/batches.tsv 自己的一行。
- 验收:`sh ./unisacc.com tools/affected.cx -- --files cx/sys/sha.cx --tier commit --list-only` 热索引 5 次最小 <=300 ms(基线 949)且 batches=120;默认档 <=600 ms(基线 1970)且 batches=397;对 20 个随机文件新旧 AFFECTED 行逐字节相同;改一个被 use 的文件后索引失效的负例通过。
- 截止:原型加对拍 15 分钟,到点交部分结果;入库另计。
- 禁区:不缩选批集合,不按 mtime 判新鲜,不删断言,不放宽时限,不提交他人路径。

(2) SPEED-15 模型缓存命中路的 47 ms 定因(先快车道测量,改动属慢车道)
- 目标:把命中后 `model stage=X` 的 46.6 ms 拆成读盘、摘要、锁与 touch、其余用户态四段;若其余段占一半以上,给出预解析形态直接 mmap 的补丁草案交集成者。
- 写域:仓外原型与测量;补丁目标 cx/runtime/modelcache.cx、cx/runtime/routestep.cx、src/stages.cx(只出补丁,不直接改)。
- 验收:四段之和与 46.6 ms 相差 15% 内,否则记 UNVERIFIED;补丁重建后最小 C 程序命中路 model 合计 <=15 ms、墙钟最小 <=60 ms(基线 89),命中与未命中产物逐字节相同。
- 截止:定因 15 分钟;重建按慢车道。
- 禁区:不去掉载荷校验,不让缓存失败变成编译失败,不改 kernel/,不把推算写成实测。
- 实测(2026-11-13,最小 C `int main(void){return 0;}`,已装 unisacc.com,UNISACC_RR_TIME=1,连续 10 次,取第 2 到 10 次;负载 uptime 2.5):墙钟 119-128 ms;每次 15 行 RR_TIME,恒为 `lookup_hit=0`、`model source=inflate`,10 次 stderr 零条 `modelcache` 行;模型各相 ns(最小/中位/最大,9 次):e3 34.27/34.95/36.99 ms,e2 14.25/14.63/16.27,e1 12.90/13.09/14.54,lower 11.14/11.35/11.55,elf 4.28/4.45/4.56,prune 1.82/1.88/1.99,六相合计约 80 ms;推理 e2 2.07、e3 2.63、lower 2.07、elf 1.98、prune 1.48、e1 0.19 ms;construction 3.7 ms。stage 目录中 /tmp/unisacc-e3cache-1001 只被 strace 见到 3 次 open(.budget.lock x2 与一个 .host),没有任何 mc-* 文件被打开。UNISACC_E3CACHE 指向空目录对本入口无效(目录仍是 /tmp/unisacc-e3cache-1001)。
- 读码定因:`src/stages.cx` 阶段装载(约 359-367 行)走 `rt_open(...ex_inflate, ex_crc32...)` + `sh_apply`,`st_stamp(stage,-1,...)` 把 rc 写死为 -1,所以「source=inflate」是硬编码标签,不代表是否命中。`mc_get` 只在 `cx/runtime/routestep.cx:60` 与 `src/executor.cx:105` 出现,而这条入口实测不打印 modelcache 行,即 SPEED-14 说的「6 hit」来自另一路径或另一版本,本入口的约 80 ms(单次 47 ms 的数量级同源)是每个进程逐阶段真 inflate + CRC,未命中缓存,不是命中后的开销。UNVERIFIED:strace -c 一次出现约 3250 次 open/read/lstat(与缓存目录 1608 项同量级),第二次 strace 仅 26 次 open,未复现,待查是否为目录扫描(淘汰/预算)。
- 方案(只出文字,不入库):(1) 先让 st_stamp 区分 hit/inflate(把 mc 结果经 rt_open 返回出来),否则无法验收;(2) 确认 rt_open 在该入口是否调用 routestep 的 mc_get,若未调用则在 sh_apply 前对 (压缩成员 sha, 阶段) 做 mc_get,命中直接拿预解压字节,跳过 ex_inflate 与 CRC;(3) 命中的核验保持原性质:键为压缩成员 sha256(内容+适用身份),文件头存载荷 sha 并在读后复核(mc_read 已做),任何不符按 miss 回退 inflate,失败不致编译失败;(4) 预期 e3/e2/e1/lower 的 inflate 约 73 ms 降到读盘+sha 复核,验收以重建后最小 C 程序 model 合计与墙钟为准,未实测前不记数。

(3) SPEED-16 求值栈槽基本块内 store->load 转发(慢车道)
- 目标:只对编译器产生的求值栈槽,在同一基本块内把「存入后紧接读回」改为直接用源寄存器;用户变量槽、.frame 与 store 不动;默认关。第一步先证明求值栈槽在 tape 中可与用户槽区分且不被取址,证不出即停。
- 写域:src/stages.cx;cx/compiler/**。
- 验收:关时 8MB sort 与 sha 输出及镜像与现产品逐字节相同;开时输出逐字节相同,swap 内层静态条数 <=70(基线 95),同算法堆排序 5 次中位 <=3094 ms(基线 3867 的 80%);工具 tools/check-bench-gencode.cx。达不到就关着入库。
- 截止:可区分性证明 15 分钟;实现单步不超过 5 分钟。
- 禁区:不做跨块活跃性,不删 store,不吞 .frame,不改函数语义。

## 本地重建 0.0.60(LOCAL-REBUILD)

条件:2026-11-13,16 核,load1 1.4 至 4.2;构造器为已装 0.0.59(sha e43ef36832939c7f);输入为 main 941712d3d7 加 speed12b.patch。
性质:lnx-only dev 形态,NOT_RELEASE,非发布候选,非 QUALIFIED;只到 gen1(编译器自身仍由 0.0.59 编出),未做 g2/g3、其余五目标、compare、s9、金样批次、CI。
未安装:`install-ape` 干跑返回 `REFUSED no-UNIPKG1-footer`(dev 形态的启动头是纯脚本),ROOT/unisacc.com 未动;备份在 ROOT/unisacc.com.059(不入库)。

做法(产物全部在 /tmp/unisacc-session-build-lr0060,仓内未建 out/):
- `git archive` 解出 HEAD,副本内套 E2 补丁,`git init` 后 fetch main 与标签 v0.0.59-src2,用 `commit-tree` 建私有提交 1e4aa2dd07
  (父为 fbdc99ca89 与 941712d3d7)。原因:构造器的构建提交 2a46e1a427 在构造源支线上、不在 main 上,而预检规则 6
  (tools/preflight.cx:155-168)要求它是 rev 的祖先;单设 BOOTSTRAP_FROM 过不了。
- 链输出目录放在副本之外时,被钉工具 kernel-route.pin.cx 找不到 `cxlib`(上级没有 cxroot.tsv,CX-U-7);在会话根补一份指向冻结快照的 cxroot.tsv 后通过。
- E2 表的改动由正规链的 jobs-gen/tbl/net 重出网、final-net 合并、p3 装包,不需要单独的网工具。
- `chain-run` 不收 `--until`,所以直接调 `candidate-chain --until`;链自带的 pack 要先过 compare,走不到,改用 `release-ape.cx dev-lnx`。

实测耗时(candidate-chain,同一输出目录续跑):

| 批 | 范围 | 并行 | 墙钟 s | 记录值(phase-times.tsv 行) |
|---|---|---|---|---|
| b1 | snap 32.6、build-objects 53.8、build-front 20.7、build 37.7 | - | 162 | 48.9/20.0/35.4(195/193/194) |
| b2 | wave-objects 至 final,315 个相位 | 4 | 364 | snap 至 p3 共 2508.9(196,宽度 1) |
| b2 续 | routes 与 p3 各子相 | 4 | 334 | p3 域 1065.3(203) |
| b3 | gen1-lnx-x86_64-objects 49.9、链接 24.7 | 12 | 79 | 88.0(198) |

四批合计 939 秒(含一次 routes 失败后的续跑复核);打包未单独计时。

新旧对拍(各 3 次,新旧交替;数据行在 build/speed-baseline.tsv,note 以 LOCAL-REBUILD 开头):

| 项 | 已装 0.0.59 最小/中位 | 本地 0060 最小/中位 | 变化(中位) |
|---|---|---|---|
| run-startup-bench first_e2e ms | 795.5/815.2 | 670.0/705.6 | -13.4% |
| run-startup-bench repeat_median3 ms | 27.1/28.4 | 28.6/29.6 | 约 +1 ms |
| ratchets.cx -o 冷编译墙钟 ms | 7835/7881 | 5352/5387 | -31.6% |
| s8.c lib int 8 排序 ms | 4052/4105 | 1084/1089 | 3.77 倍 |
| s11 bench.c 第 5 组 fd 1 write 次数 | 944452 | 615 | 1 次 |

排序与输出两项 stdout 新旧逐字节相同(md5)。ratchets 镜像新旧不同(298f88e1463f06a9 对 f67475ce4a16a785),未逐项定因。
repeat 路径慢约 1 ms 未定因。B 路线(只换两张 E2 网的包,`routeB/unisacc-local-b.sh`)也可运行,`-E` 输出 38,647 B,其余基准未跑。

回滚:删除 /tmp/unisacc-session-build-lr0060;已装产品未变。
