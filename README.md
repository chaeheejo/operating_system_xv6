# operating_system_xv6


## summary
1. [OS structures](https://blog.naver.com/60cogml/222907247735)
2. [Processes](https://blog.naver.com/60cogml/222907325011)
3. [CPU Scehduling](https://blog.naver.com/60cogml/222908075665)
4. [Main Memory](https://blog.naver.com/60cogml/222908604801)
5. [Virtual Memory 1/2](https://blog.naver.com/60cogml/222942484600)
6. [Virtual Memory 2/2](https://blog.naver.com/60cogml/222943342029)

<br/>

## 1st
> xv6 운영체제에 쉘 프로그램 추가

<br/>

파일의 n행까지 터미널에 출력
```shell 
hcat [n] [file]
```  
운영체제가 처음 부팅될 때 로그인 작업 수행하도록 설정
```shell 
ssu_login
```  

<br/>

## 2nd  
> xv6 운영체제에 시스템 콜 추가 

<br/>

호출한 프로세스의 메모리 사용량을 출력하는 'memsize' 시스템 콜  
```shell 
memsizetest
```  
호출한 프로세스 이후 생성되는 자식 프로세스를 실행할 때 설정한 시스템 콜을 추적   
자식 프로세스는 command로 입력한 명령어를 실행   
mask로 설정한 시스템 콜만 추적해 프로세스 id, 시스템 콜 이름, 리턴값을 출력  
```shell 
ssu_trace [mask] [command]
```  

<br/>

## 3rd  
> priority 기반 스케줄링 구현

<br/>

각 프로세스는 weight 값과 priority 값을 가짐  
프로세스의 priority 값은 new_priority = old_priority + (time_slice / weight) 규칙으로 갱신됨  
스케줄러는 RUNNABLE 프로세스 중 가장 낮은 priority 값을 가지고 있는 프로세스를 실행함

<br/>

5개의 자식 프로세스를 생성하고, 생성순으로 weight 값을 크게 할당함  
priority 규칙에 따라 가장 늦게 생성한 프로세스가 가장 먼저 실행됨

```shell 
sdebug
```   

debug=1로 make 실행 시 스케줄러가 선택한 프로세스의 pid, 이름, weight, priority 출력

```shell 
make debug=1 qemu-nox
```  
