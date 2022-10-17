#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define TIME_SLICE 10000000
#define NULL ((void *)0)

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  //ptable 내의 RUNNABLE 상태의 프로세스의 priority 값 중 가장 작은 값을 저장한다.
  long lowest_priority; 
} ptable;

static struct proc *initproc;

//프로세스의 weight 값을 할당하기 위한 변수이다.
uint weight=1;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

//다음 실행할 프로세스를 선택하는 함수이다.
struct proc *ssu_schedule()
{
  struct proc *p;
  struct proc *ret = NULL;

  //for문을 통해 현재 ptable에 존재하는 프로세스들을 확인한다.
  for(p=ptable.proc ; p<&ptable.proc[NPROC] ; p++){
    //RUNNABLE 상태인 프로세스 중 priority 값이 가장 작은 프로세스를 실행할 것이다.
    //ptable에 존재하는 프로세스들 중 RUNNABLE 상태인 프로세스이면,
    if(p->state==RUNNABLE){
      //return 할 프로세스와 priority 값을 비교한다.
      //return 할 프로세스가 아직 없거나, 현재 프로세스의 priority 값이 return할 프로세스의 priority 값보다 작으면,
      if((ret==NULL) || (ret->priority > p->priority)){
        //return 프로세스를 현재 프로세스로 바꿔준다.
        ret=p;
       }
    }
  }

  //만약 실행 시 DEBUG 값이 설정되어 들어오면, 프로세스의 정보를 출력한다.
  #ifdef DEBUG
  if(ret)
    //ret가 NULL이 아니라면, 프로세스의 id, 이름, weight 값, priority 값을 출력한다.
    cprintf("PID: %d, NAME: %s, WEIGHT: %d, PRIORITY: %d\n", ret->pid, ret->name, ret->weight, ret->priority);
  
  #endif
  
  //ptable을 모두 돌면서 가장 작은 priority 값을 가진 프로세스를 선택한 후 이를 return 해준다.
  return ret;
}

//전달받은 프로세스의 priority를 업데이트해준다.
void update_priority(struct proc *p)
{
  //new_priority = old_priority + (time_slice / weight) 규칙에 따라 priority 값을 업데이트해준다.
  p->priority = p->priority +(TIME_SLICE/p->weight); 
}

//ptable을 순회하면서 가장 낮은 priority 값을 찾아 ptable의 lowest_priority 변수에 저장한다.
void update_lowest_priority()
{
  struct proc *min =NULL;
  struct proc *p;
  
  //for문을 통해 현재 ptable에 존재하는 프로세스들을 확인한다.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    //RUNNABLE 상태인 프로세스들의 priority 값을 확인한다.
    //ptable에 존재하는 프로세스들 중 RUNNABLE 상태인 프로세스이면,
      if(p->state == RUNNABLE){
        //설정할 프로세스의 priority 값과 비교한다.
        //설정할 프로세스가 아직 없거나, 현재 프로세스의 priority 값이 설정할 프로세스의 priority 값보다 작으면,
	      if((min==NULL)||(min->priority > p->priority))
          //설정할 프로세스를 현재 프로세스로 업데이트해준다.
	        min=p;
      }
  }

  //ptable을 순회하면서 찾은 가장 작은 priority 값을 
  if(min!=NULL)
    //ptable의 lowest_priority 변수에 저장한다.
	  ptable.lowest_priority = min->priority;
}

//전달받은 프로세스의 priority 값을 ptable의 lowest_priority 변수에 저장해준다.
void assign_lowest_priority(struct proc *p)
{
  p->priority = ptable.lowest_priority;
}

//전달받은 weight 값으로 현재 접근하고 있는 프로세스의 weight 값을 업데이트해주는 함수이다.
//sysproc.c 파일의 sys_weightset
void do_weightset(int weight)
{
  //현재 접근하고 있는 프로세스의 proc 구조체 변수의 값을 변경하는 것이므로
  //ptable을 lock해준 뒤 값을 변경해야 한다.
  acquire(&ptable.lock);
  //매개변수로 전달받은 weight 값을 현재 접근하고 있는 프로세스의 weight 값으로 할당해준다.
  myproc()->weight = weight;
  release(&ptable.lock);
} 

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  //weight 값은 프로세스의 실행 순으로 값이 할당되기 때문에
  //전역변수로 선언한 weight 값으로 생성한 프로세스의 weight 값을 설정해주고,
  //++연산을 통해 전역변수로 선언한 weight 값을 하나 증가시킨다.
  p->weight = weight++;
  //초기 priority 값은 현재 ptable에 존재하는 프로세스 중 RUNNABLE 상태인 프로세스들의 가장 작은 priority 값이다.
  //따라서 ptable에 저장한 현재 가장 작은 priority 값을 전달받은 프로세스의 priority 값으로 업데이트해주는
  //assign_lowest_priority 함수를 호출해준다.
  assign_lowest_priority(p);


  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  //user process가 맨처음 실행되면 호출되는 함수이다.
  //맨처음 가장 낮은 priority 값은 3으로 설정해주어야 한다.
  //따라서 가장 낮은 priority 값을 관리하고 있는 lowest_priority 변수에 3을 할당해준다.
  ptable.lowest_priority = 3;

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->weight = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p; 
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    //ssu_schedule 함수는 현재 ptable의 RUNNABLE 프로세스 중 가장 낮은 priority 값을 
    //갖고 있는 프로세스를 반환한다.
    //반환받은 프로세스가 우선순위가 가장 높다는 뜻이니 다음 실행시킬 프로세스이다. 
    p = ssu_schedule();
    if(p==NULL){
      release(&ptable.lock);
      continue;
    }

    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;

    swtch(&(c->scheduler), p->context);
    switchkvm();
   
    //프로세스를 실행한 다음 new_priority = old_priority + (time_slice / weight)
    //규칙으로 프로세스의 priority 값을 업데이트 시킨다.
    update_priority(p);  
    //현재 프로세스의 priority 값을 업데이트시켜 값이 변했으므로 현재 RUNNABLE 상태인 프로세스의 priority 값 중
    //가장 작은 값인 lowest_priority 값 또한 업데이트 시킨다.
    update_lowest_priority();
 

      // Process is done running for now.
      // It should have changed its p->state before coming back.

      
    c->proc = 0;
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  //sleep에서 wakeup 한 시 프로세스의 상태가 SLEEPING에서 RENNABLE 상태가 되면
  //priority 값은 lowest_priority 값으로 업데이트해준다.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      assign_lowest_priority(p);
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}