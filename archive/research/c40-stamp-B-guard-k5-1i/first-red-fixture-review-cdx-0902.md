# 首红停夹具只读复核（cdx；0902）

依据 0902 裁定；实际跑过独立日志解析、SHA256 重算与 Git 只读比较，未执行夹具或门禁。

## 判定

**本窗“当前 chain driver 在 E3 注入首红后，命令层后续零启动”演示缺口已限定闭合。** 仅限 r3、NETWORK=1、单探针 tests/c/a_char.c、sh shim 注入 E3 rc3；不是实际 helper 内部故障或产品验收 PASS，不授 bump/freeze/Draft。

## §6 计数独立核验

- 固定 tip `7b07b72343907d06f797340ae26585f14174a377`；当前 chain.sh 与 tip 原始字节相等，sha256 `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`。源码/数据目录、gatedeps、prd 相对该 tip 无差集。
- POS：chain rc0，equal1；E3 返回0且 e3.json=yes。独立计数 tbl/net/check-net/probe-run/ua-ref = 3/3/3/3/1，证明对应包装日志具备正例可见性。
- NEG：E2 rc0、lex gen rc0，随后 E3 具名调用；首个被记录的非零子 rc 是 helper-parse2-rc=3；e3.json=no；chain rc1，stdout 为 chain: E3 gen failed，e3-gen.err 保存注入诊断。
- NEG 红事件为第17行，日志共17行；红事件后全部日志行数0。独立计数 {"tbl": 0, "net": 0, "check-net": 0, "probe-run": 0, "ua-ref": 0, "gen": 0, "cc-run": 0, "cc-other": 0, "py-other": 0}，均为0；也核实 tbl/net/check-net/probe-run/ua-ref 在整个 NEG 日志均未出现，未仅靠最后一行人为切掉其启动。
- 注入代码位于 sh shim 的 parse2 分支；红记录后 exit3，chain.sh:55 明确 exit1。后续 run/UA 的绝对路径分别经 cc 产物包装和 UA 包装观测，补上0857草案可见性缺口；计数只是命令启动可见性。

## §7 原件与副本独立核验

原件 `/tmp/cc40-prep/first-red/r3/` 与仓内 `first-red-run-0902/` 全部 13 文件成员集合相同、逐字节相等；逐项重算 SHA256SUMS 的全部条目通过。summary.txt 为固定 tip、fail0；它虽未列入清单，本审另算并与副本比对。文件哈希如下：

| 文件 | 原件及副本 SHA256 |
|---|---|
| SHA256SUMS | `71b3f97db6f1a12c7ca718dcb389f6dd15858218e2179ab25aa620cec1d56cbf` |
| neg.chain.err | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| neg.chain.out | `ac24c1fcebb2c18452bc063d21693b8c66049b423de41f19c9136a487e94a52e` |
| neg.e3-gen.err | `578fbe067e85ae46d4e518039b7e9726e9dd28a21a672c4fe1df4584188d8d90` |
| neg.rc | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| neg.shimlog | `6c4354c47fc06c9a79eb56c8e5288a9eefba1c7e2441b79ce8d0f97d9eb62735` |
| pos.chain.err | `572767a3068ad3f339b016cb8ecb23a2d9d5eaca55dcf49788e87728f8e2bf34` |
| pos.chain.out | `2ea2e7878daf229e58eb46a10e9938d33df72e51c213d62596efc824a4f8ba0d` |
| pos.e3-gen.err | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| pos.rc | `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa` |
| pos.shimlog | `9bf390dc3d74fa4732373d1ee8e1dd2be89a8dd8d14694a688db75e039c82908` |
| summary.txt | `f4a042cad2461aebff81216a431807e5105a137ccc31eb1f8db15a640b0400ad` |
| ua-build.out | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |

审查时夹具 sha256 `864613567dacaa841620511e67b9c343d86f99cb14a243a7fc694ebc7b54a205`；材料页 sha256 `b499e658d69100ae7e1ec3389e60cb1f383f7d2581bc61cd4a8e1fa667800e79`。r3.run.log 显示断言全通过、外层 exit0；r1仅 NETWORK=0 正例，r2夹具正则误判导致 exit1，均不作为本次通过依据。

## 仍保留的记录限定

- 材料 §6 仍写 stderr 含 E3 gen failed、七类写作六类；以材料 §4/§11、回执和原始 stdout 的明确更正为准，原文需整理，不能字面称 §6 完全一致。date +%s%N 为墙钟而非 monotonic，本审只用日志行序，不据此报单调耗时。
- 原件未捕获运行前后完整 HEAD/status、当时夹具字节哈希或完整 argv/env；本审记录的是当前文件哈希及只读一致性，不能冒充运行时完整身份账。summary 的 tip 与 git archive 实现、日志 scratch 路径及当前未变 driver 相容。
- PGID/SID 清空、TERM、UA 本体行为、完整进程树、真实 exec-chain 门预算及净提速仍未证；旧 K5-1i /tmp 原件没有恢复。本轮新 r3 原件只补本切口。
- 仓内副本已落工作区不等于已提交入仓；本审不代提交。未改代码、prd、键或验收，未运行夹具、门禁、bump、freeze 或 Draft。

PASS 仅当前 driver E3 注入首红后的命令层零启动及日志副本核验；其它限定不升级
