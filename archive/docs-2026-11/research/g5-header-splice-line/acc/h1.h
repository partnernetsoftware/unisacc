#define M(a) \
  ((a) + \
   1)
static int h1_line = __LINE__;
static const char *h1_file = __FILE__;
#include "sub/n.h"
