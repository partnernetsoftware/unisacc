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

## 当前待补证据

共享调用路径已按源码核实；具体候选部件所在位置、实际模型文件身份清单及 smoke 命令随首块验证补全。此页不是实现完成或 PASS 收据。
