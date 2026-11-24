# __FILE__ 等宏字面全仓扫描(2026-11-24,只读)

背景:tools/cxlib/toolprep.cx:99-106 的 tq_mentions_file 对材料闭包里的每个源文件做字节扫描,只要出现连续 8 字节 __FILE__,就拒绝该材料键(affected.cx:1198-1203 注释说明原因:构建器会把 __FILE__ 的路径写进镜像,两个根下字节不同,不能共享缓存名)。因此注释、字符串、代码里任何一处字面都会让该工具的 -tool:path 材料每次冷编(每窗约 24 秒)。

## 扫描命令与结果

    grep -rnE "__FILE__|__LINE__|__DATE__|__TIME__|__TIMESTAMP__" tools cx tests --include="*.cx" --include="*.c" --include="*.h"
结果:61 个文件命中(含注释与字符串);其中真正落在材料闭包里的只有 9 个(见下)。

闭包口径:从 tests/batches.tsv 的 materials 列取全部 NAME-tool:path(31 个),另补 11 个具名材料的生产者工具(object/pp-gold/ref/identity/net/asm/streams/old-product/opt-frame4/kernel-build/isolated-streams),逐个 `sh ./unisacc.com -cx-plan tools/NAME.cx` 取文件清单(含 use 与 include),合并去重 170 个文件后与命中集求交:
    comm -12 hits.txt closure.txt

## 材料闭包内的 9 个文件

| 文件 | 行 | 形态 | 影响 | 建议 |
|---|---|---|---|---|
| tools/affected.cx | 1203 | 注释里写了 realpath(__FILE__) | affected-tool 材料恒拒,而 affected-tool 被 s9-nightly、dcheck、kernel-material 等多批使用,是每窗 24 秒的主要来源(cc 已定位) | 改写注释:不写出连续宏名,例:"构建器会把自身文件路径宏的真实路径写进镜像" |
| tools/devsvc-core.cx | 538 | 代码里 memcmp(b+k, "__FILE__", 8) 作为"闭包里有没有文件路径宏"的检测 | devsvc-core-tool 材料恒拒,影响 12 个以上 devsvc 批次 | 拆相邻字面量:memcmp(..., "__FILE", "__", ...) 不改变运行期字节,源码不再出现连续 8 字节 |
| tools/identity-material.cx | 75 | realpath(__FILE__, self) 自路径校验 | identity-material 材料恒拒(生成器自身) | 真嵌入类,不能等价去字面;要么接受冷编并在余量页登记,要么改身份设计(用输入表给源路径) |
| tools/object-material.cx | 31 | realpath(__FILE__, self) 自路径校验 | objects 材料恒拒 | 同上 |
| tools/pp-gold-material.cx | 64 | realpath(__FILE__, self) 自路径校验 | pp-gold 材料恒拒 | 同上 |
| tools/source-package.cx | 102 | realpath(__FILE__, self) 自路径校验 | source-package-tool 材料恒拒 | 同上 |
| tools/libneed-native-material.cx | 39 | realpath(__FILE__, self) 自路径校验 | libneed-native 材料恒拒 | 同上 |
| tools/libneed-native-profile.cx | 26 | realpath(__FILE__, self) 自路径校验 | libneed-native-profile 材料恒拒 | 同上 |
| tools/ref-material.cx | 70 | digest(__FILE__, recipe) 把自身路径写进配方 | ref 材料恒拒 | 真嵌入类,同 identity |

## 不在材料闭包内的命中(无材料键影响,可不动)

- 注释:pp-differential.cx:10、cxcc.cx:14、e2stream-child.cx:3、release-build.cx:1218 等。
- 字符串与夹具内容:check-unit-frontcache.cx:217-218 的 C 源码字符串、pp-differential 的预处理用例、check-cx-word.cx:37-39 的关键字表。
- 自路径校验但工具未被材料化:run-startup-bench、source-origin-verify、sourceflat、extract-p3-models、cx-driver-build、cx-library-build、check-osx-pgroup-link、entry-build、release-build 等。
这些若将来被列成 NAME-tool 材料,会立即变成恒拒,建议在材料页加一条"新增 -tool 材料前先跑本扫描"。

## 建议(按收益排序)

1. affected.cx:1203 注释改写(一个词的事,直接恢复多批材料,收益最大)。
2. devsvc-core.cx:538 拆分相邻字面量,恢复 12 个以上批次材料。
3. 其余 7 个是真实嵌入自身路径的生产者:不建议为过材料键改身份语义;在 research/agy-h/margin-packages.md 的对应批注"该材料因闭包含文件路径宏恒拒,冷编成本计入余量缺口",按 0.1.x 的工具线统一设计(例如从输入表接收自身源路径)再解。
4. 新增材料规则检查:register 或 check-affected-product-inputs 加一条,对声明 -tool 的源做同样的 8 字节扫描,提前报"该工具不能材料化",避免每窗冷编后才被发现。

## 未覆盖

- 本次闭包只覆盖当前 batches 表声明的 31 个 -tool 材料与 11 个生产者;C 源(#include 的 .h/.c)已随 -cx-plan 输出,若 -cx-plan 不列出某类数据文件,则该文件不在本清单。
- __LINE__/__DATE__/__TIME__ 不在 tq_mentions_file 的拒绝集合里(只拒 __FILE__),本次列出仅为同类风险备查,不会导致材料键拒绝。
