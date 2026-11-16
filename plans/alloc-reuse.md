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

## realloc 大块改映射(2026-11-16 续)

状态:已落 include/stdlib.h,cx-flat.h 与三张事实表(pp-autoinc-gen、k2-gen2、k2-libraryenv)已重算。上一节「剩余差距」第一条由本节解决(仅 Linux)。

### 定因(实测,不是推断)

复现程序分相位计时(/tmp/ar2/prof.c,clock_gettime 包住各段,已装 0.0.67 默认档):malloc/free 20 万次循环 0.060 秒;realloc 倍增循环 7.335 秒,其中 realloc 调用本身 7.269 秒,循环里的 memset 0.021 秒。即 user 的 98% 在 realloc 内,是 2000 轮约 8GB 的按字拷贝(约 1.2GB 每秒)。

### 改法

- Linux:旧块与新请求都是大块(各自独立映射,判定与 malloc 相同:头加请求超过 64KB)时,realloc 用 mremap 带 MREMAP_MAYMOVE 直接改映射大小。后面有空间就原地扩,没有就由内核搬页表,不拷字节;块头随页走,只改大小字。缩小到不足一半且仍是大块时同样改映射(原地截短)。
- 门:沿用已有的 __syscall6,不新增 L0 门;调用号写在 _UNISA_MAP 旁的 _UNISA_REMAP 宏里(x86_64 为 25,arm64 为 216),与原有平台表同处。osx 与 win 不定义该宏,realloc 走拷贝路径,行为与改前相同。realloc 函数体内只有一处 #ifdef _UNISA_REMAP,没有平台名。
- 拷贝路径(小块之间、小变大、大变小、非 Linux、mremap 被拒)由每步一字改为每步八字加尾部。
- 没有新增函数,FTRIM 与 libneed 名单不变。

### 合同核对

- 失败时原块不动:mremap 失败返回负值,不改任何东西,随后落回 malloc 加拷贝;malloc 也失败则返回 NULL 且 errno=ENOMEM(由 malloc 设),原块仍有效。
- 负数请求仍先拒;超过 0x100000000000 的请求不进 mremap,交给 malloc 拒绝,errno 与改前相同。成功不动 errno(mremap 的结果不写 errno)。
- 块头格式、16 对齐、类 -1、头里记映射真实大小均不变,所以改过大小的块照常进 free 的分级表并记账。
- 有意的可见变化:(1) Linux 上大块 realloc 增长后新增部分是内核给的零页(改前是复用块的旧内容或新映射的零页,ISO C 不保证任何一种)。(2) 大块 realloc 不再从空闲表取块,也不把旧块放回表,增长中的块直到 free 才进表。(3) 被 seccomp 之类禁掉 mremap 的环境自动落回拷贝。

### 数字(LOCAL,lnx x86_64,已装 0.0.67 编译,nalloc.h 方式,/tmp/ar2)

复现程序 churn.c:

| | 墙钟 | user | sys |
|---|---|---|---|
| 改前(上一节的分配器) | 8.377 | 8.362 | 0.012 |
| 改后 默认档 | 0.390 | 0.234 | 0.147 |
| 改后 -O2 档 | 0.301 | 0.147 | 0.154 |
| 宿主 gcc -O2 | 0.419 | 0.129 | 0.289 |

strace -c(改后):mremap 11993 次、mmap 2039、munmap 2023。

只开八字拷贝、不开 mremap 时的 realloc 循环(用于估计 osx 与 win 的收益):默认档 7.3 到 6.9 秒(基本无收益),-O2 档 8.1 到 3.5 秒。附带观察:原一字循环在 -O2 档(8.1 秒)比默认档(7.3 秒)慢,属代码生成问题,未查。

parse2 gen --errors(同一时刻两路并行,输出与 /tmp/a3/genp2-e.json 逐字节相同):上一节分配器墙钟 45.84(user 43.34,sys 2.40),本节 45.37(user 42.83,sys 2.38)。该负载不受 realloc 拷贝限制,差值在噪声内,不计收益。

### 已验

- 合同小测 /tmp/ar2/t1.c(原有断言全留,新增大变大增长保字节且拷贝计数不变、块头等于取整大小、旁边有别的映射时再增长、取整余量内同指针不调用、大变大缩小、三条失败路径后原块内容完好、改过大小的块释放后被复用、小变大与大变小仍拷贝、八字循环的奇数字数尾部):默认档、-O2 档、关掉 mremap 三种编法均 failed=0。
- nalloc.h 的 realloc 段与 include/stdlib.h 改后文本改名后逐行一致(仅多计数器)。
- cx-flatten --check SAME;facts-gen --check 全 SAME(重算三张表后);freeze-gate --boot-files 四文件 units=5 ok=5;ratchets ok=15 fail=0。

### 未验

- 经产品内嵌头生效的端到端:须重建产品,已装产品编出的程序仍是旧 realloc。
- arm64(调用号 216 按内核通用表写,未实跑)、osx、win 未实跑;osx 与 win 只得到八字拷贝,原地扩张未做(需各自的门:win 无对应调用,osx 可用 vm_remap,属另一项)。
- 16TB 请求的失败路径依赖本机 overcommit 设置,本机实测为拒绝;overcommit 全开的机器上未测。
- 慢车道、affected --run 未跑。
