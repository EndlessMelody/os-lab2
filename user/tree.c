#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"

// Noi duong dan, ke ca truong hop parent da co dau '/' o cuoi.
static char*
fmtpath(char *parent, char *child, char *buf)
{
  int length = strlen(parent);
  int slash = length > 0 && parent[length - 1] != '/';

  if(length + slash + strlen(child) >= MAXPATH)
    return 0;
  strcpy(buf, parent);
  if(slash)
    buf[length++] = '/';
  strcpy(buf + length, child);
  return buf;
}

static int
tree(char *path, int level, int maxDepth, int onlyDir)
{
  int fd, n, status = 0;
  struct stat st;
  struct dirent de;
  char buf[MAXPATH], name[DIRSIZ + 1];

  if((fd = open(path, O_RDONLY)) < 0){
    fprintf(2, "tree: cannot open %s\n", path);
    return 1;
  }
  if(fstat(fd, &st) < 0){
    fprintf(2, "tree: cannot stat %s\n", path);
    close(fd);
    return 1;
  }
  if(level == 1)
    printf("%s\n", path);
  if(st.type != T_DIR || (maxDepth != -1 && level > maxDepth)){
    close(fd);
    return 0;
  }

  while((n = read(fd, &de, sizeof(de))) == sizeof(de)){
    if(de.inum == 0)
      continue;
    // Ten trong dirent co the du 14 byte va khong co ky tu ket thuc.
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;
    if(fmtpath(path, name, buf) == 0){
      fprintf(2, "tree: path too long: %s/%s\n", path, name);
      status = 1;
      continue;
    }
    if(stat(buf, &st) < 0){
      fprintf(2, "tree: cannot stat %s\n", buf);
      status = 1;
      continue;
    }
    if(onlyDir && st.type != T_DIR)
      continue;
    for(int i = 0; i < level - 1; i++)
      printf("|   ");
    printf("|-- %s\n", name);
    if(st.type == T_DIR && (maxDepth == -1 || level < maxDepth)){
      if(tree(buf, level + 1, maxDepth, onlyDir) != 0)
        status = 1;
    }
  }
  if(n != 0){
    fprintf(2, "tree: cannot read directory %s\n", path);
    status = 1;
  }
  close(fd);
  return status;
}

static int
depth(char *text)
{
  uint value = 0;

  if(text[0] == 0)
    return -1;
  for(int i = 0; text[i]; i++){
    if(text[i] < '0' || text[i] > '9' ||
       value > (2147483647U - (text[i] - '0')) / 10)
      return -1;
    value = value * 10 + text[i] - '0';
  }
  return value;
}

int
main(int argc, char *argv[])
{
  char *path = ".";
  int maxDepth = -1, onlyDir = 0, pathset = 0;

  for(int i = 1; i < argc; i++){
    if(strcmp(argv[i], "-d") == 0){
      onlyDir = 1;
    } else if(strcmp(argv[i], "-L") == 0){
      if(i + 1 == argc || (maxDepth = depth(argv[++i])) < 0){
        fprintf(2, "tree: -L requires a non-negative integer depth\n");
        exit(1);
      }
    } else if(argv[i][0] == '-' || pathset){
      fprintf(2, "tree: usage: tree [path] [-L depth] [-d]\n");
      exit(1);
    } else {
      path = argv[i];
      pathset = 1;
    }
  }
  if(strlen(path) >= MAXPATH){
    fprintf(2, "tree: path too long\n");
    exit(1);
  }
  exit(tree(path, 1, maxDepth, onlyDir));
}
