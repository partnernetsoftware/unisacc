# 结构讨论:三层 → 五层(状态页,cc,2026-11-07;0.0.48 结构性改进的单一入口)

来源:另一 fable 的架构建议(主人转交)。本页只记状态与待办,不重复建议全文(见 plans/design-review-log.md「外部架构建议」节)。

| 层 | 建议内容 | 已验证事实(命令级) | 状态 | 下一动作 |
|---|---|---|---|---|
| L0 loader | 选 os/arch→取 manifest→核 sha→交 base;不含任何切片映像 | 头部 2,631,168B=42.6% 是 win/x86_64 PE 原样存放(com-tree);win/arm64 在 APE 之外 | **可行,收益最大** | S1 已评估(archive/docs-2026-11/plans/structure-s12-findings.md):win/x86_64 头 gzip -9 为 391,110B,省 2,240,058B=APE 的 36.3%;路径=薄 PE+inflate+写 %TEMP% 再 CreateProcessW(与 Unix sh 启动器同构,零新 Windows 能力);PE 固定基址无重定位、仅导入 kernel32 26 符号;需新增 4 项原生验证。→ 0.0.48 主线 N0 |
| L1 base(×6) | 执行器、包解码、sha、mmap、syscall、dl、ffi | 薄层 20 单元 395,710B(plans/slice-nn-migration.md);建议的 30–60KB 与实测不符 | 方向确认,量级修正 | M10a 清单已合;薄层合同冻结(sys-syscalls 表化 L0') |
| L2 program | 平台无关驱动(cxmod/route/source_frame/cache/cli)存一份 tape,L1 现编并缓存 | tape.gz≈原生份额 45%;首编 25–47s;循环依赖:stages/driveunits/runexec/drivelink 是编译 L2 的必需件 | **不列主线** | S2 已量化:单元粒度可迁部分仅 16.1%(闭包外 5 单元 297,442B,实质是 as/view/ar 子命令树 driveface+asmtext);函数级死代码上界 40.5% 但散在 71 个单元不能整单元迁;若 L2 以 tape 分发 27.9%。结论:0.0.48 不把 L2 列主线 |
| L3 nets | 只放一步决策(表→网) | 现状吻合;「NN」措辞问题已记(评审③) | 保持 | README/glossary 措辞(M6) |
| L4 runtime | 用户程序运行库一格一份,对象只带引用 | 每个 .o 约 264KB 运行库;FILE 不跨单元(0.0.46 已知限制);切片 76 对象各带一份 | 未开始 | S3:link-map 归因+运行库链接期单份化的设计备忘 |

## 已被事实推翻的断言(不再引用)
- 「APE 里模型包三份」:实测 1 份。
- 「切片对象里有 m27.o/m22.o 两份模型」:是汇编器/源框架代码对象,不是模型。
- 「cx/driver/sys 与 cx/sys 重复」(评审⑦):实为软链。

## 待办(0.0.48 结构性改进)
| # | 内容 | 验收 | owner(暂定) |
|---|---|---|---|
| S1 | L0 中立化可行性备忘 | 估算省多少字节、Windows 加载压缩切片的可行路径、对「六片同候选原生证据」的影响、需要的原生验证项 | fable 读码+cdx2 实测 |
| S2 | L2 引导闭包量化 | 由 unit-text+依赖生成:引导闭包单元表(字节数)与闭包外单元表;结论=可迁部分占切片多少 | fable+cdx2 |
| S3 | L4 运行库单份化备忘 | link-map 归因(每对象运行库副本来源)+方案(链接期去重 / 运行库作为独立单元)+风险 | cdx |
| S4 | 薄层合同冻结 | `cx/sys/README.md` 升为合同:每原语六目标 YES/STUB/NOT_CLAIMED | cdx2 |
决策点:S1、S2 出结论后由主人定 0.0.48 是否把 L0 中立化或 L2 列入主线。

## 决策记录(cc,2026-11-07)
- L2 程序层:不列主线(可迁仅 16.1%,首编 25–47s,循环依赖)。M10 继续按「FINITE/TRANSFORM 迁表+VM 备忘」走。
- L0 中立化:**列 0.0.48 主线 N0**(见 plans/v0.0.48-unisacc.md)。理由:一次改动省 36.3% 体积,零新 Windows 能力,不动切片逻辑;风险=win/x86_64 原生自举路径改变,需 4 项新增原生验证,且不与其他大改同候选。
