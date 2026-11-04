#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/ptree.h"
#include "user/user.h"

struct ptreeinfo processes[NPROC];
int printed[NPROC];
int n;

// The inherited user printf narrows %lu to 32 bits. Preserve the ABI's uint64.
static void
printmem(uint64 bytes)
{
  char digits[20];
  int i = sizeof(digits);
  do {
    digits[--i] = '0' + bytes % 10;
    bytes /= 10;
  } while(bytes);
  write(1, digits + i, sizeof(digits) - i);
}

void
printtree(int index, int level)
{
  int i;

  if(index < 0 || index >= n || printed[index])
    return;

  printed[index] = 1;

  for(i = 0; i < level; i++)
    printf("  ");

  printf("%d %s state=%d mem=",
         processes[index].pid,
         processes[index].name,
         processes[index].state);
  printmem(processes[index].memsize);
  printf("\n");

  for(i = 0; i < n; i++){
    if(!printed[i] &&
       processes[i].ppid == processes[index].pid &&
       processes[i].pid != processes[index].pid){
      printtree(i, level + 1);
    }
  }
}

int
main(int argc, char *argv[])
{
  int i, j;
  int found;

  if(argc != 1){
    fprintf(2, "Usage: pstree\n");
    exit(1);
  }

  n = ptree(processes, NPROC);

  if(n < 0){
    fprintf(2, "pstree: ptree syscall failed\n");
    exit(1);
  }

  for(i = 0; i < n; i++){
    found = 0;

    for(j = 0; j < n; j++){
      if(i != j &&
         processes[i].ppid == processes[j].pid){
        found = 1;
        break;
      }
    }

    if(!found)
      printtree(i, 0);
  }

  for(i = 0; i < n; i++){
    if(!printed[i])
      printtree(i, 0);
  }

  exit(0);
}
