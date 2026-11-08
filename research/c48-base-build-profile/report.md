# BASE-2：42 个官方配方构造测量

结论：当前冻结输入的 42 个配方已全部实际构造，7 批各 6 个，均退出 0。逐配方 gen、tbl、net 三阶段墙时累计 **26,559 ms**；配方总时长 nearest-rank p50 **483 ms**、p90 **807 ms**、p95 **2,466 ms**，最长 **warnparse：3,138 ms**。本轮没有执行全键验证，不能据此宣称网与表等价。

## 身份与方法

- 配方真源：`tools/cxlib/sourcepackage.cx`，SHA256 `87e60e8a51484d6f02b1eff96b1b9178c952db31164d3d81c901904eae54d987`。
- 输入范围：git tracked 与 nonignored untracked 的 `exec/`、`seed/` regular 文件，逐路径内容 SHA 见各批 `inputs.tsv`；清单 SHA256 `c286fe10fa7da6bd2d2a218b9357ad18998081ee3bb2f1524fa593f69414e066`。每批前后重核，与配方真源及三个构造器共同绑定；这不宣称仓库外依赖已 hermetic。
- 实际构造器：仓根 `out/c47-driver-fix/model-binding/seed-{gen,tbl,net}`，SHA256 依次为 `78508fbfa88bd9a6af46132c55c490adb305005f2060038d5ef872bbcb30a913`、`7b89c188feaa8110aab001ac486fff8217e54a8a831aac94fd711384e87203d5`、`9f58cf53e046f3b8b327985c78056cab33cbff9aee238e52a435ab6635d474d1`。使用既有 HOSTCC native 构造器，宿主引导依赖仍在。`seed/{gen,tbl,net}.c` 与 `out/c47-driver-fix/snapshot/seed/` 三源码实际 cmp0。
- 每项调用现有 `sp_job_command` 官方完整 argv，仅将其中构造器路径替换为上述显式已核身份的程序；没有简化模型或配方。环境为 `env -i PATH=/usr/bin:/bin`；官方子 bound40，外层子传输42秒，批内开始新阶段前检查45秒 deadline，实际调用外 bound55。
- 计时包围每次子进程执行，含构造器启动、构造和捕获输出；不含其后产物 SHA。工具自身默认-run 编译、输入清单计算、产物哈希、全批准备总墙时均未分相位测量，标为 UNMEASURED。不能将本数字与计划自述的 71.8 秒当作同身份性能回归比较；未找到该旧数字的 durable 原始材料。

## 可复算材料

`batch-00`、`batch-06`、`batch-12`、`batch-18`、`batch-24`、`batch-30`、`batch-36` 的 `profile.tsv` 共 42 条 NET 原始行，每行保留三阶段 ms、三个产物 SHA、最终 net bytes 与 rc。每批 `commands.tsv` 保留实际官方 argv，`inputs.tsv` 保留输入清单。`summary.tsv` 是工具实际重算输出。汇总拒绝重复 recipe ID、混合 recipe/input/constructor 身份、失败行、缺完成尾与完成数量不符；不会把重复文件计成42。

执行示例（仓根；OUT 必须全新）：

```text
sh ./unisacc.com tools/net-build-profile.cx -- --root . --gen /ABS/seed-gen --tbl /ABS/seed-tbl --net /ABS/seed-net --out OUT --start 0 --count 6
sh ./unisacc.com tools/net-build-profile.cx -- --summary batch-00/profile.tsv batch-06/profile.tsv ... batch-36/profile.tsv
```

真实会话终态：start0/session81180、start6/session13688、start12/session43581、start18/session85232、start24/session21096、start30/session63728、start36/session19221 全部 rc0；汇总 session83464 rc0。

## 工具检查

`check-tool.log` 对应默认-run 外 bound55，session5633，实际 rc0，9 检查全绿：实际42配方枚举、已知统计精确、未冒称全键验证、缺完成尾拒绝、失败行拒绝、负范围拒绝、空汇总拒绝、重复配方拒绝、混合输入身份拒绝。小统计 fixture 仅为一致性检查，并非构造实证。

首版检查 session66221 实际 rc2：首个默认-run 子命令编译超过8秒。保留 `check-first-failure.stderr`。现检查先在单独 bounded40 准备一个私有可执行工具，再用原8秒功能子上限逐项执行；准备缓存编译产物，不缓存测试结论。新源码两文件都已通过当前已装产品实编（check默认-run；profile默认-run汇总及check内私有编译）。

原始 JSON/TBL/NET 产物暂留各批目录，供 BASE6 进一步全键检查及 delta/full 字节比较；不纳入Git，仅本报告、检查证据与各批TSV入库。未修改产品、seed或recipe。

集成时修正独立调用的总时限边界：原工具在每阶段前检查45秒，但单child42秒理论可跨过批deadline。现在每次child取min(42,floor(45秒剩余))、不足1秒拒绝新阶段；调用者仍用外部bound。此收窄不改官方argv/构造内容，已有实测按原调用保留，不重新计时冒充原结果。
