# A1 handoff（cc → cdx，10-08，对应 477c1a4e）

只是记录（参考路线已实际跑过，产品路线待首片）。

## 输入
- 探针源：agenterm tag v0.1.19 的 `probe.c`（146 行）+ `agenterm.h`；本机副本 `/tmp/r35-release/agt/{probe.c,agenterm.h}`。
- 库：agenterm v0.1.19 release 资产 `agenterm-0.1.19-linux-aarch64.tar.gz`（sha256 文件随附，已校验 OK），解到 `/tmp/r35-release/agl/x/`，主库 `x/libagenterm.so`（运行时 soname `libagenterm.so.1`，需 `ln -sf libagenterm.so libagenterm.so.1`）。
- 下载：`gh release download v0.1.19 -R <agenterm 仓> -p 'agenterm-0.1.19-linux-aarch64.tar.gz*'`。

## 外部函数（探针实际调用，全部 ≤6 个整数/指针参数）
| 函数 | 签名 |
|---|---|
| agt_abi_version | `uint32_t (void)` |
| agt_build_id | `const char* (void)` |
| agt_last_error | `agt_status (agt_error* out)` |
| agt_capability_query | `agt_status (agt_capability cap)`（enum） |
| agt_process_list | `agt_status (agt_process_info* buf, size_t cap, size_t* out_count)` |
| agt_process_self | `uint32_t (void)` |

## 命令
宿主（macOS）：
```
UA -c -b lnx/arm64 -I /tmp/r35-release/agt -o pua.o /tmp/r35-release/agt/probe.c   # 参考：rc=0，nm: T _start + 6×U
unisacc.com -c -b lnx/arm64 -I /tmp/r35-release/agt -o pcom.o probe.c                # 产品：reject not covered: cc interop call
```
Lima default（arm64）客机，/tmp/a1 下放 pua.o probe.c agenterm.h x/：
```
cc -no-pie -nostartfiles -o pua pua.o -L x -lagenterm -Wl,-rpath,/tmp/a1/x -Wl,-e,_start
cc -I. -o pcc probe.c -L x -lagenterm -Wl,-rpath,/tmp/a1/x
./pua > ua.out; ./pcc > cc.out; diff ua.out cc.out   # 只有 self_pid 行不同
```
不加 `-no-pie`：`R_AARCH64_ADR_PREL_PG_HI21 against agt_abi_version … recompile with -fPIC`（GOT 片）。
macOS 同因：`ld: fixup error (kind=arm64_adrp_lo12) … _agt_abi_version does not have address`（Mach-O 片）。

## 判定
比较 ua.out 与 cc.out，排除 `self_pid=` 行后逐字相等；两边退出码 0。
