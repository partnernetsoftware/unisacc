/* sys/_exit.h -- 0.0.33 D3': exit, the atexit table and the stdio flush hook, shared by
 * <stdlib.h> and <stdio.h>.  <stdio.h> needs exit in the program so that returning from main
 * goes through it and flushes buffered streams, but not the rest of <stdlib.h> (its environ
 * constructor and every other name). */
#ifndef _UNISA_SYS_EXIT_H
#define _UNISA_SYS_EXIT_H
int exit();   /* before the definition, as <stdlib.h> always had it (0.0.33 D3': after it, the redeclaration typed exit int and E3 refused assert's (printf(...), exit(1))) */
static void (*_unisa_atexit[32])(void);
static int _unisa_natexit = 0;
/* <stdio.h> sets this when a stream holds unwritten bytes; exit runs
   it after the atexit handlers, so streams are flushed last (C99 7.20.4.3p4) */
static void (*_unisa_stdio_flush)(void);
static void (*_unisa_stdio_reach)(int);   /* <stdio.h> stores exit here so the program carries it */
#if !__UNISA_FTRIM_LIBC || __UN_exit
static void exit(int __u_code) {
    /* the atexit handlers, last registered first (C99 7.20.4.3p3) */
    while (_unisa_natexit > 0) {
        _unisa_natexit = _unisa_natexit - 1;
        _unisa_atexit[_unisa_natexit]();
    }
    if (_unisa_stdio_flush) _unisa_stdio_flush();
    __exit(__u_code);
}
#endif
#endif
