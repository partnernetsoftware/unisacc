# m4pro 本机复现 FINDINGS @82e1a9f0（辅助；验收仍以 CI 为准）

## 复现
- `tests/bootstrap.sh` → FAIL B=139 U=0；bs_B/bs_U 2328418 B；insn B=396703 U=402437（与 CI macos-14/15 同形）
- 分步：① UA -c rc0；② from-tape rc0；③ codesign adhoc rc0；④ `bs_B $SELF -c` → **139**；U 同输入 rc0，tape 7727691 B 与 B.tape 同长

## 崩溃点（IPS + llvm-objdump）
- EXC_BAD_ACCESS SIGSEGV；FAR=**0xfffffffffffffff7 (-9)**；ESR Data Abort（Translation fault）
- imageOffset **0xb2d0** → linked **0x10000b2d0**：`str x0, [x1]`
- 现场：x1=-9（目标指针），x0=0x810000（写入值）；x6/x7 为帧/值栈（解释器形态）
- 前序：`*(fp-0x20)` 入栈 → `*(fp-0x10)` → pop 到 x1 → `str x0,[x1]`（`*ptr = val`，ptr 已是 -9）

## 假设（更新）
- **H1 成立（本机）**：`--from-tape` 路径产物在 Darwin arm64 运行时崩；非签名（codesign rc0）；非 UA；非 U 路径
- H5 flaky **削弱**：attempt2 macos-14/15 已再 failure（intel 仍跑）
- H2 签名 **削弱**
- 根因域：from-tape 降低/发射导致某指针局部为 -9，或该 store 的指针源错误——**最小修应落在 tape→native（arm64）store-indirect / 地址形成**，先对照同语义 U 路径的 SSA/指针来源再改

## 最小修建议（未改源；仅建议）
1. 以 `0x10000b2d0`/`str [x1]` 为锚，在 from-tape 编译轨迹里找回对应 op/局部（fp-0x20 指针源）
2. 对照 U 镜像同语义点为何不是 -9
3. CI 继续用 tip diag upload 收 bs_B；本机产物：/tmp/b139-manual-82e1a9f0/
4. 禁 bump/Draft；绿以 GitHub CI Darwin bootstrap 为准
