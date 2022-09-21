# operating_system with XV6

## 1st
> xv6 운영체제에 쉘 프로그램 추가

<br>

파일의 n행까지 터미널에 출력
```shell 
hcat [n] [file]
```  
운영체제가 처음 부팅될 때 로그인 작업 수행하도록 설정
```shell 
ssu_login
```  

<br>

## 2nd  
> xv6 운영체제에 시스템 콜 추가 

<br>

호출한 프로세스의 메모리 사용량을 출력하는 'memsize' 시스템 콜  
```shell 
memsizetest
```  
호출한 프로세스 이후 생성되는 자식 프로세스를 실행할 때 설정한 시스템 콜을 추적   
자식 프로세스는 command로 입력한 명령어를 실행   
mask로 설정한 시스템 콜만 추적해 프로세스 id, 시스템 이름, 리턴값을 출력  
```shell 
ssu_trace [mask] [command]
```  

