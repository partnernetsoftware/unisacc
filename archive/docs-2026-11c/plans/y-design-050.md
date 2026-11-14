# 0.0.50 产品候选 Y(e3 原生阶段化)可行性与 oracle 设计

依据(只读):src/drive.cx:729-802、src/driveroute.cx:360-470、src/driveunits.cx:39-124/342-451、cx/runtime/holdframe.cx、src/stages.cx:198-233、cx/compiler/fwdincrement.cx、exec/parse2/unresolved-result.tsv、exec/parse2/forward-manifest.tsv、tools/whole-front.cx + templates/*.in、out/c49-slow-cdx-979b56e4/mac-medium-forward-proof/sys-dlcall/result.tsv。

## 1. 首遍 e3 到底用了什么;e2 够不够

首遍 e3(`dv_run_lower`)在 `\0cli/run-forward` 资源下跑整程序 `units..e3`,产物是 **USLFW1 侧车**:tape 长度、host slot 计数 cnt、ccw 计数 ccwc、tape、以及每个未定义名的 `USLSIG3` 签名记录。桩文本(`dv_fwd_stub` -> `fwd_emit`)只消费:名字、参数个数/kind/width、返回 kind/isvoid/uns、variadic、ccw>0、target。随后 `uu_user_undef_multi` 用侧车名单对各单元源码文本做「有原型无定义」拒绝。

这些信息来自 e3 的三部分:
- **UD 可达性**(unresolved-result.tsv `UD.check`:扫整条 tape 的标签与调用,含取地址 `lea`):未定义集 = 被调用/取址且无定义的名,不是「声明了但没定义」的名(大量 libc 原型声明但未用,不会进侧车)。
- **类型解析**(forward 表 `k2-forward`,USLSIG3):每个参数/返回的 kind/width/struct-by-value/callback 标志,需要 e3 的类型/声明处理。
- **ccw 计数**:e3 内 `__unisa_ccw_of` 等 ccw 记录统计。

e2 输出(pp.text)没有:符号表、调用图、类型 kind/width、ccw。**e2 结果不足以推导未定义符号集**;「e2 末尾低成本补齐」等价于在 e2 里重写一个声明解析+调用扫描,这就是 e3 的 parse/type/scope 三网,不是低成本。

可行替代(保留「免首遍整程序 e3」的收益):
- **Y1 「声明切片探针」**:首遍不跑整 tokens 的 e3,而是对每单元 e1 输出按「文件作用域声明 + 函数体内标识符引用」取子集喂 e3(run-forward)。风险:UD 可达性需要函数体(调用在函数体内),切片不能去掉调用,只能去掉语句级非调用 token——收益不确定,需实测。
- **Y1' 「侧车来自上一次整程序 e3」**:首遍 e3 的产物(USLFW1)已带 tape;`dv_fwd_merge` -> `fwd_incremental` 的快路本应复用这条 tape 只增编桩单元,但 B1-diag 证实快路回退是设计内(编译器自身取函数地址需整程序重放)。**最直接的省法**:重放时不再整程序重新 e3,而是把首遍 tape 作为检查点,只编桩单元 e3 后做 tape 级合并(fwm_merge 已存在)。差别在 `fwd_incremental` 对「取函数地址」的拒绝条件;需把取址名也进 `fwm_export` 白名单后验等价。这是 §5 候选 Y-A。

结论:(1)「e2 推导」路线不成立,改为「首遍 e3 结果作为检查点、第二遍只编桩单元+合并」;(2)若 Y-A 合并不等价,则保留两遍 e3 但把第二遍分批(§2)。

## 2. e3 分批续跑的切分维度与跨批最小状态

e3 的网状态分 `fresh:U`(单元级,unit 边界重置)与 `fresh:P`(程序级)。tape 帧已按 `.unit N` 记录分单元(formats.md:38)。可行切分维度:**单元**(而非函数:函数级需切 tape 中段且 UD/forward 的 RECORDS/SEEN 跨函数;全局索引 = 单元序号 + 单元内 token 偏移,只有单元序号是稳定边界)。

跨批必须携带的最小集合:
1. **属性表**(USLATTR1:4+9*count,已经由 `st_attr_exchange` 在阶段间交换,内有 unit 索引)——已是字节产物,直接落盘。
2. **fresh:P 计数器 + UD/FW 的 RECORDS/SEEN**(程序级标签/调用/签名记录)——当前只在执行器内存。需要新增 e3 的「程序级状态侧车」输出/输入资源(类似 `\0cli/attributes` 的注入模式,st_pub 加一个 `\0cli/e3state`)。
3. **ccw 计数与 USLFW1 累积**:桩只需最终计数;可在末批合成。
4. 检查点文件 = holdframe 格式扩展:`[frn][atn][statn] frames attrs e3state` + SHA256 收据(tools 侧已有 `fr_add` 记录格式)。

缺陷修复一并入:`src/driveunits.cx:46` 的 `hf_phase_b` 提前返回必须先 `st_units(nsrc)` 再返回(单元数 1 => rt-shared 宏不加,与 whole 语义不同;plans/v0.0.49 §3-B 已登记)。

## 3. 验收 oracle

| # | 检查 | 实现 |
|---|---|---|
| O1 | 同源中等程序(六模块+`sys.dlcall` dlopen/dlsym/dlclose 转发分支)normal-whole 镜像 vs 新路径镜像 cmp0 | 复用 out/mac-medium-forward-dlcall-proof.cx 的方法:同一候选 CTOR、`--target=osx/x86_64` 与 `win/x86_64`,default 不设 UNISACC_PHASE;新路径走产品原生检查点;`cmp` rc0 且 SHA 相同;桩文件 bytes/sha 一并记录。登记为批次 `check-y-oracle`。 |
| O2 | 两代六片不动点 | tests/chain-fixed-point.cx 现有 F1/F2/F3,候选 Y 由 candidate-chain 跑。 |
| O3 | Mac/Win 每相余量 >=30% | 每相 `WORKER_TIME`/`RR_TIME` 实测墙钟 <=42s(60s 闸 x0.7);写入 phase-times.tsv;A1/A2/B 三相各自判。 |
| O4 | 产品不含物理根 | 两个不同物理根(`git archive` 到 /tmp/r1、/tmp/r2)构造同切片,切片/内核/P4 SHA 相同;帧/收据允许不同。对应 plans/v0.0.50 §1b `check-root-independent`。 |
| O5 | 负控 | O1 故意改桩一个字节 => cmp 非 0;hf_phase_b 不调 st_units => O1 多单元程序 cmp 非 0(证明修复有效)。 |

## 4. 风险与回退

- Y-A tape 合并不等价(取址函数、static 名隔离、ccw 顺序)=> O1 非 0:保留两遍 e3,仅做 Y-B 分批;X 工具保留到 Y-B 通过 O1 才删。
- 程序级 e3 状态侧车格式新增 => 网/执行器改动波及 shared-e3/warnparse/errorparse 三网(table-deps.tsv),需 gold 重建;放单独候选。
- 单元级分批每批重装 e3 模型 ~1s(RR_TIME model inflate),批数 x 开销可能吃掉余量;按实测定批大小。
- 回退办法:候选独立 SID,不合格则不进 release-decide;X 代码路径不动。

## 5. 候选拆分(每个独立 SID,不混工具改动)

| 候选 | 内容 | 文件 | 预计行数 |
|---|---|---|---|
| Y-0(最小,先做) | 修 `hf_phase_b` 提前返回前调 `st_units(nsrc)`;检查点文件加 SHA256 收据行;oracle 工具+批次 | src/driveunits.cx(+6);cx/runtime/holdframe.cx(+25);tests/check-y-oracle.cx(新,~180);tests/batches.tsv(+1) | ~210 |
| Y-A | 首遍 e3 tape 作检查点,重放只编桩单元并 fwm_merge;取址名进 export 白名单 | src/driveroute.cx(~60);cx/compiler/fwdincrement.cx(~40);cx/compiler/fwdmerge.cx(~30) | ~130 |
| Y-B | e3 按单元分批:`\0cli/e3state` 程序级状态侧车进出;st_span 新 first/last=`e3:k..m`;驱动按 UNISACC_PHASE_IN/OUT 续跑 | src/stages.cx(~50);src/drive.cx(~40);exec/parse2 forward/unresolved 表(状态导出,~60 行表)+生成器 check;cx/runtime/holdframe.cx(~40) | ~190 + 表 |
| Y-C | 删 X:tools/whole-front.cx、templates/*.in、相关批次与 pkg-gaps 第 18 行闭合 | tools/、tests/batches.tsv | -700 |

顺序:Y-0 -> Y-A(O1 判) -> Y-B -> Y-C。Y-0 不改网、不改 SID 语义以外的输出,风险最低;本窗口先实现 Y-0。

## 6. Y-B 精确设计(e3 内按单元续跑;cc 2026-11-10 采纳 Y-A 否决后转入)

**事实(已读代码)**
- e3 输入 = units 网产出的 UNITOK2:单元目录 + 带 `@ unit offset LF` 前缀的 typed token 流;`@unit0`/`@unit+` 标记 token 也带前缀(exec/pipeline/formats.md:337-342)。
- e3 网在 `startup-marker` 段识别 `unit+`:control-byte.tsv:13 `MU4 43 -> MUtd.test [ADV; unit_epoch += 1; ixcount = 0; td_unit_scan = 1]`。这是**唯一的单元边界动作**,也是 Y-B 的切分点。
- e3 是一次 core_run(cx/core/execute.cx:188);可变状态全部在 CoreMachine(execute.cx:43-50):`regs[nrg]`、`x/xattr/xn`(当前输入与属性)、`r`、`ot`、`osel`、`out`(已写 tape)、`err`、`scratch`、`stack`、`frames`、`memory`(稀疏 key/value)、`blobs`、`strings`(intern)、`resources`。
- cx/compiler/fwdmerge.cx 的合并器在 Y-B 中不参与(Y-A 已证明合并不等价,实验 diff 作负控素材:/tmp/unisacc-fable/out/ya-diag/ya-experiment.diff)。

**切分维度**:单元(unit_epoch)。批 = 单元区间 [k, m)。一个 e3 调用从检查点 k 恢复,跑到第 m 个 `unit+` 标记前停下并写检查点 m;最后一批跑到 ACCEPT 产出完整 tape(+USLFW1 侧车)。

**跨批状态最小集(USLE3ST1 侧车,全部来自 CoreMachine)**
| 字段 | 来源 | 说明 |
|---|---|---|
| model_sha | pm_model 原始字节 SHA | 模型不同即拒 |
| input_sha | UNITOK2 输入 SHA | 输入不同即拒 |
| unit_k, pos | unit_epoch 与 F->i(顶层帧读位置) | 恢复后从 pos 继续读 |
| regs[0..nrg] | s.regs | 全量,LE64 |
| r, ot, osel | s.r/s.ot/s.osel | |
| out.b[0..out.n], out.at | 已写 tape 及属性 | 后续批追加 |
| memory (n 对 key,value) | 稀疏表 | 符号表/类型/UD RECORDS/SEEN 全在此 |
| blobs[2..n](跳过 0=""、1=src) | 字节 | intern/保存的子串 |
| strings(intern 条目 b,v) | | |
| stack | 应为空(MU4 位于 UNIT 顶层) | 非空即拒绝做检查点,回退整跑 |
| frames | 仅顶层帧(n==1) | n!=1 同上 |
| resources | 不存:恢复时由 st_pub 重新注入(同批前后 cli 资源必须相同,st_pub 已保证) |
属性表 `\0cli/attributes` 与 USLATTR1 仍由 st_span/st_attr_exchange 在阶段间传递,不进此侧车(本来就跨阶段)。

**检查点格式**:`USLE3ST1\n` + LE64 字段表(count,每项 tag(u8)+len(u64)+bytes)+ 结尾 `.end`;外层沿 Y-0 的 holdframe 收据:`<path>.sha256` 64hex+LF。文件名 `<UNISACC_PHASE_OUT>.e3.<k>`。

**切分点代码位置**
1. cx/core/execute.cx core_run:新增两个注入点(不改模型):(a) 入口若 `CORE_RESUME` 非空,先 core_state_load 填 CoreMachine 再进循环,q 取保存的 state;(b) 在 action_run 之前,若 `stop_unit >= 0` 且当前状态进入 `MUtd.test`(= 刚执行 unit_epoch+1 的那条转移,用保存的 sq 标识)且 unit_epoch == stop_unit,则 core_state_save 并以 status=5(CHECKPOINT)返回。模型状态号在生成期由 generator 写进 `exec/facts/e3-checkpoint.tsv`(state 名 -> 编号),执行器读事实表不认字面量(符合「不特判」规则)。
2. cx/core/ 新模块 `cx/core/state.cx`(~220 行):core_state_save/load,序列化上表;不碰 memory.cx(那是 cm_* 所有权模块,不是 CoreMemory)。
3. src/executor.cx ex_exec:透传 resume/stop 参数(新增 ex_exec_span(..., resume_path, stop_unit, out_path));status 5 不是错误。
4. src/stages.cx st_span:first/last 形如 `e3@k`/`e3@m`:`rs_take` 只认 `e3`,后缀由 st_span 剥离并传给 ex_exec_span;其余阶段不变。
5. src/drive.cx / driveunits.cx:`UNISACC_PHASE=B` 之外新增 `UNISACC_E3_SPAN=k:m` 驱动参数(产品公开接口,仅用于分相工具,不改默认 whole 行为);Y-B 落地同时把 Y-0 在 drive*.cx 的阶段逻辑下沉到 holdframe/state(回收 srcshape 豁免)。
6. tools:无新工具;chain-step 的 Mac/Win 六片改调 `UNISACC_E3_SPAN`,删 whole-front 与 templates(= Y-C)。

**验收(O1-O5,沿 Y-0 oracle 扩展)**:check-y-oracle 增第四路 `span`:A 相(units..e3@0 写检查点)-> e3@0..3 -> e3@3..6 -> e3@6..end -> prune..elf,镜像与 default cmp0;负控:改检查点一字节拒绝;改输入一单元拒绝(input_sha);Y-A 实验 diff 作负控素材(合并产物 != 重放)。两代六片不动点由 candidate-chain;每相 <=42s。

**预计行数**:cx/core/state.cx +220;execute.cx +40;executor.cx +30;stages.cx +25;drive/driveunits 净 -20(下沉);tests/check-y-oracle.cx +60;exec/facts/e3-checkpoint.tsv + 生成器 check +30。

**风险**:(1)MU4 处 stack/frames 非顶层 => 检查点拒绝,退整跑(记 UNMEASURED);(2)blobs 可能含指向 input 的借用(INPUSH 用 blob 索引,保存时按字节复制,恢复后一致);(3)模型状态编号随网重建变化 => 事实表由生成器产出并带 --check。
