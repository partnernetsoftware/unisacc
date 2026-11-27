# 夜间 bg-candidate 与按单元红链二分

只设计, 不跑. 现有入口是 tools/bg-candidate.cx. 它不另造构造步, 只对一个 40 位 rev 起 tools/chain-run.cx, 写出 OWNER, EXIT, seal. 一次只允许一个在跑的候选 (同文件头: 别的候选还活着就拒绝).

私有结果只记 PREPARED 或 LOCAL. 不记 PASS. 不作为公开资格.

## 夜间链

触发: 每天一次, 对象是当时的 origin/main HEAD, 40 位. 不是窗口分支, 不是冻结中的 v0.1.6 预跑目录.

前置, 缺一就不启:

(1) `bg-candidate status` 没有 RUNNING. 有就跳过, 不杀别人的链.
(2) 该 rev 已有 DONE 且 seal 仍对, 不重跑.
(3) 该 rev 改了 hub (fan-in >= 8: sysnums, io, time, bytes, heap, frameparts, holdframe, env, driveutil, unitcache, text), 或 sig_sha, 或编译器单元: 不走夜间私有链, 直接全链. 全链仍要求完整预跑加六目标 CI, REUSED 0.

命令形状, 未执行:

```
sh ./unisacc.com tools/bg-candidate.cx -- start --rev <MAIN40> --root <共享检出>
sh ./unisacc.com tools/bg-candidate.cx -- status <MAIN40>
```

成功行是现有的 `BG_CANDIDATE` 状态行, state=DONE. 失败是 FAILED, ORPHANED, UNOWNED. DONE 之后可以 `promote --check`, 它只读, 不发布.

夜间链失败且不是上面第 (3) 类, 才进入按单元二分. 夜间链本身不替代公开发布前的全链.

## 红链按单元二分

单元是 plans/slice-units.tsv 的一个 .o, 共 99 个. 合同是导出符号加 sig_sha, uses 列表, 以及该单元 .o 回执的输入输出 sha.

输入: 一个红的全链或夜间链 rev, 加上一份绿的基线回执. 没有绿基线就停, 不二分, 交全链.

每步只动一个单元, 预算 5 分钟, 本设计不跑:

(1) 只重建该单元, 上限 20 秒.
(2) 重链接, 19 秒.
(3) 重打包, 7 秒.
(4) `affected.cx --files <该单元源> --run`, 上限 54 秒.
(5) 与该单元上一版做差分.

未碰证明, 任一失败则 rc 2, 这一步不算, 回到全链:

(1) 其它 .o 的 sha 等于基线回执.
(2) 没有 sig_sha 差.
(3) 声明键以外的 F 记录字节相等.
(4) entry.elf 的符号差只落在该单元内.

二分不是 git bisect 提交, 是在 99 个单元上找最小红集:

(1) 把红 rev 相对绿基线改过的单元列成候选. 候选为空则不是单元差, 交全链.
(2) 候选按 fan-in 升序. hub 不进候选, hub 差直接全链.
(3) 每次取一半候选, 只重建这一半, 跑上面的未碰证明. 证明失败不是"这一半红", 是门坏了, rc 2, 停.
(4) 证明通过且差分仍红: 红在这一半, 丢掉另一半.
(5) 证明通过且差分转绿: 红在另一半.
(6) 收到一个单元仍红, 且未碰证明通过: 该单元是红因. 记 LOCAL, 不记 PASS.
(7) 收到一个单元但未碰证明失败, 或单步超过 5 分钟: 放弃二分, 全链.

所有者不在本设计里改. 二分只读 build/unit-owners.tsv. 一个单元一个所有者. 接口变化先由 cc 落地并重建全部消费者, 夜间二分不改接口.

## 不在本提交里做的

不启动 bg-candidate. 不编译. 不写 tools/priv-relink.cx, 那是 agy-flow 的项. 不改 unit-owners.tsv.
