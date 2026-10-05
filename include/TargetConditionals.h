/* TargetConditionals.h -- 0.0.28 H4.  Apple's platform switches, for programs that include it under
 * __APPLE__ (SQLite does): this compiler's macOS targets are macOS, never iOS, watchOS or tvOS. */
#ifndef _UNISA_TARGETCONDITIONALS_H
#define _UNISA_TARGETCONDITIONALS_H
#define TARGET_OS_MAC 1
#define TARGET_OS_OSX 1
#define TARGET_OS_IPHONE 0
#define TARGET_OS_IOS 0
#define TARGET_OS_WATCH 0
#define TARGET_OS_TV 0
#define TARGET_OS_VISION 0
#define TARGET_OS_SIMULATOR 0
#define TARGET_OS_EMBEDDED 0
#define TARGET_IPHONE_SIMULATOR 0
#endif
