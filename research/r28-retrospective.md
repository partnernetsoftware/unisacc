# 0.0.28 发版复盘（2026-10-05）

版本号提交 18:02 → 公开 22:10（SGT），约 4 小时；其中真正的流水线（候选、comboot、封存、签名、Linux）约 1 小时，其余是三轮修红和两次重封。

## 各环节

| 环节 | 结果 | 备注 |
|---|---|---|
| precheck 契约层（R2 首次实用） | 封存前抓到 2 红 | ape-version、gate-infra：都是顺序问题（版本号提交后要先 make gatedeps，ape-version 要等 comboot 装上新产品） |
| 候选 + comboot ×3 | 不动点 524be271 → 9d5de43c → 9d5de43c | 第二次因修红改了头文件；第三次字节不变 |
| 队列第一轮 | 657 项 20 红 | 全在测试侧：新探针撞上 Python 对照组和 Windows 目标、libneed 注释、assert 续行、rowcov 分片重分布、parse2-8 内层超时 |
| 队列第二、三轮 | 2 红、1 红 | Python 路线清单拆分（difftest-py.knownfail）、ccrun 期望来自 Python 路线 |
| Linux 全套 | 只剩客机已知红 + scale | scale.sh 没跟上 MAXSRC 改成变量（dbabf24e） |
| 签名 | Apple 2 分钟；Windows 资格 + 公司约 10 分钟 | GitHub TLS 超时让一次部署批准丢失，补批 |
| 公开 + 冒烟 + U1 | 冒烟 6/6；unisa 仓 unisacc-v0.0.28 下载后 shasum -c 通过 | |

## 问题与持久化

| # | 问题 | 处理 | 落到哪里 |
|---|---|---|---|
| 1 | 新探针放进 tests/c 就被十几个套件消费（difftest-py、ccrun、fat、closure、chain），POSIX-only 或 Python 不支持的探针在队列里成片变红 | POSIX 宿主行为探针改放 tests/forward；Python 路线缺口有自己的清单 | RELEASE-PIPELINE §20；收官路线规则 2（封版前一天不加 tests/c 探针） |
| 2 | 队列复用率 0%（P7 实测）：gate.sh、gatedeps.json、gatequeue.py 是全局指纹，任何修红都让全部结果作废 | 政委裁定 P7 砍掉，0.0.29 新立 P7′ | plans/v0.0.29.md P7′ |
| 3 | rowcov 分片基线可以随探针重分布被调低（协调员 E22） | lower/enc 加并集棘轮 union8；parse2 进 0.0.29 T4 | tests/rowcov.py、gate.sh |
| 4 | 我在 run.c 加 <time.h> 打断了产品驱动打包；一次 git revert 失败后误 amend 了别的提交信息 | 撤回并在后续提交说明里更正 | 本复盘；规则：产品闭包里的驱动改动先建候选再提交 |
| 5 | 修红改了头文件 → 产品字节变 → 重封 | 头文件改动要在版本号提交前跑一遍队列相关套件 | 收官路线规则 3 |
