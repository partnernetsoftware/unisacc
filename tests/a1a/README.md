# tests/a1a:A1a 差分语料与先红基线(0.0.88 预备)

状态:只入库语料、期望摘要与基线,未登记批次。每单元一个 `tests/check-a1a-<unit>.cx`(先记 RED)留给 0.0.88 开槽后由实现者写并用 `tools/register.cx` 登记(域 product-frontend,同步 tests/coverage.tsv 与 tests/manifests),登记前先过 dvqualify 单提交资格布局自测。计划与单元切分见 `plans/a1a-baseline.md`。

## 内容

- `cases/*.c`:56 个样例源(t07 两文件保留 `_a`/`_b` 后缀)。前缀:a 为 asm 标签,b 为位运算与 bswap,c 为 constant_p/choose_expr/types_compatible_p,e 为 expect,t 为符号属性(alias、weak、weakref、section、used、noinline、visibility、constructor 优先级),u 为 unreachable/trap。文件名带 `_rej` 的是期望拒绝例。
- `expect.tsv`:gcc 期望摘要,开发期生成后冻结,检查运行期不调 gcc。列:sample、gcc_rc、gcc_out_sha256(运行输出原字节)、syms_norm_sha256、sects_norm_sha256、note(gcc_run_rc)。无对应输出记 `-`。规范化:符号表每项取 Type Bind Vis 节类(UND、ABS,其余记 DEF)Name,一项一行空格分隔;节表每项取 Name Type Flg;均不含地址、大小、偏移。原始 gcc 输出(编译输出、运行输出、readelf 符号表与节表)不入库,留在预备作业目录 `~/.claude/jobs/2798e440/tmp/prep-a1a/res/gcc/`。
- `baseline.tsv`:先红基线表,一样例一行,列为 feature、sample、gcc_rc、gcc_run_rc、gcc_out、u_c_rc、u_obj_und_builtin、u_run_rc、u_out、u_msg、diff_class。产品侧为已装 0.0.85,只在 lnx x86_64 跑过。u_msg 中的路径是预备时临时目录(已写成 `~/` 开头),只作证据保留。
- 生成驱动 baseline.cx 与摘要脚本不入库,留在作业目录(仓外运行须在脚本旁放 cxroot.tsv 映射 sys、mem 等前缀,见 build/pkg-gaps.tsv PREP-OUTSIDE-SCRIPT-USE-NEEDS-CXROOT-LINK;sy_run 无截止参数,见 SY-RUN-NO-DEADLINE-ARG)。

## 比对规则(给后续 check-a1a-*.cx)

- 拒绝例的期望是具名拒绝关键串,不比 gcc 原文。
- 比符号名、绑定(GLOBAL/WEAK/LOCAL)、可见性、节名与标志、运行输出与返回码,按上述规范化取摘要比对。
- u02_trap 期望为被信号终止(gcc 在 timeout 下 rc 132,即 SIGILL),按 rc>=128 判,不比字面。
- a05 a06 a07 b12 t19 五例 gcc 只警告,判为边界例,期望由 0.0.88 实现者定。
