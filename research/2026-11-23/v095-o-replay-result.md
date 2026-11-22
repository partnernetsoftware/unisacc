# 0.0.95 -o 复核结论

日期: 2026-11-23
范围: 工具顶层 `.cx` 的 `-o` 对拍(原版包 vs 补丁包 40df7913c5 / oracle-patched.pkg)
APE: `/tmp/unisacc-v095-oracle.cNgFT6T6/unisacc.com`(0.0.93)
包: `oracle-original.pkg` / `oracle-patched.pkg`
方法: 两侧 `--models` 换包,`UNISACC_RUN_CACHE=0`,单侧 `timeout 51s`,比较退出码与产物字节

## 去重累计(首轮全量)

```
CUMULATIVE	total=1154	done_same_refuse=1153	pending=0	same=1145	refuse_same=8	diff=0	unmeasured=1	timeout_rc124=1	unique=1154
```

DIFF=0。清单 1154 项去重后 unique=1154。

## UNMEASURED(1)

首轮原行:

```
ORACLE	UNMEASURED	O	tools/devsvc-core.cx	rc=124/124	sha=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855/e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855	err=0/0
```

原因: 两侧均为 `timeout` 到期退出码 124(未写出产物)。未放宽时限。

空闲负载下单独重跑(仍 51s,并行两侧)原行:

```
ORACLE	SAME	O	tools/devsvc-core.cx	rc=0/0	sha=ca6dbeba51c39434385e0be32579ca82cc05ac98d80131ac1841a6761722b577/ca6dbeba51c39434385e0be32579ca82cc05ac98d80131ac1841a6761722b577	err=0/0
```

耗时约 44s,`a.rc=0` `b.rc=0`,产物同字节。判定: 首轮在高负载下触 51s 窗;单跑可在窗内完成。计入重跑后:

```
CUMULATIVE	total=1154	done_same_refuse=1154	pending=0	same=1146	refuse_same=8	diff=0	unmeasured=0	timeout_rc124=0	unique=1154
```

## REFUSE_SAME(8)

两侧拒绝文本逐字节相同(`DIAG_SAME=YES`)。

| 路径 | rc | 诊断(两侧相同) |
|---|---|---|
| tools/chrome_dom.cx | 1/1 | `not covered: no main` (chrome_dom.cx:165) |
| tools/clang_cx.cx | 1/1 | `link: undefined reference to cxm_plan` |
| tools/core_run_spec.cx | 1/1 | `not covered: no main` (core_run_spec.cx:70) |
| tools/cxcc.cx | 1/1 | `not covered: no main` (cxproc.cx:55) |
| tools/cxproc.cx | 1/1 | `not covered: no main` (cxproc.cx:55) |
| tools/gcc_cx.cx | 1/1 | `link: undefined reference to cxm_plan` |
| tools/refimage.cx | 1/1 | `not covered: no main` (cx/mem/bytes.cx:54) |
| tools/refworkspace.cx | 1/1 | `not covered: no main` (cx/mem/heap.cx:44) |

均为改前改后同因拒绝,不是行为分叉。

## 过程要点

- 并行 N 路独立 `UNISACC_CACHE_DIR` 与分账本;合并按路径去重
- `pair-o.sh` 曾因 `set -e` 在 timeout 非零时未写 rc 导致整路中断;已修(子壳 `set +e` 必写 rc)
- shell 批跑为临时做法,缺口已记 `build/pkg-gaps.tsv` 行 `V095-O-REPLAY-PARALLEL`,后续造 `tools/pair-o.cx`

## 结论

补丁包相对原版包在本清单 `-o` 对拍中 **DIFF=0**。首轮 1 项 UNMEASURED 为 51s 窗下的 `rc=124/124`;空闲单跑同窗得 SAME。8 项 REFUSE_SAME 为两侧同文案拒绝。不据此宣称发布资格;本页为独立复核 `-o` 分片收尾证据。
