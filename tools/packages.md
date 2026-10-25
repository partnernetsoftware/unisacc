# Native development package refresh

`package-refresh.cx` supports one explicitly pinned migration baseline (P7f709),
whose frozen source checkout must be supplied. It retains the complete original
P3 directory and compressed model records byte-for-byte, and replaces exactly the
two named kernel resources with this generation's freshly constructed UNIKERN1.
The result is preparation material, not a test result or release qualification.

```sh
tools/bound 55 sh ./unisacc.com tools/package-refresh.cx -- "$PWD" /path/to/frozen-baseline "$PWD/out/kernel-fresh" "$PWD/out/package-fresh"
```

Before and after reading material it checks the kernel generation's full source,
tool and bootstrap context through its digest-verified identity helper. Both blobs
must match their successful construction receipts, ISA, extent, entry and zero
service-slot contracts. The baseline product's complete bytes must match the
pinned SHA. Product and kernel bytes are reread before output creation.

Reusing the baseline models requires equality of constructor inputs in both
directions: exec generator Python and TSV/JSON data (excluding the separate C
runtime/packaging and pipeline orchestration), unisa, weights, generated classic
kernel fragments, carried headers, iterate/kernel, the seed constructors and
package writers, and the generic C core. exec/build only contributes its top-level
Python sources; generated material is excluded. Check drivers are not generators.
Selected source symlinks are refused. New, removed or changed selected inputs are
errors; a changed constructor requires model reconstruction. The actual 1241-file
comparison manifest is written with the package. This is a narrow migration
contract, not a general-purpose dependency scanner or an arbitrary cache hit.

The current C driver must be rebuilt independently for all five APE code slices.
Windows ARM64 continues to use the existing Windows x86_64 APE route; its native
host support is a separate obligation. `seed/ape.c` already provides a stored-
DEFLATE gzip implementation when APE_ZLIB is unset, so container construction
needs neither a new compressor nor host zlib. Its compression bytes differ from
the historical zlib build. No signature, publication or release is performed.

`tests/package-controls.cx` damages a private copy of a kernel, requires refusal
before output creation, restores the bytes, and requires preparation to recover.
It does not modify live generation inputs or cache functional PASS results.

After qualifying a candidate locally, `candidate-install.cx ROOT KERNEL_GENERATION
CANDIDATE_COM PREPARED_PACKAGE` installs it as the private root development
product. It rechecks the shared `kernelcontext` module contract, verifies exact
embedded-package equality, writes the standard current-source provenance, and
binds newer construction tools plus the five exact code slices in a supplemental
receipt. The standard source identity's original ten-support-file domain is not
silently expanded. The root build JSON includes the supplemental receipt's hash;
the receipt itself remains next to the candidate as `.native.tsv`.

The installer preserves the previous product and build record in the generation
directory, creates new files exclusively, and atomically replaces each root file.
The pair of renames is not one atomic transaction: a second-rename failure reports
an installation error with the backup retained. Installation grants no new host,
full-suite or release qualification. The old generation's context subsequently
rejects further construction against the replaced bootstrap; start a fresh one.

`product-build.cx ROOT KERNEL_GENERATION PACKAGE_DIR STEP` constructs each driver
slice and the APE through default-run CX orchestration. Use separate bounded
commands with steps `slice-win/x86_64`, `slice-lnx/x86_64`, `slice-lnx/arm64`,
`slice-osx/x86_64`, `slice-osx/arm64`, then `wrap`. Each slice is actually rebuilt.
Every step checks the kernel source/bootstrap context and binds the coordinator,
package tools/module, seed APE writer and exact package material before/after.
Its `.native-build.tsv` receipts contain the construction context and artifact
SHA; wrap rehashes all five slices against their receipts before and after use.
Existing or partial output is refused. These are PREPARED receipts, not tests.

`tests/selfhost.cx ROOT PRIOR_GENERATION FRESH_GENERATION` compares all 22
normalized sources, 22 objects, both kernels, five code slices and the full fresh
product against the installed bootstrap. It checks the installed artifact/source
record and current kernel context too. A successful two-generation comparison
does not substitute for the required complete bootstrap chain or foreign hosts.
