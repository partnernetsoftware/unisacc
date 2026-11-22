# devsvc freeze 写窗口锁集成者全流程演练记录

日期: 2026-11-23
演练环境: 私有临时仓 (`mktemp` + `git archive HEAD` 干净树 + 私有 `bare` origin 仓 + 独立 `UNISACC_DEVSVC_DIR`, 零触碰共享检出)

## 一、Step 1 至 Step 8 实跑原行

- **Step 1 (`cc` 取 `freeze` 锁 `--ttl 7200`)**:
```text
OK	acquired	freeze	owner=cc	expires_in_s=7200
rc=0
```
- **Step 2a (`cc` 执行 `devsvc git merge --no-ff v0.0.94-sim`) -> 发现合同差距**:
```text
devsvc: git merge BRANCH|--continue [--worktree] [-m MSG] [--cwd WORKTREE]: unexpected --no-ff
rc=2
```
  结论: `devsvc git merge` 不接受 `--no-ff` 参数(分叉分支默认产生双父合并提交 `ff=0`, 见 Step 2b; 冻结手册已同步说明)。
- **Step 2b (`cc` 执行 `devsvc git merge v0.0.94-sim -m ...`)**:
```text
GIT_MERGE	rc=0	commit=b28cf0b8d2ec06ba7b331fed671eae0cb05c94f2	branch=main	cwd=<tmp>/repo	ff=0	synced=1	kept=0
rc=0
merge_parents=3
```
- **Step 3 (`cc` 执行 `f write release/candidate.tsv`)**:
```text
F	write	ok	sha=ee90ed99adf7248f5e1f053b4d6c32dfc1e46d8487043eb4408d41613b32c12e	lines=1	count=0	via=svc	locks=1
rc=0
```
- **Step 4 (`cc` 执行 `devsvc git commit --whole release/candidate.tsv`, 真实 `commit-own.cx`)**:
```text
COMMIT_LANDED	full=a48e5cb4152b1a2af0b8bd238bc18cebc4b43ba8	base=b28cf0b8d2ec06ba7b331fed671eae0cb05c94f2
COMMIT_TIMING	gates_ms=12	gates_rc=0	cas_ms=11	cas_rc=0
GIT_COMMIT	rc=0	commit=a48e5cb4152b1a2af0b8bd238bc18cebc4b43ba8	pushed=0	ms=646
rc=0
TRAILERS=X-Unisacc-Window: cc
```
- **Step 5 (`cc` 执行 `devsvc git push <HEAD>` 到私有 `bare` 仓)**:
```text
GIT_PUSH	rc=0	commit=a48e5cb4152b1a2af0b8bd238bc18cebc4b43ba8	attempts=1
rc=0
ORIGIN_MAIN=a48e5cb4152b1a2af0b8bd238bc18cebc4b43ba8
```
- **Step 6 (`cc` 执行 `f write release/notes-0.0.94.md`)**:
```text
F	write	ok	sha=ecdcd93bbc5b041dd81743bae7b4c39efcc311d391d4ad41358d86cadaa9de3d	lines=1	count=0	via=svc	locks=1
rc=0
```
- **Step 7 (`worker1` 试写 `release/`、`src/` 与 `git commit` 全部被拒)**:
```text
LOCK_HELD freeze owner=cc
F	write	refused	LOCK_HELD freeze owner=cc
rc=1
LOCK_HELD freeze owner=cc
GIT_COMMIT	rc=1	reason=lock-held	lock=freeze	owner=cc
rc=1
```
- **Step 8 (`cc` `lock release freeze`, 随后 `worker1` 写入全部恢复)**:
```text
OK	released	freeze
rc=0
F	write	ok	sha=f16f37b274544be127b9a89b77e415083e3b5c9c3e5dba961ed9761ab93ad9c2	lines=1	count=0	via=svc	locks=1
rc=0
```

## 二、结论
- 集成者在全流程中除 `devsvc git merge --no-ff` 参数不支持(改用 `devsvc git merge BRANCH -m ...` 自动双父合并 `ff=0`)外, 其余 S0 操作(`lock acquire`/`git merge`/`f write`/`git commit`/`git push`/`lock release`)全通且无自锁;
- 叠加 `gatekeeper` 复核整改提交甲 (`c4ab186f33`) 与提交乙 (PLAUSIBLE fail-closed 直读 `locks.tsv` + 缺省 `ttl=7200` + `flow` 写节点全查) 后, 集成者即便在 `CLAUDE_JOB_ID` 已设且未设 `DEVSVC_OWNER` 时传 `--owner` 也不再自锁。
