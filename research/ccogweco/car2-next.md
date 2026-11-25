# 第二车接续:清单重排与合并预演(ccogweco,只读,未碰车树)

## 1 0.0.108 与 0.1.0 封版前的件(按"一车装得下"重排)

### 第三车 0.0.108(诊断定位类,一版一类)

| # | 件 | 依据 | 依赖 | 私有补丁 | 预计冷态 |
|---|---|---|---|---|---|
| 1 | c15 未定义函数前移编译期 | tests/diag-contract.tsv c15 x1_undef.cx:4 KNOWN-GAP 到期 0.0.97;version-seq 第 50 行 | 无(纯前端判定;要动 exec/parse2/control 与事实表) | 无 | 改后单批 diag-contract 约 20 秒级;前端表改动需 derive 约 25 秒 |
| 2 | c16 no file 候选路径(searched/package-root) | 同上 c16 k_use.cx/x3_modseg.cx;分支只有 2644f21 准备提交(UNVERIFIED) | 需按现行树重做 2644f21,并经 gatekeeper | 2644f21 在 v0.0.108 分支(未过审) | 单批 diag-contract 约 20 秒 |
| 3 | c17 does not export 带 use 行 | 同上 c17 q_sym.cx:1 | 可能依赖 c16 的模块诊断基础 | 无 | 单批约 20 秒 |
| 4 | -o 误覆盖源文件警告 | version-seq 第 50 行(从 0.0.113 并入) | 需新合同行(现在无 diag-contract group) | 无 | 新批约 10 到 20 秒 |
| 5 | diag-contract 基线消减 | 每转一行 KNOWN-GAP 才降 DC_GAP_BASELINE(现 15) | 1-4 同批做 | 无 | 随 1-4 |

四件都不触模型网身份以外的东西,但 c15/c16/c17 都要改 exec/parse2 表并重算派生,一车四件可装下(同为诊断类,不违反一版一类)。

### 0.1.0 封版前还要做的件(与 0.0.108 并行准备)

| # | 件 | 依据 | 依赖 | 私有补丁 | 预计冷态 |
|---|---|---|---|---|---|
| A | 六目标自举不动点连续 3 版取证并归档 | 0.1.0 准入第 1 条;现状只有 0.0.100、0.0.105 两版有语句,98/101 无,四版 evidence 无 ape-receipt(见 r35 报告) | 每版整链后,归档六 fixedpoint/g2 切片回执与 compare/compare-recheck 输出 | 无 | 不新增跑;随各版整链取证 |
| B | CI 六格 QUALIFIED 且严格证据 FAIL/UNKNOWN=0 | 准入第 2 条 | 各候选的 candidate-native.yml 结果 | 无 | 随各版 |
| C | 零已知 P0 与 state-unverified 不超过 3 行 | 准入第 3、4 条 | 缺陷批 127..129 与未验证项普查 | 无 | 普查量在整理,冷态估 1 到 2 分钟级 |
| D | 前端覆盖三期完成 | 准入第 5 条;A 在 0.0.100、B1 在 0.0.107、B2 是 0.0.109 | 0.0.109(agy-flow) | 无 | 按 109 计划 |
| E | 整链起到公开连续 3 版不超过 120 分钟 | 准入第 6 条 | 第一、二、三车的实测记录 | 无 | 随各版 |
| F | 能用清单 30 过 25 | 准入第 7 条 | 清单本身要定稿 | 无 | 未估 |
| G | CXM_MAXU 256 产品级验证(CHECK_MAXU、UNIT_BOUNDARY、129/200/256 -o) | 0.0.107 第二车判据 | 新产品候选 | 有:archive/research-2026-11-25/ccogweco/cxm-maxu-256.patch | 新产品上三批合计约 1 到 2 分钟 |
| H | 变异门超时杀进程组(G38 清单) | 0.0.125 必做验收 | 静态件已有交接;动态断言待做 | 有:research/ccogweco/mutation-gate-static-1-3-6.patch(旧版,待 ompgpt 重做) | 夹具单跑目标 20 秒内 |
| I | 126 转发入口容量崩溃修复与具名拒绝 | version-seq 第 44 行;R26/R24 取证 | 生成处插桩(宿主侧只能整链候选上做) | 无 | 未估,须整链候选 |
| J | 103 新判据落地(0.1.x 首件) | cc 决定 103 归 0.1.x;新判据见 research/ccogweco/v0103-new-criteria.md | one.cx 已找回;新夹具需写 | 夹具草案在判据文件附录 | 基准每轮 5x2x5 次,空机约数分钟 |

说明:0.0.108 车装 1-5;A 到 F 是准入证据类,不占车;G、H 是第二车已备私有补丁;I、J 排在 0.1.x。

## 2 合并预检(v0.0.107 头 ac483247c7 × 第一车 6faff97eed)

base = 992cfbdbb6(git merge-base ac483247c7 6faff97eed)。
命令: `git merge-tree $(git merge-base ac483247c7 6faff97eed) ac483247c7 6faff97eed`
(git 2.34 不支持 --write-tree,用的是旧三路文本;改判来源为 `changed in both` 段与 `<<<<<<< .our` 标记)

真冲突 4 个文件(合并树里带冲突标记):
| 文件 | 冲突形态 | 解法 |
|---|---|---|
| build/iface-hash.tsv | 两侧都改了包 sha 行(cx.compile/cx.driver 相邻行) | 派生表:取任一侧后跑 `tools/iface-hash.cx -- --write`,再 `--check` |
| build/links.tsv | 两侧各加 links 行且邻接 | 派生表:取任一侧后 `tools/derive.cx -- --write --only links --check` |
| build/tlib-packages.tsv | 两侧都取了新号 | 派生/事实表:合后 `tools/layer-api.cx -- --bump-pending` 现取,写回本表 |
| tests/batch-product-inputs.tsv | 两侧各自追加行,无删除(我方 +3、车侧 +6) | 手写并集(保留两段全部行),再 `tools/prerun-inputs.cx -- --check` 等消费检查 |

另有 1 处是文档内容里的 `<<<<<<<` 字样误报:archive/research-2026-11-25/2026-11-24/car1-merge-preview.md 两侧都有该文件,内容冲突需人工取最新版(它本身就记录上一轮冲突表)。

自动合入但合后必须复核(旧 merge-tree 判为可合并,不代表语义正确):
- tests/batches.tsv(我方 +4、车侧 +21)、tests/coverage.tsv(我方 +6/-2、车侧 +20/-2):合后查重键 `grep -o '^tools-compile-[0-9]*' tests/batches.tsv | sort | uniq -d` 与 `tools/tables-gen.cx -- --check`。
- cx/compiler/fwdstub.cx、src/driveroute.cx:两侧都动过,合后必须重编与跑对应批(F1 转发桩、B1 重放)。
- build/freeze.tsv:只有车侧改(刷新时间戳与 base),我方未动,自动取车侧即可;合后不要动冻结行与号。

cc 预期的 build/freeze.tsv/links.tsv 冲突:freeze 实际不冲突(单侧),links 冲突如预期;iface-hash、tlib-packages、batch-product-inputs 是另外三个真冲突。
"稍后的 pin 补丁新提交"尚未出现,出现后按同一命令重跑;pin 补丁若只碰 tools/ 与 build/freeze.tsv 旁路,预计不新增冲突,但 freeze.tsv 两侧都改时会变成真冲突,按冻结行归属(cc)解决。

## 3 起链判据条目核对

- plans/v0.0.107.md:只有范围 1-4 与依赖、回滚;无 G45 七条、无 module-units/cxmod-tri 三批预热条目。
- plans/v0.0.107-chain-start.md:不存在(`ls plans | grep chain-start` 空,`git ls-files | grep chain-start` 空)。
缺的条目(按 G45 审阅原文):
1. c99-tentative-array-incomplete: TENTARR_SUMMARY passed=10 failed=0, rc0
2. c99-array-param-typedef: ARRPARAM_SUMMARY passed=10 failed=0, rc0(含 t10-original-repro 输出 8、N1/N2 具名、N3/N4 仍拒绝)
3. c99-extern-array-incomplete: A0_SUMMARY passed=14 failed=0
4. c99-global-redecl-conflict: 18/18 不退
5. netregen-multi 与 opt-multi 12 批 A/B 字节相同(先跑 lnx-x86_64 两片证伪)
6. 三批冷耗时各记一次,均低于 27.5 秒
7. tsv-lint unresolved 不高于 10764;notcovered 普查对 t10 行为 UNMEASURED 一致
另缺:module-units(check.cx 入口)与 check-cxmod-tri 的三批预热条目(即起链前先把这三批跑热并记冷/热耗时的那三条)在两份计划里都找不到;G45 自己也没把这三批列进七条,建议起链判据补成"七条 + 三批预热"独立小节。
