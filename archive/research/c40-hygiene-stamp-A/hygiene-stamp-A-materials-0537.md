# 卫生 A 材料刷新（tip dbb4b10d；对照 0506；cc，机房主任 05:37）
**材料 ≠ 真审 ≠ 刷戳。** 只是只读刷新：不勾已审，不刷戳，不改 gatedeps，不跑门。共享检出没动。

## 0. 方法
- 私有 wt /tmp/cc40-prep/oc-0515/wt：fetch 后 origin/main 与 HEAD 都是 **dbb4b10d98af69a3b07fae48b9bcac7761bb8b8b**，status 0 行。
- 470b2b49..dbb4b10d 之间，tests/gatequeue.py 和 tests/gatedeps.json 的 diff 都为空，算法和声明没变。
- 复算脚本：/tmp/cc40-prep/gi1/inv.py（sha256 ff730eea12c2aaa7a7162f4a9b921d84d5592402e8fea52c583ee9d29d5c1d58），用 `env -u UA -u MODEL_COM -u UA_RUN` 运行。**这次 rc 写进了文件**：/tmp/cc40-prep/sa3/inv-dbb4b10d.rc，内容为 inv_rc=0。输出文件为 inv-dbb4b10d.out 和 inv-dbb4b10d.json。

## 1. 与 0506 的差异
- 470b2b49..dbb4b10d 之间改动的文件：research 下 4 个（paper-a 笔记）和 tests/seedoptconsumercheck.sh（b4aab000、dbb4b10d），共 5 个。**exec/include/kernel/src/unisa/weights 六个审核目录下 0 个文件变化。**
- inv-dbb4b10d.json 的 sha256 是 d98c77ec1328e0b624384f58e3581fcd48187e71a9a904cc7dc8b764841b54be，**与 0506 的 inv-470b2b49.json 逐字节相同**。按目录逐个比较，6 个目录的 stamp 和成员映射都相同。
- 所以：exec 候选戳仍是 **fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336**，记录值仍是 e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34，**两者不等**；其余 5 个目录与记录值相等。差集仍是 0506 第 2 节列的三个 helper（M），没有新成员。tests/ 不在 compilercheck 的 reviewed_trees 里，所以 tests 的漂移不影响 exec 戳。

## 2. 沿用 0506
0506 材料的第 2、3、5 节（差集表、diff 摘要、执行条件以及后面追加的收窄、勾选清单）**原样适用**于 dbb4b10d：三个 helper 的字节在 470b2b49 和 dbb4b10d 之间没有变化。§5 勾选清单仍然**全空**。若审核通过，在 dbb4b10d 上重算 exec 戳，应得到 fbb12970…；把它写进 gatedeps 要另外授权。

## 3. 证据
/tmp/cc40-prep/sa3/：inv-dbb4b10d.json、.out、.rc。对照文件：/tmp/cc40-prep/sa2/inv-470b2b49.json，以及 /tmp/cc40-prep/next/hygiene-stamp-A-materials-0506.md。
