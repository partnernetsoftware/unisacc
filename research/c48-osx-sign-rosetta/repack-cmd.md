# macOS runner：用草稿 APE 自身重包四个签后成员

交给 cc 集成 `.github/workflows/macos-sign.yml`；本交付不修改该路径。以下命令承接现工作流的下载、四成员签名及严格签名验证步骤，放在构造 DMG 之前；工作目录始终是 checkout 根。没有重新构造六片或模型，也不要求本机私有 out 路径。

## checkout 与前置输入

完整 checkout `inputs.source_commit`，本候选为 `f09969d9d0efb81976b3eaf486731f120df22cbe`，不要 sparse checkout 只取两个工具。该提交已包含替换接口。直接执行的文件为 `tools/release-ape.cx`、它内层再次执行的 `tools/ape-pack.cx`，还依赖 `cxroot.tsv` 及 `cx/`、`ext/`、`include/` 等传递源；使用完整 checkout 保持一致。下面用仓内 `tools/bound`（及 `tools/bound.c`、`cx/compiler/boundgate.cx`），冷缓存会用 runner 自带 Apple cc 构造看门狗；产品打包工具仍由草稿 unisacc 自身运行，不用系统编译器构造产品。cc 原签名步骤还需 `release/entitlements-unsigned-kernel.plist`。

现有下载步骤已核输入 members.tsv SHA `4e1a51b8bc0d2dbdf8aefad6bca25cf615556fb572044abc3b89e008565225d1`，保留 `in/` 原件；`work/` 四成员已经完成本次签名。

| 路径 | 用途 |
|---|---|
| `work/ape` | 草稿原未签 APE，SHA `bdd52246c4ba52becff3c1404b20976a51f955e8c3e2218443123d1b464adaa3` |
| `in/slice-osx-x86_64` | 未签Mac原片，原members的asset name；作为来源收据，不参与最终替换字节 |
| `in/slice-osx-arm64` | 同上，ARM |
| `work/osx-x86_64`、`work/osx-arm64` | 本次已签编译器成员；原entitlements保留 |
| `work/cache-x86_64`、`work/cache-arm64` | 本次已签缓存管理器；空entitlements |

`work/ape` 在此流程中不能被新APE覆盖。尤其不能在根目录继续保留 checkout 自带的旧 `unisacc.com`：release-ape 内层固定调用 `sh ./unisacc.com tools/ape-pack.cx`，因此下面先将已核草稿 APE 复制到 runner 根 `./unisacc.com`。

其他输入均从同一草稿 `v0.0.48.1-osx-launch-rc1` 取得，不从签名后的 DMG反推。补充的三个辅助资产已经上传到此草稿，原五角色members和原资产字节/pins未改。这些辅助资产不属于原五角色清单，必须用下列独立pins核验：

| asset | 完整 SHA256 |
|---|---|
| `pkg.bin` | `fffa2311189d97d7dd0868a59a47ff788a6af17259f2664454f9a8351c2ac842` |
| `slice-win-x86_64` | `16ccd50d9f03c7c6622731c48f2f7c268bd1606914f6a8e7c2696e4f81e57dcf` |
| `slice-lnx-x86_64` | `3efdace47561b9abf2275fb754932ca7a88a2df1812317a91034396455588a96` |
| `slice-lnx-arm64` | `f3849f2169d7c547eca60e7a327476a30de2a90dccb9deee66bdb708ce712bd5` |
| `slice-win-arm64` | `0a651a5a5451670eb2c396f0227a16ce78ae0fae63f20d0bda67b16b6765ced5` |

P4 `pkg.bin` 必须含末尾16字节 UNIPKG1 footer，完整1745198字节；不能传 `p3-package.body`。两个Linux资产是裸 `entry.elf`，不含自己的包。Win x64必须是原裸PE（PE offset64、SizeOfHeaders1024、无.rsrc/证书目录）；不能把原APE或从APE剥出的仍带版本资源PE直接传给pack，因为pack会添加产品版本资源。

## 下载辅助输入（独立 step）

环境承接现工作流：`DRAFT_TAG`、`GITHUB_REPOSITORY`、`GH_TOKEN` 已设置。以下pins仅属于上述冻结候选，未来源候选换标时整组更新，不自动套给别的tag。

```bash
set -euo pipefail
cd "$GITHUB_WORKSPACE"
: "${DRAFT_TAG:?}" "${GITHUB_REPOSITORY:?}"
bound() { ./tools/bound 55 "$@"; }
mkdir -p in out
mkdir in/repack-inputs  # 必须是全新目录，不使用 --clobber
bound gh release download "$DRAFT_TAG" --repo "$GITHUB_REPOSITORY" \
  --pattern pkg.bin --pattern slice-win-x86_64 \
  --pattern slice-lnx-x86_64 --pattern slice-lnx-arm64 \
  --pattern slice-win-arm64 --dir in/repack-inputs
cat > out/repack-inputs.sha256 <<'PINS'
fffa2311189d97d7dd0868a59a47ff788a6af17259f2664454f9a8351c2ac842  in/repack-inputs/pkg.bin
16ccd50d9f03c7c6622731c48f2f7c268bd1606914f6a8e7c2696e4f81e57dcf  in/repack-inputs/slice-win-x86_64
3efdace47561b9abf2275fb754932ca7a88a2df1812317a91034396455588a96  in/repack-inputs/slice-lnx-x86_64
f3849f2169d7c547eca60e7a327476a30de2a90dccb9deee66bdb708ce712bd5  in/repack-inputs/slice-lnx-arm64
0a651a5a5451670eb2c396f0227a16ce78ae0fae63f20d0bda67b16b6765ced5  in/repack-inputs/slice-win-arm64
PINS
bound shasum -a 256 --check out/repack-inputs.sha256
```

## 重包完整命令（签名验证之后、DMG之前）

参数顺序为：两个flag的成员均 **x86_64在前、arm64在后**；位置参数为 `OUT P4 WIN_X64 LNX_X64 LNX_ARM OSX_X64原片 OSX_ARM原片 WIN_ARM`。位置参数不能省掉Mac原片或最后WinARM，否则会失去来源分账或六目标成员。

```bash
set -euo pipefail
cd "$GITHUB_WORKSPACE"
bound() { ./tools/bound 55 "$@"; }
mkdir -p out
printf '%s  %s\n' \
  bdd52246c4ba52becff3c1404b20976a51f955e8c3e2218443123d1b464adaa3 work/ape \
  f44512ec290c9dee944fc7c42ae350cdc371dba04773f866bb4ccddc299069fd in/slice-osx-x86_64 \
  cc6d0ed108028c4225711781ad110fe3782b2c8beb45256603c09eb92b5132c5 in/slice-osx-arm64 \
  > out/repack-bootstrap.sha256
bound shasum -a 256 --check out/repack-bootstrap.sha256
cp work/ape ./unisacc.com
cmp work/ape ./unisacc.com
chmod 755 ./unisacc.com
unset UNISA_CONTAINER UNISA_KERNEL UNISA_KERNEL_SHA
# 四个签后原件只补执行位，不重签、不改内容或entitlements。
chmod 755 work/osx-x86_64 work/osx-arm64 work/cache-x86_64 work/cache-arm64
for r in osx-x86_64 osx-arm64 cache-x86_64 cache-arm64; do
  bound codesign --verify --strict --verbose=2 "work/$r"
done
bound shasum -a 256 work/osx-x86_64 work/osx-arm64 \
  work/cache-x86_64 work/cache-arm64 > out/repack-signed-inputs.sha256
# pack自身允许已有OUT，所以调用方必须拒绝旧产物及悬链。
test ! -e work/repacked
test ! -L work/repacked
bound sh ./unisacc.com tools/release-ape.cx -- pack \
  --osx-slices work/osx-x86_64 work/osx-arm64 \
  --cache-manager-osx work/cache-x86_64 work/cache-arm64 \
  work/repacked \
  in/repack-inputs/pkg.bin \
  in/repack-inputs/slice-win-x86_64 \
  in/repack-inputs/slice-lnx-x86_64 \
  in/repack-inputs/slice-lnx-arm64 \
  in/slice-osx-x86_64 in/slice-osx-arm64 \
  in/repack-inputs/slice-win-arm64 \
  > out/repack.log 2>&1
bound shasum -a 256 --check out/repack-signed-inputs.sha256
bound shasum -a 256 --check out/repack-inputs.sha256
bound shasum -a 256 --check out/repack-bootstrap.sha256
# 必须有本次rc0和收据；不是只检查文件存在。
grep -Fx $'cache-manager-mode\tEXPLICIT_INPUTS' work/repacked/ape-receipt.txt
ape_sha=$(shasum -a 256 work/repacked/unisacc-next.com | awk '{print $1}')
receipt_sha=$(awk -F '\t' '$1=="unisacc-next.com" { sub(/^sha=/,"",$3); print $3 }' \
  work/repacked/ape-receipt.txt)
[[ "$ape_sha" =~ ^[0-9a-f]{64}$ ]]
[[ "$ape_sha" == "$receipt_sha" ]]
printf '%s\n' "$ape_sha" > out/repacked-ape-sha256.txt
cp work/repacked/ape-receipt.txt out/repacked-ape-receipt.txt
cp work/repacked/unisacc-next.com out/unisacc-signed.com
cmp work/repacked/unisacc-next.com out/unisacc-signed.com
```

`--cache-manager-osx` 必须显式提供；省略会从checkout源新造manager，不能用于签后重包。该命令不签名，只保持四签后成员的字节。工具内部核四输入重包前后完整SHA不变，收据分别列source/replacement、P4及输出SHA；输出包含 `work/repacked/unisacc-next.com`、`ape-receipt.txt`、`p3-package.body`。短相对路径可避免收据8192字节预算被runner长路径耗尽。

## 输出身份与DMG接线

**固定首签回放**：若四成员恰为 `members-signed.tsv` 中run42556245736的四个完整SHA，且上述所有裸片/P4/包装源码不变，本机实际回放输出为6450974字节，SHA：

```
5e110594033cbaa9e81d59c8677eef4c2727efa79269d35366ff556b37ec177c
```

2026-11-09已在独立完整f099 checkout中，把草稿bdd APE放根，用它自身执行上述pack（含其内层ape-pack），真实exit0并得到这个SHA；不是只拿原f069构造器的结果推算。证据 `out/osx-sign-rosetta/runner-exact-pack.log`。辅助三片上传也已实际成功。

**新的真签**有新的secure timestamp，签后四成员完整SHA可能变化，输出SHA随之变化。不能在长期workflow写死5e11作为所有新签的预期值。应保存本次 `out/repack-signed-inputs.sha256`、输入pins、完整收据、`out/repacked-ape-sha256.txt`，下游下载时用该次上游输出SHA再核验；原members输入hash不冒充签后成员hash。若需确定性复核，用完全相同四签后输入重包到另一个全新OUT，输出必须 `cmp` 相等；过程中不再重签。

将现DMG构造步骤的 `cp work/ape work/dmgroot/unisacc.com` 替换为：

```bash
set -euo pipefail
cd "$GITHUB_WORKSPACE"
bound() { ./tools/bound 55 "$@"; }
bound shasum -a 256 --check out/repack-signed-inputs.sha256
printf '%s  %s\n' "$(cat out/repacked-ape-sha256.txt)" out/unisacc-signed.com | \
  bound shasum -a 256 --check -
mkdir -p work/dmgroot
cp out/unisacc-signed.com work/dmgroot/unisacc.com
for r in osx-arm64 osx-x86_64 cache-arm64 cache-x86_64; do
  cp "work/$r" "work/dmgroot/$r"
  chmod 755 "work/dmgroot/$r"
  cmp "work/$r" "work/dmgroot/$r"
done
cmp out/unisacc-signed.com work/dmgroot/unisacc.com
# 然后承接原hdiutil create、DMG签名、submit、staple流程。
```

DMG中的APE必须是新输出，四个独立成员必须是同一次重包输入，不能装回原bdd APE或再次重签四成员后仍放旧APE。修改DMG后需新签/新公证，原Accepted不覆盖新DMG；新APE也不迁旧42/0资格。runner运行检查使用 `sh out/unisacc-signed.com --version` / `-run` 用户入口，不以保留quarantine的裸CLI启动等待判断内核卡死。完整新身份的CI/发行验收仍按既有流程另行绑定。
