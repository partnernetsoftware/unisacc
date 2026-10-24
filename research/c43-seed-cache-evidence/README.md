# Seed generator reference cache key repair

The gen producer used a SHA1 prefix over concatenated glob contents, while memory admission uses the measured read-set path/content SHA256 key. On this source the keys were `28a01f2255ab8b0d` and `1aba99c3d52243aa`. A produced reference therefore could not warm admission's expected path.

`tests/seedgencheck.sh` now calls the existing native `tests/seedmemorykey.cx gen`, checks the family and exactly 16 lowercase hex digits, and refuses failure before constructing a reference. The memory policy and constructor recipes are unchanged.

An isolated scratch proof executed the actual new shell key block against the checkout and a private copy of the gen read set. A real E4 model from the current serial C/P sampling populated the cache: the unmodified copy found its file under the admission key. Appending a newline to `exec/finite_rules.py` changed the key and missed that cache; removing the file returned nonzero with empty stdout. See `key-proof.log`.

This proves the cache key contract. It does not claim the full seedgen suites passed or authorize a different memory evidence identity. The observation source remains frozen separately.
