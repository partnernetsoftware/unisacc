# BASE-3：基线三网全键核对与摘要绑定

结论：原 BASE-2 官方构造产物 `shared-e2`、`tokenpp`、`warnpp-shared` 已全部实际执行 `tools/lab-loader/net-check.cx` 的全状态、全声明观测域核对，均 rc0。不是仅核摘要，也没有复用 BASE6 修改模型的验证结论。

| 原基线网 | 状态数 | 实际观测键数 | 退出 |
|---|---:|---:|---:|
| shared-e2 | 4286 | 1105532 | 0 |
| tokenpp | 961 | 247682 | 0 |
| warnpp-shared | 4293 | 1107338 | 0 |

## 实际输入与身份

仓根 `research/c48-base-build-profile/batch-00/{tbl,net}-shared-e2/result.{tbl,net}` 与 `batch-24/{tbl,net}-{tokenpp,warnpp-shared}/result.{tbl,net}` 是本轮输入。其三对内容 SHA 与 BASE-2 原 `profile.tsv` 一致，详见本目录 `identity.tsv`。未修改或重构造这些输入，原大产物仍留 BASE-2 目录供 BASE6 读用，不纳入本目录提交。

本轮再次运行现有 `tools/net-digest.cx -- --closure out/c48-net-digest-closure.tsv --all`，session4586 实际 rc0；三个摘要行与 C0.2 的 `out/c48-net-digest-all.tsv` 逐字节相同。本轮探针又逐项核了 BASE-2 `inputs.tsv` 的1548个 exec/seed regular 文件，当前内容全部相同。此为明确输入范围的内容绑定，不能扩张成未声明外部依赖均已覆盖或独立历史来源证明。

C0.2 摘要依次为：

- shared-e2：`693fcf308ab374d5ac9e8c42f59bc9cb179eff0cfe7c169ff6538582fd95fde7`
- tokenpp：`dc272a68421de31990ace810fdf57e6dff0e0e22c7af7449d2739f5546d35c9c`
- warnpp-shared：`f99e1ef9f59efaeed10d208edec40f0ee873c494b2c5bb16bdb2c1be5097e295`

三者均为78文件、1015776字节的 pp 静态上界闭包；网名和recipe flags参与摘要，不能将相同闭包误作相同模型。`identity.tsv` 同时保留验证器源码/实际可执行文件、解码器、转移实现与闭包文件 SHA。

## 运行与拒绝实证

先用当前已装产品实际构造本次私有 `out/c48-base3-net-check`，`bound50 sh ./unisacc.com -o out/c48-base3-net-check tools/lab-loader/net-check.cx`，session25922 实际 rc0；此为编译准备，不算功能 PASS。三次分别以 bound40 运行同一可执行文件，每次真正执行验证，不缓存 PASS。exec 返回 chunk `fc8f37`、`d25b3c`、`54e4fd` 均已直接终态 exit0，无在途会话。原输出保留本目录三个同名 TSV。

采用既有 `check-net-check.cx` 的畸变阈值网机制：仅在 out 私有副本把第一条 H 声明的 base_next 从18改为19，仍可解码。实际 rc1，首差 `state=0 key=0 table_next=18 net_next=19 table_seq=2 net_seq=2`，见 `refusal-mutated.tsv`。另将 shared-e2 表与 tokenpp 网交叉配对，实际 rc2，具名 `header dimensions differ`，见 `refusal-crossed.tsv`。原始网不变。

绑定与畸变探针 `out/c48-base3-bind.cx` 先有一次真实 rc2：第一条 H 没有第9个数字，未产生畸变测试结论；部分输出与stderr留 `out/c48-base3-binding-first-partial.*`。按实际 H schema 修正为第5个数字（base_next），第二次 session39284 实际 rc0，内层负控 bound8。最终 `identity.tsv` 仅来自后者完整成功输出。编译、摘要、验证各阶段性能与冷热分相未测，标 UNMEASURED，不从 exec 等待时间推算。

## 验证边界

验证器使用产品同一个 model_decode 与 core_transition，逐状态比较 table/network 的 next 与 seq。mode1 遍历 -1..topmax，其他声明模式遍历0..256，核网bank范围完全一致；同时核头部维度、观测模式、字符串逐字节、动作声明数组及序列长度。没有丢动作声明或把观测域抽样。

它不执行动作的副作用，不证明表的C语言/ABI语义正确，不是独立于产品转移实现的第三方oracle，也不独立验证 JSON→TBL 生成器。三网的 BASE-2 官方构造证据、C0.2 内容摘要与本轮 table==network 全键结论各自独立保留；其余39网本轮未验证。

本轮验证未改冻结工具、产品或源码；本研究目录保留原始核对与拒绝证据，out仅放临时执行材料。
