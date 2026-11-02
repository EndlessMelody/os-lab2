#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int tochild[2], toparent[2];
  int pid, status = 0, childstatus;
  char byte = 'x';

  if(argc != 1){
    fprintf(2, "pingpong: usage: pingpong\n");
    exit(1);
  }
  if(pipe(tochild) < 0){
    fprintf(2, "pingpong: pipe failed\n");
    exit(1);
  }
  if(pipe(toparent) < 0){
    fprintf(2, "pingpong: pipe failed\n");
    close(tochild[0]);
    close(tochild[1]);
    exit(1);
  }

  pid = fork();
  if(pid < 0){
    fprintf(2, "pingpong: fork failed\n");
    close(tochild[0]);
    close(tochild[1]);
    close(toparent[0]);
    close(toparent[1]);
    exit(1);
  }

  if(pid == 0){
    close(tochild[1]);
    close(toparent[0]);
    if(read(tochild[0], &byte, 1) != 1){
      fprintf(2, "pingpong: child could not read ping\n");
      close(tochild[0]);
      close(toparent[1]);
      exit(1);
    }
    close(tochild[0]);
    printf("%d: received ping\n", getpid());
    if(write(toparent[1], &byte, 1) != 1){
      fprintf(2, "pingpong: child could not write pong\n");
      status = 1;
    }
    close(toparent[1]);
    exit(status);
  }

  close(tochild[0]);
  close(toparent[1]);
  if(write(tochild[1], &byte, 1) != 1){
    fprintf(2, "pingpong: parent could not write ping\n");
    status = 1;
  }
  close(tochild[1]);
  if(read(toparent[0], &byte, 1) != 1){
    fprintf(2, "pingpong: parent could not read pong\n");
    status = 1;
  } else {
    printf("%d: received pong\n", getpid());
  }
  close(toparent[0]);
  if(wait(&childstatus) < 0 || childstatus != 0)
    status = 1;
  exit(status);
}
