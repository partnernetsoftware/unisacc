# 第0步中段完整输入实证收据

实际跑过。只改tests/research；没有产品改动、没有指纹/旧闸/发布动作。

|账|E3|lower|
|---|---|---|
|(a)模型可独立提取|已证：7000实件；3000历史提取件|同左|
|(b)完整输入独立执行|2用例接受/拒绝均成立|接受用例成立；拒绝域与参考不一致，完整阶段资格未证|
|(c)输出可复用|2用例×4变体stdout/stderr/rc逐字节相同，仅这些输入和共享C执行器|接受用例×4变体同；拒绝合同红，整体资格未证|

数据及逐流哈希：tests/seals/middleflow-step0/manifest.tsv、model-origins.tsv与research/c43-middle-step0-streams.tsv。动态C入口tests/middleflowcheck.cx；辅助采证/回放为tests/middleflowrunner.c，直接使用exec/c/run.c的package/runroute_range/attr_strip及exec/c/core.c。只是该共享C执行器证据，未运行两个候选分别携带的ISA核心；7000产品SHA ba11d636b54adcdc519a81af7553d15ab882ecb85622bdc16968d966acdb8f5f；3000原件已不在，只使用历史提取e3.net/lower.net，cc未独立重提3000，不编造成两原件新复验。存储去重的canonical model来自7000，model-origins记录两来源同哈希；命名闸复验冻结流而不再声称验证已删除原件。

## 完整输入与测量

真实lnx/x86_64/tape/O0的e2→e1输出，通过run.c:673–687的attr_strip形成UNITOK与4字节零计数attributes。接受frame287字节、E3输出592字节；拒绝frame284字节、E3输出0并输出不完整enum诊断。core source参数是固定逻辑名leaf/middle.c；cli/source按compiler.c:377–380/667协议包含末尾NUL，data中单独保存；本叶E3没有查询该资源，不能声称它是被读取依赖。target与f1的实际提供值也单列。

实际E3资源查询共13个：attributes4字节、error-limit空字节及run-forward/ccw/funit/object/library等明确absent。每个查询的opaque键、存在性、长度及响应哈希均落resources.tsv；core_host_fetch包装器回放未知键直接失败，不允许磁盘fallback。完整语义输入还含模型、source参数和显式燃料400000000000；fuel由执行预算层固定，不能把环境变量改小导致超时误称输出泄漏。

原始对拍使用/tmp/c43-middle/7000与3000各自的提取件，目录clone共4个，变体分别给LANG=C/C.UTF-8、TMPDIR、UNISA_ATTRIBUTES不存在路径、UNISA_CONTAINER不存在路径以及不同ambient UNISA_MAXSTEPS，stdout/stderr/rc相同。文件路径不是语言逻辑名。此有限实验说明这些条件没有改变所选输入的结果，不能全称证明不存在任何隐含宿主态或所有E3套件可复用。

动态C命名闸middle-flow-e3与middle-flow-lower-accept单跑rc0，各3s左右；后者名称明确只代表接受方向。native共享执行器编译cc -O2 tests/middleflowrunner.c exec/c/core.c，无-Iinclude。一次前置工具构建误用-Iinclude导致自带libc污染宿主头，已按cc澄清修工具后继续，并非合同红后绕过。最终回放runner曾测SHA 7d7e34cfcead1e128d0f1ee0ca4b27b9c85952ce427c486cf71f82ab903f21fc；工具源码与具体runner构建属于实现/观测见证，不加入模型核心身份。

## lower首红与参考

接受方向用实际E3输出经prune后592字节完整tape，lower输出1119字节，显式查询library/module/exit/mmap/munmap与process/argc均absent；两来源模型、4变体独立回放相同。

反例输入 ?\n 为2字节，SHA ce773b87709a04bbcb0ead74fea94b1f20fa4a4d185fc06a24a9bc703dd99613。模型rc0，输出106字节，SHA 034ed2cdf94706615f4cad4cae78c5dd8d31494f140b9aa05f71d2c8660cd286。参考按现有exec/lower/fullcheck.py:19的parse→lower路径调用，rc1，stdout空SHA e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855，诊断unisa/tape.py:335 unknown op '?'。完整stderr带本机路径留仓外/tmp/c43-middle-step0/lower/refuse/reference.stderr，哈希424913aa9d94e421881cf42495d7eb3d346f406da4c1531dc3c1a43f54956a42；仓内只留去路径诊断和比较表。该输入不是被证明合法的负例误选，应记“疑似该拒绝未拒绝/参考域不一致”，不是模型非确定性。

只读定位：exec/lower/code-scan-result.tsv:4把任意interned token写OP；code-entry-result.tsv:64 x86分派末端走GENERIC；code-print-result.tsv:10与:6–7取原始TXT，经code-print-byte.tsv:4 COPY输出，未在该路径校验tape.SHAPE；参考unisa/tape.py:335校验op∈SHAPE而拒绝。是否需要独立lower入口的前置域验证、还是收紧模型GENERIC，需要产品刀另授；本次不改规则。不用产品正常前级保证来把独立合同红改成绿。

至此停止lower进一步实测；没有换反例消红。E3只形成所选完整流的窄见证；没有自动摘钩或重验预算变更。下刀须复核拒绝域/参考契约，不能以“输入与模型同”替代域资格与原生行为义务。
