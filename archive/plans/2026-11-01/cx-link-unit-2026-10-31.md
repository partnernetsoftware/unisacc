# 链接单元：`-funit` 不抄函数体（2026-10-31）

取代 `plans/closure-once-2026-10-30.md` 里「这轮不改 cxmod、不改 entry-build」。那条是当时的范围，不是架构。不定 `STB_WEAK` 仍然有效。

与 `plans/cx-architecture-evolution-2026-10-31.md`（`fffc978409`）对齐：该文是设计提案，不改已生效条款。本文件是实施次序第 1 刀。候选、已安装、跨平台资格分账。

2026-10-31 记账：`b393ac30bc` 的 `-funit` 语义在 `66a632ff` 上只在 `out/dl-stub.cx` 手工链接下看过。`out/link-unit-3/receipt.txt` 的 `failed_phase` 是 `entry.elf`，失败是 `undefined reference to dlopen`。`66a632ff` 不是 entry-build 正规候选，四项不算完整通过。`CX-W-1` 仍未生效。宿主装载符号另刀，禁止桩、weak、吞错误。

2026-10-31 再记账：`out/link-unit-8/candidate` sha `81c50083…` 由 entry-build 自己链上，receipt 有 `entry.elf` rc=0 和 candidate 行，无 `failed_phase`。这只证明无桩静态链接和 `-funit` 符号形状。memory 路由把 process/dl 槽发布为 0；`.hostaddr` 真正使用会失败。`sys.dlcall` 独立程序成功不等于产品自身宿主绑定未退化。不记安装资格，不记宿主合同通过，`CX-W-1` 仍未生效。

2026-10-31 对照记账（只绑候选 `81c50083`，不转授已装，不授安装或跨平台资格）：
- asprintf：`out/cmp-inst-host-asprintf.out` 与 `out/cmp-cand-host-asprintf.out` 都是 `n=3 s=x=7`，两侧 stderr 空。
- `-o` strfry：两侧编译 stderr 空；`out/cmp-inst-host-strfry-run.out` 与 `out/cmp-cand-host-strfry-run.out` 都是 `host-strfry-ok`。
- 持久回归 `6a4ba645e9`：`tests/cx-funit-host.cx` 已登记。coverage 写明候选 10/0，已装 `43fbbb81` 因旧 `-funit` 复制闭包而红。已装红不是覆盖通过，也不是安装资格。`CX-W-1` 仍未生效。
- 我用候选 `81c50083` 复跑：`tests/cx-funit-host.cx -- ROOT CANDIDATE /home/devali/repos/unisacc/out/grk-fh-verify direct`，`CX_FUNIT_HOST checks=10 failed=0 unverified=0`，rc=0。这只绑定这只候选，不转授已装。

## 问题

`-c -funit` 把 `use` 闭包的函数体编进当前对象，符号是全局的。`drive.o` 已有 `tx_eqz`。`path.o` 再 `use text.text` 会再带一份，链接报重复。于是产品对象不敢 `use` 同一份 L0，上层继续留 libc。

整程序（`-run`、多源一次 `-o`）不是这个问题：`cxm_plan` 每个模块只进这一次链接一次。

## 决定

两条路径，不合成一条。

| 路径 | 链接单元 | 函数体 |
|---|---|---|
| `-run`、从源码一次 `-o` | 这一次程序 | 闭包每个模块一份，维持现在的 `cxm_plan` |
| `-c -b OS/ARCH -funit -o FILE.o` | 命令行上这一个文件 | 只编本文件的定义。直接 `use` 只给 CXIF 声明，不编被 use 模块的函数体。未定义符号是合同，不是错误 |

产品构造（`tools/entry-build.cx`）在第二条上：

1. 仍编现有根对象。
2. 再编这些根的 `use` 闭包里、按 `sy_realpath` 去重后还没编过的模块，各一只 `-funit` 对象。
3. 链接全部这些对象。
4. 不编前导 `std` / `std.cx`。定义 `main` 的模块只当根，不因被 use 再编一次（现有拒绝保持）。
5. 六只驱动对象仍是 `-O1`；新拉进来的 L0 / 闭包对象是 `-O2`。

身份用 realpath。两条软链指向同一文件只编一只。不比 inode。

## 不在这一刀

- 不安装。已装 `43fbbb81` 没有这条语义；验收必须跑候选，不能跑 `./unisacc.com`。
- 不改 `-run` 的嵌入。
- 不把 `CX-W-1` 标成已生效，候选链上且 `nm` 证明只有一份定义之后再改 spec。
- 不动 `apps/csih/`、`ujs/`。
- 不顺手把 `path.cx` / `compat.cx` 改成 `tx_eqz`。链上没有重复之后再开那一刀。

## 验收

- 候选对 `src/drive.cx` 做 `-c -funit`：对象里 `tx_eqz` 是未定义，不是定义。
- `text.text` 单独 `-c -funit`：`tx_eqz` 有且仅有一份定义。
- `entry-build` 链出的候选不报 `defined twice` / `duplicate symbol`。
- 同一候选上，一条不带 `use` 闭包冲突的 `-run` 仍退出 0（证明第二条没砸第一条；用候选自己跑，不用已装产品）。
