// Exercise the actual du formatter with values the small disk cannot hold.
#define main du_program_main
#include "user/du.c"
#undef main

int
main(void)
{
  print_size(0, "zero");
  print_size((uint64)1 << 32, "wide");
  print_size(~(uint64)0, "maximum");
  exit(0);
}
