# TEST-LOADER-COM：测试部件加载器设计

## 决策与边界

最小版只新增 `tools/test-loader.cx`、`tools/check-test-loader.cx`，先在私有副本验证，交 cc 审核补丁；不改产品源、不预扩 toolsloc。新名称已执行 `git log --all --oneline -- tools/test-loader.cx tools/test-loader.com tools/check-test-loader.cx`，无可见历史。c46-2 等最小版可用再启动。

测试工具不是第二套 loader：只校验、选择并启动显式绑定的原生 slice，以现有 `--models` 指定模型包；不额外要求重复的 container 参数。直接路径为 `src/drive.cx:575-581` 的 `hs_read → pm_open`；stream 路径为 `src/drivestream.cx:363-379` 的 `sy_setenv → hs_container → hs_read → pm_open`，后接同一 `st_span`。`src/host.cx:106-139` 现有回落只由工具的显式非空绝对 `--models` 覆盖，产品代码不动。禁止自行解析模型并运行、禁止用另一个产品替代 slice。

## 最小接口与身份

接口：`test-loader --slice ABS --models ABS [--identity] [--net STAGE=ABS_FILE --swap-tool ABS] -- PRODUCT_ARGS...`。A 的 `--identity` 为 stderr 收据开关，不是路径。B 先支持一条显式 `--net`，重复拒绝，不引入目录猜测布局；`--swap-tool` 显式绑定现有 p3-swap 可执行件，禁止隐式寻找仓根产品。所有路径必须绝对且可读，子进程工作目录变化不改变输入；缺任一必需绑定立即非零退出，不能从环境猜候选，不能回落 `./unisacc.com`。`--models` 首版仅接受 pkg 容器文件，拒绝目录；单网由既有 p3-swap 替换成私有容器后仍走原产品 loader，不宣称产品已经原生支持目录装载。

每次执行记录 loader、slice、pkg、实际使用的流/网文件 sha256、字节数、目标、参数及退出码。尚未可拆出的流/网必须明确记录由哪个 pkg 提供以及该 pkg 身份，不把源码表 sha 冒充运行网文件 sha。运行前后复核输入不变；缺文件、不可读、哈希不符均拒绝。APE 身份是 `F(loader, 有序切片, pkg, 打包参数)` 的确定性产物；开发收据不能凭部件相同推导整个 APE 相同。只有实际提取并核对该 APE 的内部件 sha 后，才能记与其部件一致。

B 块必须支持外部单个网文件替换包内同名网（至少一个网），逐网记录原始/替换 sha，不能仅换启动入口。复用 `tools/lab-loader/p3-swap.cx:60-78,163-208`：`PKG MODEL OUT NEW_NET --model`；实际实现接受整 M 记录、UNINETB1 二进制、经 `nw_binary_net` 转换的 N 文本网（文件头注释过时，不作为拒绝合同）。输出裸包正文，补 footer 后送原 slice，产品仍经 `pm_open`。同 M 记录替换要求包及行为逐字节相同；正文重压不要求压缩字节相同，但执行结果须相同。变异网必须有可观测结果差异或具名拒绝，证明实际消费替换文件，不能只验证收据。按 A（严格绑定与负控）、B（逐网身份与替换）、C（批适配与登记）小块推进，反馈环不超过 15 分钟；未分片验证不触整链/CI/全批。

## 验收与证据范围

以同一候选 APE 为 oracle，提取其单个本机 slice 与 pkg；同输入分别走整 APE 与部件 loader，比较退出码、stdout、stderr/产物字节。覆盖直接 E2 与现有 stream 批；保留现有断言、拒绝合同及原始失败，不把安装产品旧行为当新候选通过。

负控至少包括：缺 slice、缺 models、哈希不符、相对继承环境污染；从不含 `unisacc.com` 的异 cwd 启动正例应保持字节一致，删绑定则必须具名拒绝且不得读取仓根产品。原未提交适配器负控并入本项，不丢已有合同。

C 块不能把独立 E2 批偷换为直接 `-E`：`tests/e2-input-binding.cx:15-22,60-70` 实际读取带 `UNIPKG1` 的七个流容器并核模型记录相等，执行位置参数 `FILE SOURCE INCLUDE`，输出为 `UNIPP1`。`streams-material` 的薄壳是另一合同，不能混同。`work/ledger.tsv:388` 明确旧适配器与异 cwd 负控尚未实现；本项必须补实跑，不沿用历史 198/0 宣称新工具已绿。

分片结果一律标开发证据，不算发布资格；正式资格仍须同候选整 APE 六目标原生证据。每个执行与子进程有不超过 60 秒的独立上限，失败保留实际退出状态；日常车道显式 `UNISACC_WAVE_REUSE=1`，发布链关闭。

## 当前实现与证据

同一真实 APE 的部件已提取并核身份（材料 `/tmp/loader-ape-proof-dbISgu`）：APE `ae0b12d8c822efa44d6724782f32a275b4d3fb2cdb3387178cbd76541972c6f0`，6,128,709 字节；lnx/x86_64 slice `67ea0b25ec6896aa74c629c3936c4d5f3bb1c8b3cb199ea18a29380228e30d58`，1,851,254 字节；pkg 正文 `15d8af5844459de26e0ad7feec1564deb1cd1e501d58ab6a964097e7087208ec`，1,662,149 字节。直接 `-E`、stream E2 与分片输出逐字节一致；父实际现有完整 stream 批 9 PASS、`CHECK_STREAM_PIPE failed=0`。本 APE 是公开旧产品，不作为新候选资格。

A 已合入 `3475daee75`，最初证据限 stream 绑定/异 cwd 负控与 E2/E1 对照 6/0。B 已由 cc 用主树源码自建 loader 与 p3-swap 后实跑 16/0、棘轮 13/13，toolsloc 基线 78016。原默认 `-run` 的 APP_ARGS 污染已修：父最小探针从收到额外 `--models/pkg`、rc9，变为仅 `ARG[1]=sentinel`、rc0；用户 APP_ARGS 自带 `--models` 的回归通过，故解除该历史使用限定。B 逐 M 身份、自网替换等价、外网变异实际消费拒绝与输入前后不变均有开发证据，非发布资格。C 批次登记使用 `tools/register.cx`，不手写 batches/coverage；完成后同输入实测整 APE 与分片的 E2/stream 批耗时，未测相位标 UNMEASURED。

C 已由 cc 主树源码自建后实跑 `TEST_LOADER assertions=19 failed=0`、棘轮 13/13，toolsloc 基线 78040；父私有冻结 C1 另实跑 19/0。`register.cx` 实际输出 `REGISTER id=test-loader-com status=REGISTERED`；批次的无参入口必须显式提供绝对 `TEST_LOADER_ROOT/LOADER/SLICE/MODELS/SWAP`，缺绑定非零。逐字命令和原始状态见 `/tmp/loaderimpl/C-evidence.txt`，父证据 `/tmp/check-test-loader-mlznku/evidence`。E2 原消费者 single 36/0、refusal 201/0；positive 198 项有 10 项旧产品 BOM 失败，整 APE 与分片字节相同且正文仍含 BOM，原失败保留，不改断言、不归因加载器。适配器不是七个独立 UNIPKG1 容器，不声称 `e2-input-binding` 整批通过。

身份收据逐网给出压缩 M 记录与压缩载荷 SHA256，明确 `decoded_hash=NOT_MEASURED`；p3-swap 按阶段名取首条 D 对应 M，同一 M 的所有路由共同受影响，不声称替换所有同名阶段的不同 M。输入前后 SHA 检查仅对冻结开发输入提供证据，不认证恶意并发 TOCTOU。无替换分支 exec 保留调用方 deadline；公共 np_run 双管 EOF 后 waitpid 边界已交 cc/grk，本工具调用仍须外包 55 秒，不声称修复该公共接口。

## 同输入迭代耗时（单样本，非稳定性能结论）

同 APE/slice/pkg 身份见上；分片入口为冻结 `/tmp/loaderimpl/test-loader-C1`。E2 双方使用硬链接的同一适配器二进制与同一材料 pin，由显式入口路径（argv[0] 的 direct-speed/c1-speed）选择整 APE 或分片；不删 MATERIAL 行。每对 stdout、stderr、退出码逐字节一致，父以六次 `cmp` 实核全同。首次不同适配器 SHA 导致三行 MATERIAL 不同的原失败保留于 `speed-c1-e2.*`，修正比较合同后才产生以下正式数据。逐字命令及输入身份在同目录 `speed-evidence.txt`。

| 批 | 整 APE 墙时（ms） | 分片墙时（ms） | 样本数/侧 | stdout/stderr/rc | 原合同结果 |
|---|---:|---:|---:|---|---|
| E2（single + positive + refusal） | 16113.354121 | 14923.072950 | 1 | 全同；rc1/1 | 36/0；198/10（旧 BOM）；201/0 |
| stream 完整批 | 9187.984647 | 6089.602999 | 1 | 全同；rc0/0 | 9 PASS，failed=0 |

实测总墙时包含子进程启动、构造/执行、输出与校验；启动独占、执行独占、装载/推理各分相及冷/热均 `UNMEASURED`，不从单样本宣称稳定收益。未重建整链、未运行 CI 或全批。原始证据在 `/tmp/loader-ape-proof-dbISgu/speed-c1-{e2-final,stream}.{0,1}.1.{stdout,stderr,rc}`；临时 `.cx` 计时器 `speed-pair.cx` 与 `e2-speed-batch.cx` 同目录保留供审计，不作为已登记工具。

## 后续独立批的双轨边界（cc 裁定）

- test-loader 的逐网对象是出货 P3 部署模型，即 D 路由指向的 M，按阶段和模型索引命名；先做 e2/e3/lower 三模板。阶段名替换按首 D 选 M，不能将名字拒绝冒充某个未选中模型的实际消费拒绝。lower 首条是 arm64 模型，最小模板只生成 arm64 `target.text`，不声称原生执行或 x86_64 lower 模型覆盖。
- 18 事实网是 gold→kernel/nets 的独立轨；D.stage 没有这些事实网名。combo/isel/parse/pfconv/scope 等对照臂不能伪报 P3 替换消费。独立单元不删不缩：grk 已以 `aeae980e74` 保留 `net.* BLOCKED evidence=gold-kernel-nets`。本窗口交付冻结载体及 SHA 后，grk 以 `89b44591ec` 接入后段，`9ab008f1a0` 补 e4；七流 stage 全部经真实检查 READY（开发证据）。
- ISA 启动检查使用显式目标与精确版本期望；只声明真实本机 slice 的启动结果，其他五格 `NOT_CLAIMED`。旧冻结 0.0.45 可用于开发检查，但不得用当前源码版本号冒认为 0.0.46 或发布资格。

三模板已由父在当前 main 的干净私有克隆实跑 `LOADER_NET_REPLACEMENT assertions=19 failed=0`，覆盖 e2-M0、e3-M4、arm64 lower-M9；原 checker 另实跑 19/0，13 棘轮全过、工具行数实际/基线同为 78341。新增 `cxlib.loadernets` 模块移动复用 M 记录解析，未添加 `#include` 或放宽任何断言；净新增工具 60 行。为使原始与自 M 替换的产品 stdout/stderr/rc 完整逐字节比较，成功替换元数据仅 `--identity` 启用；产品错误诊断从未过滤，收据 SHA 另独立实跑核验。证据 `/tmp/check-loader-nets-leptbs`、`/tmp/check-test-loader-quevly/evidence`；精确命令 `/tmp/loader-nets-replay.txt`。
