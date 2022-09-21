#include "types.h"
#include "stat.h"
#include "user.h"

//argv 배열에서 명령어 부분을 분리함
char** split(char* cmd[], int size, char* argv[])
{
  for(int i=2;i<size;i++){
    cmd[i-2] = argv[i];
  }
  return cmd;
}

int main(int argc, char *argv[])
{
  if(argc<3){ //인자를 3개 이하로 입력했을 경우
    printf(1, "Error : please enter command\n");
    exit();
  }
  for(int i=0;i<strlen(argv[1]);i++){ //mask 값을 숫자로 설정하지 않았을 경우
    if(argv[1][i]< '0' || argv[1][0] > '9'){
      printf(1, "Error : mask is not Integer\n");
      exit();
    }
  }
  if((trace(atoi(argv[2])))<0){
    printf(1, "Error : trace failed ");
    exit();
  }
  
  char *command[argc-2]; //실행할 명령어를 저장할 배열
  split(command, argc, argv);

  trace(atoi(argv[1])); //trace 시스템 콜을 통해 mask 설정

  int pid = fork();
  if(pid>0){ //부모 process는 child process를 기다림
    pid = wait();
    exit();
  }
  else if(pid==0){ //child process는 입력된 명령어를 실행함
    exec(command[0], command);
    printf(1, "ssu_trace: exec failed\n");
    exit();
  }
  else{ //fork가 실패할 경우
    printf(1, "fork error\n");
    exit();
  }
}
