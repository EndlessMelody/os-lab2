#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

static char output[20000], errors[2048], expected[20000];
static char data[9000], other[9000], longarg[160];
static char *workdir;
static int cases;
static int spare_fds = NOFILE;

static void
fail(char *message)
{
  fprintf(2, "FAIL: %s\n", message);
  exit(1);
}

static void
pass(char *label)
{
  cases++;
  printf("PASS: %s\n", label);
}

static void
file(char *path, char *bytes, int length)
{
  int fd = open(path, O_CREATE | O_WRONLY | O_TRUNC);
  if(fd < 0)
    fail("create fixture");
  for(int done = 0; done < length;){
    int n = write(fd, bytes + done, length - done);
    if(n <= 0)
      fail("write fixture");
    done += n;
  }
  close(fd);
}

static void
load(char *path, char *buffer, int capacity)
{
  int fd = open(path, O_RDONLY), length = 0, n;
  if(fd < 0)
    fail("open command output");
  while((n = read(fd, buffer + length, capacity - length - 1)) > 0){
    length += n;
    if(length == capacity - 1)
      fail("command output exceeds test buffer");
  }
  close(fd);
  if(n < 0)
    fail("read command output");
  buffer[length] = 0;
}

static void
run(char *args[], int wanted)
{
  int out[2], length = 0, n;
  int err = open("/test.err", O_CREATE | O_WRONLY | O_TRUNC);
  int pid, status = -1;
  if(err < 0 || pipe(out) < 0)
    fail("open command capture");
  pid = fork();
  if(pid < 0)
    fail("fork command");
  if(pid == 0){
    close(1);
    if(dup(out[1]) != 1)
      exit(99);
    close(2);
    if(dup(err) != 2)
      exit(99);
    close(out[0]);
    close(out[1]);
    close(err);
    for(int i = 3; i < NOFILE - spare_fds; i++)
      if(dup(0) < 0)
        exit(99);
    if(workdir && chdir(workdir) < 0)
      exit(99);
    exec(args[0], args);
    exit(99);
  }
  close(out[1]);
  close(err);
  // Drain stdout while the child runs, including output larger than a pipe.
  while((n = read(out[0], output + length, sizeof(output) - length - 1)) > 0){
    length += n;
    if(length == sizeof(output) - 1)
      fail("command output exceeds test buffer");
  }
  close(out[0]);
  if(n < 0)
    fail("read command output pipe");
  output[length] = 0;
  if(wait(&status) != pid || status != wanted){
    fprintf(2, "FAIL: %s exit status: expected %d, got %d\n",
            args[0], wanted, status);
    exit(1);
  }
  load("/test.err", errors, sizeof(errors));
}

static void
check(char *label, char *args[], int status, char *text)
{
  run(args, status);
  if(text && strcmp(output, text) != 0){
    fprintf(2, "FAIL: %s output\nExpected:\n%sActual:\n%s", label, text, output);
    exit(1);
  }
  if(errors[0])
    fail(label);
  pass(label);
}

static void
check_error(char *label, char *args[], int status)
{
  run(args, status);
  if(errors[0] == 0)
    fail("error must be reported on stderr");
  pass(label);
}

static void
equal_files(char *a, char *b)
{
  char ba[512], bb[512];
  int fa = open(a, O_RDONLY), fb = open(b, O_RDONLY), na, nb;
  if(fa < 0 || fb < 0)
    fail("open files for byte comparison");
  do {
    na = read(fa, ba, sizeof(ba));
    nb = read(fb, bb, sizeof(bb));
    if(na < 0 || nb < 0 || na != nb || memcmp(ba, bb, na) != 0)
      fail("copied file bytes or length differ");
  } while(na > 0);
  close(fa);
  close(fb);
}

static void
test_cp(void)
{
  mkdir("ct");
  mkdir("ct/dir");
  file("ct/empty", "", 0);
  file("ct/small", "abc\n", 4);
  for(int i = 0; i < 8193; i++)
    data[i] = i % 251;
  file("ct/big", data, 8193);
  char *copy[] = {"cp", "ct/empty", "ct/dst", 0};
  check("cp empty file", copy, 0, "");
  equal_files(copy[1], copy[2]);
  copy[1] = "ct/small";
  check("cp small file", copy, 0, "");
  equal_files(copy[1], copy[2]);
  copy[1] = "ct/big";
  check("cp binary file larger than buffer (8193 bytes)", copy, 0, "");
  equal_files(copy[1], copy[2]);
  copy[1] = "ct/small";
  check("cp overwrite longer destination", copy, 0, "");
  equal_files(copy[1], copy[2]);
  copy[1] = "ct/empty";
  check("cp overwrite with empty file", copy, 0, "");
  equal_files(copy[1], copy[2]);
  copy[1] = "ct/small";
  copy[2] = "ct/small";
  check_error("cp reject same path", copy, 1);
  load("ct/small", data, sizeof(data));
  if(strcmp(data, "abc\n"))
    fail("same-file copy destroyed source");
  if(link("ct/small", "ct/alias") < 0)
    fail("hard-link fixture");
  copy[2] = "ct/alias";
  check_error("cp reject hard-link alias", copy, 1);
  equal_files("ct/small", "ct/alias");
  load("ct/small", data, sizeof(data));
  if(strcmp(data, "abc\n"))
    fail("alias copy destroyed source");
  copy[1] = "ct/missing";
  copy[2] = "ct/dst";
  check_error("cp missing source", copy, 1);
  copy[1] = "ct/dir";
  check_error("cp reject source directory", copy, 1);
  copy[1] = "ct/small";
  copy[2] = "ct/dir";
  check_error("cp reject destination directory", copy, 1);
  copy[2] = "ct/missing/dst";
  check_error("cp destination parent missing", copy, 1);
  char *missing[] = {"cp", 0};
  char *extra[] = {"cp", "ct/small", "ct/dst", "extra", 0};
  check_error("cp missing arguments", missing, 1);
  check_error("cp extra arguments", extra, 1);
}

static void
test_diff(void)
{
  mkdir("ft");
  file("ft/a", "", 0);
  file("ft/b", "", 0);
  char *compare[] = {"diff", "ft/a", "ft/b", 0};
  check("diff empty files", compare, 0, "");
  file("ft/a", "one\ntwo\nthree\n", 14);
  file("ft/b", "one\ntwo\nthree\n", 14);
  check("diff equal multiline files", compare, 0, "");
  file("ft/b", "one\nTWO\nthree\n", 14);
  check("diff changed middle line", compare, 1, "< ft/a:2: two\n> ft/b:2: TWO\n");
  file("ft/b", "one\ntwo\n", 8);
  check("diff second file ends first", compare, 1, "< ft/a:3: three\n");
  file("ft/a", "one\n", 4);
  check("diff first file ends first", compare, 1, "> ft/b:2: two\n");
  file("ft/a", "", 0);
  file("ft/b", "x\ny\n", 4);
  check("diff empty against multiple lines", compare, 1, "> ft/b:1: x\n> ft/b:2: y\n");
  char *quietfirst[] = {"diff", "-q", "ft/a", "ft/b", 0};
  char *quietlast[] = {"diff", "ft/a", "ft/b", "-q", 0};
  check("diff -q before filenames, one message", quietfirst, 1, "diff: files differ\n");
  check("diff -q after filenames, one message", quietlast, 1, "diff: files differ\n");
  file("ft/a", "x\ny\n", 4);
  check("diff -q equal files is silent", quietlast, 0, "");
  file("ft/a", "last", 4);
  file("ft/b", "last", 4);
  check("diff equal final lines without newline", compare, 0, "");
  file("ft/b", "last\n", 5);
  check("diff detect missing final newline", compare, 1,
        "< ft/a:1: last\n\\ No newline at end of file\n> ft/b:1: last\n");
  file("ft/a", "\n\n", 2);
  file("ft/b", "\n", 1);
  check("diff trailing empty line", compare, 1, "< ft/a:2: \n");
  memset(data, 'x', 4097);
  data[4097] = '\n';
  data[4098] = 0;
  memmove(other, data, 4099);
  other[4096] = 'y';
  file("ft/a", data, 4098);
  file("ft/b", other, 4098);
  strcpy(expected, "< ft/a:1: ");
  strcpy(expected + strlen(expected), data);
  strcpy(expected + strlen(expected), "> ft/b:1: ");
  strcpy(expected + strlen(expected), other);
  check("diff full long lines with change after byte 4096", compare, 1, expected);
  file("ft/b", data, 4098);
  check("diff equal long lines", compare, 0, "");
  char binary1[] = {'a', 0, 'b', '\n'};
  char binary2[] = {'a', 0, 'c', '\n'};
  file("ft/a", binary1, sizeof(binary1));
  file("ft/b", binary1, sizeof(binary1));
  check("diff equal lines containing NUL", compare, 0, "");
  file("ft/b", binary2, sizeof(binary2));
  check("diff -q detects change beyond NUL", quietlast, 1, "diff: files differ\n");
  compare[1] = "ft/missing";
  check_error("diff missing first file", compare, 2);
  compare[1] = "ft/a";
  compare[2] = "ft/missing";
  check_error("diff missing second file", compare, 2);
  compare[2] = "ft";
  check_error("diff reject directory", compare, 2);
  char *missing[] = {"diff", 0};
  char *badoption[] = {"diff", "-x", "ft/a", "ft/b", 0};
  char *duplicate[] = {"diff", "-q", "ft/a", "ft/b", "-q", 0};
  char *extra[] = {"diff", "ft/a", "ft/b", "extra", 0};
  check_error("diff missing arguments", missing, 2);
  check_error("diff unknown option", badoption, 2);
  check_error("diff duplicate -q", duplicate, 2);
  check_error("diff extra filename", extra, 2);
}

static void
directories(void)
{
  mkdir("dt");
  mkdir("dt/empty");
  file("dt/a", "abc", 3);
  mkdir("dt/sub");
  file("dt/sub/b", "hello\n", 6);
  mkdir("dt/sub/deep");
  file("dt/sub/deep/c", "12345", 5);
  file("dt/abcdefghijklmn", "z", 1);
  mkdir("dt/12345678901234");
  file("dt/12345678901234/abcdefghijklmn", "zz", 2);

  // These entries are accessible by relative paths, but exceed MAXPATH
  // when reached by walking from lp. The commands must report the limit.
  mkdir("lp");
  strcpy(data, "lp");
  for(int i = 0; i < 8; i++){
    strcpy(data + strlen(data), "/12345678901234");
    if(mkdir(data) < 0)
      fail("long-path directory fixture");
  }
  if(chdir(data) < 0)
    fail("chdir long-path fixture");
  file("abcdefghijklmn", "x", 1);
  chdir("/");
  mkdir("dp");
  strcpy(data, "dp");
  for(int i = 0; i < 20; i++){
    strcpy(data + strlen(data), "/d");
    if(mkdir(data) < 0)
      fail("deep directory fixture");
  }
  memset(longarg, 'x', 128);
  longarg[128] = 0;
}

static void
test_tree(void)
{
  directories();
  char *missing[] = {"tree", "missing", 0};
  check_error("tree missing path gives nonzero status", missing, 1);
  char *walk[] = {"tree", "dt", 0};
  char *full = "dt\n|-- empty\n|-- a\n|-- sub\n|   |-- b\n|   |-- deep\n|   |   |-- c\n|-- abcdefghijklmn\n|-- 12345678901234\n|   |-- abcdefghijklmn\n";
  check("tree full hierarchy and 14-character names", walk, 0, full);
  char *depth[] = {"tree", "dt", "-L", "1", 0};
  check("tree -L 1", depth, 0,
        "dt\n|-- empty\n|-- a\n|-- sub\n|-- abcdefghijklmn\n|-- 12345678901234\n");
  depth[3] = "2";
  check("tree -L 2", depth, 0,
        "dt\n|-- empty\n|-- a\n|-- sub\n|   |-- b\n|   |-- deep\n|-- abcdefghijklmn\n|-- 12345678901234\n|   |-- abcdefghijklmn\n");
  depth[3] = "0";
  check("tree -L 0", depth, 0, "dt\n");
  char *dirs[] = {"tree", "-d", "dt", 0};
  check("tree -d", dirs, 0, "dt\n|-- empty\n|-- sub\n|   |-- deep\n|-- 12345678901234\n");
  char *combined[] = {"tree", "-d", "-L", "1", "dt", 0};
  check("tree -d -L 1 with options before path", combined, 0,
        "dt\n|-- empty\n|-- sub\n|-- 12345678901234\n");
  walk[1] = "dt/empty";
  check("tree empty directory", walk, 0, "dt/empty\n");
  walk[1] = "dt/a";
  check("tree file root", walk, 0, "dt/a\n");
  walk[1] = "dt/";
  strcpy(expected, "dt/\n");
  strcpy(expected + 4, full + 3);
  check("tree trailing slash", walk, 0, expected);
  char *defaultpath[] = {"/tree", 0};
  workdir = "/dt";
  strcpy(expected, ".\n");
  strcpy(expected + 2, full + 3);
  check("tree default path is current directory", defaultpath, 0, expected);
  workdir = 0;
  char *baddepth[] = {"tree", "dt", "-L", "abc", 0};
  check_error("tree nonnumeric depth", baddepth, 1);
  baddepth[3] = "-1";
  check_error("tree negative depth", baddepth, 1);
  baddepth[3] = "2147483648";
  check_error("tree overflowing depth", baddepth, 1);
  char *nodepth[] = {"tree", "dt", "-L", 0};
  char *unknown[] = {"tree", "-x", 0};
  char *extra[] = {"tree", "dt", "dt/sub", 0};
  check_error("tree missing depth argument", nodepth, 1);
  check_error("tree unknown option", unknown, 1);
  check_error("tree multiple paths", extra, 1);
  walk[1] = longarg;
  check_error("tree oversized root path", walk, 1);
  walk[1] = "lp";
  check_error("tree joined path exceeds MAXPATH", walk, 1);
  walk[1] = "dp";
  check_error("tree deep recursion fails cleanly at FD limit", walk, 1);
  depth[1] = "dp";
  depth[3] = "2";
  check("tree depth limit avoids deep traversal", depth, 0, "dp\n|-- d\n|   |-- d\n");
}

static void
test_du(void)
{
  directories();
  char *missing[] = {"du", "missing", 0};
  check_error("du missing path gives nonzero status", missing, 1);
  char *walk[] = {"du", "dt", 0};
  check("du directory totals exclude directory metadata", walk, 0,
        "0\tdt/empty\n5\tdt/sub/deep\n11\tdt/sub\n2\tdt/12345678901234\n17\tdt\n");
  char *all[] = {"du", "dt", "-a", 0};
  check("du -a includes every file with exact byte sizes", all, 0,
        "0\tdt/empty\n3\tdt/a\n6\tdt/sub/b\n5\tdt/sub/deep/c\n5\tdt/sub/deep\n11\tdt/sub\n1\tdt/abcdefghijklmn\n2\tdt/12345678901234/abcdefghijklmn\n2\tdt/12345678901234\n17\tdt\n");
  char *summary[] = {"du", "-s", "dt", 0};
  check("du -s only root total", summary, 0, "17\tdt\n");
  char *combined[] = {"du", "-a", "dt", "-s", 0};
  check("du -a -s gives summary priority", combined, 0, "17\tdt\n");
  walk[1] = "dt/empty";
  check("du empty directory is zero", walk, 0, "0\tdt/empty\n");
  walk[1] = "dt/a";
  check("du file root without -a is silent", walk, 0, "");
  all[1] = "dt/a";
  check("du -a file root", all, 0, "3\tdt/a\n");
  summary[2] = "dt/a";
  check("du -s file root", summary, 0, "3\tdt/a\n");
  summary[2] = "dt/";
  check("du trailing slash", summary, 0, "17\tdt/\n");
  char *defaultpath[] = {"/du", 0};
  workdir = "/dt";
  check("du default path is current directory", defaultpath, 0,
        "0\t./empty\n5\t./sub/deep\n11\t./sub\n2\t./12345678901234\n17\t.\n");
  workdir = 0;
  char *unknown[] = {"du", "-x", 0};
  char *extra[] = {"du", "dt", "dt/sub", 0};
  check_error("du unknown option", unknown, 1);
  check_error("du multiple paths", extra, 1);
  walk[1] = longarg;
  check_error("du oversized root path", walk, 1);
  walk[1] = "lp";
  check_error("du joined path exceeds MAXPATH", walk, 1);
  summary[2] = "lp";
  check_error("du -s reports traversal error", summary, 1);
  if(output[0])
    fail("du -s must not print a partial total on error");
  walk[1] = "dp";
  check_error("du deep recursion fails cleanly at FD limit", walk, 1);
}

static void
test_resources(void)
{
  mkdir("rt");
  file("rt/a", "abc", 3);
  file("rt/b", "original", 8);
  char *copy[] = {"cp", "rt/a", "rt/b", 0};
  char *compare[] = {"diff", "rt/a", "rt/b", 0};
  char *tree[] = {"tree", "rt", 0};
  char *du[] = {"du", "rt", 0};
  char *pingpong[] = {"pingpong", 0};
  char *primes[] = {"primes", 0};
  spare_fds = 0;
  check_error("cp no descriptor available for source", copy, 1);
  check_error("diff no descriptor available for first file", compare, 2);
  check_error("tree no descriptor available for root", tree, 1);
  check_error("du no descriptor available for root", du, 1);
  check_error("pingpong first pipe fails without hanging", pingpong, 1);
  check_error("primes first pipe fails without hanging", primes, 1);
  spare_fds = 1;
  check_error("cp no descriptor available for destination", copy, 1);
  load("rt/b", data, sizeof(data));
  if(strcmp(data, "original"))
    fail("failed cp truncated destination");
  check_error("diff second open fails cleanly", compare, 2);
  check_error("tree child stat fails cleanly", tree, 1);
  check_error("du child open fails cleanly", du, 1);
  spare_fds = 3;
  check_error("pingpong second pipe fails without hanging", pingpong, 1);
  spare_fds = 2;
  check_error("primes filter pipe failure reaches generator", primes, 1);
}

int
main(int argc, char *argv[])
{
  if(argc != 2)
    fail("usage: lab01check cp|diff|tree|du|resources");
  if(strcmp(argv[1], "cp") == 0)
    test_cp();
  else if(strcmp(argv[1], "diff") == 0)
    test_diff();
  else if(strcmp(argv[1], "tree") == 0)
    test_tree();
  else if(strcmp(argv[1], "du") == 0)
    test_du();
  else if(strcmp(argv[1], "resources") == 0)
    test_resources();
  else
    fail("unknown test group");
  for(char *p = argv[1]; *p; p++)
    if(*p >= 'a' && *p <= 'z')
      *p += 'A' - 'a';
  printf("LAB01_%s_OK: %d cases\n", argv[1], cases);
  exit(0);
}
