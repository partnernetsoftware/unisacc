#!/bin/bash
# windows_sign.sh MODE TAG RECEIPT RUN_ID ATTEMPT -- dispatch windows-signing.yml (qualification or company)
# for the exact current main SHA; company runs wait for the release-signing approval (approve with
# approve_signing.sh RUN).  The receipt SHA is computed from the file; nothing secret is handled here.
set -eu; R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
MODE=${1:?qualification|company}; TAG=${2:?seal tag, e.g. v0.0.16}; REC=${3:?unsigned-receipt.json}; RUN=${4:?release-check run id}; ATT=${5:?attempt}
SHA=$(git rev-parse HEAD); [ "$(git ls-remote origin refs/heads/main | cut -c1-40)" = "$SHA" ] || { echo "main is not pushed at HEAD" >&2; exit 2; }
gh workflow run windows-signing.yml --ref main -f mode=$MODE -f source_sha=$SHA -f seal_tag=$TAG -f receipt_sha256=$(shasum -a 256 "$REC" | cut -d' ' -f1) -f upstream_run_id=$RUN -f upstream_attempt=$ATT
sleep 20; gh run list --workflow windows-signing.yml -L 1 --json databaseId --jq '.[0].databaseId'
