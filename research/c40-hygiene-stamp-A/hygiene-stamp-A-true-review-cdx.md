# 卫生 A 独立内容真审（cdx；机房主任05:39授权）

审阅基线06855852，差集落点470b2b49，当前只读tip ac39786aee63290950e3b10ae65cbd03f5adf5e6。依据0506全文含收窄、0537、sa2/exec-delta.diff及三个helper完整源码。未改仓、未跑门、未调用生成器，未刷戳。

- [x] 1. 差集恰三个M：opt/pp/prune gen-delta.sh；完整diff各一行，100755模式不变，无新增删除。470→dbb→ac的exec/算法/声明差集空，材料适用于现tip。
- [x] 2. 三改动同构：均增加_td=${TMPDIR:-/tmp}，默认路径改${_td%/}/unisacc-seedbin及同行注释；参数、key、缓存校验、build与exec其余字节不变。_td无后续用途/输出，显式非空DIR原字符串保持。
- [x] 3. 语义已核：SG0在Python exec分支提前返回；SG1非空BIN走显式二进制分支；SG1无非空BIN在计算key成功后执行赋值，非空DIR不展开默认，空/未设DIR取默认。仅删除TMPDIR默认路径最后一个斜杠；本次/tmp/与/tmp指向同目录，key、SHA核验、失败出口及无Python回退逻辑不变。此项不泛证TMPDIR=/、双斜杠宿主语义或任意环境；这些边界未测，接受限定，不宣称普适目录身份不变。
- [x] 4. 执行路径已核：compilercheck.sh显式调pipeline/elf.sh；elf在STAGE_MODELS_READY非1时调models；models只有无有效缓存且实际prepare时才经bound调用prepare（cwd=ROOT/check=True），prepare三helper每次env覆盖非空OUT/.seed-gen-cache。chain的pp明确覆盖T/.seed-gen-cache；pp/run的fresh闭包中先mktemp得非空一次性d再设DIR调helper，失败则不调。这些固定消费者不会取默认cache分支，但仍执行赋值整行；不能泛称全部间接/外部PATH工具穷举，直接检查调用可走默认，动态外部工具不在此审域。
- [x] 5. 已知限定具名接受：受控slash/noslash各20/0只覆盖私有DIR与夹具归一，不直接证默认cache运行；修前红NOT_RUN；%/只去一个末尾斜杠；并发/原缓存两次mv窗口、完整工具身份、环境特殊路径未新增证明。无满门/gate-infra绿或提速结论。
- [x] 6. 独立只读复算候选：以git ls-tree -rz ac39786a列exec普通blob，按tip gatedeps后缀/排除目录筛选，git cat-file --batch读取字节，digest=[100644或100755,sha256]，按gatequeue同形json.dumps(sort_keys=True)再sha256。1411成员，映射与sa3原JSON全部相等；候选fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336，与记录e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34不等。只读脚本直接rc0，未写gatedeps。git不可变对象复算只适用于本次已锁定内容，不能替代一般脏树/未跟踪输入规则。

审核判断限定于上述三行卫生语义；写键须机房主任另裁，不自动恢复审计资格全部谓词，不授K5-1h或inventory变更。本文件由cdx独立审，未自称cdx2独审。

PASS 可进刷戳裁
