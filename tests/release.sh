#!/bin/bash
# What has to be true before a release. [S-14]
#
# Releasing was a sequence someone remembered: run the suites, notice that
# crossnative had quietly skipped a target, build the .com, check it runs,
# push, tag.  The parts that are checkable are checked here instead, and
# the strictness that a release wants -- a SKIPPED target is a failure --
# is the difference between this and `all.sh`.
#
#   ./tests/release.sh            everything, and say whether it is ready
#   ./tests/release.sh --com      also build unisacc.com and exercise it
#
#   SUITES=0   skip step 3's single `all.sh` run (about nine minutes, one
#              unbounded aggregate).  Run the suites instead as the bounded
#              steps listed in AGENTS.md / tests/snap.sh, each <= 60 s, and
#              record them; this script then checks the tree, the version
#              string and the artifact only.
#   RELEASE_OUT=dir   where the built unisacc.com is KEPT (default: dist/).
#              The scratch copy is deleted when the script exits.
#
# It does not push, tag, or upload: those are the parts a person decides.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"
com=0; [ "${1:-}" = "--com" ] && com=1
bad=0
say() {   # say <what> <ok|FAIL> <detail>
    printf "  %-4s %-26s %s\n" "$2" "$1" "$3"
    [ "$2" = "ok" ] || bad=$((bad+1))
}

# 1. the tree is committed: a release built from a dirty tree cannot be
#    rebuilt from the tag it claims to be
# Only what the artifact is BUILT from: this tree is shared with another
# workstream, whose uncommitted files say nothing about whether this release
# can be rebuilt from its tag.  (The first version looked at everything but
# ujs/ and failed on eighteen of that workstream's files elsewhere.)
INPUTS="src unisa kernel include unisacc.c weights"
dirty=$(git status --porcelain -- $INPUTS 2>/dev/null | wc -l | tr -d ' ')
say "tree committed" "$([ "$dirty" = 0 ] && echo ok || echo FAIL)" \
    "$([ "$dirty" = 0 ] && echo "clean" || echo "$dirty file(s) uncommitted")"

# 2. the binary says which build it is
ua_ready
v=$("$UA" --version 2>&1)
case "$v" in "unisacc "[0-9]*) say "version string" ok "$v";;
             *) say "version string" FAIL "$v";; esac

# 3. every suite, with a skipped target counted as a failure
# the machines crossnative needs: started here if they are not running, and
# stopped on the way out -- failure included -- if this started them [S-15 F1]
if [ "${SUITES:-1}" = 1 ] && [ "${VMS:-1}" = 1 ]; then
    ./tests/vms.sh up
    trap './tests/vms.sh down' EXIT
fi
LOG=$(mktemp)                        # not in the tree: nobody commits it
if [ "${SUITES:-1}" = 1 ]; then
STRICT=1 ./tests/all.sh > "$LOG" 2>&1
rc=$?
say "all suites (STRICT=1)" "$([ $rc -eq 0 ] && echo ok || echo FAIL)" \
    "$(tail -3 "$LOG" | head -1)"
[ $rc -eq 0 ] || sed -n '/=== summary/,$p' "$LOG" | grep FAIL | head -10
# 3a'. ablate: every listed stage's answer must change an image; eight shards,
# each under the 60 s ceiling
for k in 1 2 3 4 5 6 7 8; do
    SHARD=$k/8 perl -e 'alarm 60; exec @ARGV' ./tests/ablate.sh > "$LOG.a$k" 2>&1 || { arc=1; tail -3 "$LOG.a$k"; }
done
say "ablate (8 shards)" "$([ ${arc:-0} -eq 0 ] && echo ok || echo FAIL)" "$(tail -1 "$LOG.a8")"
# 3a. the gate: ccparity and malloc are in no other suite list [S-16 T1]
./tests/gate.sh --com > "$LOG.g" 2>&1
rc=$?
say "gate (--com)" "$([ $rc -eq 0 ] && echo ok || echo FAIL)" "$(tail -1 "$LOG.g")"
[ $rc -eq 0 ] || grep -v 'rc=0' "$LOG.g" | head -10

# 3b. ...and a suite that SKIPPED is not a suite that passed.  fat is macOS
#     only, corpus and tools need their corpora present, crossnative needs
#     the VMs: each is legitimate on some machine and none of them is
#     legitimate on the machine a release is cut from.
# The suites' own words for a skip: "SKIPPED n: target" (crossnative) and
# "skipped (reason)" (fat, corpus, selfgap).  A bare "skipped" also matched
# crossnative's "(no target skipped)" -- the gate failed on the sentence
# saying nothing was skipped.
SKIPRE='SKIPPED [0-9]|skipped \('
skipped=$(sed -n '/=== summary/,$p' "$LOG" | grep -cE "$SKIPRE" || true)
say "nothing skipped" "$([ "$skipped" = 0 ] && echo ok || echo FAIL)" \
    "$(sed -n '/=== summary/,$p' "$LOG" | grep -E "$SKIPRE" | \
       sed 's/^  ok *//' | tr '\n' ';' | cut -c1-70)"
else
    echo "  --   all suites               not run here (SUITES=0): run them as bounded steps and record them"
fi

# 4. the artifact, if asked: built here for all six targets, then run
if [ "$com" = 1 ]; then
    T=$(scratch)
    # built as `make com` builds it: -O2 [H1]
    if bound 60 python3 -m unisa ape unisacc.c --via "$UA" -O 2 -o "$T/unisacc.com" \
           > "$T/ape.log" 2>&1; then
        chmod +x "$T/unisacc.com"
        sz=$(wc -c < "$T/unisacc.com" | tr -d ' ')
        say ".com built" ok "$sz B"
        # from a directory with nothing else in it, the way someone who
        # downloaded it has
        mkdir -p "$T/fresh"; cp "$T/unisacc.com" "$T/fresh/"
        printf '#include <stdio.h>\nint h(void);\nint main(void){printf("%%d\\n",h());return 0;}\n' > "$T/fresh/a.c"
        printf 'static int n = 7;\nint h(void){return n;}\n' > "$T/fresh/b.c"
        # the command's own status counts as well as its output: a run that
        # printed 7 and then crashed, or was killed by the watchdog, fails
        got=$( (cd "$T/fresh" && bound 60 ./unisacc.com -run a.c b.c 2>&1) ); grc=$?
        say ".com compiles two files" "$([ "$grc" = 0 ] && [ "$got" = 7 ] && echo ok || echo FAIL)" "rc $grc, output '$got'"
        OUT=${RELEASE_OUT:-$R/dist}
        mkdir -p "$OUT" && cp "$T/unisacc.com" "$OUT/unisacc.com" && chmod +x "$OUT/unisacc.com"
        say ".com kept" "$([ -x "$OUT/unisacc.com" ] && echo ok || echo FAIL)" \
            "$OUT/unisacc.com  sha256 $(shasum -a 256 "$OUT/unisacc.com" | cut -c1-16)  from $(git rev-parse --short HEAD)"
    else
        say ".com built" FAIL "$(tail -1 "$T/ape.log")"
    fi
fi

echo
if [ "$bad" -eq 0 ]; then
    echo "release  ready -- $v"
else
    echo "release  NOT ready: $bad check(s) failed"
fi
[ "$bad" -eq 0 ]
