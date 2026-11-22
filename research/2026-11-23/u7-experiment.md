# U7 实验:4200 行同内容 1 单元对 9 模块的冷编对照,与 e4 单元级同字节率

2026-11-23,omp-go 实测。结论已并入 `prd.md` 的 U7 条(实验结论与修订)。本目录只收脚本、生成方法与 RR_TIME 日志摘要;汇编大文件 `one.s`(854 KB)、`root.s`(1007 KB)不入库,按下文方法重生成。

## 1. 夹具生成

`u7-exp/gen.sh` 生成 4200 行同内容的两版:

- `one.cx`:4200 个 `static int fN(int a){...}` 加 `main`,单单元。
- `u0.cx`..`u8.cx`:每个文件 `#pragma unisa module uM` 加 466 个同形函数,加一个 `exM()` 导出;`root.cx` 9 条 `use` 加 `main`。

两版函数体逐字节相同,只有单元划分不同。

```
sh gen.sh && sh ./unisacc.com -S -o one.s one.cx && sh ./unisacc.com -S -o root.s root.cx
```

## 2. 冷编耗时(独立空 `UNISACC_CACHE_DIR`,单次)

| 项 | 单文件 one.cx | 9 模块 root.cx |
|---|---|---|
| 墙钟 | 8.7 s | 15.6 s |
| e3 | 8.16 s / 1.25e9 步 | 8.25 s / 1.29e9 步 |
| e4 | 1.09 s | (同量级) |
| lower | 1.91 s | 1.95 s |
| elf | 1.11 s | 1.11 s |
| units 阶段 | 无 | 1.43 s |
| unitcache | 单单元不走单元缓存(0 行) | 10 miss + 10 store |

关键数字:e3 步数 1.25e9 -> 1.29e9,只增 3%;e3 时间不随单元数下降。温缓存二次编译 11 行全 hit、0 miss,但 e3 仍 1.21 s / 4046 步、lower 466 ms、elf 269 ms —— unitcache 只覆盖 e2/e1,后端四相每次整闭包重跑。

`q1.log` 是 one.cx 的完整 RR_TIME,`q2.log` 是 root.cx 的。

## 3. e4 单元级同字节率

`cmpfn.cx A.s B.s` 按顶层标签切函数,只留用户函数(`f<digits>`/`ex<digits>`),去掉模块后缀 `__u<N>`,给两个口径:

- `RAW`:函数体原始字节的 sha256。
- `FLAT`:先把 `.unit` 身份行删掉,再把函数内 `__unisacc_L/R/B<digits>` 局部标号按出现顺序重编,再取 sha256。

实测(`one.s` 对 `root.s`):

```
FUNCS_A 210
FUNCS_B 216
RAW  same=0   diff=207 rate=0.00%
FLAT same=207 diff=0   rate=100.00%
ONLY_A 3
ONLY_B 9
```

原始口径 0% 的原因全在 `.unit` 行与全局计数的局部标号;抹掉这两项后 207/207 逐字节相同。ONLY_A/ONLY_B 是 main 与 9 个 `exM()` 这类只在单侧存在的函数。

`cmpdiff.cx A.s B.s` 打印前 4 个 FLAT 仍不同的函数两侧函数体,用于定位新差异。

## 4. 对 U7 的含义

- 后端按单元复用的收益上限是 e2/e1(每单元约 100 ms 级),e3(占冷 -o 44%)/e4/lower/elf 是整闭包单线程重跑,按单元复用对它们无效。
- 冷 -o 本身要进 15 秒必须靠 e3 增量或并行;按单元复用的前置是「标号函数局部化」,否则 RAW 口径 0% 会让缓存键永远 miss。
- 4200 行规模下,标号局部化后 FLAT 口径已是 100%,说明函数体生成与单元划分无关,只差标号编号。

## 5. 复现注意

- `cmpfn.cx`/`cmpdiff.cx` 依赖仓根 `cxroot.tsv` 与 `cx/`、`ext/`;在仓根跑 `sh ./unisacc.com research/2026-11-23/u7-exp/cmpfn.cx -- A.s B.s`。
- cx 单单元上限:`CXM_SYMS 2048`(`cx/compiler/cxmod.cx:38`)、`CXM_LOCS 4096`(`:118`);466 函数/单元已撞 `too many local declarations for the call check`,实际每单元不宜超过约 300 个函数。
