# E1 rule inputs and construction

`gen.py --typed OUT.json` reads declarations, expands finite rules and builds
tries. `exec/c/tbl.py` encodes the result; `exec/c/net.py` constructs threshold
weights. The production candidate evaluates those weights at runtime.
`exec/lex/net.py` is an older, separate UNS2/DENSE experiment.

## Inputs

| Input | Meaning |
|---|---|
| `weights/gold/lex.tsv` | byte-class / look-ahead dispatch decision |
| `weights/gold/lexcls.tsv`, `lexword.tsv` | byte classes, whitespace, special-word membership |
| `weights/gold/parse.tsv` token field | token names and numeric order |
| `iterate/kernel/typekw.tsv` | type keyword values |
| `number.tsv`, `literal.tsv` | numeric and literal/comment transitions |
| `ident-byte.tsv`, `ident-stack.tsv` | UCN and balanced-skip transitions |
| `ident-flow.tsv`, `ident-end.tsv` | identifier continuation and ordered word-class actions |
| `entry.tsv` | dispatch action to entry/linked machine |
| `output.tsv`, `spelling.tsv` | output sequences and spelling modes |
| `count-*.tsv` | counter rendering and halt |
| `location-*.tsv` | positioned-input envelope validation and emission |

The shared reader is `exec/finite_rules.py`, also used by the preprocessor.
Transition columns are state, observation set, next state, action tuples.
Ranges are inclusive; `*` means the remaining domain, not priority over
explicit rows. Overlapping explicit rules and incomplete states fail.
Named classes come from declarations. `@` action sequences are expanded at
construction; `["observation", shift]` substitutes the finite input key
shifted by 0..63. Neither operation runs language-specific host callbacks.

`ident-end.tsv` is explicitly ordered: applicable word classes are tried in
file order; `@next` proceeds to the next class. Its final rule emits a token.
Other `@` targets name linked trie entries, child/end transitions or self.
These are construction links, not runtime executor primitives.

## Construction boundary

The generator still implements generic trie construction, longest-prefix
selection, totality/reachability, token-format parameter substitution and
links between declared machines. Token IDs 0..4 (eof/type/id/num/str), the
class names and named registers are interface contracts in the adapter.
Changing the protocol or those contracts can require adapter changes;
this is not a claim that arbitrary lexer designs need no construction code.

Default generation reads no old kernel or reference compiler. The explicit
`--check-declarations` test mode checks the old Python/C declarations; it is
not part of product construction. The TSV reader's `load_table` does not
import gold rules; `load_stage` retains the existing construction adapter.

## Evidence boundary

Migration compared every old/new state observation and complete action
sequence in plain, typed, positions and locations modes. Network checks
separately compare actual integer inference with the encoded transition table.
Byte-stream integration checks positions and malformed location frames.
Those checks establish migration agreement, not full C99 lexer correctness.
The E3 product/signature changes in the working tree are a separate repair;
E1 agreement does not validate them or complete the compiler migration.

## Optional source provenance framing

The ordinary-action entry `SF.start` reads the exact optional resource
`\0library/sourcefacts`. Absent leaves lexer bytes unchanged; present must be
eight little-endian bytes encoding one. The input must then be `USLFACT1\n`,
stage 2, status 0 or 1, policy 1, a little-endian u64 payload length, and exactly
that many payload bytes. The payload is the existing plain or located E2 output.
The entry resets the global input to that payload using SPAN2/SWAP before the
existing lexer, including its locations reader, starts.

The output frames the complete existing E1 output with stage 1 and the same
policy. Status is sticky: the actual SKIPPAREN opening-parenthesis transition
sets it to zero before deleting an attribute/asm construct. Identifier spelling,
comments, and string contents do not set status. No execution primitive is added.
The resource-aware model uses `exec/c/tbl.py` and the shared ordinary-action core;
the historical `exec/lex/tbl.py` adapter targets the older resource-free executor.
`sourcefactscheck.py` generates private models and exercises simulator framing,
default output, loss tracking, located payloads, and malformed resource/input rejection.

Legacy untyped generation retains its old DISPATCH entry and legacy tbl.py compatibility. Typed/positions/locations models enable source-fact framing; --sourcefacts enables it explicitly for untyped simulator tests. Resource-aware models use the generic C table converter, not the legacy executor.
