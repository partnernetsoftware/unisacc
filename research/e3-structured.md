# E3, structured: grammar table + attribute tables + templates + a generic driver

Status: design (2026-09-26). The current E3 (`exec/parse/gen.py`) stays as the
verified reference delta; this is its replacement plan.

## Why

The current E3 is one flat state machine (5,230 states, ~1.3 M entries), grown
construct by construct: each new C form is laid down as new states, with the
reference tape copied into the actions. It matches the reference byte for
byte, but three different kinds of knowledge are fused in it:

1. **grammar** -- what may come next;
2. **attributes** -- types, widths, signedness, conversions;
3. **templates** -- which tape text a construct emits.

Fused, they multiply: a form is re-laid for every type and every context. The
owner's objection -- "you keep enumerating instead of designing a clever
structure" -- is exactly this.

## Structure

The delta is still one finite table run by the same generic executor, but it
is **generated** from three declarative sources:

| source | form | derived from |
|---|---|---|
| grammar | productions of the C99 subset, LL(1) after left factoring; precedence levels from `weights/gold/prec.tsv` | written once as data (`exec/parse2/grammar.txt`) |
| attributes | per operator and operand classes: result type, conversion, instruction | the existing gold stages `type`, `tyinfo`, `pfconv`, `binsel`, `irsel` |
| templates | per production: a tape template with slots (`{lvalue}`, `{push}`, `{rhs}`, `{pop}`, `{store:W}`, `{label:k}`) | measured once from the reference, kept as data (`exec/parse2/templates.tsv`) |

A **generic LL driver** turns the grammar into prediction states: one state
per grammar position (item), observation = (state, token class, stack top).
Recursion is the stack. The attribute tables become lookups the driver
performs at fixed points (after an operand, at an operator), and a template is
a short action sequence emitted at a production's reduction points.

Size then adds instead of multiplying:

    |delta| ~ |grammar items| x |token classes|  +  |attribute tables|  +  |templates|

and a new construct is one production plus one template.

## What the executor needs

Nothing language-specific. The driver is a pushdown automaton over a table;
the attribute lookups are dictionary reads; templates are output actions with
slot fills that are copies from the working store. All already exist in
`exec/pp/sim.py`.

## Plan

1. `exec/parse2/`: grammar for expressions (all C operators, precedence from
   prec.tsv), expression statements, `if/while/for/return`, blocks, int locals
   and parameters, functions. Templates measured for exactly those.
2. `gen2.py` builds the delta from the three sources; same simulator, same
   `compare.py` against the reference.
3. Gate: the E3 probes and corpus files the current E3 covers **for this
   subset** must be equal (0 differ); report states and table size against
   the current E3 on the same inputs.
4. If the structured delta is smaller and extends by adding rows, move the
   remaining constructs (pointers, arrays, structs, doubles, variadics, the
   header prelude) over in the same form, then retire `exec/parse/gen.py`.

## Theory note

Each source is finite declarative data, so each is enumerable and
constructible as a network separately (T1 per table); the driver is fixed and
language-independent. This is the "tables are networks, knowledge lives in
declared data" claim made literal, and it is what Paper A should describe --
not the hand-grown flat machine.
