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
dirty=$(git status --porcelain -- ':!ujs' 2>/dev/null | wc -l | tr -d ' ')
say "tree committed" "$([ "$dirty" = 0 ] && echo ok || echo FAIL)" \
    "$([ "$dirty" = 0 ] && echo "clean" || echo "$dirty file(s) uncommitted")"

# 2. the binary says which build it is
ua_ready
v=$("$UA" --version 2>&1)
case "$v" in "unisacc "[0-9]*) say "version string" ok "$v";;
             *) say "version string" FAIL "$v";; esac

# 3. every suite, with a skipped target counted as a failure
STRICT=1 ./tests/all.sh > "$R/.release.log" 2>&1
rc=$?
say "all suites (STRICT=1)" "$([ $rc -eq 0 ] && echo ok || echo FAIL)" \
    "$(tail -3 "$R/.release.log" | head -1)"
[ $rc -eq 0 ] || sed -n '/=== summary/,$p' "$R/.release.log" | grep FAIL | head -10

# 3b. ...and a suite that SKIPPED is not a suite that passed.  fat is macOS
#     only, corpus and tools need their corpora present, crossnative needs
#     the VMs: each is legitimate on some machine and none of them is
#     legitimate on the machine a release is cut from.
skipped=$(sed -n '/=== summary/,$p' "$R/.release.log" | grep -ci "skipped" || true)
say "nothing skipped" "$([ "$skipped" = 0 ] && echo ok || echo FAIL)" \
    "$(sed -n '/=== summary/,$p' "$R/.release.log" | grep -i skipped | \
       sed 's/^  ok *//' | tr '\n' ';' | cut -c1-70)"

# 4. the artifact, if asked: built here for all six targets, then run
if [ "$com" = 1 ]; then
    T=$(scratch)
    if bound 1800 python3 -m unisa ape unisacc.c --via "$UA" -o "$T/unisacc.com" \
           > "$T/ape.log" 2>&1; then
        chmod +x "$T/unisacc.com"
        sz=$(wc -c < "$T/unisacc.com" | tr -d ' ')
        say ".com built" ok "$sz B"
        # from a directory with nothing else in it, the way someone who
        # downloaded it has
        mkdir -p "$T/fresh"; cp "$T/unisacc.com" "$T/fresh/"
        printf '#include <stdio.h>\nint h(void);\nint main(void){printf("%%d\\n",h());return 0;}\n' > "$T/fresh/a.c"
        printf 'static int n = 7;\nint h(void){return n;}\n' > "$T/fresh/b.c"
        got=$( (cd "$T/fresh" && bound 180 ./unisacc.com -run a.c b.c 2>&1) )
        say ".com compiles two files" "$([ "$got" = 7 ] && echo ok || echo FAIL)" "$got"
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
