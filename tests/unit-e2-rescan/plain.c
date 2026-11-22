/* 0.0.95 e2 rescan fixture: a unit with no #include.  The rescan fix costs 2 extra steps here
 * (LDI INC_SUB / CMPI INC_SUB), so the patched pack must stay within [0, 2] steps of the original. */
int rescan_plain;
