#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

static void
check(int condition, char *message)
{
  if(!condition){
    printf("sysinfotest: FAIL: %s\n", message);
    exit(1);
  }
}

static struct sysinfo
snapshot(void)
{
  struct sysinfo info;
  check(sysinfo(&info) == 0, "sysinfo call");
  return info;
}

static void
test_memory(void)
{
  struct sysinfo before = snapshot();

  char *mem = sbrk(2 * PGSIZE);
  check(mem != (char *)-1, "allocate memory");

  // Touch both pages before measuring.
  mem[0] = 1;
  mem[PGSIZE] = 1;

  struct sysinfo during = snapshot();
  check(during.freemem < before.freemem,
        "free memory must decrease");

  check(sbrk(-2 * PGSIZE) != (char *)-1,
        "release memory");

  struct sysinfo after = snapshot();
  check(after.freemem == before.freemem,
        "free memory must recover");

  printf("PASS: memory allocation and release\n");
}

static void
test_process(void)
{
  int ready[2], finish[2];

  check(pipe(ready) == 0, "ready pipe");
  check(pipe(finish) == 0, "finish pipe");

  struct sysinfo before = snapshot();
  int pid = fork();
  check(pid >= 0, "fork");

  if(pid == 0){
    char c = 'x';
    close(ready[0]);
    close(finish[1]);

    if(write(ready[1], &c, 1) != 1)
      exit(1);
    if(read(finish[0], &c, 1) != 1)
      exit(1);

    close(ready[1]);
    close(finish[0]);
    exit(0);
  }

  close(ready[1]);
  close(finish[0]);

  char c;
  check(read(ready[0], &c, 1) == 1, "child ready");

  // The child is still alive, waiting on the finish pipe.
  struct sysinfo during = snapshot();
  check(during.nproc == before.nproc + 1,
        "process count must increase");

  c = 'x';
  check(write(finish[1], &c, 1) == 1, "release child");

  int status;
  check(wait(&status) == pid, "wait for child");
  check(status == 0, "child exit status");

  struct sysinfo after = snapshot();
  check(after.nproc == before.nproc,
        "process count must recover");

  close(ready[0]);
  close(finish[1]);

  printf("PASS: process creation and cleanup\n");
}

static void
test_bad_addresses(void)
{
  check(sysinfo((struct sysinfo *)-1) == -1,
        "reject invalid address");

  check(sysinfo((struct sysinfo *)MAXVA) == -1,
        "reject address outside user range");

  char *end = sbrk(0);
  check(sysinfo((struct sysinfo *)end) == -1,
        "reject buffer at end of user memory");

  printf("PASS: invalid addresses\n");
}

int
main(void)
{
  printf("sysinfotest: start\n");

  struct sysinfo info = snapshot();
  check(info.freemem > 0, "free memory must be positive");
  check(info.nproc > 0, "process count must be positive");
  printf("PASS: basic information\n");

  test_memory();
  test_process();
  test_bad_addresses();

  printf("sysinfotest: OK\n");
  exit(0);
}
