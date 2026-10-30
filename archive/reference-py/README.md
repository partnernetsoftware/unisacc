# 参考 Python 与依赖它的旧脚本（2026-10-30 归档）

这些文件从活路径原样挪来，相对路径不变：`archive/reference-py/<原路径>`。只供对照，不再往这里加逻辑，活路径不得回调这里。

归档依据：正规构造链（`tools/source-package.cx` 的 107 作业、`source-job-wave`、`kernel-build`、`product-build`）不调用 Python；新测试清单 `tests/batches.tsv` 不直接引用 `.py`/`.sh`。TSV 来源列与注释里出现的旧 `.py` 路径是历史出处，按上面的映射在本目录找到。

本次一并挪走的 `.sh` 都会执行 `python3`（多数还依赖已归档的参考 C），外加只被它们调用的 `exec/c/asm/bindingcheck.sh`、`exec/pp/shards.sh`。

## 仍留在活路径的 Python（剩余债，须先有原生替代才能归档）

1. ~~事实表生成器 `exec/facts/export.py`~~：2026-10-30 已归档到 `archive/reference-py/exec/facts/export.py`。30 张 `exec/facts/*.tsv` 全部改由 `tools/facts-gen.cx` 写出并 `--check`；原 `--check` 里的 bank 归属检查（`_bank_owners`）一并移入 `facts-gen --check`。原生产者里的 Python 字面量已改记为事实文件，包括 `exec/facts/enc-literals.tsv`、`gen2-constants.tsv`、`gen2-literals.tsv`、`image-*.tsv`、`tape-shape.tsv`、`pp-escapes.tsv` 和 `exec/nativeabi/trie-order.tsv`。它原来的导入闭包同日一并归档：`exec/assemble.py`、`exec/finite_rules.py`、`exec/facts/load.py`、`exec/build/parsebase.py`、`exec/build/parse2base.py`、`exec/build/graph.py`、`exec/build/procs.py`。依据有三条：
   - 活路径的 `.py`/`.cx`/`.sh`/Makefile 没有一处导入或执行它们（`unisa/__main__.py` 的 `.assemble` 是 `unisa/assemble.py`）；
   - `tools/facts-gen.cx` 只在注释里提到它们作为出处；
   - 在仓外副本用审计钩子实跑 `python3 -m unisa emit-kernel`，没有打开 `exec/` 下任何 `.py`。

   `exec/**/*-manifest.tsv` 开头的 `#! base build/parsebase.py` 是历史元数据，manifest 本身仍是构造与测试的数据，留在原处。
2. 18 网内核生成器 `python3 -m unisa emit-kernel`（`kernel/` 的生成者）及其实测闭包；`unisa/ckernel.py`、`unisa/gold.py`、`unisa/libneed.py` 还被 `tools/table-kernel-export.cx`、`tools/net-material.cx`、`tools/pp-gold-material.cx`、`tools/carried-header-export.cx` 以及 `tests/pp-constructed.cx`、`tests/pp-gold-controls.cx` 当作数据或身份输入读取。
3. `release/` 发布工具链（未在本次范围）；`apps/`、`m4/`、`research/` 各属其域，未动。
4. `exec/c/buildcompiler.sh` 仍是 Makefile 的构造入口，留在原处；其中 `python3 -m unisa ape` 回退分支已不可用。

闭包由审计钩子在仓外副本实跑，并在删去归档文件后反复重跑补回缺件（补回 `exec/build/graph.py`、`exec/build/procs.py`），直到输出与归档前一致；依据是 `exec/facts/export.py --check` 与 `python3 -m unisa emit-kernel` 得到。

## 归档前已存在的断点

活树里 `exec/facts/export.py --check` 在本次之前就会因输入 `src/front_pp.c` 已随参考 C 归档而 FileNotFoundError 中止，本次前后行为一致；仓外副本补回参考 C 后为“30 tables, 29 differ”，即事实表相对其声明输入已陈旧。`python3 -m unisa emit-kernel` 本次前后产物逐字节相同，但其 `unisa_headers.inc` 与仓内 `kernel/` 不同。两者均未在本次修复。

## 清单

- examples/run-speed/time_run.py
- exec/assemble.py
- exec/build/gen.py
- exec/build/graph.py
- exec/build/parse2base.py
- exec/build/parsebase.py
- exec/build/procs.py
- exec/c/apecache.py
- exec/c/asm/bindingcheck.sh
- exec/c/asm/bindprep.sh
- exec/c/asm/bindverify.sh
- exec/c/asm/blob.py
- exec/c/asmcheck.sh
- exec/c/buildffiprovider.py
- exec/c/buildlibrary.py
- exec/c/buildtarget.py
- exec/c/chain.sh
- exec/c/check.sh
- exec/c/codeccheck.py
- exec/c/codeccheck.sh
- exec/c/comboot.py
- exec/c/compilercheck.py
- exec/c/compilercheck.sh
- exec/c/compilerpack.py
- exec/c/containercheck.py
- exec/c/containercheck.sh
- exec/c/containerlinux.py
- exec/c/containerlinux.sh
- exec/c/corecheck.sh
- exec/check.sh
- exec/c/librarycallcheck.py
- exec/c/librarystackcheck.py
- exec/c/memorycheck.py
- exec/c/memorycheck.sh
- exec/c/modelboot.py
- exec/c/multicheck.py
- exec/c/multicheck.sh
- exec/c/multiwarningcheck.py
- exec/c/multiwarningcheck.sh
- exec/c/nativecheck.sh
- exec/c/neg.sh
- exec/c/netcheck.py
- exec/c/net.py
- exec/c/networkformat.py
- exec/c/packagecheck.py
- exec/c/packagecheck.sh
- exec/c/packageformat.py
- exec/c/pack.py
- exec/c/posixmemoryrun.py
- exec/c/processcheck.py
- exec/c/provenance.py
- exec/c/readtrace.py
- exec/c/refsource.py
- exec/c/targetpackage.py
- exec/c/tbl.py
- exec/c/warningcheck.py
- exec/c/warningcheck.sh
- exec/c/winmemorycheck.py
- exec/c/winmemorycheck.sh
- exec/c/winmemoryrun.py
- exec/enc/armbranchcheck.py
- exec/enc/armcheck.py
- exec/enc/armcheck.sh
- exec/enc/armfpcheck.py
- exec/enc/armimagecheck.sh
- exec/enc/arminputcheck.py
- exec/enc/armintcheck.py
- exec/enc/armlayoutcheck.py
- exec/enc/armmemcheck.py
- exec/enc/armwincheck.py
- exec/enc/armwincheck.sh
- exec/enc/check.sh
- exec/enc/dynelfcheck.py
- exec/enc/fpcheck.py
- exec/enc/imagecheck.py
- exec/enc/imagecheck.sh
- exec/enc/librarysymbolscheck.py
- exec/enc/machocheck.py
- exec/enc/machocheck.sh
- exec/enc/pecheck.py
- exec/enc/pecheck.sh
- exec/enc/pevmcheck.py
- exec/enc/realcheck.py
- exec/enc/roundtrip.py
- exec/enc/shacheck.py
- exec/enc/shacheck.sh
- exec/enc/x86wincheck.py
- exec/enc/x86wincheck.sh
- exec/facts/export.py
- exec/facts/load.py
- exec/ffi/bridgecheck.py
- exec/ffi/hostcheck.sh
- exec/ffi/productcheck.py
- exec/finite_rules.py
- exec/ledger.sh
- exec/lex/compare.py
- exec/lex/declcheck.py
- exec/lex/ledger.sh
- exec/lex/mkpre.sh
- exec/lex/run.sh
- exec/lex/sim.py
- exec/lex/tbl.py
- exec/librarycallbackpartition/check.sh
- exec/librarycallsitepartition/check.sh
- exec/librarycontract/check.sh
- exec/librarypartition/check.sh
- exec/librarytypedpartition/check.sh
- exec/lower/armcheck.sh
- exec/lower/check.py
- exec/lower/check.sh
- exec/lower/fullcheck.py
- exec/lower/fullcheck.sh
- exec/lower/hostsymbolscheck.sh
- exec/lower/largecodecheck.py
- exec/lower/librarycallsitecheck.sh
- exec/lower/librarydatacheck.sh
- exec/lower/libraryfncheck.sh
- exec/lower/libraryimportscheck.py
- exec/lower/librarytypedcheck.sh
- exec/lower/sparsecheck.py
- exec/lower/sparsecheck.sh
- exec/lower/wincheck.sh
- exec/opt/check.sh
- exec/opt/gen-delta.sh
- exec/parse2/diagnosticcheck.py
- exec/parse2/enumcheck.py
- exec/parse2/errorcheck.py
- exec/parse2/floatconstcheck.py
- exec/parse2/floatconstcheck.sh
- exec/parse2gen/gen-delta.sh
- exec/parse2/librarycallsitecheck.sh
- exec/parse2/libraryexportscheck.py
- exec/parse2/libraryfunctionimportscheck.sh
- exec/parse2/librarytypedreadercheck.sh
- exec/parse2/r21check.py
- exec/parse2/selfcheck.sh
- exec/parse2/warnprep.py
- exec/parse/compare.py
- exec/parse/cut.py
- exec/parse/mkdump.sh
- exec/pipeline/check-elf.py
- exec/pipeline/check-elf.sh
- exec/pipeline/check_pp_text.py
- exec/pipeline/_check.py
- exec/pipeline/check_src_c.py
- exec/pipeline/check_tape_text.py
- exec/pipeline/check_tokens_plain.py
- exec/pipeline/check_tokens_typed.py
- exec/pipeline/elf.sh
- exec/pipeline/models.py
- exec/pipeline/prepare.sh
- exec/pipeline/run.py
- exec/pipeline/selfcheck.sh
- exec/pipeline/simrun.py
- exec/pipeline/winbootstrap.py
- exec/pp/compare.py
- exec/pp/gen-delta.sh
- exec/pp/literalcheck.py
- exec/pp/mknoauto.sh
- exec/pp/run.sh
- exec/pp/shards.sh
- exec/pp/sharedcheck.py
- exec/pp/sim.py
- exec/prune/gen-delta.sh
- exec/tapecheck/check.sh
- iterate/construct/check.sh
- iterate/construct/tools/netdump.py
- iterate/construct/tools/uns2pack.py
- iterate/construct/tools/uns2round.py
- iterate/construct/tools/uns2slice.py
- iterate/kernel/check.sh
- iterate/kernel/order_check.py
- iterate/kernel/permoracle.py
- iterate/kernel/typekw_check.py
- iterate/kernel/vocab_check.py
- unisa/ape.py
- unisa/assemble.py
- unisa/control/segment.py
- unisa/docgen.py
- unisa/hostabi.py
- unisa/hostbind.py
- unisa/hostsymbols.py
- unisa/image/fat.py
- unisa/librarycallbackpartition.py
- unisa/librarycallsitepartition.py
- unisa/librarycontract.py
- unisa/librarydatalower.py
- unisa/libraryfnlower.py
- unisa/libraryfunctionimports.py
- unisa/librarypartition.py
- unisa/librarytypedpartition.py
- unisa/librarytypedreader.py
- unisa/opt.py
- unisa/peversion.py
- unisa/prune.py
- unisa/signature3.py
- unisa/tapebin.py
- unisa/tapebin_shape.py
- unisa/uns2.py

## 第二批（2026-10-30）

引用已归档参考 C 或已归档 Python 的旧 shell，以及 Python 时代未落地的 `plans/pending` 补丁（目标文件已归档）。`.cx` 旧检查未挪：挪动会改变 `tests/cxinclude|cxapis|cxtrunc` 棘轮的路径键，等这三份棘轮在主线恢复绿后再处理。`tools/build_ref.sh`、`tools/export_ref.sh`、`exec/c/qprefixcheck.cx` 仍被现行覆盖清单或工具引用，留在原处。

- bench/cpu_ratio.sh
- examples/win/run.sh
- exec/c/comboot.sh
- exec/stamp.sh
- plans/pending/delgov-fix3/core.patch
- plans/pending/delgov-fix3/js.patch
- plans/pending/delgov-fix3/scanner-new-sites.tsv
- plans/pending/delgov-fix3/scanner.patch
- plans/pending/delgov-fix3/triage.patch
- plans/pending/detach-w2/W2-A.patch
- plans/pending/detach-w2/W2-C.patch
- plans/pending/detach-w2/W2-D1.patch
- plans/pending/detach-w2/W2-E.patch
- plans/pending/detach-w2/W2-F.patch
- plans/pending/f1/0001-fix-libc-vsnprintf-NULL-0-measures-and-vprintf-write.patch
- plans/pending/f1/0002-fix-libc-comments-in-dup2-and-fopen-no-longer-name-o.patch
- plans/pending/f1/0003-fix-libc-setenv-unsetenv-edit-environ-execvp-system-.patch
- plans/pending/f1/0004-test-libc-setenv-takes-a-value-of-any-length-and-any.patch
- plans/pending/f1/0005-fix-libc-fileno.patch
- plans/pending/f1/0006-fix-cx-cx_read-of-a-directory-is-NULL-as-its-contrac.patch
- plans/pending/f1/0007-fix-parse2-enum-E-bare-tag-declaration-accepted-agai.patch
- plans/pending/flownet-w5/files/adapter/ujswasm/ujswasm.cx.pending
- plans/pending/flownet-w5/files/docs/flownet-migration.md
- plans/pending/flownet-w5/files/flownet/README.md
- plans/pending/flownet-w5/files/tests/flownet-coverage.tsv
- plans/pending/flownet-w5/files/tests/flownetw5.cx.pending
- plans/pending/py2cx-w7-unlanded/exec_c_librarystackcheck.cx.pending
- plans/pending/py2cx-w7-unlanded/exec_c_netcheck.cx.pending
- plans/pending/py2cx-w7-unlanded/exec_parse2_diagnosticcheck.cx.pending
- plans/pending/py2cx-w7-unlanded/exec_parse2_errorcheck.cx.pending
- plans/pending/py2cx-w7-unlanded/exec_parse2_warnprep.cx.pending
- plans/pending/README.md
- plans/pending/tapebin-selfheal/tapebinref-selfheal.partial.patch

- exec/pipeline/stages.tsv（旧 Python 流水线定义，无活使用方）

## 第三批（2026-10-30）

依赖已归档参考 C（ua_ref/build_ref/export_ref/compiler.c）或 Python 的旧 `exec/*` 检查 .cx，及只被它们使用的 cxlib 符号链接。三份 cx 棘轮已改为不计 archive/（`332208a9fd`），挪动不新增棘轮行。`exec/c/qprefixcheck.cx` 与 `exec/c/cxlib` 仍被覆盖清单引用，留在原处。

- exec/c/embeddedcheck.cx
- exec/c/linuxbridgecheck.cx
- exec/enc/cxlib/hostcheck.cx
- exec/enc/cxlib/pytempfile.cx
- exec/enc/windowshostbridgecheck.cx
- exec/lex/f1sidecarcheck.cx
- exec/lex/locationcheck.cx
- exec/lex/positioncheck.cx
- exec/lex/sourcefactscheck.cx
- exec/parse2/cxlib/pytempfile.cx
- exec/parse2/f1attributescheck.cx
- exec/parse2/libraryimportscheck.cx
- exec/parse2/locationcheck.cx
- exec/parse2/r21check.cx
- exec/parse2/returnwarningcheck.cx
- exec/parse2/unitlocationcheck.cx
- exec/pp/cxlib/e2stream.cx
- exec/pp/cxlib/pytempfile.cx
- exec/pp/cxlib/runtimecopy.cx
- exec/pp/directivecheck.cx
- exec/pp/e2bench.cx
- exec/pp/literalcheck.cx
- exec/pp/locationcheck.cx
- exec/pp/macrocheck.cx
- exec/pp/objectpredefinecheck.cx
- exec/pp/pragmacheck.cx
- exec/prune/check.cx
- release/tools/attemptchain.py
- release/tools/c99precheck.py
- release/tools/docedit.py
- release/tools/model-reseal.py
- release/tools/stagesum.py
- release/tools/unsigned_receipt.py
- release/tools/whyrerun.py
- research/tools/c38-decomp.py
- release/macosbundle.py
- release/tools/apple-sign.sh
- release/tools/release_prep.sh
- release/tools/commitgate.sh
- release/tools/derivedcheck.py
- release/tools/hooks/pre-commit
- release/tools/install_hooks.sh
