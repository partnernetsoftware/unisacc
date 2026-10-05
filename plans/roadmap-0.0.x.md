# 0.0.x 收官路线（2026-10-05 定）

主人要求：按规划早点做完 0.0.x 系列。cc 规划，下面就是全部剩余版本；0.0.31 公开即 0.0.x 完工，之后进 0.1.x。

## 终点

**0.0.31：构建链不再调用 Python。** 上一版 unisacc.com 编出 C 构造器（seed/gen.c、tbl.c、net.c）→ 种子 → 最终 unisacc.com，与 Python 链逐字节相同，comboot 不动点成立。这就是 0.0.x 的命题“由表构造、穷举验证的编译器能自己把自己造出来”的完成。

## 三个版本

| 版本 | 必须完成（版本的定义） | cc | cdx |
|---|---|---|---|
| 0.0.29 | C 构造器覆盖 parse2 全部 | 队列复用重做（P7′）、pp 死行清理（T3）、parse2 并集棘轮（T4）、后端按需增长让 sqlite shell 能跑（H4′） | B4：parse2 余下各族 |
| 0.0.30 | C 构造器覆盖 enc、lower，七个阶段全部与 Python 逐字节相同 | cdsh 16 单元 ≤5 秒（F2′，驱动侧缓存）；**H3：pthread（POSIX 线程，向 cosmocc 看齐，转发宿主 libpthread：create/join/mutex/cond 最小集）、wctype.h、fenv.h** | B4：enc、lower |
| 0.0.31 | 构建链不调用 Python（B5） | comboot 与发布流水线改走 C 构造器；Python 只作对照组；F3 引号 include 的库裁剪；H1″ 整程序一个 environ、getenv 读 environ | B5 配合，删掉构建路径上的 Python 调用 |

## 不在 0.0.x 里（0.1.x 主线）

Windows 宿主转发、DNS/TLS 转发、更大面的 POSIX/cosmocc 头文件。pthread、wctype、fenv 不在此列：它们按上表放在 0.0.30（主人 2026-10-05：POSIX 面向 cosmocc 看齐的项可以顺延，不砍）。

## 为了按时做完，执行上的规则

1. 每版只做表里那一行；新发现的问题，是红就修，不是红就记进 0.1.x，不插进当前版本。
2. 封版前一天起不往 tests/c 加探针（0.0.28 的 20 个队列红大多来自新探针撞上 Python 对照组和 Windows 目标）。
3. 顺序固定：版本号提交 → 候选 → comboot → 契约层全绿 → 封存 → 队列 → Linux → 签名 → 政委确认 → 公开。契约层不绿不封存；封存后只收 fix:。
4. 队列复用（P7′）在 0.0.29 先做，之后修一次红只重跑受影响的作业，不再整轮重跑。
5. 每版开发约一天、发版半天；到点没做完的项，原样移到下一版（同一项最多两次），不拖整版。
