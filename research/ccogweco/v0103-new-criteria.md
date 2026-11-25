# 0.0.103 重新入车的新判据(一页方案,只读起草,不跑基准)

归 0.1.x;原五夹具判据标注"不可复现"(s2..s5 源文与 e3 材料 model-004.bin/s.tok 均已丢失)。以下判据不依赖已丢材料,全部锚定"当次候选"自身。

## 1 夹具集(四类)

| 类 | 夹具 | 来源 | 判据作用 |
|---|---|---|---|
| A 身份锚 | one.cx | 仓内 `tests/cxcc_one.cx`(sha 93b0d05b...) | 唯一有历史全 sha 的夹具;仅当模型指纹与历史 raw=625155/model-004 一致时,才把输出等于 ca56363a...528a 作为额外硬检查;模型不一致时只用"内联前后相同" |
| B 历史前缀 | s1 | `int main(void){return 5;}`(V0103-ACTION-DISPATCH.md 逐字) | 输出 sha 前缀 efb9baea(8 位)可作弱交叉核对;不作硬门 |
| C 新写真实夹具 s2'..s5' | 两层 for / 40 个函数调用 / while(i<n)i++ / 四变量异或混合(按原散文定义重写) | 本方案附录给出最小写法 | 相对判据主体;不要求步数等于历史值 |
| D 合成斜率 | peraction K=1..16 | `lane-jobs2/peraction.cx`、`peraction-asm.cx` | 只作斜率显著性旁证(判据仍 >=20% 才做原型),不作回归门 |

C 类每条的步数规模用当次候选实测后记录(目标落在 1e4 到 1e6 步量级),不预设与历史数字相等;清单与实测三元组一起进证据。

## 2 材料与产物

- 模型:当次候选自带模型;记录模型文件 sha256 与 `mdl_index=4` 对应的 raw 尺寸(历史为 625155,只作注解)。不从旧 /tmp/e3s 取任何东西。
- 每夹具产物:`stream e2 -> e1 -> e3` 得到 tok 与 tape,命令沿用 core-timing 文档;保留 tok/tape 的 sha256 供复跑。
- 测量工具:`~/.local/share/unisacc/research/core-timing/e3-run-profile.cx`(现有),输出三元组 (steps, actions, out_bytes) 与每次墙钟;产品与原型核用同一工具、同一模型、同一 tok。

## 3 怎么测 ns/步(相对法)

1. 每夹具、每核各跑 5 次,交替顺序 A B A B A(先各一次丢弃预热)。
2. ns/步 = 中位墙钟 ns / steps。三元组只用于核对(内联不得改变 steps/actions/out_bytes)。
3. 报告每个夹具的 中位、四分位距(IQR)与比值 `r = 原型ns/步 / 基线ns/步`;总收益 `g = 1 - 中位(r_s1..r_s5 的均值)`。

## 4 阈值(用相对判据表达)

- 正确性:五个真实夹具(C 类及 A/B 可用部分)内联前后 `out_bytes`、输出 sha256、steps、actions 逐格相同;任一格不等即整体不落。
- 单夹具回归:`r_i > 1.02` 任一夹具即整体不落(沿用 2% 门)。
- 总收益:`g < 0.10` 即整体不落(沿用 10% 门)。
- 选择性内联:禁止按夹具或按动作选择性内联;回退是全局决定。
- 斜率旁证(仅 C/D 之前的老结论是否仍成立):peraction 重拟合斜率/固定比 >=20% 才允许做原型;否则收口,不因"新夹具好看"翻案。
- 不可测即不测:任一夹具 IQR/中位 >5%,该夹具标 NOISY-UNMEASURED,整体判据不成立,择空机重跑;不得放宽阈值。

## 5 空机条件

- 起跑前 load1 < 2(或多核时 load1 < ncpu/8,取小者)且无在途整链/预跑/变异门/宽并发 affected;冻结解禁后再跑。
- 全程串行,同一机器,不开其他基准;每次读 CPU 频率是否被降频(记录 governor),发现降频即整轮作废。
- 每轮开始结束各记一次 load1 与时戳,写进证据。

## 6 证据与标注

- 一个 TSV:夹具、核(A/B)、run#、墙钟 ns、steps、actions、out_bytes、输出 sha、tok sha、模型 sha。
- 结论行:每夹具 r_i、g、噪声标记、是否落。
- 发布证据必须写明:"原五夹具判据(s2..s5 逐字源与 model-004/s.tok)不可复现,2026-11-24 找回调查结论见 research/ccogweco/v0103-fixtures-recovery.md;本版改用上述新判据"。

## 附录 C 类最小写法(散文定义的重写样例,行数与循环界待实测后定)

    s2'  int main(void){int i,j,s;s=0;for(i=0;i<A;i++)for(j=0;j<B;j++)s+=i^j;return s;}
    s3'  static int f(int x){return x+1;} ... f0..f39 各调用一次,main 串联返回
    s4'  int main(void){int i,n;n=C;i=0;while(i<n)i++;return i;}
    s5'  int main(void){unsigned a,b,c,d;a=1;b=2;c=3;d=4;for(int i=0;i<D;i++){a^=b;b^=c;c^=d;d^=a;}return (int)(a^b^c^d);}

A/B/C 三类的循环界 A、B、C、D 与函数个数在首次实测时确定并冻结,写进证据,后续轮次不再改动。
