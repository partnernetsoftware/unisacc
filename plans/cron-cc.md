# cc 每 5 分钟巡检

定时器只发一句"按 plans/cron-cc.md 执行一轮"。要改巡检内容就改这个文件。

## 唯一目标：尽快拿到可发布的 0.0.43 unisacc.com

每轮先看 grk 在不在关键路径上。不在，当轮就拉回来。清理、工具、分层只在不挡发布时做。

## 关键路径（完成一项就打勾，写上提交号）

1. [ ] 慢车道安装三项修复：去重、48 条 use、间接原型报错。验收：`tools/check-install-fixes.cx -- 候选`。负责：grk。
2. [ ] 实测缺口：所有不需要材料的批次各实跑一遍，结果写进 `plans/release-0.0.43.md`。负责：cc（子代理在跑）。
3. [ ] 补齐 176 个 UNPREPARED 批次的材料，同一候选、一次构造。
4. [ ] 实现 19 个 PLANNED 覆盖行。分工：grk 做产品和流，cc 做工具和库，bdy 做 L0。
5. [ ] win、osx、arm64 在同一候选上实跑。需要主人允许 push 跑 CI。
6. [ ] 自举和源码资格；然后跑 `tests/run.cx -- --all`，结果要是 QUALIFIED。
7. [ ] 主人授权发布。

## 每轮怎么做

1. 看 `git log -6`、`mux capture 1:grk`，跑 `sh ./unisacc.com tools/ratchets.cx`。
2. grk 停着或跑偏了，就发信封给它下一刀；有人卡住，就去疏通。棘轮超基线，找那个车道的主人。
3. 汇报只说三件事：推进到第几项、比上一轮多了什么、卡在谁身上。没有新事实就一句话结束。

## 底线

- 只提交自己的文件；多人共改的文件用 `tools/commit-own.cx --keyed`。
- 别的车道发信封，不动它们的文件：grk 负责 src/ 和 cx 产品层；bdy 负责 L0 架构；csih 负责 apps/csih/；ujs/ 不碰。
- 技术细节自己定，不问主人。工期以实测为准，不凭感觉。
- 每条命令 60 秒以内；独立复核用 `tools/snapshot-run.cx`。工具清单见 prd.md 能力速查。
