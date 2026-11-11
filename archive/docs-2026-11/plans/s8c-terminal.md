# s8c heavy 终态（补充证据，不挡 v0.0.46）

会话 `/tmp/grk-s8c/out/s8c` candidate `884adfef…` DRAIN 2026-11-07T19:07:59 n=15。**PASS=11 FAIL=73 UNKNOWN=0**（84 已 registered heavy）。

## 根因分组（73=73）

| n | 根因 | 末行/stderr | 批 |
|---|---|---|---|
| 9 | 材料未规范化：kernel-route `ROUTE_DIR not canonical PREPARED`；`DETAIL route-input-changed unisacc.com` 与 `netpack.cx` | rc=2 KERNEL_ROUTE | kernel-route-{coverage,encoding,object,pin-mutation,pins,refusal,relocation,route-recovery,route-refusal} |
| 9 | 输入缺失 `objects: REFUSE input absent errno=2` | rc=2 | objects-{link,missing-definition,duplicate-definition,mixed-target,old-unit-version,unsupported-osx-arm64,osx-x86_64,win-arm64,win-x86_64} |
| 9 | 材料回执：`ri-flow-material: REFUSE material bootstrap differs from raw root` | rc=2 | ri-flow-profile-{valid,unsealed,elf-flip,final-flip,old-v1,pending-flip,recompile,reseal,closure-field} |
| 7 | 材料回执：`ri-chain-material: REFUSE material-receipt-mismatch …/ri-chain/material.tsv` | rc=2 | ri-chain-{valid,missingnet,tableflip,nnflip,symlinknet,receiptflip,fiforeceipt} |
| 4 | 输入缺失 `p3-library-members: REFUSE regular-input errno=2` | rc=2 | p3-library-{members,members-cxcodec,members-cxruntime,api} |
| 2 | 材料回执 `pp-gold-material: REFUSE material-content-mismatch` | rc=2 | pp-gold-controls, pp-source-constructed |
| 1 | 输入缺失 `no kernel at …/kernels/x86_64.kernel` | rc=2 | cx-core-run-diff |
| 1 | 输入缺失 `assembler-cache: …/kernels/identity-helper` | rc=2 | assembler-cache |
| 1 | 材料变化 `ri-material: REFUSE material changed` | rc=2 | ri-consumers |
| 1 | `stream-model-identity: REFUSE not canonical P3` | rc=2 | stream-model-identity |
| 1 | `prune-cases: REFUSE classic sources errno=2` | rc=2 | stream-prune-cases |
| 1 | `productpkg: REFUSE product-identity path=/tmp/grk-s8c` | rc=2 | streams |
| 1 | `e2-input-binding: fixture-footer errno=11` | rc=2 | e2-input-binding |
| 1 | `cx-library-cache: read-input` | rc=2 | cx-library-cache |
| 1 | `p3-extract-check: p3-header` | rc=2 | cx-p3-extract |
| 8 | 流用例表红 `PEEP_CASES pass=0 fail=21 unrealizable=179` | rc=1 | stream-e4-peep-0..7 |
| 2 | `OPINFO_CASES` rc=1 | stream-e4-opinfo-0,1 |
| 5 | `STREAM_LOWER_TARGETS` failed=11（tape.accept / target-text-exact） | rc=1 | stream-lower-target-{lnx-arm64,osx-arm64,osx-x86_64,win-arm64,win-x86_64} |
| 2 | `STREAM_IO_CASES` rc=1 | stream-e4-io, stream-prune-io |
| 2 | `STREAM_SEMANTICS` e4 `not_run=1` | stream-e4-semantics, stream-prune-semantics |
| 2 | `PROPERTY_CASES` det `not_run=1` | properties-cases-det, properties-cases-meta |
| 1 | **真产品缺口** `C_NEGATIVE pass=21 fail=22`（A12 静默接受） | rc=1 | c-negative |
| 1 | **真产品缺口** `PP_NEGATIVE pass=8 fail=2`（macro-name-not-identifier, header-name-unclosed） | rc=1 | pp-negative |
| 1 | **不能只用材料证明** `interfaces: REFUSE flip-range errno=11` 且 stdout 多条 `equals-hand-*-oracle` / `elf-image.equals-full-product-O2-image` | rc=2 | interfaces-lnx-x86_64 |

9+9+9+7+4+2+1×9+8+2+5+2+2+2+1+1+1=73。

## 不能用材料/环境打发

- **c-negative / pp-negative**：编译器静默接受，与 A12/矩阵一致，**产品缺口**（非 s8c 新引入）。
- **interfaces-lnx-x86_64**：stderr `flip-range errno=11`（材料/夹具）**同时** stdout 对 hand-oracle 与 O2 镜像字节失败。未在本轮做出「只修材料即绿」。**可能产品/oracle**，需独立最小复现（STREAM-LOWER-ORACLE / 壳材料）。**未证纯 INFRA。**
- **stream-lower-target-***：tape.accept 失败，同接口格，不单报新编译器回归，亦不标已证明材料问题。

## 归类验证（kernel-route，非猜）

`kernel-route-encoding` stderr：

`DETAIL route-input-changed unisacc.com`  
`DETAIL route-input-changed tools/cxlib/netpack.cx`  
`canonical=1 stale=2`  
`REFUSE ROUTE_DIR is not canonical PREPARED material for the current inputs`

材料在 **c46-2/旧 unisacc.com** 上 PREPARED，s8c 产品是 **884adfef**，指纹过期。规范化路径不能单独转绿，须 **用当前 APE 重构造 kernel-route 材料**。本轮未重构造（不挡发布）。

## PASS 11

未在 FAIL 表；heavy 84−73=11。
