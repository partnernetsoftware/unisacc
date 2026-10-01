#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# R18-1/R18-2: GNU assembly text (src/asmtext.c).
#   1. round trip: for every tests/c program and both Linux targets,
#      `-S -b lnx/ARCH` then `unisacc as` must give the object `-c -b` writes,
#      byte for byte, with no instruction printed as raw bytes;
#   2. unit objects (-funit, with .unisa.tape) round-trip too, and the
#      reassembled objects link into the same program;
#   3. the system assembler (clang's, for the Linux triples) accepts the -S
#      text and produces the same .text bytes and relocations;
#   4. hand-written tests/asm/*.s: `unisacc as` and the system assembler agree
#      on .text, .data and the relocations;
#   5. refusals by name.
# Without clang's Linux targets, legs 3 and 4 are skipped by name (a failure
# with STRICT=1).  PRODUCT_COM=path also runs leg 1 on the shipped .com.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ok=0; bad=0; skip=0
say() { if [ "$2" = "$3" ]; then ok=$((ok+1)); else bad=$((bad+1)); printf "  FAIL %-34s want [%s] got [%s]\n" "$1" "$2" "$3"; fi; }
textfall() { sed -n '1,/^	\.data/p' "$1" | grep -c '^	\.byte\|^	\.inst'; }

# 1. the corpus, both targets
for a in x86_64 arm64; do
    n=0; d=0; fb=0; pn=0
    for f in tests/c/*.c; do
        b=$(basename "$f" .c)
        "$_BOUND" 20 "$UA" "$f" -c -b lnx/$a -o "$T/c.o" 2>/dev/null || continue
        if ! "$_BOUND" 20 "$UA" "$f" -S -b lnx/$a -o "$T/s.s" 2>"$T/err"; then d=$((d+1)); echo "  FAIL $a $b -S: $(head -1 "$T/err")"; continue; fi
        if ! "$_BOUND" 20 "$UA" as "$T/s.s" -o "$T/a.o" 2>"$T/err" >>"$T/err"; then d=$((d+1)); echo "  FAIL $a $b as: $(head -1 "$T/err")"; continue; fi
        if cmp -s "$T/c.o" "$T/a.o"; then n=$((n+1)); else d=$((d+1)); echo "  FAIL $a $b: -S | as differs from -c"; fi
        fb=$((fb + $(textfall "$T/s.s")))
        # the product: its own -S, assembled by its own `as`, is its own -c (every 8th program: time)
        if [ -n "${PRODUCT_COM:-}" ] && [ $(( $(printf '%s' "$b" | cksum | cut -d' ' -f1) % 8 )) = 0 ]; then
            "$_BOUND" 30 sh "$PRODUCT_COM" "$f" -c -b lnx/$a -o "$T/pc.o" 2>/dev/null || continue
            "$_BOUND" 30 sh "$PRODUCT_COM" "$f" -S -b lnx/$a -o "$T/p.s" 2>"$T/err" || { d=$((d+1)); echo "  FAIL $a $b product -S: $(head -1 "$T/err")"; continue; }
            "$_BOUND" 30 sh "$PRODUCT_COM" as "$T/p.s" -o "$T/pa.o" 2>"$T/err" || { d=$((d+1)); echo "  FAIL $a $b product as: $(head -1 "$T/err")"; continue; }
            cmp -s "$T/pc.o" "$T/pa.o" && pn=$((pn+1)) || { d=$((d+1)); echo "  FAIL $a $b: product -S | as differs from product -c"; }
        fi
    done
    say "corpus $a round trip" "0 wrong" "$d wrong"
    say "corpus $a instructions as bytes" "0" "$fb"
    [ "$n" -gt 100 ] || { bad=$((bad+1)); echo "  FAIL corpus $a: only $n programs checked"; }
    echo "  corpus lnx/$a: $n objects identical${PRODUCT_COM:+, product $pn}"
done

# 2. unit objects with their tape, and linking the reassembled ones
for a in x86_64 arm64; do
    for u in m1 m2; do
        "$_BOUND" 20 "$UA" -I tests/multi tests/multi/$u.c -c -b lnx/$a -funit -o "$T/$u.o" 2>/dev/null
        "$_BOUND" 20 "$UA" -I tests/multi tests/multi/$u.c -S -b lnx/$a -funit -o "$T/$u.s" 2>/dev/null
        "$_BOUND" 20 "$UA" as "$T/$u.s" -o "$T/$u.r.o" 2>/dev/null
        say "unit $a $u round trip" same "$(cmp -s "$T/$u.o" "$T/$u.r.o" && echo same || echo differ)"
    done
    "$_BOUND" 30 "$UA" "$T/m1.o" "$T/m2.o" -b lnx/$a -o "$T/p1" 2>/dev/null
    "$_BOUND" 30 "$UA" "$T/m1.r.o" "$T/m2.r.o" -b lnx/$a -o "$T/p2" 2>/dev/null
    say "unit $a reassembled link" same "$(cmp -s "$T/p1" "$T/p2" && [ -s "$T/p1" ] && echo same || echo differ)"
done

# 3 and 4: the system assembler
sections() {   # .text bytes, then relocations (offset type symbol), then .data bytes
    objdump -d "$1" | awk -F'\t' 'NF>2{print $1}' | sed 's/^ *[0-9a-f]*://'
    objdump -r "$1" | grep -v '^$\|file format'
    objdump -s -j .data "$1" 2>/dev/null | grep '^ '
}
for a in x86_64 arm64; do
    tg=$([ $a = arm64 ] && echo aarch64 || echo x86_64)-linux-gnu
    if ! printf '\t.text\n\tnop\n' | clang -target $tg -c -x assembler - -o "$T/probe.o" 2>/dev/null; then
        skip=$((skip+1)); echo "  skip system assembler for $a (clang -target $tg)"; continue
    fi
    for b in a_arith b_funcptr b_varargs2 b_float fb12-21-indirect-call-six-args; do
        f=tests/c/$b.c; [ -f "$f" ] || continue
        "$_BOUND" 20 "$UA" "$f" -c -b lnx/$a -o "$T/c.o" 2>/dev/null || continue
        "$_BOUND" 20 "$UA" "$f" -S -b lnx/$a -o "$T/s.s" 2>/dev/null
        if ! "$_BOUND" 30 clang -target $tg -c "$T/s.s" -o "$T/g.o" 2>"$T/err"; then bad=$((bad+1)); echo "  FAIL $a $b: system as refuses: $(head -1 "$T/err")"; continue; fi
        say "system as $a $b" same "$(cmp -s <(sections "$T/c.o") <(sections "$T/g.o") && echo same || echo differ)"
    done
    "$_BOUND" 20 "$UA" as -b lnx/$a tests/asm/hand-$a.s -o "$T/h1.o" 2>"$T/err" || { bad=$((bad+1)); echo "  FAIL hand-$a: $(head -1 "$T/err")"; continue; }
    "$_BOUND" 30 clang -target $tg -c tests/asm/hand-$a.s -o "$T/h2.o"
    say "hand-written $a = system as" same "$(cmp -s <(sections "$T/h1.o") <(sections "$T/h2.o") && echo same || echo differ)"
done

# nm (R18-4): the same lines as the system nm, on objects of both targets
for a in x86_64 arm64; do
    for b in a_arith b_funcptr; do
        "$_BOUND" 20 "$UA" tests/c/$b.c -c -b lnx/$a -o "$T/n.o" 2>/dev/null
        if nm "$T/n.o" >/dev/null 2>&1; then
            say "nm $a $b = system nm" same "$(cmp -s <("$_BOUND" 20 "$UA" nm "$T/n.o") <(nm "$T/n.o") && echo same || echo differ)"
        else skip=$((skip+1)); echo "  skip nm $a (the system nm does not read ELF)"; fi
        say "objdump -d $a $b = -S" same "$(cmp -s <("$_BOUND" 20 "$UA" objdump -d "$T/n.o") <("$_BOUND" 20 "$UA" tests/c/$b.c -S -b lnx/$a -o "$T/v.s" && cat "$T/v.s") && echo same || echo differ)"
    done
done

# 5. refusals, by name
printf '\t.text\n\tvfmadd231pd\t%%ymm1, %%ymm2, %%ymm3\n' > "$T/bad.s"
say "unknown instruction refused" "not in the subset" "$("$_BOUND" 20 "$UA" as -b lnx/x86_64 "$T/bad.s" -o "$T/bad.o" 2>&1 | grep -o 'not in the subset')"
say "refused input leaves no file" "absent" "$([ -e "$T/bad.o" ] && echo present || echo absent)"
say "-S for Mach-O refused" "Linux targets" "$("$_BOUND" 20 "$UA" tests/c/a_arith.c -S -b osx/arm64 -o "$T/x.s" 2>&1 | grep -o 'Linux targets')"
"$_BOUND" 20 "$UA" tests/c/a_arith.c -S -b lnx/x86_64 -o "$T/tape.t" 2>/dev/null
say "-S -b without .s is the tape" "tape" "$(head -c 200 "$T/tape.t" | grep -q 'unisacc -S' && echo text || echo tape)"
say "as for Mach-O refused" "lnx/x86_64 and lnx/arm64" "$("$_BOUND" 20 "$UA" as -b osx/arm64 tests/asm/hand-arm64.s -o "$T/x.o" 2>&1 | grep -o 'lnx/x86_64 and lnx/arm64')"
"$_BOUND" 20 "$UA" tests/c/a_arith.c -S -o "$T/tape.s" 2>/dev/null
say "bare -S is still the tape" "tape" "$(head -c 200 "$T/tape.s" | grep -q 'unisacc -S' && echo text || echo tape)"
echo
echo "asmtext  ok $ok   wrong $bad   skipped $skip"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ] && { [ "${STRICT:-0}" != 1 ] || [ "$skip" -eq 0 ]; }
