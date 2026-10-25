# pp-literals 实测合同收据

采证源 56131365；runner SHA256 ba11d636b54adcdc519a81af7553d15ab882ecb85622bdc16968d966acdb8f5f；产品源闭包 c0718934bf51188719f9d1880913eb3253258d612f4bceead807b7aee5c1010b。采证 plain+locations 原样套件 rc0，44 模型运行、91构造/执行/资源依赖、6接受/38拒绝。

核心输入ID 7285656dd3b691de05a0b29c41ecbd0419cb1867cbb5230201478f1e02695d51；输出见证 db3aa9ad000fb6c672ba41d04bf81502c2273e4eae7dc411988da90ae0bc4483。数据文件哈希见 seals/exec-pp-literals/package.tsv；采证工具 ppleafcapture.cx，核验入口 ppleafseal.cx，源码 digest 在提交 Git blob 中绑定，runner 字节单列。

三个命名闸实跑rc0：pp-leaf-seal、pp-leaf-seal-negative、pp-leaf-capture-selftest。负例包含独立清单漏项、predefines.tsv 内容变动、实读 stdarg.h 内容变动；源码镜像先验证基线为绿，所有改动只在私有临时目录。原冻结数据不修改，仓库包按cc预审剥离观测原件；streams仍逐字节一致。

观测工具变化仅NOT_RULED；没有新宿主绿证据、没有自动摘钩资格、不回填7000、不改gatequeue/gatedeps/候选/产品闭包。
