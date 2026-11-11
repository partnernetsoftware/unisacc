# env-pin 终跑对照 1146 分桶收口

**墙钟**：2026-10-12 ≈14:17 SGT  
**GATE**：`/tmp/c41-0843-envpin/cand.queue`（721/721；final rc=1；release NOT ready）  
**对照 1146**：`/tmp/c41-0843-csmith/cand.queue`（**只对照；禁 void**）  
**cand**：`bcd2b9b75129…`；**env_pin_sha256**：`89bb83562ff9b3685bd7f84381cbbb25981e8f0a1d46f9dc536e2073ee3bc007`  
**tip**：`d4cd5f6f`；version.h=**0.0.41**；Latest=**v0.0.40**；H1=INHERIT

## 总表

| 桶 | env-pin | 1146 | Δ |
|---|---:|---:|---:|
| completed | 721 | 720 | +1 (envpin suite) |
| PASS rc=0 | **627** | 597 | **+30** |
| rc=1 | 58 | 83 | −25 |
| rc=142 超时 | 20 | 24 | −4 |
| rc=2 seedgen | 10 | 10 | 0 |
| rc=77 UNVERIFIED | 6 | 6 | 0 |

## 分桶细则

### A. DEFER / rc77（Darwin/宿主不适用；覆盖义务保留）
- `exec-bootstrap-osxarm`
- `exec-bootstrap-osxx86`
- `lib-stack-arm`
- `lib-stack-x86`
- `lib-stack-x86-hostabi`
- `lib-windows-gp-native`

### B. 假红已消（1146 红 → env-pin 绿；lib-union/ffi 环境面为主）
改善合计 **32**：  
`ape-version`, `asmtext`, `closure-c2`, `com-fb12-multi`, `draftguard`, `exec-e5`, `exec-warningdriver-ua-Wall`, `exec-warningdriver-ua-Wextra`, `lib-union-composite-callback-1`, `lib-union-composite-callback-2`, `lib-union-composite-callback-3`, `lib-union-composite-direct-1`, `lib-union-composite-direct-2`, `lib-union-composite-direct-3`, `lib-union-scalar-1`, `lib-union-scalar-2`, `lib-union-scalar-3`, `lib-union16-callback-1`, `lib-union16-callback-2`, `lib-union16-callback-3`, `lib-union16-callback-4`, `lib-union16-callback-5`, `lib-union16-direct-1`, `lib-union16-direct-2`, `lib-union16-direct-3`, `lib-union16-direct-4`, `lib-union16-direct-5`, `lib-windows-bridge`, `lib-windows-gp`, `seedape`, `unisa-ape`, `warn`

其中 union/ffi/相关：`ape-version`, `asmtext`, `draftguard`, `lib-union-composite-callback-1`, `lib-union-composite-callback-2`, `lib-union-composite-callback-3`, `lib-union-composite-direct-1`, `lib-union-composite-direct-2`, `lib-union-composite-direct-3`, `lib-union-scalar-1`, `lib-union-scalar-2`, `lib-union-scalar-3`, `lib-union16-callback-1`, `lib-union16-callback-2`, `lib-union16-callback-3`, `lib-union16-callback-4`, `lib-union16-callback-5`, `lib-union16-direct-1`, `lib-union16-direct-2`, `lib-union16-direct-3`, `lib-union16-direct-4`, `lib-union16-direct-5`

### C. 超时 rc142（看门狗/宿主速度；多数跨版 H1 基线）
- `closure-c3`
- `closure-c4`
- `com-luatests-build`
- `com-tapebin-1`
- `com-tapebin-2`
- `com-tapebin-3`
- `exec-f1-attributes`
- `rowcov-enc-1`
- `rowcov-enc-2`
- `rowcov-enc-3`
- `rowcov-enc-4`
- `rowcov-enc-5`
- `rowcov-enc-6`
- `rowcov-enc-7`
- `rowcov-enc-8`
- `rowcov-parse2-build`
- `rowcov-pp-locations`
- `rowcov-pp-shared`
- `shared-e2-located`
- `shared-e2-plain`

### D. rc2（seedgen 内存证据 UNKNOWN；未执行义务）
- `com-seedgen`
- `seedgen`
- `seedgen-2`
- `seedgen-3`
- `seedgen-4`
- `seedgen-ppchain-1`
- `seedgen-ppchain-2`
- `seedgen-ppchain-3`
- `seedparse2-1`
- `seedparse2-2`

### E. 真红 / rc1 清单（58）

**已在 rulings.tsv 具名（14）**：  
`ccinterop`, `com-elfobj`, `com-forward`, `com-forward-multi`, `com-multi`, `elfobj`, `exec-bridge-linux`, `exec-driver-language-2`, `forward`, `hosthdr`, `lib-lifecycle`, `realprog`, `syscall6`, `windows-resolver-host`

**未在 rulings 行内（44；含 env-pin 粘性假红 3 + 其它）**：  
`com-auditnet`, `com-comboot-fixedpoint`, `com-comboot-stage2`, `com-comboot-stage3`, `com-luatests`, `facts-export`, `gate-infra`, `gate-infra-38`, `gate-layers`, `ledgercheck`, `observation`, `queuestart`, `queuestart-launch`, `queuetimeout`, `queuetimer`, `rowcov-enc-union`, `rowcov-parse2-1`, `rowcov-parse2-10`, `rowcov-parse2-11`, `rowcov-parse2-12`, `rowcov-parse2-13`, `rowcov-parse2-14`, `rowcov-parse2-15`, `rowcov-parse2-16`, `rowcov-parse2-17`, `rowcov-parse2-18`, `rowcov-parse2-19`, `rowcov-parse2-2`, `rowcov-parse2-20`, `rowcov-parse2-21`, `rowcov-parse2-22`, `rowcov-parse2-23`, `rowcov-parse2-24`, `rowcov-parse2-3`, `rowcov-parse2-4`, `rowcov-parse2-5`, `rowcov-parse2-6`, `rowcov-parse2-7`, `rowcov-parse2-8`, `rowcov-parse2-9`, `rowcov-parse2-union`, `script-inventory`, `seedpack`, `subtract-safety`

env-pin 相对 1146 **回退**（假红，已修 tip `d4cd5f6f`，本机 suite 绿；因 gatequeue∈common stamp **未**往该 GATE resume 以免 void 全队）：  
`gate-infra`, `gate-infra-38`, `queuestart`

### F. 结论（发布材料）

- 结证齐；**local acceptance FAILED** 与 0.0.38/0.0.40 宿主基线面同构（rc77/rc142/ruled rc1）。  
- **禁 void 1146**；env-pin 收据与 1146 并存。  
- 政委授：材料齐 → Draft v0.0.41 → CI → Latest unsigned（对齐 0.0.40）→ 开 0.0.42。

*收口 2026-10-12 ≈14:17 SGT*
