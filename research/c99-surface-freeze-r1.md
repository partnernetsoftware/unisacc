> 快照说明：本冻结清单按 69124962 派生（2026-10-21）。其后的台账与转发改动（64 名台账补齐、llround/remainder/… 转发、strxfrm、wcscoll/wcsxfrm、wcstoll/wcstoull 等）未回填本表；以台账与 include/*.h 当前值为准，本表仅作边界与冻结口径的存档。

# C99 library surface freeze, r1 (measured at main HEAD 69124962, 2026-10-21)

Source of truth: `tests/c99/clauses.tsv` (part = lib) for the name list and the ledger status; `include/*.h` at HEAD for the mechanism, by grep
(`tools/classify.py` in this work dir; rows in `rows.tsv`). No file in the main repo was touched. Nothing was built or run except the grep.

## 0. How a mechanism was derived (grep rules)

- **BODY**: a `static` definition in a non-sealed header, almost always under `#if !__UNISA_FTRIM_LIBC || __UN_<name>`; a `#define` in a non-sealed header counts as BODY too (assert, the math classification macros).
- **FORWARDED**: a prototype with no body in a non-sealed header (`include/fenv.h`, the `#ifndef _WIN32` tails of `include/math.h`, `include/string.h`, `include/wchar.h`, all of `include/wctype.h`): the call goes to the host C library on Linux/macOS, absent on Windows.
- **SEALED-HEADER**: the name lives in a header pinned as a pp-leaf-seal resource (`release/derived-map.tsv` lines 150-156: `include/stdio.h`, `include/stdlib.h`, `include/errno.h`, `include/limits.h`, `include/stdarg.h`, `include/stddef.h`, `include/sys/_exit.h`). Mechanism inside is BODY/prototype as well, but any change needs a reseal, so the row is HOLD.
- **OUT-OF-SCOPE**: not declared in any header under `include/` (whole-word grep confirmed), or named by the task as out of scope (long double, `_Exit` pending reseal).
- `va_arg/va_start/va_end` and `setjmp/longjmp` are compiler intrinsics per the header comments (`include/stdarg.h`, `include/setjmp.h`); they are bucketed by their header (stdarg.h is sealed -> SEALED-HEADER; setjmp.h is not -> BODY) with detail INTRINSIC.

## 1. Manifest: every function/macro name the ledger lists (part = lib)

Flag column: `DECLARED-NOT-LEDGERED` = header declares it, ledger says unsupported ("not provided"); `NO-PROBE` = supported mechanism, ledger partial; `LEDGERED-NOT-DECLARED` = ledger covered but out of scope here.

| clause | name | header | mechanism | ledger status | detail (line) | flag | reason if not covered / note |
|---|---|---|---|---|---|---|---|
| 7.2.1.1 | assert | assert.h | BODY | covered | MACRO 14 |  |  |
| 7.3.5.1 | cacos | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: cacos) |
| 7.3.5.2 | casin | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: casin) |
| 7.3.5.3 | catan | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: catan) |
| 7.3.5.4 | ccos | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: ccos) |
| 7.3.5.5 | csin | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: csin) |
| 7.3.5.6 | ctan | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: ctan) |
| 7.3.6.1 | cacosh | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: cacosh) |
| 7.3.6.2 | casinh | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: casinh) |
| 7.3.6.3 | catanh | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: catanh) |
| 7.3.6.4 | ccosh | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: ccosh) |
| 7.3.6.5 | csinh | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: csinh) |
| 7.3.6.6 | ctanh | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: ctanh) |
| 7.3.7.1 | cexp | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: cexp) |
| 7.3.7.2 | clog | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: clog) |
| 7.3.8.1 | cabs | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: cabs) |
| 7.3.8.2 | cpow | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: cpow) |
| 7.3.8.3 | csqrt | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: csqrt) |
| 7.3.9.1 | carg | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: carg) |
| 7.3.9.2 | cimag | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: cimag) |
| 7.3.9.3 | conj | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: conj) |
| 7.3.9.4 | cproj | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: cproj) |
| 7.3.9.5 | creal | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: creal) |
| 7.4.1.1 | isalnum | ctype.h | BODY | partial | BODY 26 | NO-PROBE | no probe calls it |
| 7.4.1.2 | isalpha | ctype.h | BODY | covered | BODY 23 |  |  |
| 7.4.1.3 | isblank | ctype.h | BODY | partial | BODY 35 | NO-PROBE | no probe calls it |
| 7.4.1.4 | iscntrl | ctype.h | BODY | partial | BODY 38 | NO-PROBE | no probe calls it |
| 7.4.1.5 | isdigit | ctype.h | BODY | covered | BODY 8 |  |  |
| 7.4.1.6 | isgraph | ctype.h | BODY | covered | BODY 44 |  |  |
| 7.4.1.7 | islower | ctype.h | BODY | partial | BODY 17 | NO-PROBE | no probe calls it |
| 7.4.1.8 | isprint | ctype.h | BODY | partial | BODY 41 | NO-PROBE | no probe calls it |
| 7.4.1.9 | ispunct | ctype.h | BODY | covered | BODY 47 |  |  |
| 7.4.1.10 | isspace | ctype.h | BODY | covered | BODY 29 |  |  |
| 7.4.1.11 | isupper | ctype.h | BODY | covered | BODY 20 |  |  |
| 7.4.1.12 | isxdigit | ctype.h | BODY | covered | BODY 11 |  |  |
| 7.4.2.1 | tolower | ctype.h | BODY | covered | BODY 53 |  |  |
| 7.4.2.2 | toupper | ctype.h | BODY | covered | BODY 50 |  |  |
| 7.6.2.1 | feclearexcept | fenv.h | FORWARDED | unsupported | PROTO 34 | DECLARED-NOT-LEDGERED | not provided: feclearexcept |
| 7.6.2.2 | fegetexceptflag | fenv.h | FORWARDED | unsupported | PROTO 35 | DECLARED-NOT-LEDGERED | not provided: fegetexceptflag |
| 7.6.2.3 | feraiseexcept | fenv.h | FORWARDED | unsupported | PROTO 36 | DECLARED-NOT-LEDGERED | not provided: feraiseexcept |
| 7.6.2.4 | fesetexceptflag | fenv.h | FORWARDED | unsupported | PROTO 37 | DECLARED-NOT-LEDGERED | not provided: fesetexceptflag |
| 7.6.2.5 | fetestexcept | fenv.h | FORWARDED | unsupported | PROTO 38 | DECLARED-NOT-LEDGERED | not provided: fetestexcept |
| 7.6.3.1 | fegetround | fenv.h | FORWARDED | unsupported | PROTO 39 | DECLARED-NOT-LEDGERED | not provided: fegetround |
| 7.6.3.2 | fesetround | fenv.h | FORWARDED | unsupported | PROTO 40 | DECLARED-NOT-LEDGERED | not provided: fesetround |
| 7.6.4.1 | fegetenv | fenv.h | FORWARDED | unsupported | PROTO 41 | DECLARED-NOT-LEDGERED | not provided: fegetenv |
| 7.6.4.2 | feholdexcept | fenv.h | FORWARDED | unsupported | PROTO 42 | DECLARED-NOT-LEDGERED | not provided: feholdexcept |
| 7.6.4.3 | fesetenv | fenv.h | FORWARDED | unsupported | PROTO 43 | DECLARED-NOT-LEDGERED | not provided: fesetenv |
| 7.6.4.4 | feupdateenv | fenv.h | FORWARDED | unsupported | PROTO 44 | DECLARED-NOT-LEDGERED | not provided: feupdateenv |
| 7.8.2.1 | imaxabs | inttypes.h | BODY | partial | BODY 48 | NO-PROBE | no probe calls it |
| 7.8.2.2 | imaxdiv | inttypes.h | BODY | covered | BODY 51 |  |  |
| 7.8.2.3 | strtoimax | inttypes.h | BODY | covered | BODY 55 |  |  |
| 7.8.2.3 | strtoumax | inttypes.h | BODY | covered | BODY 58 |  |  |
| 7.8.2.4 | wcstoimax | inttypes.h | BODY | covered | BODY 61 |  |  |
| 7.8.2.4 | wcstoumax | inttypes.h | BODY | covered | BODY 64 |  |  |
| 7.11.1.1 | setlocale | locale.h | BODY | covered | BODY 46 |  |  |
| 7.11.2.1 | localeconv | locale.h | BODY | partial | BODY 62 | NO-PROBE | no probe calls it |
| 7.12.3.1 | fpclassify | math.h | BODY | covered | BODY 108 |  |  |
| 7.12.3.2 | isfinite | math.h | BODY | partial | BODY 84 | NO-PROBE | no probe calls it |
| 7.12.3.3 | isinf | math.h | BODY | covered | BODY 81 |  |  |
| 7.12.3.4 | isnan | math.h | BODY | covered | BODY 78 |  |  |
| 7.12.3.5 | isnormal | math.h | BODY | covered | BODY 117 |  |  |
| 7.12.3.6 | signbit | math.h | BODY | partial | BODY 87 | NO-PROBE | no probe calls it |
| 7.12.4.1 | acos | math.h | BODY | covered | BODY 627 |  |  |
| 7.12.4.2 | asin | math.h | BODY | covered | BODY 621 |  |  |
| 7.12.4.3 | atan | math.h | BODY | covered | BODY 557 |  |  |
| 7.12.4.4 | atan2 | math.h | BODY | covered | BODY 596 |  |  |
| 7.12.4.5 | cos | math.h | BODY | covered | BODY 528 |  |  |
| 7.12.4.6 | sin | math.h | BODY | covered | BODY 513 |  |  |
| 7.12.4.7 | tan | math.h | BODY | covered | BODY 543 |  |  |
| 7.12.5.1 | acosh | math.h | FORWARDED | covered | PROTO 1010 |  |  |
| 7.12.5.2 | asinh | math.h | FORWARDED | covered | PROTO 1011 |  |  |
| 7.12.5.3 | atanh | math.h | FORWARDED | covered | PROTO 1012 |  |  |
| 7.12.5.4 | cosh | math.h | BODY | covered | BODY 742 |  |  |
| 7.12.5.5 | sinh | math.h | BODY | covered | BODY 716 |  |  |
| 7.12.5.6 | tanh | math.h | BODY | covered | BODY 766 |  |  |
| 7.12.6.1 | exp | math.h | BODY | covered | BODY 274 |  |  |
| 7.12.6.2 | exp2 | math.h | BODY | partial | BODY 321 | NO-PROBE | no probe calls it |
| 7.12.6.3 | expm1 | math.h | BODY | partial | BODY 642 | NO-PROBE | no probe calls it |
| 7.12.6.4 | frexp | math.h | BODY | covered | BODY 190 |  |  |
| 7.12.6.5 | ilogb | math.h | BODY | covered | BODY 224 |  |  |
| 7.12.6.6 | ldexp | math.h | BODY | covered | BODY 204 |  |  |
| 7.12.6.7 | log | math.h | BODY | covered | BODY 326 |  |  |
| 7.12.6.8 | log10 | math.h | BODY | covered | BODY 379 |  |  |
| 7.12.6.9 | log1p | math.h | BODY | partial | BODY 409 | NO-PROBE | no probe calls it |
| 7.12.6.10 | log2 | math.h | BODY | partial | BODY 403 | NO-PROBE | no probe calls it |
| 7.12.6.11 | logb | math.h | BODY | covered | BODY 234 |  |  |
| 7.12.6.12 | modf | math.h | BODY | partial | BODY 179 | NO-PROBE | no probe calls it |
| 7.12.6.13 | scalbn | math.h | BODY | partial | BODY 214 | NO-PROBE | missing: scalbln; no probe calls it |
| 7.12.6.13 | scalbln | math.h | BODY | partial | BODY 217 | NO-PROBE | missing: scalbln; no probe calls it |
| 7.12.7.1 | cbrt | math.h | BODY | partial | BODY 957 | NO-PROBE | no probe calls it |
| 7.12.7.2 | fabs | math.h | BODY | covered | BODY 66 |  |  |
| 7.12.7.3 | hypot | math.h | BODY | covered | BODY 946 |  |  |
| 7.12.7.4 | pow | math.h | BODY | covered | BODY 788 |  |  |
| 7.12.7.5 | sqrt | math.h | BODY | covered | BODY 59 |  |  |
| 7.12.8.1 | erf | math.h | FORWARDED | covered | PROTO 1013 |  |  |
| 7.12.8.2 | erfc | math.h | FORWARDED | covered | PROTO 1014 |  |  |
| 7.12.8.3 | lgamma | math.h | FORWARDED | covered | PROTO 1015 |  |  |
| 7.12.8.4 | tgamma | math.h | FORWARDED | covered | PROTO 1016 |  |  |
| 7.12.9.1 | ceil | math.h | BODY | covered | BODY 139 |  |  |
| 7.12.9.2 | floor | math.h | BODY | covered | BODY 130 |  |  |
| 7.12.9.3 | nearbyint | math.h | FORWARDED | covered | PROTO 1008 |  |  |
| 7.12.9.4 | rint | math.h | BODY | covered | BODY 160 |  |  |
| 7.12.9.5 | lrint | math.h | BODY | covered | BODY 169 |  |  |
| 7.12.9.5 | llrint | math.h | FORWARDED | covered | PROTO 1005 |  |  |
| 7.12.9.6 | round | math.h | BODY | covered | BODY 148 |  |  |
| 7.12.9.7 | lround | math.h | BODY | covered | BODY 157 |  |  |
| 7.12.9.7 | llround | math.h | FORWARDED | covered | PROTO 1004 |  |  |
| 7.12.9.8 | trunc | math.h | BODY | covered | BODY 122 |  |  |
| 7.12.10.1 | fmod | math.h | BODY | covered | BODY 252 |  |  |
| 7.12.10.2 | remainder | math.h | FORWARDED | covered | PROTO 1006 |  |  |
| 7.12.10.3 | remquo | math.h | FORWARDED | covered | PROTO 1007 |  |  |
| 7.12.11.1 | copysign | math.h | BODY | partial | BODY 72 | NO-PROBE | no probe calls it |
| 7.12.11.2 | nan | math.h | BODY | covered | BODY 978 |  |  |
| 7.12.11.3 | nextafter | math.h | BODY | covered | BODY 981 |  |  |
| 7.12.11.4 | nexttoward | math.h | BODY | covered | BODY 999 |  |  |
| 7.12.12.1 | fdim | math.h | BODY | partial | BODY 974 | NO-PROBE | no probe calls it |
| 7.12.12.2 | fmax | math.h | BODY | partial | BODY 968 | NO-PROBE | no probe calls it |
| 7.12.12.3 | fmin | math.h | BODY | covered | BODY 971 |  |  |
| 7.12.13.1 | fma | math.h | FORWARDED | covered | PROTO 1009 |  |  |
| 7.12.14.1 | isgreater | math.h | BODY | unsupported | BODY 90 | DECLARED-NOT-LEDGERED | not provided: isgreater |
| 7.12.14.2 | isgreaterequal | math.h | BODY | unsupported | BODY 93 | DECLARED-NOT-LEDGERED | not provided: isgreaterequal |
| 7.12.14.3 | isless | math.h | BODY | unsupported | BODY 96 | DECLARED-NOT-LEDGERED | not provided: isless |
| 7.12.14.4 | islessequal | math.h | BODY | unsupported | BODY 99 | DECLARED-NOT-LEDGERED | not provided: islessequal |
| 7.12.14.5 | islessgreater | math.h | BODY | unsupported | BODY 102 | DECLARED-NOT-LEDGERED | not provided: islessgreater |
| 7.12.14.6 | isunordered | math.h | BODY | unsupported | BODY 105 | DECLARED-NOT-LEDGERED | not provided: isunordered |
| 7.13.1.1 | setjmp | setjmp.h | BODY | unsupported | INTRINSIC | DECLARED-NOT-LEDGERED | not provided: setjmp |
| 7.13.2.1 | longjmp | setjmp.h | BODY | unsupported | INTRINSIC | DECLARED-NOT-LEDGERED | not provided: longjmp |
| 7.14.1.1 | signal | signal.h | BODY | covered | BODY 29 |  |  |
| 7.14.2.1 | raise | signal.h | BODY | covered | BODY 39 |  |  |
| 7.15.1.1 | va_arg | stdarg.h | SEALED-HEADER | covered | INTRINSIC |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.15.1.2 | va_copy | stdarg.h | SEALED-HEADER | covered | MACRO 10 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.15.1.3 | va_end | stdarg.h | SEALED-HEADER | covered | INTRINSIC |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.15.1.4 | va_start | stdarg.h | SEALED-HEADER | covered | INTRINSIC |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.4.1 | remove | stdio.h | SEALED-HEADER | covered | BODY 589 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.4.2 | rename | stdio.h | SEALED-HEADER | covered | BODY 592 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.4.3 | tmpfile | stdio.h | SEALED-HEADER | partial | BODY 405 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.4.4 | tmpnam | stdio.h | SEALED-HEADER | covered | BODY 383 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.5.1 | fclose | stdio.h | SEALED-HEADER | covered | BODY 516 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.5.2 | fflush | stdio.h | SEALED-HEADER | partial | BODY 597 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.5.3 | fopen | stdio.h | SEALED-HEADER | covered | BODY 281 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.5.4 | freopen | stdio.h | SEALED-HEADER | covered | BODY 465 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.5.5 | setbuf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: setbuf) |
| 7.19.5.6 | setvbuf | stdio.h | SEALED-HEADER | partial | BODY 216 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.1 | fprintf | stdio.h | SEALED-HEADER | covered | BODY 1075 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.2 | fscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: fscanf) |
| 7.19.6.3 | printf | stdio.h | SEALED-HEADER | covered | BODY-unguarded 38 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.4 | scanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: scanf) |
| 7.19.6.5 | snprintf | stdio.h | SEALED-HEADER | covered | BODY 1066 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.6 | sprintf | stdio.h | SEALED-HEADER | covered | BODY 1057 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.7 | sscanf | stdio.h | SEALED-HEADER | covered | BODY 1217 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.8 | vfprintf | stdio.h | SEALED-HEADER | partial | BODY 1052 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.9 | vfscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: vfscanf) |
| 7.19.6.10 | vprintf | stdio.h | SEALED-HEADER | partial | BODY 372 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.11 | vscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: vscanf) |
| 7.19.6.12 | vsnprintf | stdio.h | SEALED-HEADER | partial | BODY 1047 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.13 | vsprintf | stdio.h | SEALED-HEADER | partial | BODY 1042 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.6.14 | vsscanf | stdio.h | SEALED-HEADER | partial | BODY 1116 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.1 | fgetc | stdio.h | SEALED-HEADER | covered | BODY 339 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.2 | fgets | stdio.h | SEALED-HEADER | covered | BODY 442 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.3 | fputc | stdio.h | SEALED-HEADER | covered | BODY 114 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.4 | fputs | stdio.h | SEALED-HEADER | covered | BODY 126 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.5 | getc | stdio.h | SEALED-HEADER | partial | BODY 365 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.6 | getchar | stdio.h | SEALED-HEADER | partial | BODY 368 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.7 | gets | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: gets) |
| 7.19.7.8 | putc | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: putc) |
| 7.19.7.9 | putchar | stdio.h | SEALED-HEADER | covered | BODY 122 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.10 | puts | stdio.h | SEALED-HEADER | covered | BODY 133 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.7.11 | ungetc | stdio.h | SEALED-HEADER | partial | BODY 198 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.8.1 | fread | stdio.h | SEALED-HEADER | covered | BODY 224 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.8.2 | fwrite | stdio.h | SEALED-HEADER | covered | BODY 164 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.9.1 | fgetpos | stdio.h | SEALED-HEADER | unsupported | BODY 571 | DECLARED-NOT-LEDGERED | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.9.2 | fseek | stdio.h | SEALED-HEADER | covered | BODY 539 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.9.3 | fsetpos | stdio.h | SEALED-HEADER | unsupported | BODY 574 | DECLARED-NOT-LEDGERED | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.9.4 | ftell | stdio.h | SEALED-HEADER | covered | BODY 550 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.9.5 | rewind | stdio.h | SEALED-HEADER | covered | BODY 586 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.10.1 | clearerr | stdio.h | SEALED-HEADER | partial | BODY 189 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.10.2 | feof | stdio.h | SEALED-HEADER | partial | BODY 175 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.10.3 | ferror | stdio.h | SEALED-HEADER | partial | BODY 182 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.19.10.4 | perror | stdio.h | SEALED-HEADER | covered | BODY 1247 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.1 | atof | stdlib.h | SEALED-HEADER | partial | BODY 563 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.2 | atoi | stdlib.h | SEALED-HEADER | covered | BODY 144 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.2 | atol | stdlib.h | SEALED-HEADER | covered | BODY 235 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.2 | atoll | stdlib.h | SEALED-HEADER | covered | BODY 251 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.3 | strtod | stdlib.h | SEALED-HEADER | covered | BODY 451 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.3 | strtof | stdlib.h | SEALED-HEADER | covered | BODY 557 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.3 | strtold | stdlib.h | OUT-OF-SCOPE | covered | BODY 560 | LEDGERED-NOT-DECLARED | long double: body returns double (include/stdlib.h aliases long double to double) |
| 7.20.1.4 | strtol | stdlib.h | SEALED-HEADER | covered | BODY 209 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.4 | strtoll | stdlib.h | SEALED-HEADER | covered | BODY 241 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.4 | strtoul | stdlib.h | SEALED-HEADER | covered | BODY 225 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.1.4 | strtoull | stdlib.h | SEALED-HEADER | covered | BODY 246 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.2.1 | rand | stdlib.h | SEALED-HEADER | partial | BODY 568 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.2.2 | srand | stdlib.h | SEALED-HEADER | partial | BODY 574 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.3.1 | calloc | stdlib.h | SEALED-HEADER | covered | BODY 105 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.3.2 | free | stdlib.h | SEALED-HEADER | covered | BODY 84 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.3.3 | malloc | stdlib.h | SEALED-HEADER | covered | BODY 48 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.3.4 | realloc | stdlib.h | SEALED-HEADER | covered | BODY 121 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.4.1 | abort | stdlib.h | SEALED-HEADER | partial | BODY 101 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.4.2 | atexit | stdlib.h | SEALED-HEADER | covered | BODY 656 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.4.3 | exit | sys/_exit.h | SEALED-HEADER | covered | BODY-unguarded 7 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.4.4 | _Exit | - | OUT-OF-SCOPE | n/a | - |  | not declared; pending reseal of <stdlib.h>/<sys/_exit.h> (sealed headers) |
| 7.20.4.5 | getenv | stdlib.h | SEALED-HEADER | covered | BODY 707 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.4.6 | system | stdlib.h | SEALED-HEADER | covered | BODY 762 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.5.1 | bsearch | stdlib.h | SEALED-HEADER | covered | BODY 639 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.5.2 | qsort | stdlib.h | SEALED-HEADER | covered | BODY 623 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.6.1 | abs | stdlib.h | SEALED-HEADER | covered | BODY 140 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.6.1 | labs | stdlib.h | SEALED-HEADER | covered | BODY 581 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.6.1 | llabs | stdlib.h | SEALED-HEADER | covered | BODY 584 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.6.2 | div | stdlib.h | SEALED-HEADER | partial | BODY 590 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.6.2 | ldiv | stdlib.h | SEALED-HEADER | partial | BODY 593 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.6.2 | lldiv | stdlib.h | SEALED-HEADER | partial | BODY 596 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.7.1 | mblen | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: mblen) |
| 7.20.7.2 | mbtowc | stdlib.h | SEALED-HEADER | covered | BODY 818 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.7.3 | wctomb | stdlib.h | SEALED-HEADER | covered | BODY 830 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.8.1 | mbstowcs | stdlib.h | SEALED-HEADER | covered | BODY 838 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.20.8.2 | wcstombs | stdlib.h | SEALED-HEADER | covered | BODY 852 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.21.2.1 | memcpy | string.h | BODY | covered | BODY 122 |  |  |
| 7.21.2.2 | memmove | string.h | BODY | covered | BODY 137 |  |  |
| 7.21.2.3 | strcpy | string.h | BODY | covered | BODY 20 |  |  |
| 7.21.2.4 | strncpy | string.h | BODY | partial | BODY 30 | NO-PROBE | no probe calls it |
| 7.21.3.1 | strcat | string.h | BODY | covered | BODY 40 |  |  |
| 7.21.3.2 | strncat | string.h | BODY | covered | BODY 159 |  |  |
| 7.21.4.1 | memcmp | string.h | BODY | covered | BODY 148 |  |  |
| 7.21.4.2 | strcmp | string.h | BODY | covered | BODY 47 |  |  |
| 7.21.4.3 | strcoll | string.h | BODY | covered | BODY 261 |  |  |
| 7.21.4.4 | strncmp | string.h | BODY | covered | BODY 56 |  |  |
| 7.21.4.5 | strxfrm | string.h | FORWARDED | covered | PROTO 273 |  |  |
| 7.21.5.1 | memchr | string.h | BODY | covered | BODY 198 |  |  |
| 7.21.5.2 | strchr | string.h | BODY | covered | BODY 69 |  |  |
| 7.21.5.3 | strcspn | string.h | BODY | covered | BODY 244 |  |  |
| 7.21.5.4 | strpbrk | string.h | BODY | covered | BODY 228 |  |  |
| 7.21.5.5 | strrchr | string.h | BODY | covered | BODY 81 |  |  |
| 7.21.5.6 | strspn | string.h | BODY | covered | BODY 211 |  |  |
| 7.21.5.7 | strstr | string.h | BODY | covered | BODY 94 |  |  |
| 7.21.5.8 | strtok | string.h | BODY | covered | BODY 173 |  |  |
| 7.21.6.1 | memset | string.h | BODY | covered | BODY 108 |  |  |
| 7.21.6.2 | strerror | stdio.h | SEALED-HEADER | covered | BODY 1229 |  | HOLD: pp-leaf-seal (release/derived-map.tsv) |
| 7.21.6.3 | strlen | string.h | BODY | covered | BODY 11 |  |  |
| 7.23.2.1 | clock | time.h | BODY | covered | BODY 332 |  |  |
| 7.23.2.2 | difftime | time.h | BODY | covered | BODY 326 |  |  |
| 7.23.2.3 | mktime | time.h | BODY | unsupported | BODY 240 | DECLARED-NOT-LEDGERED | not provided: mktime |
| 7.23.2.4 | time | time.h | BODY | covered | BODY 82 |  |  |
| 7.23.3.1 | asctime | time.h | BODY | unsupported | BODY 320 | DECLARED-NOT-LEDGERED | not provided: asctime |
| 7.23.3.2 | ctime | time.h | BODY | unsupported | BODY 323 | DECLARED-NOT-LEDGERED | not provided: ctime |
| 7.23.3.3 | gmtime | time.h | BODY | unsupported | BODY 157 | DECLARED-NOT-LEDGERED | not provided: gmtime |
| 7.23.3.4 | localtime | time.h | BODY | unsupported | BODY 237 | DECLARED-NOT-LEDGERED | not provided: localtime |
| 7.23.3.5 | strftime | time.h | BODY | unsupported | BODY 267 | DECLARED-NOT-LEDGERED | not provided: strftime |
| 7.24.2.1 | fwprintf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: fwprintf) |
| 7.24.2.2 | fwscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: fwscanf) |
| 7.24.2.3 | swprintf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: swprintf) |
| 7.24.2.4 | swscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: swscanf) |
| 7.24.2.5 | vfwprintf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: vfwprintf) |
| 7.24.2.6 | vfwscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: vfwscanf) |
| 7.24.2.7 | vswprintf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: vswprintf) |
| 7.24.2.8 | vswscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: vswscanf) |
| 7.24.2.9 | vwprintf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: vwprintf) |
| 7.24.2.10 | vwscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: vwscanf) |
| 7.24.2.11 | wprintf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: wprintf) |
| 7.24.2.12 | wscanf | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: wscanf) |
| 7.24.3.1 | fgetwc | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: fgetwc) |
| 7.24.3.2 | fgetws | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: fgetws) |
| 7.24.3.3 | fputwc | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: fputwc) |
| 7.24.3.4 | fputws | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: fputws) |
| 7.24.3.5 | fwide | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: fwide) |
| 7.24.3.6 | getwc | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: getwc) |
| 7.24.3.7 | getwchar | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: getwchar) |
| 7.24.3.8 | putwc | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: putwc) |
| 7.24.3.9 | putwchar | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: putwchar) |
| 7.24.3.10 | ungetwc | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: ungetwc) |
| 7.24.4.1.1 | wcstod | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: wcstod wcstof wcstold) |
| 7.24.4.1.1 | wcstof | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: wcstod wcstof wcstold) |
| 7.24.4.1.1 | wcstold | - | OUT-OF-SCOPE | unsupported | - |  | long double; not declared |
| 7.24.4.1.2 | wcstol | wchar.h | BODY | unsupported | BODY 318 | DECLARED-NOT-LEDGERED | not provided: wcstol wcstoll wcstoul wcstoull |
| 7.24.4.1.2 | wcstoll | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: wcstol wcstoll wcstoul wcstoull) |
| 7.24.4.1.2 | wcstoul | wchar.h | BODY | unsupported | BODY 334 | DECLARED-NOT-LEDGERED | not provided: wcstol wcstoll wcstoul wcstoull |
| 7.24.4.1.2 | wcstoull | - | OUT-OF-SCOPE | unsupported | - |  | not declared in include/ (ledger: not provided: wcstol wcstoll wcstoul wcstoull) |
| 7.24.4.2.1 | wcscpy | wchar.h | BODY | unsupported | BODY 95 | DECLARED-NOT-LEDGERED | not provided: wcscpy |
| 7.24.4.2.2 | wcsncpy | wchar.h | BODY | unsupported | BODY 104 | DECLARED-NOT-LEDGERED | not provided: wcsncpy |
| 7.24.4.2.3 | wmemcpy | wchar.h | BODY | unsupported | BODY 43 | DECLARED-NOT-LEDGERED | not provided: wmemcpy |
| 7.24.4.2.4 | wmemmove | wchar.h | BODY | unsupported | BODY 51 | DECLARED-NOT-LEDGERED | not provided: wmemmove |
| 7.24.4.3.1 | wcscat | wchar.h | BODY | unsupported | BODY 113 | DECLARED-NOT-LEDGERED | not provided: wcscat |
| 7.24.4.3.2 | wcsncat | wchar.h | BODY | unsupported | BODY 123 | DECLARED-NOT-LEDGERED | not provided: wcsncat |
| 7.24.4.4.1 | wcscmp | wchar.h | BODY | unsupported | BODY 133 | DECLARED-NOT-LEDGERED | not provided: wcscmp |
| 7.24.4.4.2 | wcscoll | wchar.h | FORWARDED | covered | PROTO 466 |  |  |
| 7.24.4.4.3 | wcsncmp | wchar.h | BODY | unsupported | BODY 145 | DECLARED-NOT-LEDGERED | not provided: wcsncmp |
| 7.24.4.4.4 | wcsxfrm | wchar.h | FORWARDED | covered | PROTO 467 |  |  |
| 7.24.4.4.5 | wmemcmp | wchar.h | BODY | unsupported | BODY 64 | DECLARED-NOT-LEDGERED | not provided: wmemcmp |
| 7.24.4.5.1 | wcschr | wchar.h | BODY | unsupported | BODY 160 | DECLARED-NOT-LEDGERED | not provided: wcschr |
| 7.24.4.5.2 | wcscspn | wchar.h | BODY | unsupported | BODY 210 | DECLARED-NOT-LEDGERED | not provided: wcscspn |
| 7.24.4.5.3 | wcspbrk | wchar.h | BODY | unsupported | BODY 225 | DECLARED-NOT-LEDGERED | not provided: wcspbrk |
| 7.24.4.5.4 | wcsrchr | wchar.h | BODY | unsupported | BODY 171 | DECLARED-NOT-LEDGERED | not provided: wcsrchr |
| 7.24.4.5.5 | wcsspn | wchar.h | BODY | unsupported | BODY 197 | DECLARED-NOT-LEDGERED | not provided: wcsspn |
| 7.24.4.5.6 | wcsstr | wchar.h | BODY | unsupported | BODY 184 | DECLARED-NOT-LEDGERED | not provided: wcsstr |
| 7.24.4.5.7 | wcstok | wchar.h | BODY | covered | BODY 247 |  |  |
| 7.24.4.5.8 | wmemchr | wchar.h | BODY | unsupported | BODY 76 | DECLARED-NOT-LEDGERED | not provided: wmemchr |
| 7.24.4.6.1 | wcslen | wchar.h | BODY | covered | BODY 30 |  |  |
| 7.24.4.6.2 | wmemset | wchar.h | BODY | unsupported | BODY 87 | DECLARED-NOT-LEDGERED | not provided: wmemset |
| 7.24.5.1 | wcsftime | wchar.h | BODY | covered | BODY 354 |  |  |
| 7.24.6.1.1 | btowc | wchar.h | BODY | covered | BODY 388 |  |  |
| 7.24.6.1.2 | wctob | wchar.h | BODY | covered | BODY 394 |  |  |
| 7.24.6.2.1 | mbsinit | wchar.h | BODY | covered | BODY 383 |  |  |
| 7.24.6.3.1 | mbrlen | wchar.h | BODY | covered | BODY 411 |  |  |
| 7.24.6.3.2 | mbrtowc | wchar.h | BODY | covered | BODY 400 |  |  |
| 7.24.6.3.3 | wcrtomb | wchar.h | BODY | covered | BODY 421 |  |  |
| 7.24.6.4.1 | mbsrtowcs | wchar.h | BODY | covered | BODY 435 |  |  |
| 7.24.6.4.2 | wcsrtombs | wchar.h | BODY | covered | BODY 449 |  |  |
| 7.25.2.1.1 | iswalnum | wctype.h | FORWARDED | unsupported | PROTO 13 | DECLARED-NOT-LEDGERED | not provided: iswalnum |
| 7.25.2.1.2 | iswalpha | wctype.h | FORWARDED | unsupported | PROTO 14 | DECLARED-NOT-LEDGERED | not provided: iswalpha |
| 7.25.2.1.3 | iswblank | wctype.h | FORWARDED | unsupported | PROTO 15 | DECLARED-NOT-LEDGERED | not provided: iswblank |
| 7.25.2.1.4 | iswcntrl | wctype.h | FORWARDED | unsupported | PROTO 16 | DECLARED-NOT-LEDGERED | not provided: iswcntrl |
| 7.25.2.1.5 | iswdigit | wctype.h | FORWARDED | unsupported | PROTO 17 | DECLARED-NOT-LEDGERED | not provided: iswdigit |
| 7.25.2.1.6 | iswgraph | wctype.h | FORWARDED | unsupported | PROTO 18 | DECLARED-NOT-LEDGERED | not provided: iswgraph |
| 7.25.2.1.7 | iswlower | wctype.h | FORWARDED | unsupported | PROTO 19 | DECLARED-NOT-LEDGERED | not provided: iswlower |
| 7.25.2.1.8 | iswprint | wctype.h | FORWARDED | unsupported | PROTO 20 | DECLARED-NOT-LEDGERED | not provided: iswprint |
| 7.25.2.1.9 | iswpunct | wctype.h | FORWARDED | unsupported | PROTO 21 | DECLARED-NOT-LEDGERED | not provided: iswpunct |
| 7.25.2.1.10 | iswspace | wctype.h | FORWARDED | unsupported | PROTO 22 | DECLARED-NOT-LEDGERED | not provided: iswspace |
| 7.25.2.1.11 | iswupper | wctype.h | FORWARDED | unsupported | PROTO 23 | DECLARED-NOT-LEDGERED | not provided: iswupper |
| 7.25.2.1.12 | iswxdigit | wctype.h | FORWARDED | unsupported | PROTO 24 | DECLARED-NOT-LEDGERED | not provided: iswxdigit |
| 7.25.2.2.1 | iswctype | wctype.h | FORWARDED | unsupported | PROTO 28 | DECLARED-NOT-LEDGERED | not provided: iswctype |
| 7.25.2.2.2 | wctype | wctype.h | FORWARDED | unsupported | PROTO 27 | DECLARED-NOT-LEDGERED | not provided: wctype |
| 7.25.3.1.1 | towlower | wctype.h | FORWARDED | unsupported | PROTO 25 | DECLARED-NOT-LEDGERED | not provided: towlower |
| 7.25.3.1.2 | towupper | wctype.h | FORWARDED | unsupported | PROTO 26 | DECLARED-NOT-LEDGERED | not provided: towupper |
| 7.25.3.2.1 | towctrans | wctype.h | FORWARDED | unsupported | PROTO 30 | DECLARED-NOT-LEDGERED | not provided: towctrans |
| 7.25.3.2.2 | wctrans | wctype.h | FORWARDED | unsupported | PROTO 29 | DECLARED-NOT-LEDGERED | not provided: wctrans |

## 2. Counts (all measured)

| mechanism | names | covered | partial | unsupported | n/a |
|---|---|---|---|---|---|
| FORWARDED | 45 | 16 | 0 | 29 | 0 |
| BODY | 142 | 88 | 21 | 33 | 0 |
| SEALED-HEADER | 77 | 54 | 21 | 2 | 0 |
| OUT-OF-SCOPE | 59 | 1 | 0 | 57 | 1 |
| **total names** | **323** | 159 | 42 | 121 | 1 |

Reconciliation with the ledger: the ledger has **452** lib rows (covered 175, partial 40, unsupported 146, n/a 91; measured with awk). **302** of them name a function or macro ("The X function(s)/macro") and yield **323** names
(rows that name several, e.g. 7.20.1.4 strtol/strtoll/strtoul/strtoull, give one manifest row per name: 302 rows -> 323 names, +21). The other **150** ledger rows carry no name: n/a 90, covered 29, unsupported 30, partial 1.
They are clause headings (`n/a`, "heading or definitions"), header-level rows (`<ctype.h>` etc., 29 covered + 7 unsupported + 1 n/a for iso646), 23 "header not provided" headings under complex/fenv/setjmp/tgmath/wctype, and 7.12.2 FP_CONTRACT (partial, pragma accepted and ignored).
So 452 = 302 named + 150 unnamed, and 323 names = 302 named rows + 21 extra names from multi-name rows. The difference between "names" and "rows" is entirely that multi-name expansion; nothing is missing.

Flags: DECLARED-NOT-LEDGERED 64, LEDGERED-NOT-DECLARED 1, NO-PROBE 21.

## 3. Freeze criterion

- The **supported finite surface at HEAD 69124962 is the FORWARDED (45) + BODY (142) rows = 187 names**, of which 125 are ledgered covered/partial and 62 are declared in a header but still ledgered unsupported (fenv.h 11, wctype.h 18, math.h comparison macros 6, setjmp.h 2, time.h 6, wchar.h 19: the ledger notes "not provided" are stale against the headers).
  Freeze rule for those 62: they are **in the header surface and frozen as declared**, but they do not enter the supported set until each has a probe and its ledger row is flipped; until then they are HOLD-UNLEDGERED, not covered.
- **SEALED-HEADER (77 names)** in stdio.h, stdlib.h, errno.h, limits.h, stdarg.h, stddef.h, sys/_exit.h are frozen exactly as sealed; any change (including adding `_Exit`, `setbuf`, `fscanf`/`scanf`, `gets`, `putc`, `mblen`) requires a reseal contract (pp-leaf-seal in `release/derived-map.tsv`).
- **OUT-OF-SCOPE (59 names)** stay out: complex.h 22 (decision 0.0.16 R16-2: not doing), the 7 stdio names above, the 27 wchar.h names (wide stdio 22 + wcstod/wcstof/wcstold + wcstoll/wcstoull), mblen, strtold (long double is double here), `_Exit` (pending reseal). Adding any of them is a separate decision, not a patch.
- **No new function enters the frozen set without (a) a probe under a gated location and (b) a ledger row with status covered**, checked by `tests/c99ledger.py`. The 21 NO-PROBE names (BODY/FORWARDED with ledger partial) are in the surface but not in the evidence; they are the cheapest gain.

## 4. C99 headers: fully covered (every ledgered name has an in-product mechanism) vs not

| header | present | names | mechanisms | fully covered? | reason |
|---|---|---|---|---|---|
| assert.h | yes | 1 | BODY 1 | yes | assert macro BODY; macro-only header |
| complex.h | no | 22 | OUT-OF-SCOPE 22 | NO | header absent; decision R16-2 not to provide |
| ctype.h | yes | 14 | BODY 14 | yes | all 14 BODY; 5 lack a probe |
| errno.h | yes | 0 | - | yes | sealed; macros only, no functions ledgered |
| fenv.h | yes | 11 | FORWARDED 11 | yes | all 11 FORWARDED but ledger says not provided and no probe includes <fenv.h> |
| float.h | yes | 0 | - | yes | macros only |
| inttypes.h | yes | 6 | BODY 6 | yes | all 6 BODY (imaxabs no probe) |
| iso646.h | yes | 0 | - | yes | macros only |
| limits.h | yes | 0 | - | yes | sealed; macros only |
| locale.h | yes | 2 | BODY 2 | yes | setlocale, localeconv BODY (C locale only; localeconv no probe) |
| math.h | yes | 69 | BODY 56, FORWARDED 13 | yes | 56 BODY + 13 FORWARDED; 13 no probe; 6 comparison macros declared but ledgered unsupported |
| setjmp.h | yes | 2 | BODY 2 | yes | intrinsics with prototypes; probe tests/c/a_setjmp_nested.c exists but ledger says unsupported |
| signal.h | yes | 2 | BODY 2 | yes | signal, raise BODY (in-process only) |
| stdarg.h | yes | 4 | SEALED-HEADER 4 | yes | sealed; va_* intrinsics |
| stdbool.h | yes | 0 | - | yes | macros only |
| stddef.h | yes | 0 | - | yes | sealed; types/macros only |
| stdint.h | yes | 0 | - | yes | types/macros only |
| stdio.h | yes | 46 | OUT-OF-SCOPE 7, SEALED-HEADER 39 | NO | sealed; 7 names absent (setbuf fscanf scanf vfscanf vscanf gets putc); fgetpos/fsetpos declared but ledgered unsupported |
| stdlib.h | yes | 36 | OUT-OF-SCOPE 3, SEALED-HEADER 33 | NO | sealed; mblen absent, strtold is double, _Exit absent (pending reseal) |
| string.h | yes | 22 | BODY 20, FORWARDED 1, SEALED-HEADER 1 | yes | 20 BODY + strxfrm FORWARDED; strerror is SEALED (its body is in include/stdio.h:1229); strncpy no probe |
| tgmath.h | no | 0 | - | NO | header absent |
| time.h | yes | 9 | BODY 9 | yes | all 9 BODY, but mktime/asctime/ctime/gmtime/localtime/strftime ledgered unsupported |
| wchar.h | yes | 59 | BODY 30, FORWARDED 2, OUT-OF-SCOPE 27 | NO | 30 BODY + 2 FORWARDED; 27 absent (wide stdio, wcstod family, wcstoll/wcstoull); 21 BODY ledgered unsupported |
| wctype.h | yes | 18 | FORWARDED 18 | yes | all 18 FORWARDED but ledger says not provided; no probe includes <wctype.h> |

Fully covered (19): assert.h, ctype.h, errno.h, fenv.h, float.h, inttypes.h, iso646.h, limits.h, locale.h, math.h, setjmp.h, signal.h, stdarg.h, stdbool.h, stddef.h, stdint.h, string.h, time.h, wctype.h. Not fully covered (5): complex.h, stdio.h, stdlib.h, tgmath.h, wchar.h.

## 5. Ledger-vs-header discrepancies found by the grep (for the ledger owner; not changed here)

- 64 names `DECLARED-NOT-LEDGERED`: ledger notes say "not provided" / "<fenv.h> not provided" / "<wctype.h> not provided" / "<setjmp.h> not provided" while the headers at HEAD declare them (fenv.h 0.0.30 H3, wctype.h 0.0.30 H3, setjmp.h intrinsics, time.h 0.0.19 broken-down time, wchar.h wide string bodies, math.h isgreater..isunordered, stdio.h fgetpos/fsetpos).
- 7.12.6.13 note "missing: scalbln" and 7.20.6.2 note "missing: lldiv" are stale: both have guarded bodies (math.h:216, stdlib.h:595).
- 7.20.1.3 strtold is ledgered covered via tests/c99/56_strtod_family.c but the body returns double; it is frozen OUT-OF-SCOPE as a long double function (the probe measures the double path).

## Verdict

FROZEN-MANIFEST-READY (measured): 323 names from 302 named ledger rows (452 lib rows, 150 unnamed headings/header-level). FORWARDED 45, BODY 142, SEALED-HEADER 77 (HOLD), OUT-OF-SCOPE 59.
Supported finite surface = 187 names (FORWARDED+BODY); of these 125 are ledgered covered/partial and 62 are declared but ledgered unsupported (HOLD-UNLEDGERED until probe + ledger row). 21 supported names have no probe.
Headers fully covered: 19 of 24; not: complex.h, stdio.h, stdlib.h, tgmath.h, wchar.h.
