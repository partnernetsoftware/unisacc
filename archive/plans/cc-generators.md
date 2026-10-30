# cc 的事实生成用例工具索引（2026-10-28）

全部为 `.cx`，经 `sh ./unisacc.com TOOL.cx -- ARGS` 运行；每次调用 <60 s；输出目录只放日志与产物；结果行格式 `STATE<TAB>cell/label<TAB>detail`，末行 `SUMMARY`，stdout 一行标记供 run-batch 的 expect 核对。未实跑/未实现的状态（UNREALIZABLE、UNVERIFIED、INFO、VACUOUS、TABLE_SUSPECT）单独计数，从不记 PASS。登记批见 tests/batches.tsv（35 行，723f6f5a46）；证据与收据见方法学 §19–§21。

| 工具 | 参数 | 对象 | oracle | 片/预算 | 方法学 |
|---|---|---|---|---|---|
| tools/peep-cases-gen.cx | ROOT STREAM4 ABS_OUT FIRST COUNT | 旧 stream4 容器 | weights/gold/peep.tsv 1632 键按 src/opt.c 关系实现为两指令 tape，机械改写期望 | 8 片 ≤200 键，≈40 s | §10 |
| tools/opinfo-cases-gen.cx | ROOT STREAM4 ABS_OUT FIRST COUNT | 旧 stream4 容器 | opinfo.tsv 三头经 peep.tsv 查表（a_dead/none、const0） | 2 片 | §11 |
| tools/o2-cases-gen.cx | ROOT REF STREAM4 ABS_OUT | 旧 stream4 容器 + 经典 REF | REF -O0 tape 经 E4 == REF -O2 tape（字节），O0≠O2 与见证守卫 | 1 次 | §12 |
| tools/prune-cases-gen.cx | ROOT STREAM5 ABS_OUT [oracle-only] | 旧 stream5 容器 + 产品编译的经典 tapeprune.c | 声明闭包 == 经典 == stream5（三方字节） | 1 次，27 例 | §13 |
| tools/properties-cases-gen.cx | ROOT STREAM4 STREAM5 ABS_OUT det\|meta\|bound\|tu\|lower\|random [REF] | 产品（六目标镜像）、stream4/5/6、REF | SHA/字节相等与字面金值；随机程序由生成器自模拟 | 6 组各 <40 s | §14 |
| tools/layout-cases-gen.cx | ROOT STREAM6 ABS_OUT len12\|len3-0..5 [REF] | 产品 tape→六目标、stream6、REF | 参考重排规则的枚举布局表（258 序列） | 7 片 ≈35 s | §18 |
| tools/opt-corpus-gen.cx | ROOT ABS_OUT corpus-0..6\|closure\|cli [REF] | 产品编译 C（六旗×六目标）、REF | lnx 字面 + 产品镜像/tape == REF | 9 组 ≈30 s | §16 |
| tools/formats-macho-pe.cx | ROOT ABS_OUT | 产品 osx/win 镜像 | 公开布局手写解析（含页哈希重算）+ 变异控制 | 1 次 12 s | §18 |
| tools/matrix-flip-gen.cx | ROOT MAP OUT_DIR \| --check [OUT_DIR] [--migrate OLD NEW LOGDIR] | 五张矩阵 | 从登记批与证据映射生成/校验换标 | 秒级 | §20 |

配套：plans/batches-cc-proposal.tsv、plans/coverage-cc-proposal.tsv（已并入正式表）、plans/matrix-flip-cc-map.tsv、plans/peep-tighten-proposal.md。REF 由冻结导出 `tools/build_ref.sh` 构建（§17）；私包差分用包装 `unisacc.com` 使子进程继承 `--models`（§21 B1）。
