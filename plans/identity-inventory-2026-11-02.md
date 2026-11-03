# 身份/指纹机制盘点(2026-11-02,只读调研;路径行号抽验 3 处属实,其余未逐条复验)

| # | 机制 | 计算处 | 输入集合 | 输出形态 | 消费者(前5) | 何类改动使其变 |
|---|---|---|---|---|---|---|
| 1 | SID(源码身份) | `seed/ident.c:101-136`(`keep()` 过滤 `:47-85`;额外项 `:112`、`:115-116`);identity-helper 由 `tools/kernel-build.cx:50` 编出 | dirs `exec,src,kernel,include,weights,seed,cx` 内 .py/.c/.cx/.h/.inc/.tsv/.json/.sh + `iterate/kernel/typekw.tsv` + 10 个 `tools/` 支持文件 + `exec/c/asm/*.S`;每文件 path+NUL+32B sha,按路径字节序 | 64hex | `tools/cxlib/allsession.cx:149`、`kernelcontext.cx:16`、`rebuildcontext.cx:48`、`tools/source-package.cx:92,98`、`tools/cx-driver-build.cx:13` | 上述目录内被 `keep()` 接受的源码/表任一增删改;`tests/` 与大部分 `tools/` 不变 |
| 2 | PC(package context) | `tools/source-package.cx:94`、`source-package-pack.cx:140`、`cxlib/sourcepackage.cx:164,205` | SID + bootstrap + root-product-sha(pp digest)+ cc 版本 + helper/self/netpack/bound/ident 的 sha + context 路径 | `source-package-context-v2` / `source-package-pack-context-v1` 文本 + 其 64hex | `rebuildcontext.cx:71-106`、`product-build.cx:52`、`candidate-install.cx:36`、`candidate-promote.cx:45-46`、`tests/hostseed-source-bind.cx:28` | SID、构建工具字节、root 产品、CC 版本、context 路径 |
| 3 | GEN(两代) | 无独立公式:`cxlib/rebuildcontext.cx:211-229` 的 `native-product-construction-v1/v2/v3`;两代=同工具在 `tests/selfhost.cx:22-31`、`tools/selfhost-mixed.cx:15-18` 各算一次并逐字节比对 | package kind + builder 原字节/build.json + kernel context + P3 材料 + 9–11 个构建工具 sha + 五片 | 64hex | `candidate-install.cx:75`、`candidate-promote.cx:48-50`、`product-build.cx:59`、`rebuildcontext.cx:231`、`selfhost.cx:25` | 构建工具、kernel context、P3 包字节、包 schema、header 目录 |
| 4 | context(kernel) | 表 `cxlib/kernelinputs.cx:10-24,40-63`;写方 `tools/kernel-build.cx:35-52`;校验 `cxlib/kernelcontext.cx:14-19` | 24 kernel 源 + 6 工具 | `unisacc-native-kernel-context-v3` 体 → `context.sha256` | `product-build.cx:52`、`candidate-install.cx:68,78`、`package-refresh.cx:42,47`、`tests/assembler-cache.cx:10`、`candidate-promote.cx:50` | 24 源或 6 工具任一字节;`tests/`、薄头不影响(by design) |
| 5 | pins | 声明表 `tools/pins.tsv`;检查器 `tools/check-pins.cx:100-306` | pin→sha256(target);literal/count/path/host-dep 行 + 权威计数 | `CHECK_PINS pins=N failed=M stale=S hostdep=H` | `tests/batches.tsv`、`tests/coverage.tsv`、被钉工具(`source-package-adopt.cx:26`、`source-package-mixed.cx:39-42`、`source-package-pack.cx:94`) | 被 pin 目标字节变;新增 64hex 常量未登记 |
| 6 | receipts | `ape-receipt.txt`=`tools/release-ape.cx:167-170`;`build.json`=`seed/ident.c:166-203`;`*.receipt`=`tools/cx-driver-build.cx:19-21,65`、`tools/entry-build.cx:789` | ape-receipt:成员名+sha+bytes;build.json:SID+artifact sha+bytes+commit | 行式 TSV / JSON | `cxlib/productpkg.cx:225-260`、`tools/check-productpkg.cx:189`、`tools/ci-candidate-check.cx:45`、`candidate-promote.cx:56-57`、`rebuildcontext.cx:46` | 被记录 artifact 或 SID 变;字段增删 |
| 7 | identity-helper | `tools/kernel-build.cx:50-51`:用当前产品编 `seed/ident.c` | seed/ident.c 字节 + 当前产品 | ELF + 64hex | `kernelcontext.cx:15`、`candidate-install.cx:54`、`candidate-promote.cx:55`、`rebuildcontext.cx:48`、`sourcepackage.cx:200` | ident.c 或编译器产品字节变 → helper 变 → 所有 pin 断言变 |
| 8 | pp_product_digest / pp_identity_kind | `cxlib/productpkg.cx:286-302` / `:140-145` | split2:启动器 + `ape-receipt.txt` 列出的旁目录成员(无 receipt 时固定 entry.elf, p3-package.bin),basename 排序,每行 `name\tsha\n` 再 sha | 64hex + kind `ape1|split2|none` | `allsession.cx:157`、`tests/run-batch.cx:37`、`tools/zero-wave.cx:34,37`、`tools/check-productpkg.cx:134`、`tools/check-root-identity.cx:69` | 启动器/entry.elf/p3-package.bin/receipt 成员字节;成员集合 |
| 9 | split1 | 无产出路径,仅作历史读法被接受(`allsession.cx:226,239`) | — | — | `allsession.cx`、`check-productpkg.cx:68,116` | 只存在于旧收据文本;`check-root-identity.cx:9` 文档仍写 split1,与实现不一致 |

## 输入集合重叠(同一批文件被各自再哈希一遍)
1. `exec/c/asm/*.S`、`src/internal_types.h`:SID(`seed/ident.c:101-136`)与 kernel context(`kernelinputs.cx:10-24`)各算一次。
2. 十个 `tools/` 支持文件(`seed/ident.c:115-116`):SID、pins 表行、被钉工具内 64hex 常量三处并存。
3. `cxlib/{kernelsha,netpack,sourcepackage}.cx`:kernel context 的 6 工具、product context 的 9–11 工具(`rebuildcontext.cx:213-214`)、pins 行交叉覆盖。
4. 产品字节:`pp_product_digest`、`ape-receipt.txt` 成员行、`build.json` 的 artifact_sha256、`ci-candidate-check.cx:45` 四处哈希同一批字节。
5. `seed/ident.c`:SID 输入、identity-helper 源、hostseed helper-source 身份、pins 血缘常量。
6. `cx/` 文件:`allsession.cx:142-145`(harness)与 SID 对同一批 `cx/` 文件算两次,摘要不可比。

## 理论可合并
- pp digest + ape-receipt + build.json artifact 段:同一成员集,可归一为"成员集摘要"(productpkg 内已实现,差统一调用点)。
- SID 与 harness:都是 "path\0+sha 排序拼接",只差根集合,可统一为 `tree_digest(root, selector)`。
- PC 与 GEN:都哈希"工具 sha + 材料",同一函数族(`rebuildcontext.cx:161-229`)。
- pins 的 64hex 常量可由上述摘要派生后删字面量(`plans/tools-pins-audit` §6.4 已列路线)。

## 不可合并(理由)
- kernel context 必须独立于 SID:`kernelinputs.cx:6-7` 用途是判断 kernel blob 能否复用;SID 覆盖全树会让无关改动使 kernel 永不可复用。
- SID 与 pp_product_digest:一个是源码集合,一个是已构建产物集合,字节集不相交。
- pins 的 literal/host-dep 行:测试向量与宿主依赖,无内容来源可派生。
- 两代 GEN 不能缩成一代:`selfhost.cx:1-3` 要求两个独立构造逐字节互证。
- `context.sha256` 与 PC 不能合并:变化粒度不同(本轮 kernel 收窄的目的)。
