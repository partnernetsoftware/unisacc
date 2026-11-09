/* Shared fixed packaging slot for the reference and model drivers.
   build_ref.sh embeds this declaration in the standalone unisacc.c.
   This is not a release version: keep the 32-byte spelling identical to
   drive.cx. Packaging injects the sole release/candidate.tsv version after
   compilation, without changing this source or its identity. */
#define UNISACC_VERSION "UNISACC_VERSION_SLOT_0000000000"
