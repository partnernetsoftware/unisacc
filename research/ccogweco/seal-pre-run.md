# 封版车起手前并行预做清单(0.1.0;只写,2026-11-25 ccogweco)

对象:0.1.0 零改动封版车。可提前做的只限"不产生结论、不写 INDEX、不绑定候选身份"的准备动作;
复用类(107 CI 资格、107 strict 表)在 fable 裁定前只列不做。所有命令原行取自 tools/ 源码。

## 0 边界(先说)

- 可以预做:说明骨架、取证模板的空表头、目录与文件名、命令清单、回滚文本、能用清单冻结稿、seal-precheck 输入清单。
- 不可以预做:strict 摘要、CI 结论、INDEX 行、fixedpoint 六行、ape-receipt 内容、warm 收据(除非键不绑版本,见 M3)、
  old-product 登记值、任何"通过/合格"字样。
- 铁律:未实跑不记 PASS;不缓存测试结论;不提前写 INDEX 冒充实跑;不放宽时限;复用须 fable 裁定并留原行。

## 1 可执行清单

### M1 说明骨架(notes-0.1.0)

- 命令:直接编辑 research/ccogweco/notes-0.1.0-draft.md(已含前序版本节与占位),107 公开后只填:
  真实身份表(冻结提交、APE sha、SID、pkg、双 build.json、SHA256SUMS、CI run)、严格证据汇总、条 6 四时刻、证据入口路径。
- 前置:107 公开(其 APE/SID 作为对照),0.1.0 冻结提交未定。
- 可提前时点:107 公开前后都可,但只改占位文字,不填数字。
- 做错后果:把 107 数字抄进 0.1.0 说明 = 未测写成通过,须整段撤回并更正。
- 边界:占位保持 `<...>`;不写"满足"。

### M2 取证模板预填(fixedpoint-6targets / ape-receipt 目录)

- 命令(106 的归档步骤可直接照抄,只换版本与 rev):

      mkdir -p release/evidence/0.1.0/
      # ape-receipt:从链目录 ape-final/ape-receipt.txt 脱敏拷贝(路径用 ~ 简写)
      sh ./unisacc.com tools/devsvc.cx -- f write release/evidence/0.1.0/ape-receipt.txt --from <链目录>/ape-final/ape-receipt.txt
      # fixedpoint-6targets 的六行在链止后由 g2/g3 逐对摘要生成,不得预填内容

- 前置:0.1.0 链完成(内容依赖链);目录与文件名可先定。
- 可提前时点:目录与文件名现在定;内容链止后。
- 做错后果:预填任何 sha 或 SAME 行 = 伪造证据。
- 边界:只准备文件名与命令,不准备值。

### M3 warm 预热(release-exec --warm)

- 命令原行(release-chain.cx:15、:730):

      sh ./unisacc.com tools/release-exec.cx -- --warm <pack>/candidate.tsv --layout <LAYOUT> --out <STATE>/warm

- 前置:0.1.0 候选表与 layout 存在(warm 键按候选与 layout 计算,见 release-exec 的 warm 收据);
  因此只能在 0.1.0 pack 之后做,不能拿 107 的 layout 冒充。
- 可提前时点:pack 完成到第 5 步之前;链内 0b 也可自动做。
- 做错后果:用 107 layout 预热会写错 key 的收据,读回时 RECEIPT_DIFFERS。
- 边界:预热不产生结论;不得把预热当读回通过。106 的教训(读回冷编 45 秒回滚)用精确 argv 预跑避免。

### M4 release-run --prepare 草稿预检

- 命令原行(release-run.cx:51):

      sh ./unisacc.com tools/release-run.cx -- --prepare VERSION --layout DIR [--out DIR]

- 前置:0.1.0 的 layout 已由 pack 产出;VERSION=0.1.0。
- 可提前时点:pack 后、Step 5 内;不能在 pack 前做(没有 layout)。
- 做错后果:用 107 版本或 107 layout 预检会得到错误的草稿结构结论。
- 边界:--prepare 只检草稿结构,不是发布资格;输出不写 INDEX。

### M5 seal-precheck 输入清单(七条机械判据)

- 内容:条 1 三版固定点回执路径、条 2 strict 汇总与 CI run、条 3 P0 账本、条 4 state-unverified 表与三值、
  条 5 B1/B2 状态、条 6 T1/T3_R6/T4 原行、条 7 能用清单 30 项绑定。
- 命令:seal-precheck 工具尚未在 main(cdx 的 seal-precheck.patch 在 107 追加集),落地后按其用法跑;
  现在只准备路径清单(见 seal-0.1.0-evidence-map.md 七行)。
- 可提前时点:现在。
- 做错后果:清单缺项会让封版机械判据 UNKNOWN,不能算满足。
- 边界:清单不是结论。

### M6 回滚文本与 post 预演

- 命令:从 release/rollback.txt(0.0.106 版)复制骨架,换版本与备份 sha;107 的 release-post 步骤先在 107 上跑通
  (backup-evidence、old-product 登记、version-seq 标公开),0.1.0 只换值。
- 前置:107 的 release-post 完成(有实际原行可照)。
- 可提前时点:107 公开后立即。
- 做错后果:回滚文本写错版本/备份 sha,撤回时找不到正确备份。
- 边界:不预写 INDEX 行;备份 sha 必须来自真实 install-ape/backup-evidence。

### M7 并行窗口(107 CI 等待期)

- 命令:无新命令;把 M1/M5/M6 放在 107 的 CI 等待窗内做,不占关键路径。
- 前置:107 已起链或已派 CI。
- 可提前时点:107 CI 进行中。
- 做错后果:只影响文档质量,不影响候选;但若把 107 结论抄进 0.1.0 文档即越界。
- 边界:并行的是文档,不是测试结论。

## 2 复用类:fable 已裁否(不执行)

- 107 CI 资格复用:**已裁否**。版本槽注入使 0.1.0 的 APE 字节与 107 必不相同,CI 六格资格不能作为 0.1.0 的同候选证据;0.1.0 必须自己派 CI。
- 107 strict 表复用:**已裁否**。同上,APE 必不同,strict 表不能复用;0.1.0 必须自己跑严格预跑。
- 链复用:本来不允许(条件 1 要三版各自链与不动点回执)。
- 因此本页只保留 M1-M7 的合规准备动作;任何"已复用"字样不得出现在 0.1.0 文档。

## 3 禁止清单

- 不预写 strict 汇总、CI 结论、INDEX、fixedpoint 六行、ape-receipt 内容、old-product 登记。
- 不用 107 的 layout/候选表给 0.1.0 预热或预检。
- 不把预热命中当读回通过;不把 --prepare 当发布资格。
- 不为压时间放宽 45/120/55 秒任何一条。
