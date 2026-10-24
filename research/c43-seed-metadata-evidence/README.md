# Seed construction metadata cold-plan repair

Actual Linux x86_64 observations in an isolated archive export. Product bytes remain SHA256 4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda.

The original K2b fixture failed its unchanged ten-second gate-plan timeout while compiling the full seed construction CLI in a new private path. The new gate-plan reader imports only the metadata parser and generated constants. The 45 part/manual rows live in seedconstruct-parts.tsv; the full CLI consumes the same generated constants. The seed-metadata contract gate checks the raw source digest, every ordered row and byte-identical regeneration of the constants module.

plan-proof.log records a cold complete plan in a newly created path within 4.82 seconds, under the original ten-second bound. After removing only the added seed-metadata job and normalizing the fixture ROOT prefix, all original NUL-delimited job names and arguments match the saved prior candidate plan byte-for-byte. Both captures use C.UTF-8. Appending one newline to the source data in a private fixture makes gate --plan return 2.

The original Python list, the native metadata reader and the compiled CLI produce the same 44 automatic names. The CLI also lists identically from a directory without tests/. Missing data, empty rows, duplicate names, invalid flags, changed ordered rows and raw newline drift each return 2 with empty reader stdout. Original K2b assertions completed under bound40 on the candidate; final index-gate status is recorded by commitgate, independently of these observations.

Reproduce the derived contract with `sh tests/seedmetadatacheck.sh`; run the existing `k2b` gate in an isolated export containing the same product pair. The original constructors, their preparation and oracle routes are retained. Python baseline remains 1749 real calls, 32 data calls, 92 importers and 409 files. These checks do not complete the full F4 release queue.
