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

## 补测工具(已写,纪元后用)

`tools/speed-probe.cx`(批次 `speed-probe`,check 6/6 PASS):输入 `JOBS.tsv`(每行 `NAME<TAB>ARGV...`,
经 `sh ./unisacc.com <ARGV...>` 跑,`UNISACC_RR_TIME=1`,stderr 收 receipt),输出到 stdout 的
`per-object` TSV(name/total_ms/lower_ms/elf_ms/tapecheck_ms/rc)与 `top10` 排序。零 shell、零 libc,
只走 `sy_spawn_pipe`/`sy_read`/`cx_read`/`tx_*`。
67 个对象的 `JOBS.tsv` 由调用方按同一 recipe 生成(纪元后跑):
`for o in out/c44-five-lnx/*.o; do ...; done` → 再 `sh ./unisacc.com tools/speed-probe.cx -- JOBS.tsv out/speed`。
