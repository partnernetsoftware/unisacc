#!/bin/bash
# approve_signing.sh RUN -- approve the pending release-signing deployment of a company signing run.
set -eu; RUN=${1:?run id}; REPO=$(gh repo view --json nameWithOwner --jq .nameWithOwner)
ENV=$(gh api repos/$REPO/environments --jq '.environments[] | select(.name=="release-signing") | .id')
for i in $(seq 1 12); do [ "$(gh api repos/$REPO/actions/runs/$RUN/pending_deployments --jq length)" = 1 ] && break; sleep 10; done
printf '{"environment_ids":[%s],"state":"approved","comment":"approved by release/tools/approve_signing.sh"}' "$ENV" | gh api -X POST repos/$REPO/actions/runs/$RUN/pending_deployments --input - >/dev/null
gh api repos/$REPO/actions/runs/$RUN/pending_deployments --jq 'if length==0 then "approved" else "still pending" end'
