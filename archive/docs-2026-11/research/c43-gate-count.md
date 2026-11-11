# 闸套件数：722（0.0.42 正式闸）与 748（当前树）

**结论**：748 − 722 = 26，全部是新增，没有删除。`tests/gate.sh --list --com` 与队列实际调度的 `tests/gate.sh --plan --com` 在当前树上是同一个 748 集合（逐名比对，只在 --list 中、队列不跑的为 0；只在 plan 中的为 0）。722 是 `f871dbaf` 上 `--list --com` 与 `--plan --com` 的结果（都是 722，逐名一致）。

**26 个新增套件（按引入提交分组）**

| 组 | 套件 | 数 | 引入提交 |
|---|---|---|---|
| com-tapebin 拆分（0.0.43 T-1） | com-tapebin-4、-5、-6、-ref、-six、-cover | 6 | 6420cfb3（原 com-tapebin-1..3 保留） |
| tapecheck 有限域模型 | tapecheck-model-1、-2、-3（循环生成） | 3 | b7aa5db2 |
| 库 / late-binding / 宿主 | com-latebind、lib-latebind、com-hostenv、com-tapedomain、exec-librarypartition-model、exec-hostsymbols-model、exec-librarycontract-model | 7 | ea46504a、618dd4da、01262f28、66032554、c37a790e、a972eddb、23e85e12 |
| 自测与阶段封存 | abi-manifest、abi-manifest-selftest、staticimage、staticimage-selftest、early-stage-seal-selftest、pp-leaf-capture-selftest、pp-leaf-seal、pp-leaf-seal-negative、middle-flow-e3、middle-flow-lower-accept | 10 | 0d79e978、42f0bdee、e563a1dc、fa3a06f1、27b4e33b |

合计 6 + 3 + 7 + 10 = 26。

**口径注意**
- 计数只对本机（Linux x86_64）有效。`gate.sh` 里有按平台的 `job`（Darwin 专属套件），在 macOS 上 --list 与 --plan 的数字会不同；本表未在 Darwin 上核对。
- 正式运行的 `RELEASE_SUITES` 为空，跑完整 plan；`RELEASE_SUITES` 只用于 contract 层的观测，不改变正式集合（tests/release.sh）。
- 722 的结证记录见 research/r42-release-acceptance.json（`local_gate.suites = 722`，rc_counts 与 exit_table 合计 722），其中 PASS 632、RULED_BASELINE 30、HOST_TIMEOUT 11、BLOCKED 28 等不是通过项。
- 748 尚未跑过整闸；本表只说明集合差，不说明结果。

**方法**：`tests/gate.sh --list --com` 与 `TERM_SH=0 tests/gate.sh --plan --com` 在当前树与 `f871dbaf` 归档（仓外 git 副本）上分别取名、排序、逐名 comm 比对；每个新增套件的引入提交用 `git log -S` 在 tests/gate.sh 上定位（tapecheck 与 tapebin 分片用循环生成，按生成串定位）。
