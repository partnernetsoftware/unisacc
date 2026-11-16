# 大块分配复用(alloc-reuse)

状态:补丁已出,未落共享检出的 include/,未提交。真源是 include/stdlib.h,include/cx-flat.h 由 `tools/cx-flatten.cx --write` 重生成(补丁两者都带)。

## 定因

- include/stdlib.h 的分配器:头加请求不超过 64KB 走 12 个二次幂小类,有空闲表;超过 64KB 的每块单独 mmap(按 64KB 取整,类 -1),free 直接 __munmap,没有任何缓存。realloc 对大块增长一律 malloc 新块、按字拷贝、free 旧块。
- 所以大块每次分配释放是一对系统调用加整块缺页清零。

## 改法

- free:大块不超过 _UNISA_LMAX(32MB)时挂到按大小分级的 9 条表上(第 b 条收 128KB<<b 到 256KB<<b 之间),超过的照旧直接 __munmap。
- 上限:各表合计不超过 _UNISA_LCAP(64MB);放入前按轮转游标逐块 __munmap 旧块直到放得下,所以缓存有界,且会向当前工作负载的大小迁移。
- malloc:大块请求先在本级表的前 16 块里取第一块够大的(复用块小于取整后请求的两倍),取不到再 mmap。
- 没有新增函数(避开 FTRIM 与 libneed 名单),没有平台分支;只用原有 __mmap、__munmap 两个门,六目标同一份代码。

## 合同核对

- 块头不变:16 字节,大小(映射的真实大小)加类 -1;返回指针 16 对齐;realloc 的容量仍从块头读。
- 失败语义不变:负数或超过 0x100000000000 返回 NULL 且 errno=ENOMEM;成功不动 errno;calloc 溢出检查未动。
- calloc 仍自己清零(复用的脏块上实测全零)。
- 有意的可见变化(须集成者知情):(1) 复用的大块不再是全零页,内容是上一个持有者留下的,与小块一致,ISO C 允许;依赖「大块 malloc 恰好为零」的程序会变。(2) 大块重复释放以前是第二次 munmap 失败,现在会坏链表,与小块一致。(3) 进程最多多占 64MB 已释放内存不还给系统。

## 数字(LOCAL,lnx x86_64,已装 0.0.67 产品编译,同机)

复现程序 /tmp/ar/churn.c(20 万次 64KB 到 1MB 的 malloc/free,加 2000 轮 1KB 倍增到 4MB 的 realloc):

| | 墙钟 | user | sys | mmap | munmap |
|---|---|---|---|---|---|
| 改前 | 14.99 | 8.73 | 6.23 | 214001 | 214000 |
| 改后 | 8.53 | 8.51 | 0.01 | 49 | 0 |
| 宿主 gcc -O2 | 0.41 | 0.19 | 0.22 | 未量 | 未量 |

parse2 gen --errors(/tmp/a3/gen2.c,同一时刻两路并行对照,输出与 /tmp/a3/genp2-e.json 逐字节相同):

| | 墙钟 | user | sys |
|---|---|---|---|
| 旧分配器 | 54.14 | 46.37 | 7.64 |
| 新分配器 | 45.13 | 42.42 | 2.65 |

新分配器下 strace -c:munmap 3 次(改前证据 /tmp/a3/stp.txt 为 473885 次)。

注意:本次对照的绝对值(54 秒)高于任务给的 34.9 秒,原因未查(并行两路加机器负载,或包装编译方式不同);相对差是同条件的。

## 验证方式的限制

产品的头是内嵌的,改 /tmp 树里的 include 不影响已装产品编出的程序。所以以上数字用的是把新分配器段抽出改名(na_malloc 等,/tmp/ar/nalloc.h,由补丁后的 stdlib.h 机械抽取)再用宏替换程序里的 malloc/free/calloc/realloc 得到的;运行库内部(stdio、getline 等)仍用旧分配器,块头兼容,可互相释放。

## 已验

- 合同小测 /tmp/ar/t1.c 共 20 余项断言 failed=0:同大小复用同一块、持有量记账、calloc 脏块清零、小块不顶替大请求、复用块小于两倍、超 32MB 直接归还、200 个 1MB 块全释放时持有量始终不超过 64MB、realloc 增长与收缩保字节、三条失败路径 errno、成功不动 errno。
- cx-flatten --write 在 /tmp 树上 rc 0;补丁 git apply --check 通过。

## 未验(阻塞直接提交)

- 真正经产品内嵌头生效的端到端(需 entry-build 或慢车道重建产品)。
- tools/affected.cx 选出的 mem、stdio、runcache 等批次;freeze-gate --boot-files 多单元自举;ratchets。
- 事实表未重算:上次改头的提交同时重算了 exec/facts/pp-autoinc-gen.tsv、k2-gen2.tsv、k2-libraryenv.tsv(facts-gen),本补丁落地时须同做;build/libc-decls.tsv 经 derive 核对。
- arm64、osx、win 未实跑;Windows 上 __munmap 门对缓存后再归还的块的行为未实测(逻辑与改前同一调用)。
- 多单元程序各单元各有一份静态表(与小类空闲表相同),每单元各 64MB 上限,未测。

## 剩余差距(本补丁不解决)

- 复现程序改后 user 仍有 8.5 秒,宿主 0.4 秒:realloc 大块增长是按字循环拷贝(2000 轮约 8GB),宿主用 mremap 原地扩。要补需要 L0 的 mremap 门(Windows、osx 无对应)或块拷贝门,属另一类改动。
- parse2 gen 的 user 42 秒主要是代码生成质量,不是分配器。
