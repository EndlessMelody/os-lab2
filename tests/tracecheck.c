#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/syscall.h"
#include "user/user.h"

static void
fail(char *message)
{
  trace(0);
  fprintf(2, "TRACECHECK_FAIL: %s\n", message);
  exit(1);
}

static void
reap(int pid)
{
  int status;
  if(wait(&status) != pid || status != 0)
    fail("child status");
}

static void
masks(void)
{
  int pid = getpid();
  printf("MASK_PID %d\n", pid);
  if(trace(1U << SYS_getpid) != 0 || getpid() != pid || getpid() != pid)
    fail("getpid mask");
  close(-1);
  if(trace(1U << SYS_close) != 0 || close(-1) != -1 || getpid() != pid)
    fail("replacement mask or negative return");
  if(trace(0) != 0 || getpid() != pid || close(-1) != -1)
    fail("disable mask");
  // A high, unused bit must not select another syscall.
  trace((int)(1U << 31));
  getpid();
  trace(0);
  // The kernel API stores the complete int bit pattern, including bit 31.
  trace(-1);
  getpid();
  trace(0);
  printf("TRACE_MASKS_OK\n");
}

static void
inherit(void)
{
  int p[2];
  int parent = getpid();
  if(pipe(p) < 0)
    fail("pipe");
  int sibling = fork();
  if(sibling < 0)
    fail("sibling fork");
  if(sibling == 0){
    char byte;
    close(p[1]);
    if(read(p[0], &byte, 1) != 1)
      fail("sibling synchronization");
    getpid();
    exit(0);
  }
  close(p[0]);
  printf("INHERIT_PARENT %d SIBLING %d\n", parent, sibling);
  trace((1U << SYS_fork) | (1U << SYS_getpid));
  int child = fork();
  if(child < 0)
    fail("child fork");
  if(child == 0){
    close(p[1]);
    getpid();
    int grandchild = fork();
    if(grandchild < 0)
      fail("grandchild fork");
    if(grandchild == 0){
      getpid();
      exit(0);
    }
    reap(grandchild);
    exit(0);
  }
  // Wake an existing untraced sibling while this process is tracing.
  if(write(p[1], "x", 1) != 1)
    fail("wake sibling");
  close(p[1]);
  for(int i = 0; i < 2; i++){
    int status;
    int pid = wait(&status);
    if((pid != child && pid != sibling) || status != 0)
      fail("inherit wait");
  }
  trace(0);
  getpid();
  // Reuse freed process slots without inheriting their former masks.
  for(int i = 0; i < NPROC + 4; i++){
    int pid = fork();
    if(pid < 0)
      fail("reuse fork");
    if(pid == 0){
      getpid();
      exit(0);
    }
    reap(pid);
  }
  printf("TRACE_INHERIT_OK\n");
}

static void
run(char **args, int expected)
{
  int pid = fork();
  if(pid < 0)
    fail("CLI fork");
  if(pid == 0){
    exec(args[0], args);
    fail("CLI exec");
  }
  int status;
  if(wait(&status) != pid || status != expected)
    fail("CLI exit status");
}

static void
cli(void)
{
  run((char *[]){"trace", 0}, 1);
  run((char *[]){"trace", "1", 0}, 1);
  char *bad[] = {"", "-1", "+1", "abc", "1x", "2147483648",
                 "999999999999999999999999999999", 0};
  for(int i = 0; bad[i]; i++)
    run((char *[]){"trace", bad[i], "echo", "UNEXPECTED_EXEC", 0}, 1);
  run((char *[]){"trace", "0", "no-such-command", 0}, 1);
  run((char *[]){"trace", "0", "echo", "TRACE_ARGV", "first", "last", 0}, 0);
  run((char *[]){"trace", "0000", "echo", "TRACE_ZERO_OK", 0}, 0);
  // Exercise forwarding with more arguments than the interactive shell allows.
  char *many[MAXARG + 1];
  many[0] = "trace";
  many[1] = "0";
  many[2] = "echo";
  for(int i = 3; i < MAXARG - 1; i++)
    many[i] = "a";
  many[MAXARG - 1] = 0;
  run(many, 0);
  printf("TRACE_CLI_OK\n");
}

int
main(int argc, char **argv)
{
  if(argc != 2)
    fail("usage");
  if(strcmp(argv[1], "masks") == 0)
    masks();
  else if(strcmp(argv[1], "inherit") == 0)
    inherit();
  else if(strcmp(argv[1], "cli") == 0)
    cli();
  else
    fail("unknown test");
  exit(0);
}
