# unisacc 0.0.98 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 238c84b0711a7d360631bc0fc6ac9dee3066e1de |
| 发布 APE SHA256(unisacc.com) | 07351e2cbfddbc98cb6dfa2945334acf8c1c2d13b0ece5de19af2fe2b5d81a45 |
| 源快照 SID | 248310665f4919586deea25c57f8c7f859a4de45e757befa013d7e4428d12b0e |
| pkg.bin | e0a90a12103a6b738f3bcf365f960e6a275623d56b48665f505d31b69528071a |
| slice-win-arm64 | 8faf8599e24e16a0e1a0b87ad284596ba74e532ebd607abffeadce83886b6277 |
| SHA256SUMS | 8a443086ba7bffcbe52ebbb76a2cb3ac57b7217387a2a3948ab2a8c2037d312e |

## 严格证据

459 批:PASS 457、FAIL 0、UNKNOWN 2、INVALID 0

## 产品改动

### 标号函数局部化一期(e3 命名类,使能项)

- **范围与目的**:按 2026-11-23 裁定将原 `0.0.102`(R60 一期)提前至 `0.0.98` 作为单一产品类交付。本版为使能项,旨在消除跨函数标号名碰撞、为后续 `0.0.103` 大单元冷编译 U7 后端按单元复用与并行解锁函数级标号隔离;一期自身为使能项,不直接省毫秒。
- **核心改动**:
  - (1) `L` 与 `R` 标号按函数名限定,确保不同函数内的局部标号命名相互隔离;
  - (2) 引入独立函数序号寄存器供 `goto` 键使用,隔离不同函数的跳转键空间;
  - (3) 新增先红后绿回归靶子:双函数同名标号 `goto` 用例,验证局部化前冲突、局部化后编译运行正确且既有功能字节等价。
- **回滚入口**:按提交回退 `v0.0.98` 合入提交与 `plans/v0.0.98.md`。

## 已知缺口

- (1) **整体裁决 `NOT_QUALIFIED`**:全套 `s9` `daily`/`release` 长跑未跑通,原生 CI 仅覆盖冒烟子集,不宣称全批资格;
- (2) **流程与工具顺延**:`long/heavy jobs=2` 预跑并发提速与 `tools/release-assemble.cx` 自动拼装工具按 2026-11-23 冻结改裁顺延至 `0.0.99` 落库,本版仍按 `0.0.97` 流程组装与验证;
- (3) **一版一类顺延**:`0.0.99` 定为驱动与运行缓存类(修复 `run-cache` 跨目录缓存键),前端 A(`c99-extern-array-incomplete`、`c99-paren-lvalue-postinc`、库模式结构体成员上限 `64 -> 512` 及 `CXM_MAXU 128 -> 256`)按一版一类原则顺延至 `0.0.100`;
- (4) **已知缺陷(`run-cache` 跨目录缓存键)**:不同目录、同源字节、同相对 `argv` 顺序编译时,后者可能命中前者缓存对象,`.unisa.tape` 内嵌路径是别的目录;影响 `0.0.97` 与 `0.0.98`(非回归,`0.0.98` 未动 `run-cache`),对象代码是否不同未核;规避方式为设置 `UNISACC_RUN_CACHE=0` 或不同目录不共用缓存,定于 `0.0.99` 修复;
- (5) **全批严格预跑结果**:合入后全批严格预跑 `459` 批(`PASS=457`、`FAIL=0`、`UNKNOWN=2`、`INVALID=0`),无失败批;2 项 `UNKNOWN` 为无生产者挂账项(`cx-booter-ape`、`tools-default-regression`),已按第 (6) 条登记见证;
- (6) **无生产者挂账**:`cx-booter-ape` 与 `tools-default-regression` 仍为无生产者 `UNKNOWN`(按 `expected-unmeasured.tsv` 与 `*-deferred.tsv` 见证登记);
- (7) **非 Linux 平台样本与 `ci-early` 证据复用**:`osx`/`win` 实机验证由原生 CI 六格实跑(`run_id=45167911448`, 6/6 `success`)提供;该运行由封口后 `ci-early` 在临时私有 draft 标签 `v0.0.98-early-575a613f47`(跑完已自动删除)上对候选提交 `575a613f474ad9aca4be157dcd6f097eb93404ed` 派发;合入 `main` 得冻结提交 `238c84b0711a7d360631bc0fc6ac9dee3066e1de`(中间仅工具与表合入,不触 SID 闭包与 `.github/`,源快照 SID `24831066` 不变),其摆入的 `unisacc.com`(`07351e2cbfddbc98cb6dfa2945334acf8c1c2d13b0ece5de19af2fe2b5d81a45`)、`slice-win-arm64`(`8faf8599e24e16a0e1a0b87ad284596ba74e532ebd607abffeadce83886b6277`)、`pkg.bin`(`e0a90a12103a6b738f3bcf365f960e6a275623d56b48665f505d31b69528071a`)与正式 `release-run --pack` 产物逐字节相同,经 `release/evidence/0.0.98/INDEX.tsv` 采信并不再重复派发。

## 回滚方案

公开后如需撤回,按 `release/rollback.txt` 执行:(1) 将 `v0.0.98` release 转回 draft 并把 Latest 移回 `v0.0.97` 后读回;(2) 在 `main` 按逆序前向 `git revert` Y2/Y1/X 及功能合入提交并推送,绝不删 tag 或 release;(3) 使用 `tools/install-ape.cx` 从 `~/.unisacc-root-backups/0.0.97-public/unisacc.com` 回装本机 Root 并核对版本。
