#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"

static int option_a, option_s;

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

// printf cua xv6 nay van rut gon %lu ve 32 bit; doi uint64 sang chuoi.
static void
print_size(uint64 size, char *path)
{
  char digits[21];
  int i = sizeof(digits) - 1;

  digits[i] = 0;
  do {
    digits[--i] = '0' + size % 10;
    size /= 10;
  } while(size != 0);
  printf("%s\t%s\n", digits + i, path);
}

static int
du(char *path, uint64 *size)
{
  int fd, n, status = 0;
  struct stat st;
  struct dirent de;
  char buf[MAXPATH], name[DIRSIZ + 1];
  uint64 total = 0, child_size;

  if((fd = open(path, O_RDONLY)) < 0){
    fprintf(2, "du: cannot open %s\n", path);
    return 1;
  }
  if(fstat(fd, &st) < 0){
    fprintf(2, "du: cannot stat %s\n", path);
    close(fd);
    return 1;
  }
  if(st.type != T_DIR){
    *size = st.type == T_FILE ? st.size : 0;
    close(fd);
    if(option_a && !option_s)
      print_size(*size, path);
    return 0;
  }

  while((n = read(fd, &de, sizeof(de))) == sizeof(de)){
    if(de.inum == 0)
      continue;
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;
    if(fmtpath(path, name, buf) == 0){
      fprintf(2, "du: path too long: %s/%s\n", path, name);
      status = 1;
      continue;
    }
    if(du(buf, &child_size) != 0)
      status = 1;
    else
      total += child_size;
  }
  if(n != 0){
    fprintf(2, "du: cannot read directory %s\n", path);
    status = 1;
  }
  close(fd);
  *size = total;
  if(!option_s && status == 0)
    print_size(total, path);
  return status;
}

int
main(int argc, char *argv[])
{
  char *path = ".";
  int pathset = 0;
  uint64 total;

  for(int i = 1; i < argc; i++){
    if(strcmp(argv[i], "-a") == 0){
      option_a = 1;
    } else if(strcmp(argv[i], "-s") == 0){
      option_s = 1;
    } else if(argv[i][0] == '-' || pathset){
      fprintf(2, "du: usage: du [path] [-a] [-s]\n");
      exit(1);
    } else {
      path = argv[i];
      pathset = 1;
    }
  }
  if(strlen(path) >= MAXPATH){
    fprintf(2, "du: path too long\n");
    exit(1);
  }
  if(du(path, &total) != 0)
    exit(1);
  if(option_s)
    print_size(total, path);
  exit(0);
}
