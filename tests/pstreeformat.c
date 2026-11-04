// Exercise the actual pstree formatter with values beyond physical xv6 RAM.
#define main pstree_program_main
#include "user/pstree.c"
#undef main

int
main(void)
{
  memset(processes, 0, sizeof(processes));
  memset(printed, 0, sizeof(printed));
  n = 2;
  processes[0].pid = 1;
  processes[0].state = 3;
  processes[0].memsize = ~0UL;
  strcpy(processes[0].name, "wide");
  processes[1].pid = 2;
  processes[1].ppid = 1;
  processes[1].state = 4;
  processes[1].memsize = 4294967296UL;
  strcpy(processes[1].name, "child");
  printtree(0, 0);
  printf("LAB02_FORMAT_OK\n");
  exit(0);
}
