/* An external name resembling a private printf name is still an ordinary function. */
int printf__u1(const char *s, ...){return 7;}
int alias_call(void){return printf__u1("must not print %d",99);}
