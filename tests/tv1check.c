#include "kernel/types.h"
#include "user/user.h"

// Run inside xv6: its shell does not display a command's exit status.
static void
check(char *args[], int expected)
{
  int pid, status;

  pid = fork();
  if(pid < 0){
    fprintf(2, "tv1check: fork failed\n");
    exit(1);
  }
  if(pid == 0){
    exec(args[0], args);
    fprintf(2, "tv1check: exec failed\n");
    exit(99);
  }
  if(wait(&status) != pid || status != expected){
    fprintf(2, "tv1check: unexpected exit status from %s\n", args[0]);
    exit(1);
  }
}

int
main(void)
{
  int start, elapsed;
  char *zero[] = {"sleep", "0", 0};
  char *ten[] = {"sleep", "10", 0};
  char *missing[] = {"sleep", 0};
  char *negative[] = {"sleep", "-1", 0};
  char *invalid[] = {"sleep", "abc", 0};
  char *overflow[] = {"sleep", "2147483648", 0};
  char *empty[] = {"sleep", "", 0};
  char *extra[] = {"sleep", "1", "2", 0};
  char *pingpong[] = {"pingpong", 0};
  char *primes[] = {"primes", 0};
  char *badpingpong[] = {"pingpong", "extra", 0};
  char *badprimes[] = {"primes", "extra", 0};

  check(zero, 0);
  start = uptime();
  check(ten, 0);
  elapsed = uptime() - start;
  if(elapsed < 10){
    fprintf(2, "tv1check: sleep 10 returned before 10 ticks\n");
    exit(1);
  }
  printf("sleep 10: %d ticks elapsed\n", elapsed);
  check(missing, 1);
  check(negative, 1);
  check(invalid, 1);
  check(overflow, 1);
  check(empty, 1);
  check(extra, 1);
  check(pingpong, 0);
  check(primes, 0);
  check(badpingpong, 1);
  check(badprimes, 1);
  printf("TV1_STATUS_OK\n");
  exit(0);
}
