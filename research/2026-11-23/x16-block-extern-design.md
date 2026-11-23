# 0.0.101 x16 块作用域 extern:设计与最小复现

分支:`v0.0.101`(工作树 `.claude/worktrees/v0.0.101`)
主题:覆盖 `not covered: block-scope extern object without a visible file-scope declaration`
入口:control-result `global5-common` `BX.unknown` -> DEAD (`exec/parse2/control-result.tsv:2415`)
合同表:`tests/notcovered.tsv` 行 x16;案例 `x16_block_extern_decl`,`v03_unclosed_main_fs`
本版只做设计与最小复现,不改 `exec/`/`src/` 产品输入(冻结域仍禁共享 main;本工作树可写 research/plans/tests 预备,产品网改留给实现窗)。

## 1. 现状网路径

```
BX.entry
  -> BX.visible   LDX LOC; CMPI GMARK
       1 -> BX.epoch   ALUI bx_epoch=unit_epoch+1; LDX GHE; CMP
            1 -> BX.scan -> BX.token ... (合法块外声明扫描,含 equal/comma/end)
            * -> BX.unknown
       * -> BX.unknown
BX.unknown * -> DEAD REJECT "not covered: block-scope extern object without a visible file-scope declaration"
```

可见性合同(现状):

- `LOC == GMARK` 才算「已有文件域对象槽」
- `GHE` 纪元须等于本单元 `unit_epoch+1`(本单元内已见过该名的文件域登记)
- 否则一律 `BX.unknown` 拒绝

旁路拒绝(实现后仍保留):

- `BX.token @token31` -> unterminated block-scope extern
- `BX.equal 1` -> initializer 非法(C99 6.7.8p5)

## 2. C99 对照与缺口

| 形状 | C99 | 现状 | 目标 |
| --- | --- | --- | --- |
| 文件域先声明/定义,块内 `extern` 引用 | 合法(6.2.2p4 与既有可见) | 接受(v01) | 保持 |
| 块内 `extern` 在前,文件域定义在后(同 TU) | 合法;块声明引入外部链接,后文定义闭合 | REJECT x16(v02) | 接受,运行输出 10 |
| 块内 `extern`,全 TU 无文件域对象 | 合法声明;链接期未定义才失败 | REJECT(v03) | 编译通过;链接报 undefined |
| 嵌套块 + 文件域已可见 | 合法 | 接受(v04;程序 return 1) | 保持 |
| `extern int x = 1;` 块内带初值 | 非法 6.7.8p5 | 现先撞 x16(因无文件域);有文件域时应走 equal 拒绝 | 保留 equal 拒绝文案 |

根因:可见性用「声明点之前是否已登记 GHE/GMARK」做单遍判定,不满足「同 TU 后文文件域定义」与「纯链接期未定义」两类合法形。

## 3. 修复方向(候选,择一并实装)

(A) 延迟闭合(推荐首选)

- `BX.visible`/`BX.epoch` 失败时不立刻 `BX.unknown` DEAD
- 记 pending:`bx_id` + 源位,挂到单元尾或 TU 尾扫描
- 文件域定义/声明写入 GHE 时回填 pending;仍无闭合则发 tentative 外部符号交给链接(对齐 v03)

(B) 两遍符号

- 第一遍只收文件域对象名到 GHE;第二遍再跑 BX;与现单遍控制网摩擦大

(C) 宽松 BX.unknown

- unknown 直接当外部 tentative 继续 `BX.scan`
- 最短路径,须核未定义名误用、多单元、initializer 旁路顺序

建议验收顺序:先 (A) 或 (C) 最小补丁让 v02/v03 过编译;再核 equal/unterminated 旁路。

## 4. 不动边界

- 不放宽 `BX.equal` 初值拒绝
- 不改 GMARK/GHE 银行布局语义以外的旁路
- 产品网改(`exec/parse2/control*.tsv` 及生成)不在本设计提交;留给实现窗 + derive/check
- omp-go 0.0.98 若已含 x16 实装,本分支设计作对照与复现基线,合入时避免双改同一 BX 行

## 5. 最小复现(同目录 cases/)

实跑基线(已装产品经工作树 `unisacc.com` 符号链接,单条 <55s):

| 文件 | 预期现状 | 目标 |
| --- | --- | --- |
| `cases/v01_visible_fs_first.cx.txt` | rc=0 输出 `10` | 保持 |
| `cases/v02_fs_after_use.cx.txt`(=x16) | rc=1 REJECT x16 文案 | 输出 `10` |
| `cases/v03_no_fs_decl.cx.txt` | rc=1 REJECT x16 | 编译过;链接 undefined |
| `cases/v04_nested_visible.cx.txt` | 程序 return 1(进程码 1,非编译拒) | 保持编译接受 |
| `cases/v05_extern_init.cx.txt` | 现撞 x16(因无文件域) | 有文件域时 equal 文案 |

注:全部落盘为 `.cx.txt`(避免 f write 实编拒写与 cxlibc 棘轮);复现时 `cp` 去 `.txt` 再喂 `unisacc.com`。


## 6. 下一动作(实现窗)

1. 选定 (A)/(C),改 `control` 源表并 `--check` 生成 `control-result`
2. `tests/notcovered-cases/x16_block_extern_decl.cx` 与 v03 从 REJECT 迁到正例批次
3. 补链接期 undefined 探针(v03 目标)
4. `tools/affected.cx --files exec/parse2/control.tsv --run` 局部核;不做整批/整链

## 7. 证据入口

- 拒绝点:`exec/parse2/control-result.tsv:2410-2429`
- 计划对照:`plans/notcovered-plan.md` x16/v03 行
- 范围备忘:`research/2026-11-23/frontend-phase3-scope.md` x16 节
- 本设计:分支 `v0.0.101` 本文件 + `x16-block-extern-cases/`
