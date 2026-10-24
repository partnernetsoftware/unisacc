#!/bin/sh
set -eu
R=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$R"
sh tests/cxbin tests/metadatalist.cx tests/seedconstruct-parts.tsv >/dev/null
sh tests/cxbin tests/metadataemit.cx --emit-module tests/seedconstruct-parts.tsv | cmp - tests/cxlib/seedparts.cx
echo 'seed metadata: source digest, ordered rows and regenerated module identical'
