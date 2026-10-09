# C2 g4 当前源码复验（cdx，2026-10-09）

实际跑过；只跟 g4，非正式 .com 验收。共享 main 源码身份 d87c8917。
本轮没有重复修代码、没有修改生成表；实现已经在 5a632535。

## 构造源与旧版本差异

公开 v0.0.36 的拒绝由 cc 在独立克隆复现；那是旧发布物行为。
当前 `callcontrol-result.tsv` 已有 `CL.directmany.test` 对 sys=0 的
普通无原型直接调用走 `CL.vdone`，`CL.vend` 也按恢复的 sys=0 选择
`CL.vdirect`。因此不能将旧发布物的 DEAD.na 拒绝当成当前 main 的缺口。

`callcontrol.py` 已在 15bbf1bf 迁成 begin/finish manifest 后删除；云机
/home/box 和 /tmp 未找到所谓 staging 副本。当前构造入口为
`exec/build/gen.py parse2`，manifest 读取声明表并构造 δ JSON。
本轮按此入口重新构造 tbl/net，没有手改 TSV 或恢复旧生成器。
新 JSON/network 与 g5 冻结身份一致，完整哈希及逐例收据见
过程JSON未入库（本文即脱敏摘要）。

## 云机预检

六类九组：原始 g4_a+g4_b 双序、加权无原型单文件、加权原型单文件、
默认 float→double 提升、加权无原型两单元双序、加权原型两单元双序。
每项均将 typed/sourcefacts E1 输出送入原生 C 网络执行器，完整 E3 tape
同参考；系统 cc 与参考 `-run` 均退出0。无原型/原型同顺序的 tape 也一致。

为 cc 补入可直接复跑的精确探针：

- `research/n1-gaps/g4_one.c`：cc 更正的完整单文件，先 `int f8();`，
  调用八个1，之后带原型定义 f8；退出0。
- `research/n1-gaps/g4_prototype.c`：相同调用/定义，仅声明变成完整原型；退出0。
- `research/n1-gaps/g4_float.c`：无原型八参调用，float 变量1.5f 经默认
  提升，double 参数加权结果204.5；退出0。

这三份仓内探针各以当前 E3 原生网络、参考与系统 cc 实跑，E3 tape 同参考。
全部进程经 ≤60秒看门狗，每个编译/执行步骤另限10秒。

cc 已确认：原 `g4_a.c` 单文件缺 f8 定义，只用于显示拒点；真正验收用
完整单文件或 a+b。K&R 定义参考本身报 expected '{'，已由 cc 撤销，
不据此扩大本刀范围、不将它当通过项。

## 独立验收交接

实现 commit：5a632535（早已在 main）；本次交付是复验/探针证据 commit。
cc 在 `~/repos/unisacc-cc` fetch 后，待 m4pro 给正式0.0.37候选，对以下三项
各跑参考与产品，必须退出0且行为一致；验收措辞保持原要求：

```sh
python3 tests/bound.py 60 sh "$COM" -run research/n1-gaps/g4_a.c research/n1-gaps/g4_b.c
python3 tests/bound.py 60 sh "$COM" -run research/n1-gaps/g4_one.c
python3 tests/bound.py 60 sh "$COM" -run research/n1-gaps/g4_prototype.c
```

默认提升探针 `g4_float.c` 是补充覆盖，不代替这三项。
云机 `exec/c/buildcompiler.sh:9` 硬要求 Darwin，候选仍由 m4pro 构建；
没有把原生 E3 对拍当正式 .com 通过，没有结算整个 C2。已向 cc 说明
构造源迁移与当前源码已有实现，不改封版位置，不开其他切口。
