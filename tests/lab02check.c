#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "kernel/ptree.h"
#include "user/user.h"

static struct ptreeinfo records[NPROC];
static int checks;

static void
check(int condition, char *message)
{
  checks++;
  if(!condition){
    fprintf(2, "LAB02_FAIL: %s\n", message);
    exit(1);
  }
}

static struct sysinfo
info(void)
{
  struct sysinfo value;
  check(sysinfo(&value) == 0, "sysinfo valid buffer");
  return value;
}

static struct ptreeinfo *
find(int n, int pid)
{
  for(int i = 0; i < n; i++)
    if(records[i].pid == pid)
      return &records[i];
  return 0;
}

static void
badbuf(void)
{
  // The last mapped heap page extends beyond the logical break.
  char *old = sbrk(1);
  check(old != (char *)-1, "extend logical break by one byte");
  char *end = sbrk(0);
  check(ptree((struct ptreeinfo *)end, 1) == -1,
        "ptree rejects buffer at unaligned logical break");
  check(sysinfo((struct sysinfo *)end) == -1,
        "sysinfo rejects buffer at unaligned logical break");
  check(sbrk(-1) != (char *)-1, "restore logical break");
  printf("LAB02_BADBUF_OK: %d checks\n", checks);
}

static void
sysinfo_edges(void)
{
  char *start = sbrk(0);
  check(sysinfo((struct sysinfo *)-1) == -1, "sysinfo negative pointer");
  check(sysinfo((struct sysinfo *)MAXVA) == -1, "sysinfo MAXVA pointer");
  check(sysinfo((struct sysinfo *)start) == -1, "sysinfo heap end");
  check(sysinfo((struct sysinfo *)(start - 3 * PGSIZE)) == -1,
        "sysinfo guard page");
  check(sysinfo((struct sysinfo *)sysinfo_edges) == -1,
        "sysinfo read-only text page");
  struct sysinfo before = info();
  check(before.freemem > 0 && before.freemem % PGSIZE == 0,
        "sysinfo reports free bytes in whole pages");
  char *mem = sbrk(2 * PGSIZE);
  check(mem == start, "allocate two pages");
  struct sysinfo *cross = (struct sysinfo *)(mem + PGSIZE - 8);
  check(sysinfo(cross) == 0 && cross->nproc == before.nproc,
        "sysinfo copyout across writable page boundary");
  check(cross->freemem <= before.freemem - 2 * PGSIZE,
        "sysinfo allocation decreases free bytes");
  check(sysinfo((struct sysinfo *)(mem + 2 * PGSIZE - 8)) == -1,
        "sysinfo rejects structure crossing heap end");
  check(sbrk(-2 * PGSIZE) != (char *)-1, "release two pages");
  check(info().freemem == before.freemem, "free byte count recovers");
  printf("LAB02_SYSINFO_OK: %d checks\n", checks);
}

static void
ptree_edges(void)
{
  char *end = sbrk(0);
  check(ptree(records, 0) == -1, "ptree zero max");
  check(ptree(records, -1) == -1, "ptree negative max");
  check(ptree(0, NPROC) == -1, "ptree null buffer");
  check(ptree((struct ptreeinfo *)-1, 1) == -1, "ptree negative pointer");
  check(ptree((struct ptreeinfo *)MAXVA, 1) == -1, "ptree MAXVA pointer");
  check(ptree((struct ptreeinfo *)end, 1) == -1, "ptree heap end");
  check(ptree((struct ptreeinfo *)(end - 3 * PGSIZE), 1) == -1,
        "ptree guard page");
  check(ptree((struct ptreeinfo *)ptree_edges, 1) == -1,
        "ptree read-only text page");
  struct {
    struct ptreeinfo one;
    uint64 canary;
  } limited;
  limited.canary = 0x123456789abcdef0UL;
  check(ptree(&limited.one, 1) == 1, "ptree truncates to one entry");
  check(limited.canary == 0x123456789abcdef0UL, "ptree respects output limit");

  memset(records, 0xa5, sizeof(records));
  int n = ptree(records, 2147483647);
  check(n >= 3 && n <= NPROC, "ptree caps a large maximum at NPROC");
  for(int i = 0; i < n; i++){
    struct ptreeinfo *p = &records[i];
    check(p->pid > 0 && p->ppid >= 0, "ptree valid identifiers");
    check(p->state >= 1 && p->state <= 5, "ptree excludes UNUSED");
    for(int j = 0; j < i; j++)
      check(p->pid != records[j].pid, "ptree unique PID");
    int stop = 0;
    while(stop < sizeof(p->name) && p->name[stop])
      stop++;
    check(stop < sizeof(p->name), "ptree terminated name");
    for(int j = stop; j < sizeof(p->name); j++)
      check(p->name[j] == 0, "ptree clears unused name bytes");
    char *padding = (char *)&p->state + sizeof(p->state);
    while(padding < (char *)&p->memsize)
      check(*padding++ == 0, "ptree clears ABI padding");
  }
  struct ptreeinfo *self = find(n, getpid());
  check(self != 0 && self->ppid > 0, "ptree contains calling process");
  check(self->memsize == (uint64)end, "ptree memsize equals user break");
  check(strcmp(self->name, "lab02check") == 0, "ptree reports executable name");

  char *mem = sbrk(2 * PGSIZE);
  check(mem == end, "ptree allocate page boundary buffer");
  check(ptree((struct ptreeinfo *)(mem + PGSIZE - 8), 1) == 1,
        "ptree crosses writable page boundary");
  check(ptree((struct ptreeinfo *)(mem + 2 * PGSIZE - 8), 1) == -1,
        "ptree rejects record crossing heap end");
  check(sbrk(-2 * PGSIZE) != (char *)-1, "ptree release page boundary buffer");
  printf("LAB02_PTREE_OK: %d checks\n", checks);
}

struct relative {
  int pid;
  int ppid;
  uint64 size;
};

static void
relations(void)
{
  int ready[2], finish[2];
  check(pipe(ready) == 0 && pipe(finish) == 0, "family pipes");
  int parent = getpid();
  struct sysinfo before = info();
  int child = fork();
  check(child >= 0, "family child fork");
  if(child == 0){
    close(ready[0]);
    close(finish[1]);
    check(sbrk(PGSIZE) != (char *)-1, "child grows memory");
    int childpid = getpid();
    int grandchild = fork();
    check(grandchild >= 0, "family grandchild fork");
    check(sbrk(PGSIZE) != (char *)-1, "relative grows memory");
    struct relative me = {getpid(), grandchild == 0 ? childpid : parent,
                          (uint64)sbrk(0)};
    check(write(ready[1], &me, sizeof(me)) == sizeof(me), "relative ready");
    char byte;
    check(read(finish[0], &byte, 1) == 1, "relative finish");
    if(grandchild != 0){
      int status;
      check(wait(&status) == grandchild && status == 0, "reap grandchild");
    }
    exit(0);
  }
  close(ready[1]);
  close(finish[0]);
  struct relative relatives[2];
  for(int i = 0; i < 2; i++)
    check(read(ready[0], &relatives[i], sizeof(relatives[i])) == sizeof(relatives[i]),
          "parent reads relative information");
  int n = ptree(records, NPROC);
  check(n > 0, "family snapshot");
  for(int i = 0; i < 2; i++){
    struct ptreeinfo *p = find(n, relatives[i].pid);
    check(p && p->ppid == relatives[i].ppid, "family parent-child relation");
    check(p->memsize == relatives[i].size, "family memory size");
  }
  check(info().nproc == before.nproc + 2, "sysinfo includes both relatives");
  check(write(finish[1], "xx", 2) == 2, "release relatives");
  int status;
  check(wait(&status) == child && status == 0, "reap family child");
  check(info().nproc == before.nproc, "family process count recovers");
  close(ready[0]);
  close(finish[1]);

  before = info();
  child = fork();
  check(child >= 0, "zombie fork");
  if(child == 0)
    exit(0);
  int zombie = 0;
  for(int i = 0; i < 100 && !zombie; i++){
    n = ptree(records, NPROC);
    struct ptreeinfo *p = find(n, child);
    zombie = p && p->state == 5;
    if(!zombie)
      sleep(1);
  }
  check(zombie, "ptree observes zombie before wait");
  check(info().nproc == before.nproc + 1, "sysinfo counts ZOMBIE as non-UNUSED");
  check(wait(&status) == child && status == 0, "reap zombie");
  check(info().nproc == before.nproc, "wait removes zombie from process count");
  printf("LAB02_RELATIONS_OK: %d checks\n", checks);
}

static void
concurrent(void)
{
  for(int worker = 0; worker < 2; worker++){
    int pid = fork();
    check(pid >= 0, "snapshot worker fork");
    if(pid == 0){
      for(int i = 0; i < 128; i++){
        struct sysinfo value;
        int n = ptree(records, NPROC);
        check(n > 0 && n <= NPROC, "concurrent ptree snapshot");
        check(sysinfo(&value) == 0 && value.nproc > 0 && value.nproc <= NPROC,
              "concurrent sysinfo snapshot");
      }
      exit(0);
    }
  }
  for(int i = 0; i < 24; i++){
    int pid = fork();
    check(pid >= 0, "churn fork");
    if(pid == 0)
      exit(0);
  }
  for(int i = 0; i < 26; i++){
    int status;
    check(wait(&status) > 0 && status == 0, "concurrent child completion");
  }
  printf("LAB02_CONCURRENT_OK: %d checks\n", checks);
}

int
main(int argc, char **argv)
{
  check(argc == 2, "lab02check mode");
  if(strcmp(argv[1], "badbuf") == 0)
    badbuf();
  else if(strcmp(argv[1], "sysinfo") == 0)
    sysinfo_edges();
  else if(strcmp(argv[1], "ptree") == 0)
    ptree_edges();
  else if(strcmp(argv[1], "relations") == 0)
    relations();
  else if(strcmp(argv[1], "concurrent") == 0)
    concurrent();
  else if(strcmp(argv[1], "snapshots") == 0){
    struct sysinfo value = info();
    int n = ptree(records, NPROC);
    check(n > 0 && n == value.nproc && value.freemem > 0,
          "combined stable snapshots");
    printf("LAB02_SNAPSHOTS_OK\n");
  } else
    check(0, "unknown mode");
  exit(0);
}
