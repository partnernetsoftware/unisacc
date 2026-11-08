# 0.0.48 行为变化

本说明对应私有 c48-1b 候选的协议合同，不是已发布公告；全网、平台与字节等价资格以本候选实际验收记录为准。

## 行为变化

- 对象默认切换方案（本 out 候选待集成，不表示已安装或取得发布资格）：所有目标的裸 `-c` 统一采用既有 unit-object 接口，包括 Linux；允许没有 `main` 的编译单元，并保留外部可见的 GLOBAL 定义（如 `lib_add`、`lib_mul`）供多翻译单元链接，这是行为改进而非失败。image/run 仍拒绝没有 `main` 的完整程序；`-S` 与显式 tapebin 的合同不变。
- Linux 裸 `-c`（无 `-S`、无 `-funit`）现在产出单元对象，允许无 `main` 的源码分开编译并链接；整程序对象需显式 `-c -S -b lnx/x86_64` 且输出非 `.s`。整程序对象与单元对象混链仍拒绝并点名整程序对象；`driver-diagnostics` 保留该拒绝合同，并新增裸 `-c` 对象与显式单元对象成功链接、执行跨单元调用的断言。
- 裸 tape 和 target 文本 payload 必须以恰好一个末行 `.end` 加 LF 结束。缺尾、重复尾、尾后任意字节均拒绝；不提供旧 tape 兼容模式。外层属性/侧带容器先核自己的长度与身份，随后检查其声明的裸 payload。
- `.unit N B:HEX` 是定位帧：N 始终是翻译单元号，B 是文件名字节数，HEX 为该字节串的小写十六进制，不能含 NUL。B 必须与解码长度一致；B 为 0、HEX 为空表示显式清空当前逻辑文件名，不表示继承。
- 同 N 可重复切换逻辑文件名，包括嵌套 include、返回主文件及 `#line` 改名。链接重编号只按 TU，不为头文件另占 TU 号；不同 TU 即使同名也不能串来源。有语义行但无来源帧的真实 TU，由链接器沿原 N/UNIT_MAX 分配独立号并发 `.unit N 0:` 清名，不编造文件名；空对象不分配，末尾仍统一一个 `.end`。
- image tape 保留 `_start` 首行；对象文本原两字段 `.unit 2` 版本头仍与三字段定位帧区分。定位帧不占代码、数据或重定位地址。无真实来源的裸模型不伪造名字帧，但仍须有 `.end`。
- tapebin 使用 major 1 / minor 2：kind 8 编码 ULEB TU 号、ULEB 名字节数与原名字节，kind 9 编码结束；旧对象版本头仍为 kind 6。旧 minor 0/1 不隐式补尾或作为兼容输入接受。

## 预处理来源帧

`pp.locations` 升级为 `UNIPP2` 加 NUL。头部六个 LE32 为 textlen、prelines、autoinclines、nsplice、ninclude、nline；无填充。splice 保留 u32 位置；include 记录为 rawline、nlines、namelen、完整名字节，不再截成 62 字节；line 记录为 LDRAW、LDUSER、LDNUM、namelen、名字节。

splice 与 `#line` 位置严格递增；include 按稳定遇见顺序、IRLN 非递减，同起点外层先于内层。IRNL 是插入时原 body 的 NL+1，不是递归展开后的最终跨度，子项可长于父项。wire 未携带已解析物理路径或父深度，不能据此判尽同起点父子关系或拒绝所有同形记录；不同目录的同名头可能有合法相同 wire。可判定的乱序、内外总长不一致、乘加溢出、短读和名字中的 NUL 仍拒绝，既有有效数值域不扩大。

仅 line.namelen 的 `0xffffffff` 为无显式名哨兵，随后没有名字节；0 表示显式空名。序列化记录解析后的结果：继承前一个显式名字时写该名字节，只有结果仍为默认 TU 来源时写哨兵。显式空名后的无名 `#line` 继续保持空名，不能误恢复 TU 默认名。

## 诊断与参考实现

生成器在有输入上下文时报告真实输入 `PATH:LINE`，延迟规则、模板与嵌套清单保留各自来源。JSON 诊断回调不改变原语法拒绝退出码；没有输入上下文时不伪造文件行。

`archive/reference-c/src/front_pp.c` 在本候选登记为活参考，而非历史归档；独立 UNIPP2 对拍入口显式使用 host C、共享声明及既有 kernel 输入。它们不是产品训练，也不意味着历史宿主 C/Python 依赖已经清零。

## 外层属性与构造产物

USLATTRT 外层表在复制、移动 payload 或交换状态之前，先按既有 `cxsa_valid` 核记录数与总长度；sidecar 与 frame 入口共用该边界，坏外层不能被后续缺 sidecar/payload 错误遮蔽。ATTR1 原有完整检查保留。

正规 `compiler.pkg` 是打包器的原始裸 body，不直接作为 `--models` 输入。seal 派生 `compiler.models.pkg`，仅追加标准 `UNIPKG1` 加 LF 与 LE64 body 长度；verify 同时核 footer 的无前缀/填充/证书边界，以及 body 对已验证裸包逐字节与 SHA256 完全相同。原生构造收据和 raw 产物不改写。该产物仍是 PREPARED，不自动授予产品或发布资格。

## T1 正式候选与 s9 harness 披露

冻结基线为 `79c09c401`，产品 SID 为 `e586721c7767275e1b5af84fe5a3cd199600e2550fc665c26cec642c6f4c9325`。最终一轮 131 个构造作业与 42 个全键检查均实际 rc0，35,080,489 个观察、135,914 个状态的动作与字符串完全相同。正式 raw 包 SHA256 为 `a1797e8cc9d6e09164337627ee383ac4e200628c98766932b657358040b56413`，消费载体为 `0c7f5355e638aee84f74446906b98d5a56ac8f6f3528f708c115ac5d5ae635bb`；最终纯读 verify 实际通过。

正式载体消费者已实跑：六目标 30 组 A/B 镜像字节相同且 30 组 legacy 参考亦相同；工具 629 源/79 片为 620 对象、7 XFAIL、2 scanner 拒绝，unknown/residue 均 0；格式 275/0、独立 UNIPP2 对参考 C 61/0、无来源 TU 13/0、stream-pipe 12/0、tapebin codec 9/0 与实际镜像退出码 7。此处只证明 candidate-consumer，不授予根 APE、安装、完整 s9 或发布资格；参考 C 的宿主依赖仍在。

s9 必须披露本轮 harness 变更：测试与工具的输入相关复制改为检查返回值的 `tx_copy`、`mm_copy` 与 `cx_fmt`；阶段检查器补直接模块依赖；E2 域测试修复源码里意外 NUL 与未闭合局部块；独立参考临时 adapter 只替换路径/SHA，旧误绑定证据不删除；tapebin 回归新增成对绝对 `--compiler/--models`，默认旧路径须保持标准流、退出状态及所有产物字节相同，参数不完整或无效必须拒绝。默认回归代码归独立测试，不进入生产工具的自检 CLI。

完整 tapebin 实跑为 67 PASS + 5 `TAPEBIN-ORIGIN-5` KNOWN-GAP：五 fixture 的 tape-route 与 C-route 实际 origin 均为 1，原断言要求 tape-route 为 0；旧 fde 默认同样失败。按 cc 裁定保留断言、不 repin、不改 e586，0.0.49 独立调查断言过期还是产品缺陷。旧新默认回归均 48/18、rc1，不得冒充默认全绿。

其他边界：`check-stages` 原默认未绑定正式 models 的整体运行仍有 13 红，新增 7 个定位/end 与 11 个 chain/anchor 项实际通过；旧独立 stream1 的认证材料不可得，完整 E2 domain positive/refusal 未运行，只实编完整测试并以已装产品模块计划及 funit 链接执行原 `frame_cells` 得 21/0 oracle-smoke，不转授实际 stream 资格。原编译拒绝及所有 INVALID_BINDING 结果保留。

八个冻结 `exec/*-result.tsv` 的裸立即数按 cc 裁定仅在 `tests/literal.tsv` 精确登记限期豁免；不改检查逻辑或产品输入，0.0.49 首候选必须改用事实/动作命名常量。其余七个产品棘轮项由 cc 集成时精确登记。79c 的主线既有红只用于历史归因；当前 main 已含 R1，集成资格必须以最新 main 的实测为准。
