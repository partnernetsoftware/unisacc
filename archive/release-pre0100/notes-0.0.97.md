# unisacc 0.0.97 发布说明

状态: 草稿(csr,通用部分 + e2 一节;执行核一节见同目录 `exec-core-section.md`,由 agyneon 填写;身份/整链/预跑/CI 待合入与起链后回填)

整体裁决 NOT_QUALIFIED(草稿口径;公开前按实跑证据再核)

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | a64cb13439bbca8dc4cf352ec843e94772890467 |
| 候选产品整链提交 (T) | d59040be1a01e3acad11628ec21111df63925b79 |
| 合并提交 (v0.0.97 -> main) | 15cf2019d7611ff25bbac78b6d7d3a1c72312fed |
| e2 重扫修复合入 main | ca2f342aab (Merge branch v0.0.95; 分支提交 40df7913c5) |
| 发布 APE SHA256(unisacc.com) | fa14390abbf4239045146c2723b6d63a4a616bf62c0e9d7a9a91a2c45b8236fc |
| 源快照 SID | b047ca3f98eeca31609cb3774aaaaf74f45d9faa04e6d7e1f0702e4a28e4d32c |
| pkg.bin | b84d4f37d957fbda8bc1b2c52c2d9b4a4b15b0cbf72751f8d91e817edeb0cbfc |
| slice-win-arm64 | c5cf6465cc115ab74db3ec8f616a9035ff84a097506678b623cbabc07fadc6bf |
| SHA256SUMS | f6a9ac05c7c36b7fa1077d9b3bfc8e9d7ced8edee7a47a54b0c280bb20ba743b |
| 正式原生 CI (candidate-native.yml) | TBD |

| 对象 | 身份与字节数 | 回执来源路径 |
|---|---|---|
| e2 修复分支提交 | `40df7913c5dbd0a57b95b145928f9b550ca10a94` (`40df7913c5`) | `git rev-parse 40df7913c5` / `plans/v0.0.95.md` 第七节 |
| e2 修复合入 main | `ca2f342aab` | `git rev-parse ca2f342aab` |
| 链产出 APE / SID / 切片 / CI | TBD(合入起链后填) | `release/evidence/0.0.97/` / `release/candidate.tsv` |

## 跳号说明(所有者授权)

- 所有者 2026-11-23 裁定从 `0.0.97` 重新出发:`0.0.95`、`0.0.96` 两号跳过。
- `0.0.97` 为「一个候选只含一类大改」的**授权例外**:合并两类产品改动
  - (1) 模型与网:`e2` 重扫修复(原 `0.0.95`,已在 main `ca2f342aab`);
  - (2) 执行器内核:执行核每步成本 19.8 档(分支 `v0.0.97`;若 13:00 门未过则本版只发 e2,执行核顺延下一空号)。
- 进程生命周期(原 `0.0.96` 分支)不进本版,做完按下一空号发;`0.0.98` 至 `0.0.102` 分支保留、暂停加活。
- 入口:`prd_mempalace.md` 当前行、`plans/v0.0.97.md`、`build/version-seq.tsv`。

## 严格证据

459 批:PASS 457、FAIL 0、UNKNOWN 2、INVALID 0

TBD:候选整链、严格预跑批次数、PASS/FAIL/UNKNOWN/INVALID、`expected-unmeasured`、六格 CI。草稿阶段不填未实跑数字。

## 产品改动

### e2 重扫修复(原 0.0.95,已在 main)

- **改了什么**:仅 3 张 `exec/pp` 手写事实表(分支 `40df7913c5`,合入 `ca2f342aab`):
  - `exec/pp/directive-body-byte.tsv`
  - `exec/pp/text-byte.tsv`
  - `exec/pp/text-result.tsv`
- **语义**:修 `#include` 前缀重扫。`INCH`(行首 `#` 后 256 字节头体)原先 `INPOP` 回 `P0` 把已读头体重新入队再扫一遍;补丁改为 `P0.inc`/`P1.inc` + `INC_SUB` 计数 + `INPUSHXE RESUME INC_END`,只重入剩余部分。这是**步数**修复;字节等价是合同不是目的。
- **证据(字节对拍,全 diff=0)**:
  - (1) `-o` 重放 1154 项:`same=1146` `refuse_same=8` `diff=0` `unmeasured=0`(未取步数);
  - (2) pp-equiv 全量 5128 项:`PP_EQUIV total=5128 same=5078 refuse_pp=50 diff=0 unmeasured=0 waived=0 pending=0 COMPLETE=yes`(未取步数;分层 headers/c-tests/corners 与 cdx 对得上);
  - (3) unit-e2-compare 113 项:`UEC_STATUS total=113 same=106 refuse_same=7 diff=0 unmeasured=0 pending=0 COMPLETE=yes`。
- **证据(步数,阳性对照;不得写成墙钟)**:
  - 113 单元 `run-all --steps 1`:`STEPS_SUM rows=113 step_moved=112 step_grew=4`;步数合计 `455988310 -> 265763887`(**降 41.7%**);
  - 永久夹具 `tests/unit-e2-rescan/{plain,inc1,inc2}.c` 关系断言守住(plain 至多贵 2 步、inc1/inc2 严格省且 inc2 更省);
  - 墙钟:39 单元各 7 轮中位**只降 19.0%**(中位比值 0.974,p75 1.007,13/39 更慢);原因是 e2 墙钟含模型装载、verify、layout。**发布说明不得把 41.7% 写成墙钟收益**。
- **证据入口**:`plans/v0.0.95.md` 第七节;`research/2026-11-23/e2-rescan-positive-control.md`;`research/2026-11-23/v095-o-replay-result.md`;账本 `~/.local/share/unisacc/research/v095-rescan-review/`(uec-113、pp-equiv-full、oracle 两包,不入仓)。
- **单点对照(plans 第七节,提交后树)**:`examples/hello.c -run` e2 步数 `1200813 -> 484709`;`phase-budget.cx -c` e2 步数 `3960979 -> 2478603`;主语料已测 `2798 SAME / 1354 REFUSE_SAME / 0 DIFF`,`2415/2415 UNIPP2 SAME`,`tests` `.c` `116 SAME`。

### 执行核每步成本(分支 v0.0.97)


## 1. 计划必做项交付状态核对 (plans/v0.0.97.md)

gatekeeper 推断实情准确: 本版实际交付 2 项, 其余 3 项未交付并列入已知缺口。

(1) 按状态打包表挂 CoreModel (宿主建表): 已交付
- 证据入口: 提交 `a353daf3c7` (`cx/runtime/model_decode.cx`, `cx/runtime/model_lifetime.cx`, `src/executor.cx`, `exec/c/core.h`, `exec/facts/core-abi.tsv`), `CHECK_CORE_ABI_GEN passed=10 failed=0`。
(2) 转移内联: 已交付
- 证据入口: 提交 `a353daf3c7` (`exec/c/asm/run_x86_64.S`, `exec/c/asm/run_arm64.S`), `CORE_RUN_DIFF cases=3 failed=0`。
(3) 动作循环并入 core_action: 未交付 (见已知缺口)。
(4) 输出追加快路径: 未交付 (见已知缺口)。
(5) 尾部线程化分派: 未交付 (见已知缺口)。

## 2. 改了什么

- 宿主建表与生命周期: 在 `cx/runtime/model_decode.cx` 为解码后的模型构建 64B 对齐的 `CorePackedState` 表并挂接至 `CoreModel.packed`, 在 `cx/runtime/model_lifetime.cx` 与 `src/executor.cx` 同步释放与传递。
- ABI 与布局事实表: 更新 `exec/facts/core-abi.tsv`、`exec/c/core.h`、`exec/c/asm/layout.h`、`exec/c/asm/layoutcheck.c`, 经 `check-core-abi-gen` 校验生成物同步。
- 汇编核内联转移: 在 `exec/c/asm/run_x86_64.S` 与 `exec/c/asm/run_arm64.S` 引入 `PS_*` 宏, 在 `packed != NULL` 时直接读取 64B `CorePackedState` 执行状态转移, 减少多数组指针追逐。

## 3. 等价与性能证据

- ABI 与生成物一致性: `CHECK_CORE_ABI_GEN passed=10 failed=0`。
- 双路径差拍等价: `CORE_RUN_DIFF cases=3 failed=0` (CX 参考核 vs 汇编核 `packed==NULL` vs 汇编核 `packed!=NULL` 三方逐字节一致)。
- UNIKERN1 尺寸:
  - `x86_64`: `UNIKERN1 arch=2 bytes=7728 entry=7536 slot=7712`。
  - `arm64`: 本轮未实测链接尺寸 (`UNMEASURED`, 先前原型验证见 `plans/v0.0.97.md`)。
- 9 对交错基准 (`load1=3.78 < 5`):
  - `SUMMARY pairs=9 med_a_ns=40.00 med_b_ns=32.00 med_diff_ns=8.00 med_diff_pct=20.00%`。
  - `4096-state` 输入口径说明: 构造 4096 状态 (256 KB 打包工作集) 伪随机跳转 N-model (`mode=0, lo=0, hi=256, count=2, next=(s*257+13)&4095`), 单次 `core_run` 连续执行 `2,000,000` 步, 在同进程内对 `packed==NULL` (A) 与 `packed!=NULL` (B) 做 9 对交错计时。

## 4. 回退方式

- 运行期 `CoreModel.packed == NULL` 自动回退至原 `core_transition` 路径, 无需重编内核即可零风险回退。

## 5. 撤出条件

- `load1 < 5` 下 9 对交错基准中位降幅 `>= 6 ns/step` 且 `>= 18%`, 且 `CORE_RUN_DIFF cases=3 failed=0`、`CHECK_CORE_ABI_GEN` 全绿。

## 6. 已知缺口

(1) `plans/v0.0.97.md` 中 3 项未交付 (动作循环并入 `core_action`、输出追加快路径、尾部线程化分派), 留待后续版本。
(2) `arm64` 本轮 `UNIKERN1` 尺寸未实测 (`UNMEASURED`)。
(3) `osx` / `win` 首个真实样本依赖 CI。

## 已知缺口

### 资格与未跑范围(草稿)
- (1) 全套 `s9` `daily`/`release` 长跑默认未跑;整体裁决草稿保持 `NOT_QUALIFIED`,不得宣称全批资格。
- (2) e2 三条字节证据未取步数的两条(`-o` 1154、pp-equiv 5128)不得外推为步数收益;步数收益只引用 113 单元与 plans 第七节单点。
- (3) 墙钟 19.0% 与步数 41.7% 不可混报;有单元在中位上更慢。
- (4) 执行核若未过门:本版不交付执行核改动,缺口与顺延号写入公开说明。
- (5) `osx`/`win` 无本机样本前不得声称经实机验证(仅 CI 样本或未测)。

### e2 历史未判定项(plans/v0.0.95.md 第七节,冻结前须补跑或写入公开已知范围)
- (1) 主语料曾列 `1139` 个 `-o` 样本未跑(后续 csr 复核已补到 1154 项 diff=0;公开说明以复核账本为准并点名入口);
- (2) 主语料 `90` 个 `UNMEASURED`;
- (3) `tools -E` 多数仅验证拒绝合同(`multiple preprocessing outputs not migrated`);
- (4) `4000` 次引入上限同因拒绝,真实帧容量溢出未测;
- (5) 带嵌套 `#include` 的强制头两边 `rc=2`,未判定。

## 回滚方案

公开前备 `release/rollback.txt`:草稿撤回 `v0.0.97`、Latest 移回上一公开版、前向撤销 Y2/Y1/X、e2 表回退至合入前、执行核按提交回退(若已合入)、本机 Root 回退至上一公开 APE。细则待集成者填实提交号。
