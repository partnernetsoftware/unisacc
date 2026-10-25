# C99, feature by feature

`corpus` says 214 of 220, and that number is too kind: c-testsuite's
programs are short and they overlap. "Can it compile and run C99" deserves
a denominator that is the *language*, not a sample of it.

So each file here exercises **one** feature of C99 — the ones the standard
added over C89, plus the C89 constructs a compiler has to get right for any
of it to matter. Every file prints something and returns 0; the suite
compiles it with the system `cc` and with `unisacc`, runs both, and
compares. A feature counts as supported when the two agree.

The list is written from the standard's own summary of changes (C99 §6 and
the foreword's list), not from what we happen to support — otherwise the
denominator moves whenever the numerator does.

Naming: `NN_name.c`, where NN groups them:
  0x  lexical and preprocessor
  1x  declarations and types
  2x  initialisers and literals
  3x  statements and control flow
  4x  functions
  5x  the library C99 added
