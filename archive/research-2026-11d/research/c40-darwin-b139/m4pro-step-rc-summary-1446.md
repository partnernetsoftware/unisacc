# m4pro 分步 rc 摘要 @82e1a9f0（1446）

来源：`/tmp/b139-manual-82e1a9f0/`（本机；验收仍以 CI 为准）

| 步 | 命令 | rc / 产物 |
|---|---|---|
| ① | UA `$SELF -c` → bs_B.tape | rc0；tape 7727691 B；step1.err 空 |
| ② | `unisa compile --from-tape` → bs_B | rc0；`osx/arm64 2328418 B (text 1839624 B, 396703 insns)` |
| ③ | codesign adhoc | rc0；Identifier=bs_B-26a15c5e…；Signature=adhoc |
| ④ | `bs_B $SELF -c` | **139** SIGSEGV；IPS FAR=-9 |
| U② | python compile → bs_U | rc0；`2328418 B (text 1832492 B, 402437 insns)` |
| U④ | `bs_U` 同输入 | **0**；tape 同长 7727691 B |

硬证文件（本机路径；入仓见同目录 m4pro-* 副本，若 Copy 受阻则以本摘要+FINDINGS 正文为准）：
- FINDINGS.md；bs_B-crash.ips / crash2.ips；lldb_B.txt；step2.out；sign_dv.out；U_build.log
