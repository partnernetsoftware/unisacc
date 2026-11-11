# C2 g5：产品 typedef 单元隔离，云机预检（2026-10-09）

实际跑过；不是正式候选或完整 C2 结算。基线 main `235ca5bd`，cwd
`/home/box/repos/unisacc`，原生 Linux x86_64；封版仍由 m4pro 执行。
身份及八变体收据见 [JSON](c2-g5-unit-typedef-cloud.json)。

## 反例与修片

原 `research/g5-typedef-leak` 六组双向输入，在 typed E1 下的产品 E3 已与
修后参考逐字节相同，不能将参考原来的参数遮蔽反例直接算产品 bug。
真正产品反例为两个独立单元：

```c
/* a.c */
typedef struct { int x; } code;
int helper(void) { return 0; }
/* b.c，单独的翻译单元 */
int main(void) { return (code){1}.x != 1; }
```

参考拒绝 `unknown identifier`，旧产品 E3 接受了第一单元泄漏的 typedef。
同单元显式声明 code 时应接受。C99 6.2.1p4 的文件作用域以翻译单元为界。

修片仅在 `TD.put` 记下 typedef intern ID 上界，并在 `@unit+` 逐 ID 撤销
有效 TDN 及其 TDE 标记；TDB/TDD、结构描述池、函数签名保持共享。
无新执行核原语。清理工作随已见 intern ID 上界增长；尚未测 F4′ 的
15 单元冷编影响，不据此结算性能项。只有有效 typedef 标记才清 TDE，
不会将所有同名枚举值或描述符全表清空。

新增回归接入既有 `exec/c/multicheck.py` 的 isolation 段；七份原最小例
复制入 `tests/multi/typedef-*.c`，处于已有门禁输入闭包。原测试保持。

## 云机实跑

每个进程经 `tests/bound.py` 约束 ≤60 秒，生成变体内层看门狗 55 秒。

- Python 构造普通 E3、typed/sourcefacts E1；原六组双向完整 E3 tape 同参考。
- 同一新 E3 转为 tbl/net，原生 C 网络执行器复跑六组同字节；合法全局
  同名对象仍同字节，非法复合字面量与 sizeof 未声明名均拒绝。
- 原生网络穷举 `2559878` 观察、`9923` 状态，动作/字符串同表。
- 同单元结构 typedef 复合字面量、跨单元枚举 typedef 同名对象、枚举名
  泄漏各双向：前两类接受同 tape，最后一类双向拒绝；合法例系统 cc 退出0。
- 七份测试 fixture 的三组、两种顺序：系统 cc 与本机参考 `-run` 都退出0。
- `exec/parse2/unitlocationcheck.py` 全过：独立位置序列化及完整 tape/诊断，
  10 map、7 input-frame 拒绝。
- 另以 errors 变体原生执行器复跑上述结构复合字面量非法/合法 × 双序
  四组，完整退出码、stdout/tape、stderr/诊断与参考完全相同。
- r21 使用临时副本，唯一构造变更是 E3 复用本轮刚生成且记录哈希的普通
  JSON；PP/E1 重建，原生网络结果 `48 identical / 5 named refusal /
  0 accepted-different`。没有改仓内 r21 判据或探针。
- `tests/seedfactscheck.py`：1076 same、0 differ、311 both-error。
- 八个 E3 变体各构造成功，fresh 哈希/计数保持旧基线；只重录对应八条
  `tests/graphhash.tsv`。`tests/freshordercheck.py 8` 的 units 顺序也通过。
- Python 语法检查及 `git diff --check` 通过。

## 阻塞及 m4pro 待验

并发两 Python 变体与 C seed/gen 时，内核 OOM killer 杀掉 C 构造器，
`anon-rss 5932868kB`、退出137；两 Python 变体亦触发55秒看门狗。
已向董秘报告，后续变体串行全部通过；不放宽时限、不将被杀进程算通过。
C 构造器逐字节同 Python 的验证仍未完成。

m4pro 在正式候选窗口需要补：

1. 同源 C seed/gen 构造完整 parse2 与 Python 逐字节对拍。
2. 既有 `exec-multi-cc-isolation`、`exec-multi-ua-isolation`、
   `exec-multi-asm-isolation` 全片，含新增 driver `-run`；云机没有跑完
   这三个整门禁，不能把 E3 同字节代替它们。
3. 正式候选 `.com` 编译/运行 `tests/multi/typedef-{a,b,s1,s2,sm,g1,g2}.c`
   的三组双序，并复验非法跨单元复合字面量拒绝与同单元合法例通过。
4. 完整发布验收仍按原计划和门禁；本片不结算 C1/C3/F4′ 或整个 C2。

云机未生成或发布 `.com`，未 push；封版不迁到云机。

提交时 main 已前移 `666bb0bc`（论文及 0.0.38 计划文档更新，编译输入未改）。
云机缺 Git 作者配置，本提交用命令级 `cdx-unisacc <cdx-unisacc@localhost>`；
未修改全局身份。
