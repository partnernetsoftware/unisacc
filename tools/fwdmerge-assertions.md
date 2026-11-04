# fwm_merge 断言链打点表(D 方案,2026-11-04)

在 out/fwm-probe 的 fwdmerge 副本里给每条 goto done 前插入 stage 序号,
逐条定位"哪条断言在什么输入下失败"。**未改源、未进仓。**

| stage | 断言(cx/compiler/fwdmerge.cx) | 失败含义 | 构造合法输入的要点 |
|---|---|---|---|
| 1 | `na<0||nb<0||nu<0||!fwm_nexports||fwm_overflow` | 磁带没以换行结尾 / 没注入导出符号 | 三个磁带都必须每行以 LF 结尾;调 `fwm_export` 至少一次 |
| 2 | `nu<1 || !fwm_eq(u[0], ".unit 2")` | unit 磁带首行不是 `.unit 2` | unit 首行必须 `.unit 2` |
| 3 | `0<body && body<main && main<sinit && sinit<sret && sret+1<nb && 0<ainit && ainit<aret` | 标签顺序/位置不对 | stub(base 参数 b)里 `main:` 必须在索引 ≥2;`__init:` 在 a 里索引必须 ≥1 |
| 4 | `fwm_eq(b[sret-1], "  ret")` | `__main_ret:` 前一行不是 `  ret` | stub 的 `__main_ret:` 紧前必须是 `  ret` |
| 5 | `lastret<0`(`a[ainit+1..aret)` 里找不到 `  ret`) | base 的 `__init:` 与 `__main_ret:` 之间没有 `  ret` | base 里 `__init:` 之后要有 `  ret` |
| 6 | `b[sret+1]` 必须是 `  .exit r0` 或 `call exit`+`  .exit r0` | stub 的 `__main_ret:` 后一行不是退出指令 | 紧接 `  .exit r0` |
| 7 | `labmax>1000000 || strmax>1000000` | 标签/字符串编号越界 | 正常磁带不会触发 |

## 实测:一条能过全部断言的 ELF 磁带(合成)
```
unit:  .unit 2
       .global helper
stub:  .global main
       .bss x 8
       main:
         call __init_u
         ret
       __init_u:
         call exit
         .exit r0
         ret
       __main_ret:
         .exit r0
base:  .str S0 "x"
       __init:
         ret
         call helper
       __main_ret:
         ret
```
结果:`merge=1 stage=7 n=94`。

## 与真实磁带的差距(必须声明)
这是**合成**夹具,不是 driver 实际产出的磁带。真实磁带由 fwdstub/fwdsign 生成,
行序与 `.bss`/`.str` 布局可能不同;等 INSTALL44 装好、能落盘真磁带后再补 C 正例。
