# 7000 三层重验计数旁核

历史名称说明（2026-10-16）： `closure-c2/c3/c4` 与 `stages-1/2/3` 也保留为历史名，现各由 a/b/c 三片执行；下文 `closure-c1` 是冻结 722 项清单中的原名；现行执行门已改为 `closure-c1a`/`closure-c1b`/`closure-c1c`。不改历史计数或结果，不以该原名调度当前队列。2026-10-23：`tapebin-roundtrip-structure-1/2` 亦为历史名（原为 structure-K/2 两半），现行执行门为 `tapebin-roundtrip-structure-1`…`-6`（structure-K/6 六片）；下文 722 项清单仍按原名计数。`seedgen-ppconsumer-1` 亦为历史名（0.0.43 超时债拆分），现行执行门为 `seedgen-ppconsumer-1-lnx-x86_64`/`-lnx-arm64`/`-osx-x86_64`/`-negative` 四片；下文仍按原名计数。

只是设计；实际读取归档 jobs 与源码，未跑套件、未改指纹、未授任何摘钩资格。

## 基数与保守口径

基数是归档 `/tmp/c42-7000/cand.queue/results.json` 的 `jobs`，恰好 722 项；文件 SHA256 `1f8fcc781648bcbb5bfd887d2773fa2c2dbe4f90d467cea9af5edf47d3239283`。当前新增 ABI/staticimage/early-seal 合同不属于这722项，不回填历史 PASS。 当前 gatedeps 的652条声明中与归档jobs对上576条，146个归档job没有同名声明；stagemap第一匹配尚有58个unknown。这些全部保守重验，不能将现行分类当审计资格。

tests/gatelayers.py 明言层标签只引导诊断、不删job；tests/stagemap.c 也只标签且有 also 重叠。gatedeps 的声明不是实测闭包证书。tests/gatequeue.py:240 的 common 包含所有工具、完整环境等；现行机制不支持本文的三层省跑。

身份：C=输入字节流、规则/模型、显式资源；输出与覆盖/拒绝是结果见证，执行器是独立实现见证；X=作为显式输入流的 ABI/外部合同；O=oracle cc、执行工具链、宿主/环境及对拍归一化规则。执行器构建产物字节属于独立 runner 见证，其构建 cc 属来源/观测；cc 变更不能默认产物不变。

## 当前安全上线情景（只完成 pp-literals 一叶实测）

|改动|当前实现实际重验|未来获批该叶分层后整套件重验|可保留的子证据|
|---|---:|---:|---|
|ABI小改、核心输入/规则/runner字节均不变|722|721|仅 exec-pp-literals 的核心推导/拒绝证据|
|host cc 变化|722|722|该叶已有核心字节结果可保留，但观测必须重验|

721 的唯一摘钩候选是 exec-pp-literals，且必须先有本批完整实测闭包、漏依赖负例、独立核心运行路径并获批准；目前尚未授摘钩。剩余721名单为下面全集减这一项。host 722 是整个旧job验收口径：exec/pp/literalcheck.py:52 构建执行器，:68/:104/:129 以cc作oracle，故宿主cc变了整job并不复用。新数据合同仅核验旧哈希不能冒充重新观察新的cc。

## 可量化未来目标（条件假设，不能拿来上线）

以阶段表 E2+E1 的 23 项为第一圈候选；逐套件全部完成实测闭包及负例且证实不消费 X 后，典型 ABI 小改保留这些核心叶，重验其余 **699/722**。这是有界阶段目标，非“十几个 ABI 套件”估算；parse/type 到 image、chain/lib/build/meta 未审部分全部保守重验。

同一早段目标用于 host cc 小改：若这 23 个旧job全部拆成纯核心合同与独立观察job、runner字节维持相同，则核心重推为 **699/722**，另重验受影响 O；若继续要求旧job的完整对拍结果新鲜，则保守仍 **722/722**。O的精确数量不能从阶段名字推出，未逐套件审计前上界722、下界至少pp-literals 1；不要报告未经证据的宿主“只重验若干”数。

未来理论更窄目标：所有722逐套件证据齐备，选择 changed identity 的传递消费者集合；这里不能给未审的 ABI consumers/host consumers 猜数。所谓“典型ABI小改”须指定哪一条外部合同流变动，不能把重写清单注释与实际调用约定变更混算。

## 可复现名单与传播规则

读取归档 jobs 键，不用当前 --list；重验集合 R 初始为所有 unknown/未审job，加变化身份的直接消费者，沿输入产物边向下直到固定点。任何上游输出哈希变化须传播；没有变化仍需检查覆盖/拒绝语义身份；全链/产品行为job只有证明只依赖 unchanged sealed bytes 才可复用。

下面阶段标签按 tests/stagemap.c rules 第一匹配重现：com-去前缀；另外尝试去数字分片及去 -lnx-/-osx-/-win- 后缀；按规则顺序匹配glob。标签只生成将来审计名单，不给予复用资格。

### E2 (14)

```text
exec-macros
exec-pp-directives
exec-pp-literals
exec-pp-object
exec-pp-pragmas
exec-pploc
pptruth
prevheaders
rowcov-pp
rowcov-pp-locations
rowcov-pp-shared
shared-e2-located
shared-e2-plain
shared-e2-tokens
```

### E1 (9)

```text
exec-f1-sidecar
exec-lex-sourcefacts
exec-lexloc
exec-lexpos
rowcov-lex
strconvert-classic
strconvert-host
strconvert-model
strconvert-seed
```

### E3 (142)

```text
c99
ccparity
cli
com-c99
com-ccparity
com-cli
com-declmatrix-1
com-declmatrix-2
com-declmatrix-3
com-declmatrix-4
com-declmatrix-5
com-diag
com-diagunits
com-formatonce
com-hostile
com-parserbounds
com-staticinit
com-staticunits
com-tagforward
com-tapebin-1
com-tapebin-2
com-tapebin-3
com-volatile-comma
declmatrix-1
declmatrix-2
declmatrix-3
declmatrix-4
declmatrix-5
declshape
diag
diag-units
exec-decimal
exec-diag
exec-e3self
exec-errors-1
exec-errors-2
exec-errors-3
exec-errors-warn-1
exec-errors-warn-2
exec-errors-warn-3
exec-f1-attributes
exec-formatwarn0
exec-formatwarn1
exec-intwarn
exec-multiwarn
exec-neg
exec-parse2-libimports
exec-parseloc
exec-r21-e3
exec-returnwarn
exec-unitlocations
exec-unitparse
exec-unusedwarn
exec-warningdriver-asm
exec-warningdriver-cc
exec-warningdriver-ua-Wall
exec-warningdriver-ua-Werror
exec-warningdriver-ua-Wextra
exec-warnprep-0
exec-warnprep-1
exec-warnprep-2
exec-warnprep-3
formatonce
hostile
parserbounds
rowcov-parse2-1
rowcov-parse2-10
rowcov-parse2-11
rowcov-parse2-12
rowcov-parse2-13
rowcov-parse2-14
rowcov-parse2-15
rowcov-parse2-16
rowcov-parse2-17
rowcov-parse2-18
rowcov-parse2-19
rowcov-parse2-2
rowcov-parse2-20
rowcov-parse2-21
rowcov-parse2-22
rowcov-parse2-23
rowcov-parse2-24
rowcov-parse2-3
rowcov-parse2-4
rowcov-parse2-5
rowcov-parse2-6
rowcov-parse2-7
rowcov-parse2-8
rowcov-parse2-9
rowcov-parse2-build
rowcov-parse2-union
seed-construct-parse2
seed-construct-parse2-10
seed-construct-parse2-11
seed-construct-parse2-12
seed-construct-parse2-13
seed-construct-parse2-14
seed-construct-parse2-15
seed-construct-parse2-16
seed-construct-parse2-17
seed-construct-parse2-18
seed-construct-parse2-19
seed-construct-parse2-2
seed-construct-parse2-20
seed-construct-parse2-21
seed-construct-parse2-22
seed-construct-parse2-23
seed-construct-parse2-24
seed-construct-parse2-25
seed-construct-parse2-26
seed-construct-parse2-27
seed-construct-parse2-28
seed-construct-parse2-29
seed-construct-parse2-3
seed-construct-parse2-30
seed-construct-parse2-31
seed-construct-parse2-32
seed-construct-parse2-33
seed-construct-parse2-34
seed-construct-parse2-35
seed-construct-parse2-36
seed-construct-parse2-37
seed-construct-parse2-38
seed-construct-parse2-39
seed-construct-parse2-4
seed-construct-parse2-40
seed-construct-parse2-41
seed-construct-parse2-42
seed-construct-parse2-43
seed-construct-parse2-44
seed-construct-parse2-5
seed-construct-parse2-6
seed-construct-parse2-7
seed-construct-parse2-8
seedparse2-1
seedparse2-2
staticinit
staticunits
tagforward
tape-reader
tapebin-roundtrip-codec
tapebin-roundtrip-structure-1
tapebin-roundtrip-structure-2
tapebin-shape
volatile-comma
warn
```

### E4 (17)

```text
exec-e4-1
exec-e4-2
exec-e4-3
exec-e4-4
exec-e4self
exec-prune-0
exec-prune-1
exec-prune-2
exec-prune-3
opt-run-1
opt-run-2
opt-run-3
opt-run-4
opt-run-5
opt-run-6
opt-run-7
opt-run-8
```

### lower (23)

```text
ccinterop
com-forward
com-forward-multi
exec-armlower
exec-bridge-linux
exec-lowdata
exec-lower
exec-sparse
exec-winlower
forward
forward-multi
libneed
rowcov-lower-1
rowcov-lower-2
rowcov-lower-3
rowcov-lower-4
rowcov-lower-5
rowcov-lower-6
rowcov-lower-7
rowcov-lower-8
rowcov-lower-build
rowcov-lower-union
windows-resolver-host
```

### E5 (19)

```text
asmtext
combo
exec-arm
exec-asm
exec-e5
exec-memory-asm-1
exec-memory-asm-2
exec-memory-asm-3
exec-sha
rowcov-enc-1
rowcov-enc-2
rowcov-enc-3
rowcov-enc-4
rowcov-enc-5
rowcov-enc-6
rowcov-enc-7
rowcov-enc-8
rowcov-enc-build
rowcov-enc-union
```

### E6 (53)

```text
com-auditnet
com-elfobj
com-lifecycle
com-linkunits
com-memalign
com-run
elfobj
exec-armelf
exec-armwin
exec-binaryio
exec-codec
exec-core
exec-crcllp64
exec-elf
exec-elfdata-bridge
exec-elfobject
exec-embedded
exec-macharm
exec-machx86
exec-memory-cc-1
exec-memory-cc-2
exec-memory-cc-3
exec-memory-ua-1
exec-memory-ua-2
exec-memory-ua-3
exec-memwinarm
exec-memwinx86
exec-modelobject
exec-net
exec-objectfacts
exec-objectpack
exec-objectplan
exec-package
exec-pearm
exec-pex86
exec-srcelf-1
exec-srcelf-2
exec-srcelf-3
exec-x86win
fat-1
fat-2
fat-3
fat-4
fat-5
fat-6
hosthdr
lifecycle
linkunits
malloc
run
syscall6
target-package
winposix
```

### chain (179)

```text
apps-real
apps-structure
bigclosure-lnx-arm64
bigclosure-lnx-x86_64
bigclosure-osx-arm64
bigclosure-osx-x86_64
bigclosure-win-arm64
bigclosure-win-x86_64
ccrun-1
ccrun-2
ccrun-3
ccrun-4
ccrun-5
ccrun-6
ccrun-7
ccrun-8
ccrun-malloc
closure-c1
closure-c2
closure-c3
closure-c4
closure-ex
com-closure
com-corpus-1
com-corpus-2
com-corpus-3
com-corpus-4
com-csmithdiff-1
com-csmithdiff-10
com-csmithdiff-11
com-csmithdiff-12
com-csmithdiff-13
com-csmithdiff-14
com-csmithdiff-15
com-csmithdiff-16
com-csmithdiff-17
com-csmithdiff-18
com-csmithdiff-19
com-csmithdiff-2
com-csmithdiff-20
com-csmithdiff-21
com-csmithdiff-22
com-csmithdiff-23
com-csmithdiff-24
com-csmithdiff-25
com-csmithdiff-26
com-csmithdiff-27
com-csmithdiff-28
com-csmithdiff-29
com-csmithdiff-3
com-csmithdiff-30
com-csmithdiff-31
com-csmithdiff-32
com-csmithdiff-33
com-csmithdiff-34
com-csmithdiff-35
com-csmithdiff-36
com-csmithdiff-37
com-csmithdiff-38
com-csmithdiff-39
com-csmithdiff-4
com-csmithdiff-40
com-csmithdiff-5
com-csmithdiff-6
com-csmithdiff-7
com-csmithdiff-8
com-csmithdiff-9
com-difftest-1
com-difftest-2
com-difftest-3
com-difftest-4
com-difftest_o-1
com-difftest_o-2
com-difftest_o-3
com-difftest_o-4
com-fb12-multi
com-luatests
com-luatests-build
com-minicon
com-multi
corpus-1
corpus-2
corpus-3
corpus-4
csmithdiff-1
csmithdiff-10
csmithdiff-11
csmithdiff-12
csmithdiff-13
csmithdiff-14
csmithdiff-15
csmithdiff-16
csmithdiff-17
csmithdiff-18
csmithdiff-19
csmithdiff-2
csmithdiff-20
csmithdiff-21
csmithdiff-22
csmithdiff-23
csmithdiff-24
csmithdiff-25
csmithdiff-26
csmithdiff-27
csmithdiff-28
csmithdiff-29
csmithdiff-3
csmithdiff-30
csmithdiff-31
csmithdiff-32
csmithdiff-33
csmithdiff-34
csmithdiff-35
csmithdiff-36
csmithdiff-37
csmithdiff-38
csmithdiff-39
csmithdiff-4
csmithdiff-40
csmithdiff-5
csmithdiff-6
csmithdiff-7
csmithdiff-8
csmithdiff-9
difftest-1
difftest-2
difftest-3
difftest-4
difftest-py-1
difftest-py-2
difftest-py-3
difftest-py-4
difftest_o-1
difftest_o-2
difftest_o-3
difftest_o-4
exec-chain-1
exec-chain-2
exec-chain-3
exec-chain-4
exec-chain-5
exec-formats
exec-formats-f1
exec-multi-asm-isolation
exec-multi-asm-m-forward
exec-multi-asm-m-reverse
exec-multi-asm-n-forward
exec-multi-asm-n-reverse
exec-multi-asm-pool-forward
exec-multi-asm-pool-reverse
exec-multi-asm-static
exec-multi-cc-frame
exec-multi-cc-isolation
exec-multi-cc-m-forward
exec-multi-cc-m-reverse
exec-multi-cc-n-forward
exec-multi-cc-n-reverse
exec-multi-cc-pool-forward
exec-multi-cc-pool-reverse
exec-multi-cc-static
exec-multi-ua-isolation
exec-multi-ua-m-forward
exec-multi-ua-m-reverse
exec-multi-ua-n-forward
exec-multi-ua-n-reverse
exec-multi-ua-pool-forward
exec-multi-ua-pool-reverse
exec-multi-ua-static
exec-native-chain
exec-native-resources
exec-native-stages
luatests
minicon
multi
realprog
sqlspeed
stages-1
stages-2
stages-3
```

### lib (84)

```text
lib-artifacts
lib-bindings
lib-bitfield-source
lib-callable-catalog
lib-callable-mechanism
lib-callable-model
lib-callable-variadic-mechanism
lib-callable-variadic-model
lib-callable-variadic-native
lib-callback-forward
lib-callback-graph-equality
lib-callback-graph-host
lib-callback-graph-protocol
lib-callback-native
lib-callback-outcome
lib-callback-plan
lib-callback-source-graph
lib-callplans-host
lib-carrier-callback
lib-carrier-factory
lib-carrier-import-model
lib-carrier-mechanism
lib-carrier-model
lib-carrier-native-plan
lib-carrier-plan
lib-carrier-relay
lib-context
lib-data
lib-ffi-provider
lib-format
lib-fp-rank-source
lib-fp-value-rank-source
lib-layout-source
lib-lifecycle
lib-module-exports
lib-native-exports
lib-ordered-carrier-host
lib-ordered-carrier-model
lib-resolver
lib-resolver-host
lib-sig2-host
lib-sig2-native
lib-sig3-canonical
lib-sig3-equality
lib-sig3-host
lib-sig3-source
lib-source-import-compat
lib-source-prep
lib-source-provenance
lib-source-provenance-located
lib-stack-arm
lib-stack-x86
lib-stack-x86-hostabi
lib-typed-host
lib-typed-native
lib-union-composite-callback-1
lib-union-composite-callback-2
lib-union-composite-callback-3
lib-union-composite-direct-1
lib-union-composite-direct-2
lib-union-composite-direct-3
lib-union-native
lib-union-scalar-1
lib-union-scalar-2
lib-union-scalar-3
lib-union16-callback-1
lib-union16-callback-2
lib-union16-callback-3
lib-union16-callback-4
lib-union16-callback-5
lib-union16-direct-1
lib-union16-direct-2
lib-union16-direct-3
lib-union16-direct-4
lib-union16-direct-5
lib-variadic-host
lib-variadic-native
lib-variadic-protocol
lib-variadic-resolver-host
lib-windows-bridge
lib-windows-gp
lib-windows-gp-native
lib-windows-imports
lib-word64
```

### build (74)

```text
com-comboot-fixedpoint
com-comboot-seed
com-comboot-stage2
com-comboot-stage3
com-seedgen
exec-armself
exec-armself-package
exec-bootstrap-osxarm
exec-bootstrap-osxx86
exec-driver-core-build
exec-driver-core-contracts
exec-driver-core-dependencies
exec-driver-core-modes
exec-driver-language-1
exec-driver-language-2
exec-driver-resources-macros
exec-driver-resources-headers
exec-driver-resources-printf
exec-driver-resources-isolated
exec-macself
exec-macself-package
exec-macxself
exec-macxself-package
exec-selfelf
exec-selfelf-package
exec-selfprep-arm
exec-selfprep-table
exec-selfprep-x86
exec-tableself-1
exec-tableself-2
exec-winself
exec-winself-package
exec-winx86self
exec-winx86self-package
nativeboot-cross-a
nativeboot-cross-b
nativeboot-self
seed-construct-base
seed-matrix-features-1
seed-matrix-features-2
seed-matrix-features-3
seed-matrix-lnx-arm64
seed-matrix-lnx-x86_64
seed-matrix-object
seed-matrix-osx-arm64
seed-matrix-osx-x86_64
seed-matrix-shared
seed-matrix-win-arm64
seed-matrix-win-x86_64
seedape
seedfacts
seedgen
seedgen-2
seedgen-3
seedgen-4
seedgen-construct-prep
seedgen-opt
seedgen-optconsumer-1
seedgen-optconsumer-2
seedgen-optconsumer-3
seedgen-parse2consumer-1
seedgen-parse2consumer-2
seedgen-parse2consumer-3
seedgen-pp-1
seedgen-pp-2
seedgen-ppchain-1
seedgen-ppchain-2
seedgen-ppchain-3
seedgen-ppconsumer-1
seedgen-ppconsumer-2
seedgen-pprun
seedgen-pruneconsumer-1
seedgen-pruneconsumer-2
seedgen-pruneconsumer-3
seedmemory
seedpack
seedpy
```

### meta (50)

```text
ape-version
bound
c99-ledger
com-full-signature
com-tools
decision-ledger
docs
dsl-ops
facts-export
finite-template
freezecheck
fresh-order-0
fresh-order-1
fresh-order-2
fresh-order-3
fresh-order-4
fresh-order-5
fresh-order-6
fresh-order-7
fresh-order-8
front-bounds
gate-infra
gate-infra-38
gate-layers
kernel
ledgercheck
lib-bindings-registry
manifest-entries-enc
manifest-entries-enc-arm
manifest-entries-lex
manifest-entries-lower
manifest-entries-parse2
manifest-entries-pp
modelbenchcheck
package-footer
pipeline-cache
proc-enum
publish-order
qprefix
referee-binsel
referee-pfconv
revivedscan
script-inventory
source-layout
subtract-safety
subtract-safety-selftest
tools-1
tools-2
tools-3
tsv-build-account
```

### unknown (58)

```text
attemptchain
c99precheck
checkrun
commitgate
docedit
draftfetch
draftguard
envpin
exittable
gatesummary
gqalive
h1source
hostcheck
installpair
k2b
namedrefuse
observation
precommithook
queuestart
queuestart-launch
queuestart-strict
queuetimeout
queuetimer
rulingwait
stagelog
unisa-ape
unisa-bits
unisa-catalog
unisa-emit-arm-nwmem
unisa-fp
unisa-gold-binsel-label
unisa-gold-irsel-label
unisa-gold-lex-label
unisa-gold-opinfo-label
unisa-gold-parse-label
unisa-gold-peep-label
unisa-gold-pp-label
unisa-gold-prec
unisa-gold-reloc-label
unisa-gold-scope-label
unisa-gold-type-label
unisa-hostabi
unisa-image-elf
unisa-image-fat
unisa-image-macho
unisa-image-pe
unisa-libneed
unisa-linalg
unisa-padded
unisa-peversion
unisa-tape
unisa-tapebin
unisa-tapebin-shape
unisa-uns1
unisa-vm
unisa-zdata
warmup
whyrerun
```

## 防漏检但绿的最小机制

1. 每个被摘钩job须登记真实读取闭包、构建/运行子路径、runner/artifact/source digest、输入/输出流文件、外部合同、观测身份、拒绝集合；目录新增/缺失/未跟踪输入 fail closed。
2. 每个闭包依赖逐一变异：改字节、移走、增加潜在读入文件；必须使合同红或失效。漏掉规则表/资源/共享执行器任一个的负例必须红，不能只测试manifest格式。
3. X负例：改实际消费的ABI名/布局/调用约定，parse2及下游消费者必须失效，早段封存保持；若真实读了X而未传播，撤销摘钩资格。
4. O负例：替换cc wrapper或cc1/as/ld/版本、让oracle有不同结果，观察必须失效；若runner编译字节变则提升C变动并向下传播。
5. 保留未审全量回退、抽样强制全树比较与周期整链；复用结果必须标明旧观察身份，绝不能改写旧7000的PASS或盖住UNVERIFIED。

## 已识别不确定性

E2/E1 名字可能测build、located metadata、头文件裁剪或完整产品route，also字段不足以证明边界。上面未来早段目标要求逐项证实；若任何一项仍混入X/宿主输入，其目标计数加1。ABI清单当前包括导出名而不穷尽驻留/加载行为；8b792a08类变化需镜像性质/产品行为证据，不以ABI名称未变视为安全。

## 迁移审批与防漏检

本提案不更改 gatequeue.py/gatedeps.json。先给每个拟摘钩job登记：实测命令/范围、输入/输出数据、实际读取闭包、独立expected索引、动态C核验入口、规则内容与资源内容负例、runner字节、观测数据入口；原有Python只作对照并登记ungated-checks。缺记录者仍全量参与重验，静态stage标签不得授资格。

分三步审：第一步只试点pp-literals且不改变现行闸；第二步审上述23个早段候选的独立纯核心执行路径；第三步再逐套件审外部合同字段的消费者。工具改变若使runner字节变动，先重验实现见证/确定性对拍再决定核心结果是否保留；不以“只是cc”直接复用。观测变化单独NOT_RULED，不能由核心绿替宿主义务判绿。

一次典型ABI小改在本文专指外部函数签名/布局字段变化、pp源/宏/头输入未变；一次宿主工具改动指oracle CC换版本。当前两者仍722重验；单叶获审后分别721与722整job；23叶全部获审后ABI699，工具改动核心699加受影响观测（未审仍按722上界）。这些是条件数与审批阶段，不是本刀实际减少闸项。

## 按流身份重验：第三批实证与条件估算

中段探针首红停止，见 research/c43-middle-flow-first-red.md。7000与3000实际P3解码的28个模型全部相同，1236条route/stage目录亦相同；两候选字节不同。因此候选字节变化确实不能等同模型变化，但还不能等同测试结果可复用：首个E3输入误取dump-tokens调试路由，缺定位帧，rc1。尚无一项新的中段流复用获证。

另做只读模型提取（没有续跑红的阶段探针）：codeprivate前的实验候选93ec722e11e6866b0fafc51836fb52aea0c5465daf9d07b38780e96b41221c95，build收据标018b4509；A+C后的3000候选85cc9b33，收据标cf7f594a。两包各28模型/1236路由，按(route,stage)比较，新增/移除0，变动120路由行，均为lower，涉及6对不同模型字节；E2/E1/E3/prune/elf等其余模型不变。Linux x86_64 image/O0的lower从dfc0b06c1f431a686ca9a851557f419d5ab578b93dbe3135477d61c55d495f35变为bbdb5695e641164855a0e3b10e9afad30de4b2abedc4734a0f4b20ef51075fe5。

数字原件 research/c43-middle-route-model-diff.tsv；提取器tests/modelstreamextract.cx，在产品-run下实际执行且验证网络解压长度/CRC。4ada2b91同时改run.c/libunisacc.c缓存和codeprivate两表，故不是隔离单变量实验；不能把所有候选差异归因于ABI。这里能回答的是历史改动后的模型变化确仅在lower，不是重建/测量所有历史源。codeprivate-manifest前后源码SHA分别0ecb501cdd1ef3f5c5d14697f296c0b4f4e101eb594c03374eb50cd66f2804a1与b9b3615d9b60a95f098ea5524087b024063be8e4544f5bd2443c73ef98b26eec。

722历史套件的结构审计候选：阶段类277（E2 14+E1 9+E3 142+E4 17+lower 23+E5 19+E6 53），chain179，可考虑显式流/DAG拆分共456；lib84+build74+meta50+unknown58共266暂保持完整验收。456是按现有标签产生的候选数，不是已证明可以纯流表达的数；目前中段成功接受并双候选对拍的数为0。lib中有原生运行/生命周期义务，不能把它们简单化成模型输出相等。

|口径|codeprivate一类lower模型变化的重验数|前提|
|---|---:|---|
|当前已授权实现|722|指纹未改，首红探针未证明复用|
|456项全部逐套件获审后的保守直接集合|468=266+23+179|277阶段/179chain均已拆合同，链的精确消费关系仍保守全取；pp/lex/E3/E4的完整输入和模型不变|
|下游流也变化时的保守上界|540=468+19+53|全部E5/E6均受影响；若lower输出哈希不变则这72项可保留，不能仅看elf模型没变|

未来实际数须从changed model/external-input的消费者开始，运行它们并按输出哈希固定点向下传播；复用的是同完整输入+模型的纯阶段见证，不是整套候选的内存/loader/宿主行为证据。一次普通ABI改动若影响头资源/宏/目标类型流，E2/E1/E3也进入集合，468并非所有ABI改动的常数。观测更换仍独立NOT_RULED/重验义务，不能靠核心绿免掉。

建议主路：优先接齐E1→E3定位/属性输入合同，证明E3和lower叶的纯接口；随后拆chain DAG，再研究lib的编译流与原生运行两类子证据。无需先把23早段叶全部做完才进入中段。上述468–540只是有明确资格条件的结构估算，零自动摘钩。
