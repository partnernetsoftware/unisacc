# 0.0.43 交接（d8c4ead0，r1）

## 已落主
- `d8c4ead0` block-scope 空声明：`exec/parse2/control-result.tsv` local0-common 的 `@token3` 行改为 type_enum / tb==SBB 检查，拒绝经 `local_reject0`（"not covered: declaration"）。同提交刷新 `tests/gatedeps.json`（reviewed-trees exec 戳及 tests/cli.sh、src/front_parse.c 的 guard，此前已因 tip 修复提交而陈旧）与 `tests/seedmemory.py` 的 REFERENCE_KEYS。commitgate 通过（exec-unitparse rc0，派生检查全过）。

## 已验证
- 探针对照宿主 `cc -std=c99 -pedantic-errors`：`int ;` 拒；`struct T{...};`、`enum E{...};`、`int x;`、`;` 收。
- 差集：修复产品对 tip 产品，132 行修复刀输入与 915 行语料均无差异。
- 门（修复产品，逐套件单调用）：exec-unitparse、manifest-entries-parse2、exec-librarycontract-model、exec-librarytypedpartition-model-1/2/4、lib-typed-host、lib-variadic-host、lib-sig2-host 均 rc0；model-3 首跑 rc142（60 秒上限，负载下），单独复跑 rc0（38 秒）。

## 未落 / 未做
- 匿名 struct `struct{int a;};`：修复后仍接受，宿主拒绝（文件作用域修复同样的既有缺口，已记录，不挡本刀）。
- `exec/c/buildcompiler.sh` 的 JOBS=1 串行改动：仍仅本地未提交（机房主任 代裁方案 2）。
- 根包 `unisacc.com` 仍为旧引导件（sha `0e0531e3…f8`），未换为新产品（未授权）。
- Draft、Latest、签名、公开、push、resume7000、987：均未做。

## 下一窄刀（建议）
- 匿名 struct 缺口：文件作用域与块作用域的 bare_decl 检查目前对无标签 struct 放行。需在模型侧区分"有标签 struct"与"无标签 struct"（宿主以 bare_decl_ok 为准，见 src/front_parse.c:149）。先定位 struct 标签位在 TSPEC 之后的寄存器，再加一行检查；门与差集同本刀流程。
