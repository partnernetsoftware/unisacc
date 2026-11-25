# 0.0.103 五夹具找回(只读调查,2026-11-24)

结论:one.cx 已找回(身份=仓库内 tests/cxcc_one.cx);s1 源文在派发文档里逐字存在;s2、s3、s4、s5 只剩定义与三项数字,源文未找到;测量工具 e3-run-profile.cx 与合成微基准 peraction.cx/peraction-asm.cx 仍在盘;e3 模型材料(/tmp/e3s)与 /tmp/q1 已丢。按原样(步数/动作数/输出 sha 全等)复现五夹具判据,只有 one.cx 与 s1 能,其余不可。

## 1 找回的东西

| 项 | 状态 | 位置 / 内容 |
|---|---|---|
| one.cx | 找回(按身份) | `tests/cxcc_one.cx`,内容 `int main(void) { return 5; }`,29 字节,源文件 sha256 `93b0d05bfca448329d526c1561013f46d838bacb1979032066eb1c78c025eae6`;证据:core-timing 文档明写 one.cx 就是 tests/cxcc_one.cx,且 e3 冷编步数 1136829 与 PHASE2 文档一致 |
| one.cx 输出 sha | 找回 | `ca56363acfcabd2d0f7fe8785930885a69d93a478e11ad5f437f896d03b52ca8`(26218 字节 stdout 含 25 字节载体,正文 26193 字节) |
| s1 源 | 找回(逐字) | `int main(void){return 5;}`(V0103-ACTION-DISPATCH.md 夹具表) |
| s1 三项 | 找回 | steps 12410,actions 18829,动作/步 1.517;输出 sha 前缀 `efb9baea`(文档只给前缀) |
| s2 | 只有定义与数字 | 两层 for 循环,60140 / 77765 / 1.293,输出 sha 前缀 `9071d926` |
| s3 | 只有定义与数字 | 40 个函数调用,775008 / 952301 / 1.229,输出 sha 前缀 `2ea216d1` |
| s4 | 只有定义与数字 | `while(i<n)i++`,31205 / 41944 / 1.344,输出 sha 未记录 |
| s5 | 只有定义与数字 | 四变量整数异或混合,90062 / 113384 / 1.259,输出 sha 未记录 |
| 测量工具 | 找回 | `~/.local/share/unisacc/research/core-timing/e3-run-profile.cx`;合成微基准 `lane-jobs2/peraction.cx`、`peraction-asm.cx` |
| 口径 | 找回 | e3 模型 models 索引 4,raw=625155;每夹具 5 次读数、同机交替取中位;动作/步只在 1.23 到 1.52;护栏为参考核与产品 stream e3 输出逐字节相同 |

## 2 找过的地方(全部只读)

- git 全部分支与对象: `git rev-list --all --objects | grep -E '(^|/)(s[1-5]\.(c|cx)|one\.cx)$'` 只命中 `exec/parse2/probes/s1.c..s5.c` 与 `archive/docs-2026-11/research/g5-typedef-leak/s{1,2}.c`;没有 V0103 夹具对象。
- 关键词 `git log --all -S`: `abab-bench`、`V0103`、`peraction`、`action dispatch`、`ns/step`。唯一相关提交是 `982bd7f4de`(只新增 plans/v0.0.103.md 与 v0.0.111.md,不含夹具源)。
- `exec/parse2/probes/s1.c..s5.c`: 内容是 parse2 语法探针(例 s2 为三格 for + 逻辑表达式,s3 为 printf 探针),与 V0103 的"两层 for / 40 调用 / while / 异或"不是同一组;步数也对不上(B2 的判断成立),不可替代。
- 文件名扫描(`~/.local/share/unisacc`、仓内、/tmp 深度 6): `one.cx` 只命中 `.trash/layer-api-*/gx/ga/one.cx`(layer-api 夹具,内容无关)与 `/tmp/unisacc-run-1001` 下指向 `/proc/self/fd/63` 的失效 run-cache 符号链接。
- sha256 前缀 `ca56363a` 扫描: 对 `~/.local/share/unisacc`(含 research 全树)、`~/.cache/unisacc`、`/tmp`(深度 3)、仓内全部 `*.cx`/`*.c` 做 sha256sum,0 命中。原因已定位:`ca56363a...` 是 e3 运行输出(26193 字节正文)的 sha,不是任何源文件的哈希,按文件哈希找永远找不到;one.cx 是靠 core-timing 文档的身份说明找回的。
- `/tmp/q1` 与 `/tmp/e3s` 均不存在;`/tmp/e3s/model-004.bin`、`s.tok`、`/tmp/q1/cx/core/transition.cx` 私有副本都没有备份副本被找到。

## 3 缺什么

1. s2、s3、s4、s5 的源文(逐字),以及它们的 out_bytes 三元组值。
2. s4、s5 的输出 sha(连前缀都没有);s1、s2、s3 只有 8 位前缀,全 sha 未记录。
3. e3 模型材料 `model-004.bin` 与 `s.tok`(决定性材料):即便夹具重写,没有同模型就没法与记录的步数/动作数对拍。
4. one.cx 的 `.tok`(e3 输入流)也未归档,但可由 e2/e1 重建(`stream e2 --source one.cx`,然后 e1),命令在 core-timing 文档里。

## 4 对 0.0.103 重新入车的含义

- 能按原判据做的只有 one.cx(冷编 1136829 步、输出 ca56363a)与 s1(12410/18829、输出 sha 需前缀级核对,只能核 8 位)。
- s2..s5 无法复原成"逐格相同"的验收夹具;按散文定义重写得到的是新夹具,步数/动作数不会等于记录值,不能当原判据用。
- 若 0.0.103 还要入车,推荐把验收改成:one.cx 用全 sha、s1 用前缀,另三条以新写夹具做相对判据(同机交替中位、任一夹具回归 >2% 不落),并在 release 证据里把原五夹具判据标注为不可复现。此建议只陈述,不代编排决定。
