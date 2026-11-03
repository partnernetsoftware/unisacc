# 身份机制收敛设计(草案,2026-11-02;基于 identity-inventory-2026-11-02.md)

目标:同一批字节只在一处被哈希;身份变化只经一个函数族传播;pins 字面量由摘要派生。不求"一个身份",求"一个原语 + 少数独立身份"。

## 保留的独立身份(盘点证明不可合并)
- kernel context(24 源 + 6 工具):判断 kernel blob 可否复用,必须不随全树变。
- 两代 GEN:独立构造互证,不可缩一代。
- 产物摘要(pp_product_digest):已构建字节集,与源码集不相交。

## 收敛成的两个原语(新模块 tools/cxlib/identity.cx,唯一实现)
1. `id_tree(root, selector) -> 64hex`:path\0 + sha 按路径字节序;selector = 目录集 + 过滤规则(表驱动,事实表 exec/facts/identity-selectors.tsv)。SID、harness 的 cx/ 摘要、PC 的 SID 项都改调它。
2. `id_members(dir, member_list) -> 64hex`:name\tsha 排序;pp_product_digest、ape-receipt 成员行、build.json.artifact 段、ci-candidate-check 都改调它(productpkg.cx 已有雏形,上提)。

## 派生而非各算
- PC = id_tree(SID selector) + id_members(构建工具集) + root 产品摘要 + cc 版本。工具集表与 GEN 共用(rebuildcontext.cx:161-229)。
- pins:pin 目标 sha 由 id_members/文件摘要在 `--check` 时派生并与 pins.tsv 比对;literal 行只留测试向量与 host-dep。

## 迁移顺序(每步字节等价回归 + 棘轮 9/9)
1. 提取 `id_tree`:先让它复现现 SID(seed/ident.c 输出逐字节相同)——ident.c 的 CX 预演已通过(W6),直接复用。
2. harness(allsession.cx:142-145)改调 id_tree,删重复实现。
3. `id_members` 上提,四处产品字节哈希改调。
4. pins 字面量派生化(tools-pins-audit §6.4)。
5. PC/GEN 共用工具集表。

## 风险
- SID 值变化=换标,必须并入一次纪元(第 1 步若输出逐字节相同则不变)。
- identity-helper 编自 seed/ident.c;换成 CX 后引导链要重新证明(selfhost 两代)。
- 不得为合并而放宽 kernel context 的收窄边界。

## 未决(需要裁定)
- id_tree 的 selector 是否沿用现过滤(含 .py/.sh)还是借机收紧——收紧会换标。
