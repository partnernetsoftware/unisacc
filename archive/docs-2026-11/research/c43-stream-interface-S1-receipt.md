# S1 收据：资源流严格模式（59150d12）与 RF-LEGACY 补跑

**只是收据**：记录已跑的命令、身份与结果，不新增设计。未推。

## 提交与身份

- S1 提交：`59150d12`（已推，cc 确认）。
- 产品候选（HEAD 7a17d238 + S1 改动）：`/home/box/c43/s1b/cand/unisacc-next.com`，sha256 `665334b0edf32df52e845539e3a53db0f19430f1b1120c6b2dce33a9365454e8`。
- 基线产品（7a17d238，干净 worktree 构建）：`/home/box/c43/base7a-build/cand/unisacc-next.com`，sha256 `83e6e7e4624c1a6d06771af3347f152187468a01893665e0972860961ca2dafb`，build.json `commit` = `7a17d2386ef8a357821774d8880036f2db3a3ee1`。该 sha 与 3dc7f348 的候选相同，说明两者产品闭包相同（7a17d238 只改了派生映射与 abi.manifest）。

## RF-LEGACY 补跑（本条债关闭）

命令（从 S1 树运行检查器，第二参数为基线）：

```
sh <候选> -run tests/resourceflowcheck.cx -- <候选> <基线>
```

结果：`RF-LEGACY PASS default output identical to base`，rc 0。

同一次运行的其余行全部 PASS：CAPTURE、RF-POS、RF-ENV、RF-MISSING、RF-DUP、RF-LENGTH、RF-RESET。输出见 `/home/box/c43/s1b/rf-legacy.out`（仓外）。

## 仍未覆盖（S1 step1 拟合同中的行）

以下行在 `research/c43-stream-interface-S-step1.md` §6 拟定，但 `tests/resourceflowcheck.cx` 尚未实现：

- RF-ABSENT：显式 ABSENT 且磁盘/包存在诱饵时不读诱饵。
- RF-EMPTY：PRESENT_EMPTY 不能变成 absence。
- RF-STAGE：前阶段产新属性、下一阶段换 ledger，检查无旧资源 cache 残留。

这三行列为新债，待 S2 或独立刀补。

## 限制

- 测量范围：一个程序、一条路径（C → Tape，`-S -o`）。
- 未做 win/osx/lnx arm64 的 RF 运行。
