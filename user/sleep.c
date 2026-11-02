#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i, ticks;
  uint value = 0;

  if(argc != 2){
    fprintf(2, "sleep: usage: sleep ticks\n");
    exit(1);
  }

  // Check the number before atoi, which does not report invalid input.
  if(argv[1][0] == '\0'){
    fprintf(2, "sleep: ticks must be a non-negative integer\n");
    exit(1);
  }
  for(i = 0; argv[1][i]; i++){
    if(argv[1][i] < '0' || argv[1][i] > '9' ||
       value > (2147483647U - (argv[1][i] - '0')) / 10){
      fprintf(2, "sleep: ticks must be an integer from 0 to 2147483647\n");
      exit(1);
    }
    value = value * 10 + argv[1][i] - '0';
  }

  ticks = atoi(argv[1]);
  if(sleep(ticks) < 0){
    fprintf(2, "sleep: system call failed\n");
    exit(1);
  }
  exit(0);
}
