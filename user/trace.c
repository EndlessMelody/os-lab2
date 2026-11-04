#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  uint mask = 0;

  if(argc < 3){
    fprintf(2, "trace: usage: trace mask command [args ...]\n");
    exit(1);
  }

  if(argv[1][0] == '\0'){
    fprintf(2, "trace: mask must be an integer from 0 to 2147483647\n");
    exit(1);
  }
  for(int i = 0; argv[1][i]; i++){
    int digit = argv[1][i] - '0';
    if(digit < 0 || digit > 9 || mask > (2147483647U - digit) / 10){
      fprintf(2, "trace: mask must be an integer from 0 to 2147483647\n");
      exit(1);
    }
    mask = mask * 10 + digit;
  }

  if(trace((int)mask) < 0){
    fprintf(2, "trace: system call failed\n");
    exit(1);
  }

  // Reuse the existing terminated argv so every command argument is preserved.
  exec(argv[2], argv + 2);
  fprintf(2, "trace: exec %s failed\n", argv[2]);
  exit(1);
}
