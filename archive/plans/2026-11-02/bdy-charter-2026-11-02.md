# omp-bdy 章程:重启 unisacc.com 开发(单入口文件,先读这一份)

写给 omp-bdy。读完这一份即可开工;其余文档按"读什么"清单按需读。回信格式:≤8 行,`结论/证据/阻塞/要我做什么`。

## 一、为什么重启
本纪元(11-02)在慢车道上反复暴露"构造链互相抄、纪元日才爆"的腐烂,耗时远超预期,0.0.44 第一优先级(提速)0 进展。主人裁定:**停止本纪元,改为分层开发**——先做 booter + 外部基座 + 外部阶段权重,让每一层单独固化、单独测试(`plans/booter-layering-2026-11-02.md`)。工作方式按 `plans/dev-pipeline-2026-11-02.md`。

## 二、状态(已核实,开工前用括号里的命令再核一遍)
- HEAD=`048e54003f`;已装产品=快车道候选(NOT_RELEASE dev-lnx,entry 648a47ca / p3 436004f0)。(`git rev-parse --short HEAD`;`sha256sum unisacc.com.dev/*`)
- 本纪元产物**保留不安装**:`out/epoch-2026-11-02/p3/p3-package.bin`(1725284 B,sha a0daec1f…)、entry/candidate(3458882 B)、kernel-gen(x86_64 32cecc8a…/arm64 4d6e45a1…,与旧 blob 字节相同)。它们是 B0 的真实输入。
- 已在 HEAD 且有效:kernel 收窄(`tools/cxlib/kernelinputs.cx`)、host.cx cwd 头、H(撤销 --host-provider)、补丁 05/06、pack.cx 对齐 split2、release-build 裸包补 footer、speed-probe、ELF .text 提取器、FFI 桥。
- **已回退/作废**:补丁 07(pp-negative,会误拒全部预处理)。不要重新应用。
- 你的未提交改动 `tools/cxlib/rebuildcontext.cx`(rb_record 分角色+诊断 printf):**丢弃**(用 `git show HEAD:tools/cxlib/rebuildcontext.cx > tools/cxlib/rebuildcontext.cx`,不用 checkout)。理由:booter/manifest 会替换这套 build.json 口径,不在旧口径上继续修补。
- 不装主树、不换标、不继续纪元第 12 步以后。

## 三、你的工作项(顺序执行,每项 gate 是可执行命令,过了才算完成)
**W-B0 P3 explode/implode**(第一件事)
- 写 `tools/p3-explode.cx`(P3/P4 容器 → 目录:每个模型/资源一个文件,文件名=内容 sha256;`manifest.tsv`: `name kind arch sha256 size order`)与 `tools/p3-implode.cx`(目录+manifest → 容器)。纯 cx,用 `sy_*`,不写 `#include`,不加平台 `#if`。
- 输入:`out/epoch-2026-11-02/p3/p3-package.bin` 与 `unisacc.com.dev/p3-package.bin`(后者是 P4 压缩,也要过)。
- **gate**:`tools/check-p3-roundtrip.cx` 对上述两个输入 explode→implode 后逐字节相同(输出 sha 相等);另含负例:篡改一个分片字节 → implode 前校验拒绝并具名;manifest 缺行 → 拒绝。登记批次(`tools/register.cx`)。
- 域:`tools/p3-*.cx`、`tools/check-p3-roundtrip.cx`、自己的批次行、`tests/toolsloc.tsv` TOTAL(新增行数内上调)。
**W-B1 resolver 原型**(B0 完成后)
- `tools/cxlib/resolver.cx`:按 manifest 找文件→校验 sha256→返回字节;来源顺序 `UNISA_HOME` → com 旁目录 → 内嵌 pack;命中必校验,不符具名拒绝,**不静默回退**。
- gate:外部目录与内嵌 pack 两种来源跑同一批 smoke(`tests/run.cx -- --smoke` 16/0)结果一致;`-run` 冷/热毫秒记录(现热≈10ms,上限 +20%)。
**W-B2 阶段独立测试台**(B1 完成后):7 流各有输入/输出 golden,单改一个权重文件只重跑它与下游。先做 `lower`(耗时占 50.5%,见 `plans/speed-hotspots-2026-11-02.md`)。

## 四、硬规矩(今天用代价换来的)
1. **验证=执行**。`git apply --check`/只编译不算验证;提交请求必须附"执行了什么、输出什么"。没有输出的请求会被退回。
2. release-build 取源码用 `git archive HEAD`:补丁/改动不提交,构造出来的就是旧代码。
3. 构造链每步有 55–60s 硬 bound;并行负载会造成假超时(见过 rc148)。构造期间别并行跑批次。
4. 身份口径(product_kind/product_digest/build.json canonical)在多个工具里各抄了一份:**不要再去补旧口径**;遇到身份相关失败,先停,写一行到我这里。
5. 一个工作项一个文件域。域外要改,先报我。不碰 `apps/csih/`、`ujs/`;不用 stash/reset/clean/checkout;不 `pkill -f`;单条命令 ≤60s。
6. 新 `.cx` 带 `check-*.cx` 并登记批次;提交前 `sh ./unisacc.com tools/ratchets.cx` 必须 9/9。cxtrunc:不要丢弃 memcpy/snprintf 的返回值。
7. 文档不单独提交(AGENTS.md 已改);只有代码/表/工具进展才发提交请求。
8. 上下文超 60% 先把状态写进 `plans/bdy-state-*.md` 再继续,不等耗尽。

## 五、读什么(按序,按需)
1. `plans/booter-layering-2026-11-02.md`(目标与分步)
2. `plans/dev-pipeline-2026-11-02.md`(工作方式)
3. `plans/identity-inventory-2026-11-02.md`、`identity-unify-2026-11-02.md`(身份,理解为什么不补旧口径)
4. `plans/omp-bdy-pause-state-2026-11-02.md`(你自己的暂停记录)
5. 需要时:`plans/epoch-runbook-2026-11-02.md`、`speed-hotspots-2026-11-02.md`

## 六、停止条件
W-B0 的 gate 在干净副本上通过 → 发提交请求并等我回复,**不要自行进入 W-B1**。任何一步卡住超 20 分钟,先写一行状态给我,不要空转。
