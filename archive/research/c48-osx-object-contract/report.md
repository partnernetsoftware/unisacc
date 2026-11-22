# X5 osx unit-container 对象合同

最终归档状态：下文原六批174/0保留为安全门修复前的历史。新增unchecked格式化/拷贝已定点改为检查结果的cx_fmt及mm_copy，未调整基线。最终源码material为fd3206bf523e4e4f9f2f6d7baf09913735786441f5e6d395278a841dbd4f16ac、objects为fdd677036681df54b1f3fc5e6ae43e1160f02989274e7524b483090e575bc924；最终runner源码实际SHA见重生HASHES。

最终新材料位于out/c48-x5-material-final/{arm64,x86_64}，prepare会话79105/7801均实际exit0，仅PREPARED。两份final-material收据及final七批全文日志/状态归档本目录。父逐句柄核六正式91595/37352/14846/56309/56466/41621均实际exit0，仍为27+27+29+31+29+31=174/0；最终runner82752实际exit0/17项。最终三静态门cxtrunc/cxapis/outdep全仓exit1，完整日志保留为final-static-*；四个本刀关联源objects/run-batch/runner-platform/object-material在这些报告无匹配，不能把全仓旧红写为PASS。最终资格仍仅限unit-container及harness局部合同；未扩大ABI/跨平台或发布资格。

实现范围仅`tools/object-material.cx`与`tests/objects.cx`的additive osx profile/case。默认Linux v1材料格式、Linux检查路径与Win旧mode保留；本轮不在Darwin冒称Linux/Win回归通过。未改产品src/cx/exec，未安装、发布或改资产。

## 实际合同及边界

当前产品osx两格写出MH_OBJECT格式外壳，但只有`__UNISA,__unisa_tape`，其载荷为`UNISATAPE1 osx/ARCH N\n`及unit tape。独立parser核CPU/subtype、MH_OBJECT、完整命令表、LC_SEGMENT_64、section名称/范围/DEBUG flags、payload目标/长度/LF、唯一`.unit 2`；不是直接调用产品parser当oracle。对象无机器码、无LC_SYMTAB、reloff/nreloc为0，因此输出：

```
UNIT_CONTAINER code_bytes=0 symbol_records=0 relocation_records=0 unit_tape_only=1
NOT_SUPPORTED object.machine-code-symbols
NOT_SUPPORTED object.machine-code-relocations
NOT_CLAIMED system-linker-object-interop full-Mach-O-object-domain
```

这些未支持项不算成功覆盖。unit tape自身含标签/元数据，不等于机器码Mach-O符号记录。没有系统ld/cc互操作或全symbol/relocation类型资格。

两个同目标真实材料经产品链接生成MH_EXECUTE，再在匹配Darwin执行环境实际exit7、空输出且image hash不变。arm64为Darwin/arm64 native；x64为同ARM宿主Rosetta translated x86_64，不是Intel原生硬件或性能证据，亦不是APE自动选中ARM分片替代x64镜像运行。

每次case均实际执行7个独立坏CPU/cmdsize/nsects/section offset/载荷目标/载荷长度/截断负控。旧unit版本与混目标均另有产品具名拒绝、无输出和正例恢复；不是仅parser拒绝。所有材料、产品、源码均前后hash不变。

## 材料与身份

唯一prepare根：`out/c48-x5-material/{arm64,x86_64}`，每profile3次当前产品真实`-O2 -c -funit -b TARGET`；prepare sessions8367/98991真实exit0，仅PREPARED。收据本目录`material-arm64.tsv`与`material-x86_64.tsv`，包含main/value字面源与三个对象SHA；不复制.o或可执行镜像。

材料source为`9cc514d322062052a5fc0590766c5f592f7256de3a2b7833135a16d1b547eb54`，product为`f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`；material工具SHA`c984113f6e23c2f7d518cd52c53c02c872972326ef1020815820ebba28da827f`，最终检查SHA`e96c2e40f8f0bf4f512b85ff9322fe0b760a4d7e059e53edd7e3e30d3c31feff`。工具在prepare后未改动，未重构造同材料。

新CLI：

```
object-material prepare ROOT NEW_DIR --profile osx/arm64|osx/x86_64
object-material --verify ROOT MATERIAL --profile osx/arm64|osx/x86_64
objects ROOT MATERIAL_ROOT NEW_OUT osx-{arm64,x86_64}-{link,mixed-target,old-unit-version}
```

tests按架构选材料子目录；显式`CASE --profile osx/ARCH`仍可用于直接子目录。旧unsupported-osx直接mode具名retired/rc2，不再假装产品NOT_IMPLEMENTED；历史batch ID保留但参数指向新正alias，此迁移由父在元数据/矩阵明示。Win分支不变。

## 原始失败与精确修复

1. 首次harness metadata检查父实际exit2，`registry-list.stderr`记录M8 `tier-state-conflict row=113 id=objects-unsupported-osx-x86_64`。platform批应release，父修对应行后list实际exit0、metadata_valid1、656批；这不是对象功能PASS。
2. 首轮6formal真实全exit1（父句柄集合49353/98181/45505/57382/9626/8127）；材料verify前后通过，但三对象literal parser都FAIL，尚未执行link/controls。见`first-failures.tsv`与原`out/c48-x5-formal/`，不覆盖旧失败。
3. 原oracle把section flags误作整数2。独立CX字段诊断13282真实exit0，实际两对象S+64=33554432，即`S_REGULAR|S_ATTR_DEBUG=0x02000000`，字节`00 00 00 02`；所有其它实际offset184/cmd152/nsects1均吻合。精确修flag常量，保留所有范围/类型断言，不放宽规则。
4. 当前主生产者证据为`exec/enc/unitcontainer-result.tsv` SHA`2c692cd0891de79087bd4dd6476f632c9420850ff02fb7a49319805e1f0c2118`的m1/UC.section动作行，本目录`writer-section-row.tsv`为该完整行。字段证明为`macho-fields.tsv`。归档参考`archive/reference-c/src/back_image.c:443`也使用w32(0x02000000)，只旁证flag编码：该旧参考本身还输出text/symbol/reloc，不能把其完整对象能力迁给当前metadata-only writer。
5. 最终修复tests实编56728真实exit0；material已prepare实编exit0，默认.elf/osx.image命名已修，diff检查无空白错误。没有在测试读取输入时改源码。

## 正式结果

父使用最终frozen源码、正式harness，每批外bound55，子verify20/link8/run2/refuse2，独占新输出目录`out/c48-x5-formal-v2/`；不缓存PASS。第一组92198 arm positive27/0、31660 x64 positive27/0、53877 arm mixed29/0已由父核真实exit0，BATCH_RUN PASS/rc0/transport0/OBJECTS，x64日志明确Rosetta。

第二组三批父已逐句柄poll真实exit0：77255 arm old31/0、86953 x64 mixed29/0、76798 x64 old31/0。六份完整日志、摘要与`actual-session-status.tsv`归档本目录，合计174项断言/0失败，均有BATCH_RUN PASS/rc0/transport0/OBJECTS；最终sourceSHA与小证据见`HASHES.sha256`。旧Linux默认合同不变，但本机未取得Linux真机执行资格。metadata-v2-summary保留当时656批metadata_valid1；最终657批list由父核session15355实际exit0，本目录另存最终PLAN摘要，不覆盖早期拒绝。

## Darwin harness独立边界

另归档`runner-platform-report.md`与`darwin-harness-review.md`：旧新harness对四个既有失败作业对照一致，不把既有5红迁为对象合同PASS。新`tests/runner-platform.cx`独立批父核session67256实际exit0，17/0，完整日志在本目录；这是合成harness宿主选择/拒绝合同，不能替代Linux/Win真机证据。runner sourceSHA同列HASHES；DARWIN-RUNNER-FAULT-CONTRACT仅READY，不冒称既有故障已经修复。

构造、启动、装载、推理、输出、校验冷/热各相位为UNMEASURED；本任务不报速度收益。正式结果只绑定上述当前产品/源/材料身份，不迁为公开版/其它候选/完整发布资格。
