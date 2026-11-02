#include "kernel/types.h"
#include "user/user.h"

// Each child continues as the next filter, without growing the call stack.
static int
sieve(int input)
{
  int prime, number, n, p[2], pid, status, childstatus;

  for(;;){
    n = read(input, &prime, sizeof(prime));
    if(n != sizeof(prime)){
      close(input);
      if(n == 0)
        return 0;
      fprintf(2, "primes: read failed\n");
      return 1;
    }
    printf("prime %d\n", prime);

    // A new filter is needed only after a number survives this filter.
    while((n = read(input, &number, sizeof(number))) == sizeof(number)){
      if(number % prime != 0)
        break;
    }
    if(n != sizeof(number)){
      close(input);
      if(n == 0)
        return 0;
      fprintf(2, "primes: read failed\n");
      return 1;
    }

    if(pipe(p) < 0){
      fprintf(2, "primes: pipe failed\n");
      close(input);
      return 1;
    }
    pid = fork();
    if(pid < 0){
      fprintf(2, "primes: fork failed\n");
      close(input);
      close(p[0]);
      close(p[1]);
      return 1;
    }
    if(pid == 0){
      close(input);
      close(p[1]);
      input = p[0];
      continue;
    }

    close(p[0]);
    status = 0;
    do {
      if(number % prime != 0 &&
         write(p[1], &number, sizeof(number)) != sizeof(number)){
        fprintf(2, "primes: write failed\n");
        status = 1;
        break;
      }
    } while((n = read(input, &number, sizeof(number))) == sizeof(number));
    if(n != 0 && n != sizeof(number)){
      fprintf(2, "primes: read failed\n");
      status = 1;
    }
    close(input);
    close(p[1]); // Let the next filter observe EOF before waiting for it.
    if(wait(&childstatus) < 0 || childstatus != 0)
      status = 1;
    return status;
  }
}

int
main(int argc, char *argv[])
{
  int p[2], pid, number, status = 0, childstatus;

  if(argc != 1){
    fprintf(2, "primes: usage: primes\n");
    exit(1);
  }
  if(pipe(p) < 0){
    fprintf(2, "primes: pipe failed\n");
    exit(1);
  }
  pid = fork();
  if(pid < 0){
    fprintf(2, "primes: fork failed\n");
    close(p[0]);
    close(p[1]);
    exit(1);
  }
  if(pid == 0){
    close(p[1]);
    exit(sieve(p[0]));
  }

  close(p[0]);
  for(number = 2; number <= 280; number++){
    if(write(p[1], &number, sizeof(number)) != sizeof(number)){
      fprintf(2, "primes: write failed\n");
      status = 1;
      break;
    }
  }
  close(p[1]);
  if(wait(&childstatus) < 0 || childstatus != 0)
    status = 1;
  exit(status);
}
