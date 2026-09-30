/* R13-0b #31 follow-up: the mirror of fb12-31.  There, a static nothing calls
   must NOT make its body a reference.  Here the static IS called -- through a
   forward prototype, with its definition below main -- so its body IS reached
   and the never-defined function inside it MUST be reported.
   Getting this wrong is not a missed diagnostic: the reference used to accept
   this, and the program it emitted called offset 0 and looped forever. */
static int a(void);
int main(void) { return a(); }
static int a(void) { return nosuch2(); }
