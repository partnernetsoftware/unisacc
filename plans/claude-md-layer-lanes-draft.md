# CLAUDE.md 分车道条文改写稿(暂不生效)

状态:草稿,暂不生效(主调度 2026-11-20 决定)。待 build/lane-inputs.tsv 生成器与包门工具就位(0.0.87)再生效;生效时由主调度改 CLAUDE.md 并在 prd_mempalace.md 记决定。在此之前版本分支规则不变。来源:plans/reviews/2026-11-20-layer-lanes.md 第 4 节,原样拷贝如下。

## 改写稿一:新增"分车道"条

分车道(项目所有者 待批):产品输入按 build/lane-inputs.tsv 分两类。A 类(编译器与核心层,即源身份 SID 收集到的全部文件)只上版本分支 v0.0.NN,冻结时合入 main,起链即内容截止。B 类(扩展层库包,不在 SID 内的 ext 路径与今后迁入的库包)可直接提交 main,条件是:过包门(六目标实编、layer-api 版本与 api_sha 同步、affected 选批全绿、上一公开产品兼容实编),只提交自己的路径,在 build/pkg-release.tsv 记一行。B 类提交不改变候选身份,不触发整链与预跑,冻结窗口内照常允许。一个提交不得同时含 A 类与 B 类路径。B 类单元不得被 A 类路径 use 或符号链接引用,由 check-lane-boundary 守。

## 改写稿二:产品输入变更条

"产品输入变更先出补丁给集成者核"改为:"A 类产品输入变更先出补丁给集成者核;B 类过包门后由作者直接提交"。

## 改写稿三:版本分支条

"版本分支"条首句后加:"B 类库包改动是本条的第二个例外(第一个是 devsvc),在共享检出的 main 上改与提交。"

## 生效前置

- tools/lane-inputs-gen.cx 与 build/lane-inputs.tsv 落库,lane-key-probes 批次由红转绿(0.0.86 工具线)。
- 包门工具落库(0.0.87,agy-flow)。
- 去四个 sysx 目录别名前,先查非 use 形式使用者(未核)。

回滚:本文件不生效,删除即可。
