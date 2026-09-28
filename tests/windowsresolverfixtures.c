/* Native owned-module facts, independently compiled with the Windows SDK. */
#include <stdint.h>
#ifndef FIXTURE_ID
#error FIXTURE_ID must be 1 or 2
#endif
#if FIXTURE_ID != 1 && FIXTURE_ID != 2
#error invalid fixture identity
#endif
__declspec(dllexport) int64_t owned_only(int64_t n) { return n+10*FIXTURE_ID; }
__declspec(dllexport) int64_t layered(int64_t n) { return n+11*FIXTURE_ID; }
__declspec(dllexport) int owned_value = 100*FIXTURE_ID+1;
