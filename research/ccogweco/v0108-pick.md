# v0.0.108 入车清单(给 v0.0.106 集成车单写手)

基线: origin/main 1ec29fc8(合并时以当时 origin/main 为准)。
顺序: 先取表改, 再取已过审的表级用例, 最后取文档与覆盖说明。检查 check-forward-count 三档自红, 不入车。

## 1 表改: exec/parse2/forward-result.tsv(取自 daae75b8e7f4849757b54bea355d3b6948278069, 只取这一个文件)

    git checkout daae75b8e7f4849757b54bea355d3b6948278069 -- exec/parse2/forward-result.tsv

内容: CMPI fw_count,8192 的观察值 1-2 走 FW.full, 0 走 FW.capture, 即 8192 就拒(相等即拒), 修掉原来的 8193 才拒。
理由: 这是 S0 的实质表改; 同提交里的 tests/check-forward-count.cx 三档自红且未登记, 按 owner 要求不入车, 因此不整提交 cherry-pick。

## 2 表级用例: 623ac860f8625ed3a63d3779f20b6fa47497dd08(整提交)

    git cherry-pick -x 623ac860f8625ed3a63d3779f20b6fa47497dd08

带 tests/check-forward-limit-table.cx(秒级, 读表断言 obs 1、2 -> FW.full, 0 -> FW.capture), batch forward-limit-table(3 断言), manifest, 以及 tests/batches.tsv、tests/coverage.tsv、build/links.tsv 的登记行。
变异证据: 把观察值 '1-2' 改回 '2', obs-1 断言转红, 已实跑。
预期冲突: 若 tests/coverage.tsv 或 build/links.tsv 有号冲突, 保留 main 侧再补本行。

## 3 文档与覆盖说明: 0ef29e7811def721357126b213276be9f8efc358(建议只取文档与覆盖行)

    git checkout 0ef29e7811def721357126b213276be9f8efc358 -- plans/v0.0.108.md
    git checkout 0ef29e7811def721357126b213276be9f8efc358 -- tests/coverage.tsv

作用: 覆盖行写成 UNREACHABLE 并列出三条更早的限制(约 355 个不同被调 forward 符号时生成的 forward-entry.c 畸形; 356-412 报 model driver cli; 大于等于 413 段错误), 计划文档补同一结论。
注意: 该提交对 tests/coverage.tsv 的改动依赖第 2 步之后的覆盖行文本; 以本提交版本为准即可。不要带入 tests/fixtures/forward-count/gen_forward_count.cx(它只服务未入车的 red 检查)。

## 4 文档纠错: 58535db5d4ecf2f3596db70ef971bd4479e5a9e9(可选, 只取计划文档)

    git checkout 58535db5d4ecf2f3596db70ef971bd4479e5a9e9 -- plans/v0.0.108.md

内容: 记录 daae75b8 的 Green 声明无实跑、shared-e3 未部署的教训; 与第 3 步同文件, 若冲突以更晚版本为准。

## 不入车(明确排除)

- daae75b8e7f4849757b54bea355d3b6948278069 的 tests/check-forward-count.cx 与 tests/fixtures/forward-count/gen_forward_count.cx: 三档 8191/8192/8193 在当前产品上都红(产品级 8192 键位不可达), 未登记批次。
- ff247826102ab60d738986b78d1e54dbc601d220: 只改该 red 检查。
- b7449bb131523b3265d1ecb76c121037a224e0df: 只加 interfaces.forward-count PLANNED 行, 已被第 2、3 步的 UNREACHABLE 行取代。
- 6c3ae17a2b8e486278d5b403d016269137b4e3d8: 主要内容是修该 red 检查与覆盖措辞, 已被第 2、3 步覆盖。
- 2644f21ff0ae7593eb5e54ae6ebba21038814a7b(c16 准备): 提交说明自标 UNVERIFIED until 0.0.108 chain, 未过审, 不入车。

## 入车后自检

    sh ./unisacc.com tests/check-forward-limit-table.cx --
    sh ./unisacc.com tools/tables-gen.cx -- --check
    sh ./unisacc.com tools/derive.cx -- --check

预期: 3/3 PASS, 派生无 DRIFT。S0 的产品级 8193 拒绝在 0.0.106 整链里不承诺, 发布说明按表级证据写。
