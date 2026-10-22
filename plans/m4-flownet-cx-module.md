# 需求：把 m4 的「流×网」范式做成可复用 `.cx` 模块

> 提出：2026-10-23（董事长要求）· 执行：cc（cc-unisacc）· 状态：W0 已裁定（三问皆是；W2 读外部 `.tsv`，生成 `.cx` 表为备选；API 只导出函数）；W1 基线已落（`tests/flownet-baseline.tsv` 371 行，连跑两次逐字节相同，8 个闸步）；W2 已落（`flownet/{contract,nets,flow,api}.cx` + 边界适配器 `adapter/ujswasm/`，D1 成立、D2 33/33 逐字节 + 迹相同、acc 10/10、gold 回退 0、D5 11 例具名拒绝，7 个闸步）；W2 补 D5 已落（字面量越界/化 inf 在词行具名拒绝 + wat2wasm 末道收成一行，refuse 18 例、accepts 加 2 个边界）；W2 补 2 已落（静默截断一类根除：flownet + 适配器全部文本改为按长分配，d1 步加 bounds 扫描；accepts 11 例含 4 个长文本、refuse 22 例）；W3 已落（标签源 + 发射钩子为注入面，flownet 五单元改 `use` 模块并能自建整数网；第二消费方 `tests/tdescprobe.cx` 经 flownet 推导 106 探针，5 个 tdescprobe 闸结果逐字节不变；D3 变异 5/5 红、D4 无 python 进程，4 个闸步）；W3 补已落（验证方反驳：把 fn_ask 换成本地标签函数后闸仍绿——闸只验脚手架、不验网被问；新增 API `fn_expect(BOOKS)` 钉住一次运行的逐相位逐网命中数，不符即在钩子前具名拒绝；tdescprobe 钉 adj 77 / ap 283 / sz 326，D3 变异 6/6 红，tdescprobe 闸结果不变）；W3 补 2 已落（验证方再反驳：账目钉只证网被问、不证答案被用——问了再丢掉答案改用本地标签仍绿；新增 flow-through 闸：tdescprobe 对一份带运行时故障开关的 scratch flownet 编译一次，无故障绿、adj/ap/sz 各自答错皆红、sz:8:4 在 oracle 报 MISMATCH；自证 3/3：丢弃全部答案、只丢 sz、换问且解钉三个副本各被抓并点名其使之成摆设的故障；2 个闸步）；W3 补 3 已落（验证方第三次反驳：探针 A6 保留少数答案、其余用本地标签仍全绿——根因是消费方持有标签副本且整网故障太粗；改架构：TDESC 代数移入 flownet 表 `flownet/tdesc/`，新 API `fn_construct(DIR)`，tdescprobe 不再有任何标签；域收紧为推导恰问的 46 键（旧 80 键中 34 键从不被问，实测旧消费方 46/80）；flownet 测试钩 `FLOWNET_SWEEP` 逐键逐类扰动一个答案并交发射钩子判定，tdescprobe 46/46、150 次扰动 0 接受，自证 4 个自身闸绿的副本各被抓且逐键点名；tdescprobe 闸结果不变，oracle 加一行全空间 cc 判定），待 W4
> 依据：`m4/README.md` 实测、`research/m4-flow-net-contract-draft.md`、`m4/flows.py` / `nets.py` 现状

## 0. 一句话

m4 已经证明「gold → IntNet → 四流 → 字节」这条链能逐字节复现；本需求要把它从 **ujs 的 Python 适配层**变成 **不依赖 ujs、可被第二个消费方复用的 `.cx` 模块**。

## 1. 现状与缺口（实测，非推断）

| 项 | 现状 | 缺口 |
|---|---|---|
| 依赖 | `flows.py` 直接 import `ujs.seed.construct.front.compile.compile_src` / `emit_wasm` / `Oracle`；`nets.py` 吃 `ujs.seed.construct.gold.STAGES` | 硬绑定 ujs，无法移植 |
| 消费方 | 全仓只有 `m4/__main__.py`、`m4/selftest.py` 引用 | 只有自己用，可复用性零证据 |
| 覆盖 | 28 个 ujs 语料，未覆盖即报错 | 覆盖率不足 |
| 决策网 | 10 网中仅 lex/parse/isel/enc 真决策；type/shape/ic 是观测，scope/irsel/reloc 保留 | 6 张网不参与推导 |
| 语言 | Python | 与 .cx 工具链（gate 检查器已 .py→.cx）脱节 |
| 流边界 | 流 1+2 共用 `compile_src`，流 3+4 共用 `emit_wasm` | 四流不是四个独立调用点 |

## 2. 目标定义：什么叫「可复用模块」

满足全部五条才算：

1. **依赖倒置**：模块不 import 任何 ujs/Python 符号；外部通过注入提供「标签源」与「发射钩子」。
2. **单一入口**：对外只暴露一个 `.cx` API 面（构造 / 推导 / 验收 / 记账查询），内部结构可自由重构。
3. **第二消费方**：至少一个**非 m4** 的调用方在闸内使用它，且行为可判。
4. **自举**：在 m9（当前 root 产品）上编译、运行，不依赖 Python 运行时。
5. **拒绝域完整**：未覆盖输入必须具名拒绝，绝不静默回退 gold。

## 3. 验收判据（硬，可机检）

| 编号 | 判据 | 检查方式 | 失败即回退 |
|---|---|---|---|
| D1 | `flownet/` 目录内不出现 `ujs` 符号 | `grep -rn "ujs" flownet/ --include=*.cx` 为空 | 本波不合并 |
| D2 | 字节 parity 不退化 | 28 语料 → 与 ujs gold 逐字节相同 28/28；acc 10/10 全键 exact；gold 回退计数恒 0 | 本波回退 |
| D3 | 第二消费方真实调用 | 该消费方在闸内跑通，且**移除模块调用后必须红**（变异证明，非摆设） | 本波不算完成 |
| D4 | m9 root 自举 | 在 m9 上编译 + 运行；不调 Python；无 `python3` 进程 | 本波回退 |
| D5 | 拒绝域 | 每个未覆盖语料：rc1、stdout 空、不产出文件、stderr 点名原因与位置 | 本波回退 |
| D6 | 单步 ≤60s | 每波新增的闸步都 ≤60 秒看门狗；整波可逐步复现 | 拆步 |

## 4. 工作流（W0–W5，每波独立可回退）

### W0 —— 可行性裁定（**先做，不做完不写代码**）

只需回答三个问题，用最小探针，不碰产品：

| 问题 | 探针 | 若否，方案调整 |
|---|---|---|
| `.cx` 能否表达 10 张网的键空间（合计约 3.6K 键：lex 100 / parse 330 / type 1863 / scope 48 / shape 54 / irsel 117 / ic 975 / isel 42 / enc 144 / reloc 4） | 生成一张最小的表驱动 `.cx`，在 m9 上编译并跑通 acc 的一个子集 | 改为「按需构造 + 稀疏表」，或把网降级为纯函数形式 |
| `.cx` 有无文件读写，能否读外部 gold 表（`.tsv`） | 一个 `.cx` 读表 → 打印；确认路径/错误处理 | 把 gold 表**编译进** `.cx`（生成源码），代价是编译时间 |
| `.cx` 有无模块/库引用机制（A.cx 用 B.cx） | 两个文件的最小示例，经 `commitgate` | 合并为单文件，用命名前缀分区 |

**产物**：`research/flownet-w0-feasibility.md`（三个结论 + 探针路径 + 实测输出）。
**闸**：探针各自可复现、≤60s。
**W0 结论（2026-10-23 实测，详见产物）**：①是——生成的表驱动 `.cx` 在 m9 上编译 2.85 s，10 网 3677 键穷举 10/10 exact（acc 子集与全量皆过，变异即红）；②是——单文件 `.cx` 只读外部 `.tsv`（网参数 + gold）即 10/10 exact，缺文件/目录/坏行均「路径[:行]: 原因」rc 1；③是——多翻译单元即模块机制（`unisacc.com A.cx B.cx`），入树探针 `tests/flownetw0.cx`+`tests/flownetw0mod.cx` 两个闸步经 commitgate，去掉 B 即 rc 127 具名拒绝。实测约束：未定义的外部**对象**被当作 0 读出不拒绝，故 flownet API 只导出函数、`fn_` 前缀。按 §8-3：不降级，W2 输入选运行时读 `.tsv`。

### W1 —— 行为基线快照（纯测量，零改代码）

- 固化当前 m4 行为：28 语料的输出 SHA256、acc 逐网 (exact,total)、trace 记账、selftest 的 EXPECT 表。
- 落 `tests/flownet-baseline.tsv` + 一个 `--check` 步骤。
- **闸**：基线自身稳定（连跑两次逐字节相同）。
**W1 结论（2026-10-23 实测）**：`tests/flownet-baseline.tsv`（371 行，sha256 前缀 `c451375c083305bd`）由 `tests/flownetbaseline.cx` 经 m4 oracle（`python3 -m m4 …` 作子进程，m4 零改动）写出，8 节：`acc`（10 网 exact == total，合计 3677 键）、`out1`–`out4`（28 语料 + 5 探针，按 m4 次序连续四分片：源 sha256、输出字节数与 sha256、trace 逐阶段逐网 (hits, gold) 共 253 行，gold 回退全 0）、`check`（same 33/33、负例 9：拒 4、已知接受 5）、`selftest`（isel choose 证明 + 10 网决策种类，PASS）、`expect`（从 `m4/selftest.py` 源码直读 EXPECT，不跑 Python）。整份连写两次逐字节相同（`cmp` 通过）；`--check PART` 即逐节重算对拍。8 个闸步：`flownet-baseline-file`（无 Python，整文件形状 + 不变量：acc 全 exact、gold 回退 0、check same == 输出且字节数一致、selftest decides == EXPECT、语料源未变；contract 层）与 `flownet-baseline-{acc,out1..4,check,selftest}`（oracle 重算 == 记录；pipeline 层）。单步墙钟：file 约 3 s、acc 约 8 s、每个 out 片约 18 s、check 约 16 s、selftest 约 40 s（最紧，oracle 子进程限 50 s、外层 `tests/bound 58`）。变异即红：输出 sha 改一位、trace hits +1（`--check` 点名节与行）；gold 回退置 1、EXPECT 去掉 trap、acc 1862/1863、删一条 same（`--check-file` 各自点名）；缺表 rc 1 具名。未登记 `release/derived-map.tsv`：它要求套件先在 `tests/gatedeps.json` 声明，本波不碰 gatedeps，`flownet-baseline-file` 每次闸跑都会抓语料/EXPECT 漂移。

### W2 —— `.cx` 骨架 + parity（仍不接第二消费方）

- 新增 `flownet/{contract,nets,flow,api}.cx`，语义对齐 m4 四模块。
- 输入仅来自 W0 定的表机制；**此波允许临时适配器**（读 W1 基线做对拍），但不得 import ujs。
- **闸**：D2 全绿（28/28、acc 10/10、gold 回退 0）；D1 已成立。
**W2 结论（2026-10-23 实测）**：`flownet/` 四个单元只认表、不认语言：`contract.cx`（读 `contract.tsv`：网名单、流→决策网、reserved/observed、choice、记账阶段；三类恰好划分即 check_contract）、`nets.cx`（每网读 `NET.gold.tsv` 标签源 + `NET.net.tsv` 整数网，域/类序两文件互校；整数前向 + 全键 acc）、`flow.cx`（ask：键不在域内即具名拒绝；built 驱动记 hit、gold 驱动记 gold 回退；按阶段记账；gate 合法集；迹检查 gold 回退 >0 即红）、`api.cx`（唯一对外面，只导出 `fn_` 函数，错误「路径[:行]: 原因」）。ujs 侧全在边界外 `adapter/ujswasm/`：`ujswasm.cx` 把 m4 路径（ujs 前端 lex+compile、m4 emit 的 isel 分派/enc 门控与 WAT、wat2wasm）移植为 .cx，**每个决策都经 `fn_ask*`**（含 m4 的副信道询问与 Compiler 重词法的记账）；`tables.cx` 写表：contract 由 `m4/contract.py` 转为数据，gold 由 `ujs/.../gold.py` 的 .cx 移植给出，整数网由既有 CLI `python3 -m ujs build-weights`（oracle 子进程）构造后转文本。实测：acc 10/10 exact（3677 键；网是 Python 构造、gold 是 .cx 移植，两者全键一致即证移植正确）；W1 记录的 33 个程序（28 语料 + 5 探针）经 flownet 输出与 ujs gold 记录**逐字节相同 33/33，且每程序迹（阶段、网、hits、gold 回退）全同**，gold 回退 0；9 个负例拒 4 / 接受 5 与记录一致，5 个接受者与 `python3 -m m4 build` 逐字节相同。D1：`flownet/*.cx` 无 ujs、无 Python。D5：11 个未覆盖输入（keys/in/spread 发射缺口、`%=` 越域、字符串外非 ASCII、坏 UTF-8、未终止串、循环外 break、嵌套函数、缺源、目录源）各 rc 1、stdout 空、无输出文件（预置的旧 OUT 也被删）、stderr 一行点名原因与位置。变异即红：gold 驱动（回退 14 次）、缺 `enc.net.tsv`、isel 网变异（acc 3/42 且构建被 enc 门控拒）、lex 网变异、enc gold 标签翻转（acc 143/144）、基线 sha/迹/负例/acc 改一处、表文件改一字节、flownet 内写入 ujs。7 个闸步：`flownet-w2-{d1,tables-gold}`（contract 层）与 `flownet-w2-{parity,refuse,negative,accepts,tables}`（pipeline 层；accepts/tables 跑 oracle 子进程，其余无 Python），单步 3–22 s。如实记下的边界：(1) 字符串外的非 ASCII 只覆盖拉丁/希腊/西里尔/CJK/谚文字母区段，其余码点具名拒绝（Python 的 isalpha 全表未移植）；(2) f64 字面量以规范化十进制文本交给 wat2wasm（值相同即字节相同，非 Python repr 逐字）；(3) isel 的 eq/ne 第二形态（f64_cmp）已接线（`fn_choice` + 门控）但本波未做选择探针，留 W4；(4) 未登记 `release/derived-map.tsv`（要求 gatedeps 声明，本波不碰 gatedeps），表的派生一致性由 `flownet-w2-tables(-gold)` 每次闸跑核对。

**W2 补 D5（2026-10-23 实测，验收方驳回 D5 后补）**：缺口是「过了前端、被 wat2wasm 拒」的输入——`return 1 + 99999999999999999999;`、309 位十进制 `.0` 化为 f64 inf——旧适配器把 wat2wasm 约 10 行带色诊断（含临时路径）原样透传，且无 SRC:LINE。两道补法：(a) 适配器在发射常量处按十进制数字精确比较（不用 strtod）：整数 > 2^64−1（wat2wasm 1.0.39 的 i64.const 上限；2^63..2^64−1 照 oracle 的 `int()` 文本回绕，仍接受）、浮点整数部分 ≥ 2^1024−2^970（DBL_MAX 加半 ulp，平局入偶即 inf）即 `SRC:LINE: integer literal … exceeds 2^64-1` / `float literal … rounds to infinity` 一行拒绝；(b) wat2wasm 的 stderr 改为捕获（fd 2 换到暂存文件，`cx_runv_timeout` 只收 stdout），去 ANSI、去路径，取第一条 `error:` 成一行 `wat2wasm refused the generated WAT (rc N) at WAT L:C: …`，超时/不可执行各自具名，作末道拒绝。如实记：g17 这类化 inf 的浮点，m4 oracle 写 `f64.const inf` 并**接受**，flownet 具名**拒绝**（拒绝，不是静默不同）；(b) 的位置是生成 WAT 的行:列，不是源行。闸：`flownet-w2-refuse` 由 11 例增至 18 例（+整数越界于表达式/恰 2^64/case 标签三例、浮点 inf/恰平局带前导零两例、wat2wasm 真拒（PATH 上的包装把一个字面量改坏后交真 wat2wasm，带色带路径的诊断被收成一行）与缺 wat2wasm 两例），每例 rc 1、stdout 空、无输出文件（预置旧 OUT 被删）、stderr 一行点名原因，15 例带位置；`flownet-w2-accepts` 加 2 个边界接受（2^64−1、2^1024−2^970−1 带小数与前导零）与 `python3 -m m4 build` 逐字节相同。变异即红：去掉 (a) 的检查 → 5 例原因不符（此时 (b) 仍把它们收成一行）；不换 fd 2 → wat2wasm 例非一行、带 ANSI 与路径。D1/D2 不变：d1 PASS、parity 33/33 逐字节 + 迹同、acc 10/10、gold 回退 0、negative 6/6。

**W2 补 2（2026-10-23 实测，验收方驳回 W2 补 D5 后补：静默截断）**：缺口是固定长缓冲——`emit_wat` 以 `char t[512]` + snprintf 写 `(f64.const %s)`，小数位 ≥500 的浮点字面量（`return 1.<500 个 3>;`，1000..6000 同）被截成坏 WAT，wat2wasm 拒，而 m4 oracle 接受。按类修，不按例：(1) flownet 四单元与适配器两单元（`ujswasm.cx`、`tables.cx`）里所有「输入定长度的文本进定长缓冲」——snprintf/strcpy/strcat 进 `char m[N]`、`%.60s` 一类精度截断、路径 `char path[1024]`、`realpath` 进 2048 字节静态区、契约名 64 字节表、choice 512 字节、`fn_list` 截一半——全部改为按长分配：flownet 用 `fnx_fmt`/`fnx_failf`（api.cx，唯一格式化器），适配器用 `xf`/`ppf`/`refusef`/`erefusef`/`badf`（同一 XF 宏）；名称整串复制；`fn_list` 容量不足时输出空串并具名失败（报所需字节数），不再给半截表。剩下的硬限全是计数（网/流/阶段 32、choice 32、每键字段 4、每行列 4096 等），各在 PATH:LINE 具名拒绝；剩下 6 个定长 char 数组都按构造有界（UTF-8 四字节、一个 i32、`\xx` 转义、sha256 十六进制 65、mkdtemp 模板 19，模板长度在填入处再核），每个都在同行写 `bounded:` 注明界。实测附带发现（产品缺口，不在本需求内修，红线 4）：m9 libc 的 `vsnprintf(NULL, 0, "%s", s)` 在 s ≥255 字节时挂死（snprintf 同参不挂；带缓冲的 vsnprintf 正常），故格式化器先写入 256 字节堆缓冲、按返回长度重试一次；`sizeof "字面量"` 不是常量表达式（m9 拒），故模板缓冲写成 19 并在运行时核长度。(2) 防回归：`flownet-w2-d1` 步（命令不变，未改 `tests/gate.sh`）加 `ujswasm-bounds` 扫描 flownet/ 与 adapter/ujswasm/ 的全部 `.cx`（注释与字面量内容置空后）：printf-到-缓冲/字符串拷贝族（vsnprintf 除外）、未写 `bounded:` 的定长 char 数组、字符串字面量里的 `%.N` 精度，任一即 FILE:LINE 红；扫描先自测 9 个样例全中才算数。对 HEAD（补 2 之前）的源扫出 332 处。(3) 闸内探针：`flownet-w2-accepts` 由 7 增至 11（+ 小数 500 位、`0.0` 后 6000 位、700 字符标识符、200000 字节字符串），全部与 `python3 -m m4 build` 逐字节相同；`flownet-w2-refuse` 由 18 增至 22（+ 700 字符名的嵌套函数——拒绝行须含整个名字、6000 位整数越界、6000 位整数部分浮点化 inf——带显式「前 24 位...（位数）」缩写、20 万字节未终止串），每例 rc 1、stdout 空、无输出文件、stderr 一行带原因，19 例带位置。变异即红：补 2 之前的适配器对小数 500/6000 位两例 wat2wasm 拒（m4 接受）、对 700 字符嵌套函数名只报前 60 字符；bounds 扫 HEAD 源 332 处。D1/D2/D5 不变：d1 PASS、parity 33/33 逐字节 + 迹同、acc 10/10、gold 回退 0、negative 6/6、tables(-gold) 与新做一致。单步墙钟：accepts 约 29 s（最长，含 11 次 oracle 子进程）、check 约 18 s、negative 约 12 s、refuse 约 12 s、d1 约 11 s。

### W3 —— 接口反转 + 第二消费方（可复用性的唯一证据）

- 把「标签源 / 发射钩子」变成注入面；写 ujs 侧**薄适配层**（放在 `m4/` 或 `adapter/`，允许 Python 或 .cx）。
- 选一个既有 gate 检查器（建议 `inventory` 或 `auditnet`）改用 `flownet` 做一次真实推导。
- **闸**：D3（移除调用必须红）+ D4（m9 自举）。
**W3 结论（2026-10-23 实测）**：(1) **注入面**。`flownet/fnx.cx`（新，基模块：唯一格式化器 + 唯一失败汇 + 注入类型）定义 `fn_domain_fn`/`fn_label_fn`（标签源：每网的域文本 = gold 表头的 field/class 行，每键一个标签，构造时每键只问一次）与 `fn_flows_fn`/`fn_emit_fn`（流与发射钩子）。API 新增 `fn_label_source(DOMAIN, LABEL, CTX)`（注入；`(0,0,0)` 退回存货源 `DIR/NET.gold.tsv`）、`fn_build(CONTRACT_TEXT)`（契约是文本、gold 来自注入源、**整数网由 flownet 在 .cx 内自建**：minterm 构造——恒亮单元给默认类投 1，每个非默认键一个单元 b1 = 1−字段数、投其类 2，构造即精确，仍逐键 acc，行文本走 `NET.net.tsv` 同一读取器；不求最小）、`fn_out` + `fn_run(FLOWS, FCTX, EMIT, ECTX)`（重置记账 → 跑消费方的流 → 迹检查（gold 回退 0）→ 才调一次发射钩子；被拒或回退 gold 的一次运行永远到不了钩子，即 m4 flows.run 的「编译、检查、再发射」）。`fn_open(DIR)` 在有注入源时 gold 取注入源、网取 `NET.net.tsv`。(2) **模块化**。flownet 五单元改为 `#pragma unisa use` 模块（DAG：fnx ← contract、nets ← flow ← api；各单元不再手写他单元原型；`FNN_MAXF` 改 enum 以入 CXIF），消费方 `#pragma unisa use flownet.api` 即得 `fn_` 原型与注入类型。v1 解析只认「使用者所在目录」，故以逐模块文件链接接入：`tests/flownet/*.cx -> ../../flownet/*.cx`、`adapter/ujswasm/flownet/*.cx -> ../../../flownet/*.cx`（各 5 个；`cx_walk` 用 lstat 跳过链接，d1/bounds 扫描与 inventory 不重复计数；`research/cx2-module-design.md` 延后的 `-cxroot` 落地后可去链接）。实测：整目录链接会被 commitgate 的 `release/tools/derivedcheck.py` 拒（导出工作树时 `read_bytes` 一个目录链接即 IsADirectoryError；该工具不在本波可改路径内，如实记下），文件链接导出为普通文件副本，`inventory-derived`/`script-inventory` 在导出树上仍绿。(3) **ujs 侧薄适配**（边界外 `adapter/ujswasm/`）：gold 移植从 `tables.cx` 拆为模块 `ujsgold.cx`（tables.cx 与 ujswasm.cx 共用；`flownet-w2-tables-gold` 逐字节不变），`ujswasm.cx` 只做注入与运行——标签源 = `.cx` gold 移植（`fn_label_source(ujsgold_domain, ujsgold_label, 0)`）、整数网 = oracle 的 `NET.net.tsv`、流 = ujs 前端 + m4 select/encode（`fn_out` 写 WAT）、发射钩子 = wat2wasm → `\0asm`；5 条 W2 闸命令改为 `sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- CMD .`（不再列 flownet 单元）。D2 不退：parity 33/33 逐字节 + 迹同、acc 10/10（此时 gold 来自注入源，故「移植 == gold.py」在 3677 键上仍由 acc 证）、refuse 22、accepts 11（约 38 s）、tables(-gold) 一致、d1 PASS（5 个 flownet 单元无 ujs/Python，bounds 8 个文件）；negative 6 → 7（+ 注入标签源里翻一个 enc 标签 → acc 143/144 红）。(4) **第二消费方** `tests/tdescprobe.cx`：TDESC 代数改为三张有限网——adj（构造子可否装另一构造子，12 键）、ap（对某位置施 `*`/`[`：下一位/原位/非 C，32 键）、sz（构造子尺寸由内层尺寸定，36 键，超域为 over 具名拒绝）——其 gold 是本文件注入的标签源，flownet 自建整数网、逐键验收；每个声明符的合法性、每个上下文的位置、每个尺寸都问网（built 驱动）；流按探针写行，**发射钩子在账目干净后把行交出，P[] 别无来源**。原 `sz()/ap()/valid()` 三个纯函数删去（其规则成为标签源）。`--check` 加一条：基线行数须等于推导出的探针数（防「少推导即空绿」）。实测：`--baseline` 三分片输出与改前逐字节相同（106 行）；`tdescprobe-oracle`、`tdescprobe-classic`（$UA）、`com-tdescprobe-1..3`（按 `tests/bound 58 sh ./unisacc.com -run tests/tdescprobe.cx -- --check ./unisacc.com tests/tdescprobe-product.tsv WORK K/3` 手跑；commitgate 不带 --com）stdout 与 rc 改前改后逐字节相同；com 分片墙钟 24–26 s（改前 18–22 s）。`#include <cx.h>` 删去（棘轮 2 → 1），`<fcntl.h>` 保留（root m9 prelude 无 O_WRONLY/fileno）。(5) **D3**（`tests/flownetw3.cx d3`，两半各 ≤35 s）：在 scratch 复制 tdescprobe.cx 并旁置 flownet 链接，原样 `--oracle cc` 绿；5 个变异各红且点名——去掉 `use flownet.api`（rc 1，`undefined function 'fn_label_source'`）、去掉 `fn_run`（rc 2，derived no probes）、去掉 `fn_build`（rc 2，no tables open）、插入 `fn_drive("gold")`（rc 2，adj 回退 gold 77 次）、改一个注入标签 P→4（rc 1，oracle MISMATCH）；每个变异是须恰好命中一次的精确编辑，tdescprobe.cx 变了即本检查红。(6) **D4**（`tests/flownetw3.cx d4 tdesc|adapter`）：在 root m9 上跑消费方的闸命令，同时每 2 ms 列 /proc、收集运行的全部子孙进程，名或 argv[0] 以 python 起头即红；先自证（sleep 被看到；sleep 复制成 python3 被判红）。实测：tdescprobe `--oracle cc`（14 个子孙进程）、`--check 1/6`（单跑 1/3 时 49 个；闸内取 1/6 留墙钟余量）、ujs 适配器 check（86 个）均无 python。4 个新闸步：`flownet-w3-d3-1/-2`、`flownet-w3-d4-tdesc/-adapter`（pipeline 层，单步 22–41 s）。如实记下的边界：(a) 适配器的整数网仍是 Python oracle 构造的表（为与 m4 逐网对齐）；flownet 自建网的路径只由 tdescprobe 使用；(b) `cxinclude` 闸在 HEAD 已红（`tests/cx/p1.cx` 注释里的 `#include` 字样无基线行，非本波），本波只使其 tdescprobe 行下降；(c) 未登记 `release/derived-map.tsv`（无新派生文件）。

**W3 补（2026-10-23，独立验证方反驳后）**：验证方把 tdescprobe 的 `ask()` 里 `fn_ask3/fn_ask2` 换成直接调本文件的 `tdesc_label`，`--oracle cc`（106/0）与 `--check 1/6`（changed 0）仍 rc 0——flownet 只剩相位簿、行缓冲与发射钩子，原 5 个变异都只拆脚手架（need() 或发射路径失败），迹检查只看 gold 回退 > 0，**没有任何东西要求网真的被问**。修为一类：flownet 新增注入面式的账目钉 `fn_expect(BOOKS)`（`flownet/flow.cx` `fnf_expect` + `books_match`）：消费方给出一次运行应记的全部 `PHASE\tNET\tHITS` 行（迹行顺序），`fn_run` 在迹检查之后、钩子之前逐行比对，任一行不同（某网没问、多问少问、换了相位）即具名拒绝并报第一处差异；`0` 解钉，不钉则行为不变（适配器另有逐程序迹记录，不受影响）。tdescprobe 钉 `declare adj 77 / context ap 283 / size sz 326`（实测账目）。D3 加第 6 个变异「nets answered locally」（即验证方的探针 A：`fn_ask*` 换成 `tdesc_label`），实测 rc 2、`books: the run booked 0 row(s); the consumer pinned more, next declare adj 77`；另手测只把 sz 换成本地答也红（`booked 2 row(s) … next size sz 326`）。`flownet-w3-d3-2` 改为 3 个变异（实测 27–36 s）。复跑全绿：tdescprobe-oracle、tdescprobe-classic、flownet-w2-d1/-parity/-refuse/-negative/-accepts、flownet-w3-d3-1/-2、-d4-tdesc/-adapter；com-tdescprobe-1..3 按手跑命令 rc 0、changed 0（`gate.sh --com` 此刻被产品新鲜度拒——他人会话改了产品输入，本波不得重建 unisacc.com）。顺修 `tests/gate.sh` 两条过时注释（d1 五单元含 fnx；refuse 22 例）。

**W3 补 2（2026-10-23，验证方第二次反驳后）**：验证方的探针 A2（`ask()` 照常调 `fn_ask3/fn_ask2`，随后丢掉答案改返回本文件 `tdesc_label`）在 `--oracle cc`（106/0）与 `--check 1/3`（changed 0）上仍绿；对照：flownet 副本把 sz 的 `8` 答成 `4` 时诚实的 tdescprobe 红（51 处不符），A2 副本仍 106/0；探针 A3（换问 + 去掉 `|| fn_expect(BOOKS)`）也绿。根因：账目钉在消费方内部，而消费方持有建网所用标签的本地副本——计数相同的变异、或去掉钉的变异，在消费方内部与诚实程序不可区分；`flownet/flow.cx` 注释「the nets' answers are proven used」因此不实，已改为如实表述（账目只证网被问；答案被用只能由「网答错时消费方的闸变红」证明）。修为一类、闸在外部：`tests/flownetw3.cx flow`——把 tdescprobe.cx 对一份 scratch flownet 副本编译一次（`sh unisacc.com -o`，约 8 s），副本的 `flow.cx` 经须恰好命中一次的精确编辑加一个运行时故障开关 `FLOWNET_FAULT`（`NET`：该网每个答案轮换到下一类；`NET:FROM:TO`：该答案替换；flownet 本体无此开关，D1 不变）。`flow . 1/2`（闸步 `flownet-w3-flow-1`）：同一二进制无故障 `--oracle cc` 106/0 绿；adj 错 → rc 2（books：declare adj 56 ≠ 77）、ap 错 → rc 2（context ap 279 ≠ 283）、sz 错 → rc 2（int x[3][3] 尺寸超出 sz 域，具名拒绝）、`sz:8:4` → rc 1 且 oracle 报 `MISMATCH int *x; sizeof(x) model 4`（错答进入推导出的尺寸）；任一网答错而闸仍绿即判该网为摆设、红。`flow . 2/2`（闸步 `flownet-w3-flow-2`）自证检查本身：三个自身闸为绿的副本——丢弃全部答案（A2）、只丢 sz 的答案、换问且解钉（A3）——各被抓，且所报摆设故障集恰为 `adj ap sz sz:8:4` / `sz sz:8:4` / `adj ap sz sz:8:4`（逐网定位）。故障经 `env(1)` 传入（m9 libc 的 exec 用自己的 environ，`setenv` 传不到子进程，实测）。实测墙钟：flow 1/2 约 12–13 s、2/2 约 30 s（负载 11–14）。tdescprobe.cx 只改头注释（指向此闸），探针、账目与 5 个 tdescprobe 闸结果不变。

**W3 补 3（2026-10-23，验证方第三次反驳后；改架构，不改闸口径）**：验证方的探针 A6（照常问网，随后只保留 adj 1/12、ap 1/32、sz 9/36 的答案，其余改用本文件 `tdesc_label`）在全部 tdescprobe 闸与 flow 闸上仍绿：整网故障（每个答案都错）时保留的少数答案足以让闸变红，分不出「全用」与「只用几个」。根因两条：消费方持有建网所用标签的本地副本（丢掉答案总能补回），故障粒度是网而不是键。修法：(1) **知识移出消费方**。TDESC 代数成为 flownet 的表 `flownet/tdesc/{contract,adj.gold,ap.gold,sz.gold}.tsv`（各带 C 依据注释），新 API `fn_construct(DIR)`：读 `DIR/contract.tsv`，每网 gold 取注入源、否则存货 `DIR/NET.gold.tsv`，整数网一律在 .cx 内自建（`nets.cx` `fnn_load_gold`，不需 `NET.net.tsv`），逐键精确。`tests/tdescprobe.cx` 删去 `tdesc_domain`/`tdesc_label`/`CONTRACT`，只剩拼写声明符与上下文、提问、写行、判定；拼写抽成 `spell()` 供推导与真值共用。(2) **域收紧**。旧 80 键（adj 12、ap 4×4×2=32、sz 4×9=36）中 34 键从不被问（ap 的 int 持有 P/A3/F、非法相邻对 F.A3/F.F/A3.F；sz 的 int:非 inc、P/A3:72/108/over、A3:inc、F:12.. 等）——用新钩子对补 2 的消费方实测 `adj 12/12, ap 20/32, sz 14/36 = 46/80`，34 个摆设键恰是这些：任何消费方都不能让它们有用。新域恰为推导所问：adj 不变（op×holds，12）；ap 以「该位置描述符的前两级」为一字段（P.P P.A3 P.F P.int A3.P A3.A3 A3.int F.P F.int int）× apply，20 键；sz 以 `OP:INNER` 为一字段（int、P:inc/4/8/12/24/36、A3:4/8/12/24/36、F:4/8），14 键；合计 46，n/N 报 46/46，并如实记下旧口径 46/80。账目钉不变（adj 77 / ap 283 / sz 326）。(3) **逐键扫荡**（flownet 测试钩，`flownet/flow.cx`）：环境变量 `FLOWNET_SWEEP=all|NET,...` 时，`fn_run` 在真实运行通过且发射钩子接受之后，对每个被扫网的每个键、每个非本答案的类各扰动**恰一个**答案（tdesc：150 次，同一进程），重跑流并把输出原样交给发射钩子；每次须被 flownet 具名拒绝（越域 ask、账目、迹、流自拒）或被钩子拒绝，否则该键为摆设（输出未变＝未用；输出变了但钩子接受＝在钩子里丢弃），整次运行具名拒绝并逐键、逐扰动点名；末了重跑未扰动运行须逐字节复现且钩子再次接受。摘要写 stderr。不设则行为不变（适配器不受影响）。(4) **发射钩子即判定路径**。tdescprobe 的钩子把行变成声明符/探针/无尺寸三表（别无来源），随即按模式判定：`--oracle` 在运行**之前**由 cc 独立求出全空间真值（39 个声明符是否被接受；23 个被接受者 × 8 上下文的 sizeof 是否被接受、表达式是否被接受、尺寸；`-std=c99 -pedantic-errors -fsyntax-only`，8 项一文件、每项夹在 `#define x xI`/`#undef x` 之间以免块作用域函数声明的链接冲突，错误按项的三行归属，`too many errors` 具名拒绝；尺寸仍一个程序），表与真值逐格比对；`--check` 以 BASELINE 的模型列（编号、声明符、上下文、尺寸）逐行比对；`--baseline` 不判。实测：tdescprobe `46/46 keys; 150 perturbations: 98 refused by flownet (ask 49, books 49), 52 refused by the emit hook, 0 accepted`（`--oracle cc` 与 `--check 1/106` 皆然）；解钉账目的副本 `--oracle` 下仍 46/46（49 越域 + 101 钩子拒），扫荡不靠账目。(5) **自证**（`tests/flownetw3.cx flow . 2/2`）：4 个自身 `--oracle cc` 为绿的副本各被扫荡拒绝且所点名的键恰为其丢弃的键——全部答案本地重推（A2，0/46，点名 46 键）、仅保留 1 adj/1 ap/9 sz（A6，11/46，点名 35 键）、仅 sz A3:36 本地重推（45/46，点名 `sz (A3:36)`）、钩子内从基线文件取尺寸（35/46，点名 11 个 P/A3 尺寸键；int 的每个错答都会使下一问越域、F:4/F:8 的错答会把行移到另一张表，故不在其列）。`flow . 1/2`：原样 tdescprobe 在 `--oracle cc` 与 `--check 1/106` 下皆 K/K（K 由 `flownet/tdesc` 的 gold 行数算出）、解钉副本 `--oracle` 下 K/K。(6) **闸结果不变**：`tdescprobe-oracle` 首行 `106 probes, 0 mismatches against cc` 逐字节不变，新增第二行 `39 declarators (23 valid), 78 contexts with no size (32 incomplete, 46 not C), 0 mismatches against cc`（单步约 11–14 s）；`tdescprobe-classic` 85/8/13 changed 0；`com-tdescprobe-1..3`（手跑）OK 33/28/22、REFUSE 2/7/14、WRONG 0、changed 0。D3 6 个变异按新代码改写（use 去掉→编译拒绝点名 `fn_out`、run 去掉、construct 去掉、gold 驱动、adj 答案丢弃→下一问越 sz 域、ap 问换常量→账目拒），副本改为并行编译，单步约 18 s；flow 两步约 20–29 s；D4 两步、W2 七步复跑全绿，D1 不变（flownet/*.cx 无 ujs/Python，bounds 8 文件）。如实记下的边界：(a) 钩子返回之后改写表的代码在进程内不可见——`--oracle` 的判定在钩子内完成，`--check` 的逐探针判定所用的表已在钩子内对过基线；(b) `--check` 的基线只有尺寸列，解钉账目时只影响声明符/无尺寸表的扰动不会被 `--check` 的钩子拒，故 `--check` 下的 46/46 依赖账目钉（`--oracle` 下不依赖）；(c) 扫荡要求消费方的钩子会判定，对只写文件的钩子（如 ujs 适配器）它会把未被 flownet 拒的扰动都报为摆设，故只用于 tdescprobe；(d) 专门检测扫荡模式再改变行为的对抗式副本不在此证明之内。闸步名与命令不变（`flownet-w3-d3-1/-2`、`flownet-w3-flow-1/-2`），只改其内容与 `tests/gate.sh` 注释。

### W4 —— 决策网扩展（观测 → 决策）

- 对 type / scope / shape / ic 四网，先定义「决策」的测量口径：**变异必须改变输出或拒绝**。
- 逐网做：改前端消费该网结果 → 用 selftest 变异探针证明输出/拒收变化 → 更新 EXPECT。
- 只做到「测得出 + 口径钉死」；`scope` 的不可达分支、`irsel` 的无询问点需先给出可达性方案，否则如实标 RESERVED。
- **闸**：每网一条变异探针，全绿才升格；不可达就保持 RESERVED，不得虚报。

### W5 —— 覆盖与文档

- 语料从 28 扩到全量（以仓内可编译集为准），逐类记录覆盖/拒绝。
- 交付 `flownet/README.md`（API、注入契约、错误码）与 `docs/flownet-migration.md`（如何从 m4 迁过来）。

## 5. 红线（违反即回退，不讨论）

1. **不许静默回退 gold**：gold 回退计数 >0 即失败（沿用 m4 现有强制）。
2. **不许把 28 语料当全覆盖**；覆盖率必须报「覆盖/拒绝/未测」三态。
3. **不许在 `flownet/` 里出现 Python 调用**；适配层只能在边界外。
4. **不许改编译器语义来迁就模块**：本需求不动 R0–R10 的 TDESC/state 切片。
5. **每波必须独立可复现、单步 ≤60s**；不接受「一次性大改 + 整闸」。
6. **D3 的变异证明不可省**：第二消费方移除调用后不红 = 可复用性未成立。

## 6. 与既有路线图的关系

- **不替代** `research/c43-roadmap-0043-and-refactor.md` 的 R0–R10（那是编译器内部 TDESC/state/cell 重构）。
- **并行**：本需求只做「范式模块化」，不改字节；R3 起会改字节，两者不要在同一个 commit 里混。
- **协同点**：R 路线 (E) 第 4 项（`export.py` → `stategen.cx` 产出 k2 事实）与本需求 W3 的「注入面」同向——都在把 Python 从关键路径上撤出。

## 7. 交付物

1. `research/flownet-w0-feasibility.md`（W0 结论）
2. `tests/flownet-baseline.tsv` + check 步骤（W1）
3. `flownet/*.cx` + 适配层（W2–W3）
4. 第二消费方的改动 + 变异证明（W3）
5. `flownet/README.md`、`docs/flownet-migration.md`（W5）
6. 每波一个 commit，message 带 `fix:`/`tests:` 前缀与闸结果

## 8. 开放问题（主人授 cc 自裁，2026-10-23 已裁，见各条「裁定」）

1. **模块归属**：`flownet/` 是新顶层目录，还是并入 `exec/`？（涉及打包清单与 derived-map）
   - **裁定：新顶层 `flownet/`，不并入 `exec/`。** `exec/` 是产品闭包（打包、provenance、reseal、freeze 口径都按产品管）；本需求承诺不改字节、不动编译器语义（§5-4、§6），放进闭包会把每一波都变成产品刀。`flownet/` 的检查步在 `release/derived-map.tsv` 与 `tests/ungated-checks.tsv`/gate 中单独登记。
2. **第二消费方选谁**：`inventory` / `auditnet` / 新写的 `.cx` 自测，三者成本与风险不同。
   - **裁定：`tests/tdescprobe.cx`（声明符 → 规范类型描述 TDESC 的派生）。** 理由：它本身就是「有限表上的一条流」，`.cx` 范式实验 E2 已用流单元跑通过同一推导（`research/c43-cx-paradigm-experiments.md`）；它在闸内（`tdescprobe-oracle`、`com-tdescprobe-1..3`），106 条结果可对 cc 机检，D3「移除调用必须红」可直接变异证明。`inventory` 的逻辑与流×网无关（硬套会成摆设）；`auditnet` 依赖完整产品构建与 model-audit，W3 成本与风险都更高；新写自测不满足「既有闸内调用方」。
3. **W0 若 `.cx` 表达不了表驱动**：是降级为「稀疏构造 + 纯函数」，还是先做 `.cx` 侧的最小表机制（那就是语言特性工作，范围外）？
   - **裁定：先按 W0 实测；若不行，首选「生成 `.cx` 源码、表编译进去」（§4 W0 表第二行已列的方案），其次稀疏构造 / 纯函数；`.cx` 语言特性工作不纳入本需求。** 注：`.cx` 已有文件读写（`include/cx.h` 的 `cx_read`/`cx_write`），读外部 `.tsv` 预期可行，W0 第二探针实测确认。
4. **W4 的 scope/irsel**：若前端不改就不可达，是标 RESERVED 收口，还是把「改前端」纳入本需求？
   - **裁定：不可达即标 RESERVED 收口，「改前端」不纳入本需求。** 改 ujs 前端属另一产品线，且 §5 红线 4 禁止为迁就模块改编译器语义；升格另立需求。
