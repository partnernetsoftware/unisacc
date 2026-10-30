# 0.0.43 发布：重新来（2026-10-30）

## 目标

可发布的 `unisacc.com` 0.0.43 要同时满足三条：

1. 由 HEAD 源码用新 CX 入口自己构造出来。
2. 两代判词：第一代编出的产品再编一次，五片逐字节相同。
3. 同一候选上 `tests/run.cx -- --all` 的结果为 QUALIFIED。

签名、Draft、公开等主人授权。

## 原则（主人定）

核心是 nn 推理字节流，用它把 8 个流和 18 张网串起来，产品应当优雅、简约。
- 能写成模型步的差异，就不在代码里写特判。
- 平台差异只放进 L0（cx/sys）或表里。
- 构造链也照这条办：不在旧链上打补丁，写一条短的新链。

## 主刀与分工

- **grk 一人主刀发布链**（AGENTS.md：慢车道一次只一个主刀）。构造、安装、产品源都由它改。
- **cc 只做三件事**：给方向、审每一个提交（看是否符合范式、有无回退）、按 grk 的请求补 L0 和工具。构造和安装不另派子代理。
- **bdy、csih**：不碰发布链的文件。

## 新链（取代 package-refresh、product-build、candidate-install 这条旧链）

旧链读的是已归档的 C 源（exec/c/core.c、asmcompiler.c），收的是旧的五片收据。它不再修，等新链跑通后归档。

1. **hostnative 变薄**：clock、utimes、loader 都转到 cx/sys，去掉只认 Linux 的 `#error`。osx 和 win 片要能原生运行。
2. **新写一个工具 `tools/release-build.cx`**，一个命令走完以下五步，每一步都写出身份（sha256）：
   1. **P3**：模型加上 `include/` 携带的头，其中 `\0hdr/cx-flat.h` 必须与树里的文件逐字节相同。复用现有的打包模块，不经 package-refresh。
   2. **五片**：用 `tools/entry-build.cx` 每个目标编一片。lnx 走 33 个对象；osx 和 win 在 object 模型步落地之前先用整只 `-o`，并在记录里写明 route。
   3. **wrap**：用 `seed/ape.c` 包装，生成 `unisacc-next.com`。
   4. **第二代**：用第一代的产品再编一遍，五片两两逐字节比较。
   5. **安装**：两代相同，且 `tools/check-install-fixes.cx` 4/4 通过，才替换 `./unisacc.com`。新身份写进 prd.md。
      安装后恢复这三处 `use`，删掉手抄原型：`cx/driver/input.cx` 的 `sys.fs`，`cx/runtime/model_decode.cx` 和 `cx/runtime/model_lifetime.cx` 的 `mem.bytes`。三处注释标了 `TEMP-UNTIL-INSTALL`。根子是已装产品不按真实路径去重模块（安装修复 dedupe）。
3. **osx 和 win 的 object 模型步**：在模型里补上路由，走"事实表 → 生成器 → 网"。不阻塞第一次安装，装好以后再做。
4. **测试**：在新产品上跑 `tests/run.cx -- --list`，用 all-waves 跑所有不需要材料的批次，再补材料、实现 PLANNED 的 19 行，最后跑 `--all`。

## 任务板（`sh ./unisacc.com tools/release-board.cx` 按此表计数；每轮巡检对比 open 有没有下降）

发布缺口以域矩阵里的必需格计，不靠估算。2026-10-30 起点：必需 825 格，已覆盖 463，MISSING 211，NEEDS_FOREIGN_HOST 79，NEEDS_PRODUCT_CHANGE 20。

- MISSING 的格：写批次或断言，在 lnx 上实跑 PASS 后改标 COVERED_LOCAL，带 batch@candidate 证据。
- PRODUCT 的格：先修产品。
- FOREIGN 的格：安装之后由 CI 在各宿主上实跑。

grk	stream-e3,stream-elf,stream-lower,stream-e1,stream-e2,stream-e4,stream-prune,bootstrap,candidate,phase-timing,optimization,formats
cc	ri,resources,negative,hosts,library,interfaces,properties

- **grk**：先做发布链（hostnative → release-build → 安装），再做所负责域的 PRODUCT 格和 MISSING 格。libunisacc 的 us_* 库 API 在活跃区没有实现（实现随 e16028ffa1 一起归档了），这是产品缺口，也归 grk。
- **cc**：做 ri、resources、negative、library、interfaces、properties 这几个域的 MISSING 格；安装之后负责在 CI 上跑 FOREIGN 格。
- **bdy 不在发布板上**：主人安排它专做 cx 核心底层和上层封装，cc 不给它派发布格。

### bdy 域格的材料依赖（2026-10-30，bdy 实测）

- **properties 已清零**（missing=0）。
- **library 30 格卡在「产品缺口 + P3 材料」**：`tests/cases/p3_library_members.c` 第 15 行 `#include "exec/c/libunisacc.c"`，但 `exec/c/libunisacc.c` 与 `exec/c/run.c` 已被 e16028ffa1（10-29）归档到 `archive/reference-c/exec/c/`，活跃区无 `us_new`/`us_free`/`us_add_*` 等库 API 的任何实现（symindex `def us_new` count=0）。14 格断言已写在 p3-library-members.cx 的 api_expect，另 16 格缺断言；全部要等 grk 重新提供 libunisacc 活跃实现（产品缺口，归 grk）+ P3 包（release-build 第一步）才能实跑。
- **interfaces 11 格缺的材料**（等 P3 或材料打包，reason 按域矩阵）：
  - draft（2）：if.attr.sidecar-ownership、if.edge.consumer-frames —— 对应流/网草案未定。
  - 缺语料（1）：if.edge.corpus —— 需要语言形态语料，非两个 fixture。
  - 缺 tapecheck 材料（1）：if.route.tapecheck-pass —— streams.json tapecheck_included=false。
  - 缺注册批次（1）：if.route.tapecheck-neg001 —— 无批次；产品现报 `unknown opcode`（非 spec 的 `Tape lexical form`），产品诊断文本待 grk 对齐。
  - units/multi 未打包（1）：if.route.units。
  - memory entry 未打包（1）：if.route.memory。
  - 材料仅 O2（1）：if.route.optimization-levels —— 需 O0/O1 链材料。
  - 缺断言（1）：if.route.prune-placement。
  - 无六目标链批次（1）：if.target.foreign-chains —— 需外宿主 CI。
  - 本地列不全（1）：if.host.lnx-x86_64 —— 同一候选汇总行。

## 进度（grk 每完成一步，在这里写上提交号）

- [ ] 1 hostnative
- [ ] 2 release-build：P3 / 五片 / wrap / 二代 / 安装
- [ ] 3 object 模型步
- [ ] 4 测试到 QUALIFIED
- [ ] 主人授权发布
