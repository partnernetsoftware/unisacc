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
