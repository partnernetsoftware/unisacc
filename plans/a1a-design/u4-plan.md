# 0.0.90 U4 汇编标签拆片

依据 `archive/plans-pre090/v0.0.89.md:8-27`、`plans/v0.0.90.md:10-22`、`prd_mempalace.md:225`、`plans/a1a-baseline.md` 与现行源码。两份仓外设计原稿 `a1a-design/d-u4-flow.txt`、`a1a-u4/locations.txt` 在约定路径缺失，作业目录和 /tmp 按文件名查找均未命中。以下写域是按现行源码重建的开工提案，计数是手改预算，非原作者已确认的清单。

## (1) 验收清单与先红证据

已装 0.0.89 实跑 `cx tests/check-a1a-u4.cx`，退出 1，`A1A_U4 pass=0 fail=8`，见 `evidence/out/red.out`。现行批 `a1a-u4` 是 PLANNED。冻结 `tests/a1a/expect.tsv` 保留 GCC 原始口径，运行检查不重调 GCC。

| 用例 | 0.0.90 验收 | 现有测试与缺口 |
| --- | --- | --- |
| a01，先声明标签后定义并直接调用 | lnx ELF 两 ISA 输出和退出码同冻结值，定义符号为 `renamed_sym` | 已有运行断言，缺对象定义名、旧名不存在、arm64 运行断言 |
| a02，外部声明改名 | 对 libc `puts` 的 UND/重定位与链接运行正确，不能留 `my_puts` 未定义引用 | 已有运行断言，缺中间对象 UND/重定位、跨两翻译单元与 arm64 |
| a03，变量改名 | 按已裁范围具名拒绝，非零且无产物 | 旧测试期待成功，须保留此样例并把这一格改为显式范围负例，GCC 冻结行不动 |
| a04，`__asm` 拼写 | 输出退出码同冻结值，函数符号为 `g_alt` | 已有运行断言，缺符号断言和 arm64 |
| a05，重声明标签不一致 | 拟要求具名拒绝不同标签，非零且无产物。加同标签重声明正控 | 已有弱拒绝，GCC 该例接受并警告。产品合同待确认 |
| a06，定义后新增标签 | 拟要求具名拒绝，非零且无产物 | 已有弱拒绝，GCC 该例接受并警告。产品合同待确认 |
| a07，块作用域标签 | 拟要求具名拒绝，非零且无产物 | 已有弱拒绝，GCC 该例接受。加块内函数声明负例以区分变量路径 |
| a08，标签参数非字符串 | 具名拒绝并定位，非零且无产物 | 已有弱拒绝。目前只断言失败且 stderr 没有旧 inline asm 文案，任意语法错会误绿 |
| 函数取址、间接调用 | lnx x86_64 链接运行 rc0，`nm` 有改名后定义且无原定义名 | 原检查缺。新增私有 `fixtures/address.c` 和验证器，先红已实跑 |
| 非 ELF 两系统四目标 | 合法函数标签的 `-c` 与 `-o` 均具名拒绝，定位在标签且无产物 | 现有测试无目标检查。私有补齐四目标 `-c` 先红。`-o` 待正式夹具接上 |
| inline asm 定位 | c10 指向 `u_asm.cx:3:列`，普通/volatile/顶层仍拒；标签拒绝原因与语句拒绝分开 | c10 为 KNOWN-GAP。已私有复现普通/volatile 无位置，现行 `check-asm-attr-reject` 对语句分支已有负控 |
| 记号与字节 | plain、typed、positions 三路旧 68 项码位保持，新增第 69 项；ordinary-id 与字符串/注释不误识别；合法 26 示例 `-o` 与 stderr 原样 | 缺 schema 负控、旧码位逐项对照、token/位置负控。以正式生成收据与公开件对拍补证 |

现有 `check-asm-attr-reject` 第 72 行把有效 ELF 声明标签也要求报旧 inline asm。同步把这条覆盖改成同一声明在明确不支持的目标遭具名拒绝，并在 U4 给 ELF 正控，保留其余 asm/attribute 断言。诊断以定位、非零、无产物和核心文案同时判，超时/外层拒绝不算命中。arm64 编译/符号与运行分栏，运行用相应原生执行或 qemu，缺运行材料记 UNMEASURED。

私有加测使用 `probes.tsv` 与 `check-probe-result.cx`。从仓根用 `devsvc flow "$W/probes.tsv" --par 3 --deadline-ms 50000 --run "$W/probe-next"`，然后 `cx "$W/check-probe-result.cx" -- "$W/probe-next" "$W/fixtures" 'asm label unsupported for target'`。W 为本目录。每轮用新输出目录并先清空私有 fixture 的旧产物，避免上轮对象遮蔽无产物断言。当前 `probe-before-judged.log` 实跑 0/7。该平台文案是提案，发布前按批准合同锁定。

## (2) 15 分钟分片与写域

T0 是 0.0.90 固定产品基线和 token/诊断接口获确认后。每片到截止即交 diff 和已实跑原行，构造和复测独立拆窗。表内仅列准写 A/B 路径，未列 A/B 路径冻结；C 类准备与派生按表后另列。

| 片 | 目标 | 准写 A/B 路径 | 验收命令与预期 | 截止与禁区 |
| --- | --- | --- | --- | --- |
| U4.1 | E1 识别同一标签记号，锁定新增码位和位置三路 | A 4，`weights/gold/parse.tsv`、`weights/gold/lexword.tsv`、`exec/lex/output-manifest.tsv`、`exec/lex/f1-template.tsv` | `cx tools/facts-gen.cx -- --check . lex-gen` rc0。跑正式 jobs-gen 中 E1 相，plain/typed/locations 各 rc0。token 对照旧 68 项逐项同，新加一项，两种拼写同码 | T0+15m。旧码位不可重排，禁止把裸 asm 语句或 volatile 变成成功 |
| U4.2 | 接 E3 token 前缀、常量与标签字符串输入形状 | A 3，`exec/parse/token-prefixes.tsv`、`exec/facts/parse-constants.tsv`、`exec/parse2/strings-byte.tsv` | `cx tools/facts-gen.cx -- --check . k2-gen2-tokens k2-units-tokens k2-gen2` rc0。真实 parse2 设备生成器定位配方 rc0，字符串/非字符串探针分流 | T0+15m。可与 U4.1 并行写，先约定码位和位置格式。两边合齐才做端到端 |
| U4.3 | 声明绑定和定义标号，覆盖先声明再定义与局部边界 | A 1，`exec/parse2/control-result.tsv` | `cx -c tests/a1a/cases/a01_asmlabel_def.c -o A01.o` rc0，`nm A01.o` 有改名且无旧定义。a05/a06/a07 按裁定报错并无对象 | T0+30m。依赖 U4.1/2 的输入合同。只从同一 C 符号绑定查 label，禁止每个发射点各维护一份改名表 |
| U4.4 | 直接调用、外部引用、取址三条统一消费 | A 2，`exec/parse2/callcontrol-result.tsv`、`exec/parse2/update-result.tsv` | a01/a02/a04 经 `cx tests/check-a1a-u4.cx` 对拍。对象级 `nm/readelf -r` 核 UND/reloc。私有 address 运行 rc0 且符号检查 PASS | T0+30m。可和 U4.3 并行写，事先固定绑定槽，运行验证待 U4.3 输出。禁止改 ELF 后端、绕过符号绑定及扩展变量改名 |
| U4.5 | 具名诊断与位置，锁平台门 | A 1，`exec/parse2/errors-template.tsv`。平台/作用域分流行由 U4.3 写手在其表内实现 | `cx tools/check-diag-contract.cx -- --live` 让 c10 变 PASS，`CHECK_DIAG_CONTRACT failed=0`。非 ELF 四格只报合同文案，无 .o/镜像，普通与 volatile 仍拒 | T0+30m。可与 U4.3/4 并行写，先固定拒绝标签；禁止从后端通用链接错冒充平台门 |
| U4.6 | 受影响生成配方、六目标编译、Linux 两 ISA 运行、全选集对拍 | A/B 0，只读。材料与日志仓外 | 真实构造的 jobs-gen 全相 rc0，`cx tools/check-diag-contract.cx` passed，`A1A_U4 pass=8 fail=0` 逐目标正式断言与消费方都核到；26 合法示例对拍不变；断言计数与时限不改 | T0+45m，必要时按材料、目标、对拍分多个 15m 窗；无实际完成证据不记绿。不得为赶窗减断言或放宽上限 |

上表为 11 个手改 A，B 为 0。保留第 12 个 A 名额，仅在真实生成配方需要时，报准一个确切的 manifest 或 spelling 路径，例如 `exec/lex/spelling.tsv` 或 `exec/parse2/gen2-manifest.tsv`。两者同时需要或有新表/后端路径时，重新核封顶后再写。纯自动派生实际差分另列，例如 `exec/facts/lex-gen.tsv`、`exec/lex/attr-byte.tsv` 与 k2 投影，禁止手改；如 volatile 分支必须从 lexer 模板改路由，C 类 `tools/lex-attr-gen.cx` 由 U4.1 独占，并用 `--check` 重算 attr-byte。C 类验收写域由一人负责现有 U4/asm/diag 检查、`tests/a1a/cases`、对应批次登记/manifest。门的 `a_files` 只数已提交差分中有实际车道 A 行且不在 exec/kernel 的路径（`tools/chain-ready.cx:813,918-955`），本提案手改口径与该门分开核。当前清单中非 exec A 仅两个 weights 源，最终以分支相对公开标签的真实差分合并去重。官方计划 cap=16 还须计同版别项与实际派生路径。

## (3) 需要裁定与已定边界

(1) lnx ELF 函数改名范围已经定案。尚请定 a05/a06/a07 是否一律采用上述更严拒绝，以及是否接受同标签重复、合法标签字符集和不同 C 名指向同一 ELF 名的碰撞行为。建议允许同标签重复，限定单个普通字符串，有歧义具名拒绝。a03 变量改名保持范围拒绝。

(2) `__asm__` 与 `__asm` 两拼写已经定案。裸 `asm` 在语句位置保持现行拒绝，声明位置是否也作为第三个扩展拼写请裁。预算按仅交两拼写。旧的标识符回退、注释/字符串及关键字前缀须回归。

(3) osx/win 不接收合法 ELF 标签，范围已定。请确认核心文案 `asm label unsupported for target` 与诊断优先级。建议合法形状用平台原因，非法语法用本身具名语法错；四目标 `-c/-o` 一致。

(4) inline asm 拒绝接位置已定。建议普通、顶层、volatile 仍保留核心 `not covered: inline asm`，位置锚在实际 asm 关键字，c10 由 KNOWN-GAP 转 PASS。若定位需要多个新 A 路径，先核 12 路径预算。

(5) `tok_names` 68 -> 69 已定，建议附在末尾保留旧值。实际码名及 alias 合流方式由实现者据生成约束固定并写收据；词法全量失效是预期，验收要求是可复算和既有合法程序字节不变。

本轮只写私有拆片和先红测试资产。未应用产品补丁，未修改版本分支或登记表。
