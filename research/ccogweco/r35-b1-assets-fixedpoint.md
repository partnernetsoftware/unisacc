# R35 三项只读核对(ccogweco,2026-11-24)

## 1 B1 3/3 是否在 v0.0.107:不在。分支头部只有部分 2/3 与部分 3/3 的引擎行

事实链:
- `v0.0.107` 头 = `a183bd4e5d`,标题"v0.0.107 B1 补丁 2/3: 7.2 文件域不完整数组暂定定义与 t10 形参退化 (da9fd52fea)"。
- 该提交全量改动只有 2 个文件:
  - `exec/parse2/shape-result.tsv` 1 行:`parameter-dimensions FN.shiftdone` 的 RET 行末尾加 `["COPYW","parrank","drk"]`(基线 ac48dad0f5 -> fb0a2384e0)。
  - `tests/notcovered.tsv`:删 `incomplete array declaration before another definition` 一行,把 `...without an initializer` 计数 3->2、来源去掉 `exec/parse2/tentative`。
- 与三份原补丁(`772d5614fa` 内 `research/2026-11-23/v099-b1-series/`)逐块比对:
  - 0001 gredecl:文件齐全(`tests/check-global-redecl-conflict.cx`、manifest 均在 107 树,batch 行 `c99-global-redecl-conflict` 在 tests/batches.tsv:2006)。1/3 在。
  - 0002 7.2:补丁要求 `exec/parse2/tentative-result.tsv`、`tests/check-tentative-array-incomplete.cx`、`tests/manifests/c99-tentative-array-incomplete.tsv`、batches/coverage 行。107 树里夹具与 manifest 均不存在,只有 notcovered 的删行与 0002 完全相同。2/3 = 引擎/表数行在,夹具与登记缺。
  - 0003 t10:补丁要求 `exec/parse2/control-result.tsv`、`shape-result.tsv` 一行、`tests/check-array-param-typedef.cx`、`tests/manifests/c99-array-param-typedef.tsv`、batches/coverage 行。107 树里只有 shape-result 那 1 行(hunk index 与 0003 相同 ac48dad->fb0a238);control-result 改动、夹具、manifest、登记全缺。3/3 = 只有引擎一行,不是整块。
- 结论:agyneon 所说"2/3 与 3/3 合在 a183bd4e5d"不准确;a183 只是把 0002 的 notcovered 删行与 0003 的 shape-result 引擎行一起提交,两块补丁的夹具、manifest、control-result/tentative-result 与登记行都没有。fable"分支上只见 1/3 与 2/3"更接近事实:可见 1/3 完整、2/3 半块、3/3 只一行。两份原补丁在 107 树上直接 `git apply --check` 都失败(batches/coverage 上下文冲突,夹具 new file 可加但引擎上下文已变),不能原样补打,需按现行树重放。

## 2 0.0.106 基线登记行(只准备,不提交)

现表 `tools/old-product-assets.tsv` 六列,行按登记先后追加;最新一行是 0.0.101(8827155B, bb715a8bd3...)。上一版 0.0.105 尚未登记,故 0.0.106 公开时,基线(最新一行)会直接跳到 0.0.106。

准备行(字段与 0.0.101 完全同构,后两列待公开后从资产核出):

    0.0.106	partnernetsoftware/unisacc	v0.0.106	unisacc.com	<bytes>	<sha256>

填写来源与步骤(公布后由 agy-flow 做):
1. 取资产:`gh release download v0.0.106 -p unisacc.com` 或从 release/evidence/0.0.106/INDEX.tsv 的 pack 行读 `release-run --pack` 产物路径。
2. 核值:`stat -c %s unisacc.com` 与 `sha256sum unisacc.com`;两者必须与 pack 回执里的 APE 字节数/sha 一致,不一致不登记。
3. 只追加这一行(不重排),提交说明写"old-product-assets: register v0.0.106 as new baseline (<bytes>B, <sha8>)",与 c4a154dfea 的写法一致。
4. 登记后 `tools/check-affected-oldproduct.cx` 的前驱会随最新行变为 0.0.106;0.0.105 目前缺行,若需要它作为前驱,须先补 0.0.105 行再加 0.0.106。

## 3 0.1.0 准入"六目标自举不动点连续 3 版":目前不连续,且六目标粒度缺证据

已发布且带 release 目录的版本序列:0.0.98 -> 0.0.100 -> 0.0.101 -> 0.0.105。
- 明确写"固定点"的只有两版:
  - 0.0.100 notes 第 26 行:"两代自举固定点(gen1 190/190 全绿 obj_same=1506 obj_diff=0,完整 499/499 链产物打包并经全批严格预跑验证)"。
  - 0.0.105 notes 第 28 行:"整链固定点(0bca20928b,499/499 步、compare + compare-recheck 四同证明通过)与 gen1 全工具 190 划冒烟"。
- 0.0.98 与 0.0.101:release/evidence/<v>/INDEX.tsv 的 prefreeze-strict 只写"后台整链 499/499 加全批严格预跑"(98 为 459 批、101 为 466+4 批),notes 与 evidence 目录里没有固定点/compare 语句,也没有 ape-receipt 文件。
- 六目标粒度:`release/evidence/0.0.{98,100,101,105}` 四个目录 grep `ape-receipt|fixedpoint|fixed-point` 全部 0 命中;能列出六个固定点 g2 执行切片的只有 `release/notes-0.0.94.md`(六个 slice 的 sha 与字节),且它指向的回执在 `~/.cache/unisacc/chains/bg-b1/.../ape-receipt.txt`,未归档进仓。
- 结论:按"连续 3 版"要求,现存证据最多只覆盖 0.0.100 与 0.0.105 两版,中间 0.0.101 没有固定点语句,0.0.98 也没有;六目标固定点回执在 98、100、101、105 四版都没有归档。该条现在不成立。
- 缺的证据(逐项):
  1. 0.0.98、0.0.101 的 compare/compare-recheck 四同输出或固定点语句;
  2. 0.0.100、0.0.101、0.0.105 的 ape-receipt(六个 fixedpoint/g2 切片行)入仓;
  3. 若要"连续 3 版",需要三版相邻(如 0.0.105、0.0.106、0.0.107 或后续)都归档同类回执,并在各自 notes 写明。
