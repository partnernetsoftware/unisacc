# A1a 先红基线与单元切分(0.0.88 预备)

状态:预备成果(2026-11-20,预备者实跑,文档写者落库)。语料与基线在 `tests/a1a/`(见其 README.md)。未核项原样标注,未扩写。原预备报告(REPORT.md)未随材料保存,下文单元表按主调度简报转录。

## 基线摘要(已装 0.0.85,lnx x86_64,56 样例)

| 分类 | 数 | 说明 |
|---|---|---|
| 静默留下未定义符号 | 16 | -c 成功,对象留 UND __builtin_*,链接或运行才报错 |
| 具名拒绝 | 19 | not covered 等具名文案 |
| 非具名拒绝 | 3 | a04 c02 c05,报语法错误类文案 |
| 行为错误 | 1 | t18 constructor 优先级被忽略,执行顺序与 gcc 相反 |
| 接受非法写法 | 1 | t20 constructor("x") 被接受 |
| 已一致 | 4 | e01 t14 t15 t19 |

边界例:a05 a06 a07 b12 t19 五例 gcc 只警告,改判边界例。

注:baseline.tsv 的 diff_class 列原始计数为 silent-und-at-c 15、named-reject 16、both-reject-named 7、both-reject 7、reject-other 3、same 4、wrong-behavior 1、u-accepts-invalid 1(合 54 行,t07 两文件一行);与上表 16/19 的归并口径未逐条对齐(未核)。

## 最严重发现

位运算内建(clz ctz popcount parity ffs bswap)与 unreachable/trap 在 0.0.85 下 -c 静默成功,对象留 UND `__builtin_*` 未定义符号,链接或运行才报错。这是错误代码,不是具名拒绝。

## 单元切分

| 单元 | 内容 | 验收样例 | 期望 | 并行与合入顺序 |
|---|---|---|---|---|
| U0 | 未实现内建一律具名拒绝 | b、c、u 系全部现静默例 | 不再出现 UND __builtin_*,改具名拒绝 | 最先合 |
| U1 | 位运算 clz ctz popcount parity ffs 加常量折叠 | b01 b02 b03 b04 b06 b07 b08 b09 b10;拒绝例 b11 b14 b15 | 运行输出与返回码同 gcc;拒绝例具名 | U1 到 U4 写域不相交,可完全并行 |
| U2 | bswap | b05 | 同 gcc | 同上 |
| U3 | unreachable trap | u01 u02;拒绝例 u03 u04 | u02 按 rc>=128 判 | 同上 |
| U4 | asm 标签 | a01 a02 a03 a04;a08 拒绝;a05 a06 a07 边界 | 符号表有改名符号 | 同上 |
| U5 | weak | t04 t05 t06 t07 | 绑定 WEAK,覆盖语义同 gcc | U4 之后 |
| U6 | alias | t01 t21;拒绝例 t02 t03 | 别名符号存在 | U5 之后 |
| U7 | section | t10;拒绝例 t12 t13 | 节名与标志同 gcc | 字节风险最大,最后合 |

每单元输入身份:开工时的 v0.0.88 分支提交号,验收比对冻结期望 `tests/a1a/expect.tsv` 与 `baseline.tsv`(原报告逐单元输入身份表未保存,未核)。

## 造价

- 核心约 3.6 单位,超 3 单位线。
- 压回约 3.2 的办法:parity ffs 用通用展开;section 不做 __start_/__stop_ 边界符号(t11 留具名拒绝)。
- 建议移出本版:visibility 0.2(t16 t17)、weakref 0.2(t08 t09)、constant_p choose_expr types_compatible_p 1.1(c 系)。
- constructor 优先级 0.3 是现有缺陷,走快车道单独修(build/pkg-gaps.tsv CTOR-PRIORITY-SILENTLY-IGNORED)。

## 风险

- 28 示例与 17 整程序 -o 逐字节不变;netregen 60 格不变。
- attr-byte 与 IDROOT 重生成后网形变,要对拍 e3 步数增量。
- elfobject 改动影响所有 ET_REL 对象。
- osx win 只保持具名拒绝。

## 未验证

- 只跑了 lnx x86_64,arm64、osx、win 没跑。
- 单位数为推断。
- 28 示例与 17 整程序清单没核。
