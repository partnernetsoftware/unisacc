# 缺陷:调用实参个数与原型不符,编译器不报错(2026-11-02)

默认-run(已装快车道候选)实测,均无诊断:

1. 过少:`tx_find(a,"b")`(原型 4 参)→ rc0 编译通过,运行读未初始化参数(另一变体在 `sy_exists` 参与的三目里 rc139 段错误)。
2. 过多:`static int f(int a,int b)`;`f(1,2,3)` → rc3,多余实参被静默接受。

ISO C99 6.5.2.2:有原型的调用实参个数必须匹配(约束违反,应诊断)。今天已造成两次误判(omp-go 的 tx_find 两参导致 check 段错误)。

复现:`out/elft/tern-segv.cx`(rc139)、`tests/fixtures/defect-arity/arity1.cx.repro`、`tests/fixtures/defect-arity/arity2.cx.repro`(在 out/elft 目录下运行,目录内 sys/text 为符号链接)。
