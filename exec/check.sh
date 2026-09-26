#!/bin/sh
# E0 check: build exec.c with cc and with unisacc, T1 the loaded table against
# gen.py's delta_ref on its whole domain, then run every case through both
# builds and ref.py.  Every step is bounded (perl alarm), including the
# programs built here.  UA = a unisacc host binary, default: build it with
# ../tests/build_ref.sh into build/.
set -u
cd "$(dirname "$0")"
B=build; mkdir -p $B/cases
bound() { perl -e 'alarm shift; exec @ARGV' "$@"; }
UA=${UA:-$B/ua_ref}
if [ ! -x "$UA" ]; then
  (cd .. && bound 60 ./tests/build_ref.sh exec/$B/ua_ref.c exec/$B/ua_ref) || exit 1
fi
fail=0
bound 20 python3 toy/gen.py table > $B/toy.tbl || exit 1
cmp -s $B/toy.tbl toy/toy.tbl || { echo "toy.tbl stale: python3 exec/toy/gen.py table > exec/toy/toy.tbl"; fail=1; }
bound 30 cc -std=c99 -Os -w -o $B/exec_cc exec.c || exit 1
bound 30 "$UA" exec.c -o $B/exec_ua || exit 1
for x in cc ua; do
  bound 20 ./$B/exec_$x -dump toy/toy.tbl > $B/dump_$x || { echo "dump $x failed"; exit 1; }
  printf 'T1 %s: ' $x; bound 30 python3 toy/gen.py check $B/dump_$x || fail=1
done
rm -f $B/cases/c*
i=0
mk() { i=$((i+1)); printf '%s' "$1" > $B/cases/c$i; }
for s in '' '1' '1+2' '1*2' '1+2*3' '(1+2)*3' '((((7))))' '1+2+3+4' '2*3*4+5*(6+7)' \
         '(' ')' '1+' '+1' '1**2' '(1+2' '1+2)' '12' 'a' '1 +2' '()' '9*(8+(7*(6+5)))'; do
  mk "$s"
done
mk '1+2
'
p=''; q=''; k=0
while [ $k -lt 200 ]; do p="$p("; q="$q)"; k=$((k+1)); done
mk "${p}1${q}"; mk "${p}1+2${q}*3"; mk "${p}1${q})"; mk "${p}1"; mk "${p}${q}"
s=''; k=0; while [ $k -lt 500 ]; do s="${s}1+"; k=$((k+1)); done
mk "${s}1"; mk "$s"
n=0
for f in $B/cases/c*; do
  bound 10 python3 toy/ref.py $f > $B/r.out 2> $B/r.err; rr=$?
  for x in cc ua; do
    bound 10 ./$B/exec_$x toy/toy.tbl $f > $B/e.out 2> $B/e.err; er=$?
    if [ $er != $rr ] || ! cmp -s $B/e.out $B/r.out || ! cmp -s $B/e.err $B/r.err; then
      echo "DIFF $x $f: exit $er vs $rr: $(cat $B/e.err) | $(cat $B/r.err)"; fail=1
    fi
  done
  n=$((n+1))
done
[ $n -gt 0 ] || { echo "no cases"; exit 1; }
echo "cases $n x 2 builds, fail=$fail"
exit $fail
