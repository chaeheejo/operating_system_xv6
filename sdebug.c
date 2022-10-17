#include "types.h"
#include "stat.h"
#include "user.h"

#define PNUM 5
#define PRINT_CYCLE 100000000
#define TOTAL_COUNTER 500000000

void sdebug_func(void)
{
  printf(1, "start sdebug command\n");  
  int i, pid;

  //PNUM 개수만큼 for문을 돌면서 자식 프로세스 생성 
  for(i=0;i<PNUM;i++){
    //fork를 통해 자식 프로세스 생성
    pid = fork();

    //pid가 0이면 자식 프로세스
    if(pid==0){ 
      //uptime을 실행해 현재 시간 기록
      int start=uptime();
      //weightset 시스템 콜을 통해 프로세스 생성순으로 weight 값을 1부터 설정해줌
      if(weightset(i+1)==-1){
        //weightset에 weight를 0이하로 줄 경우 에러 발생, 이에 대한 에러 처리
         printf(1, "wrong weight value\n");
      }

      //counter 변수가 TOTAL_COUNTER과 같을 때까지 while문 반복
      long counter=0;
      while(1){
          counter++;

          //TOTAL_COUNTER 값과 같으면 break
          if(counter==TOTAL_COUNTER){
            printf(1, "PID: %d terminated\n", getpid());
            break;
          }
          //PRINT_CYCLE 값과 같으면 프로세스 정보 출력
          else if(counter==PRINT_CYCLE){
            //현재 시간을 저장하고, 프로세스가 실행된 시간을 저장한 start 변수와의 차이에 10을 곱해 실행 시간을 구함
	          int end=uptime();
            printf(1, "PID: %d, WEIGHT: %d, TIMES : %dms\n", getpid(), i+1, (end-start)*10);
          }
      }

      //자식 프로세스 종료
      exit();
      
    }
    else if (pid<0){
      break;
    }
  }
  
  //생성한 자식 프로세스의 개수만큼 wait를 실행해 자식 프로세스가 정상 종료를 하도록 해줌
  for(int i=0;i<PNUM;i++){
   wait();
  }  

  printf(1, "end of sdebug command\n");  
  return;
}

int main(void)
{
  sdebug_func();
  exit();
}