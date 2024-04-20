#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <assert.h>
#include "ec440threads.h"
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

//-------------------------------------Initial Definitions ----------------------------------------------------------
#define MAX_THREADS 128			/* number of threads you support */
#define THREAD_STACK_SIZE (1<<15)	/* size of stack in bytes */
#define QUANTUM (50 * 1000)		/* quantum in usec */

//static useconds_t quant = (useconds_t)QUANTUM;
static struct sigaction handler;

/* 
   Thread_status identifies the current state of a thread. What states could a thread be in?
   Values below are just examples you can use or not use. 
 */
enum thread_status
{
 TS_EXITED,
 TS_RUNNING,
 TS_READY,
 TS_FREE,
 TS_BLOCKED
};

enum lock_status
{
 MTX_OPEN,
 MTX_LOCKED,
 MTX_KILL
};

enum barrier_status
{
 PB_FREE,
 PB_KILL
};

/* The thread control block stores information about a thread. You will
 * need one of this per thread. What information do you need in it? 
 * Hint, remember what information Linux maintains for each task?
 */

struct my_mutex_t {
  enum lock_status locked;
  pthread_t holding_thread;
  struct thread_control_block *head;
  struct thread_control_block *tail;
};

struct my_barrier_t {
  int count;
  int w_threads;
  struct thread_control_block **w_list;
  enum barrier_status status;
};

struct thread_control_block {
  pthread_t tid;
  enum thread_status t_stat;
  jmp_buf buf;
  unsigned long int *s_ptr;
  void *retval;
  struct thread_control_block *next_blocked;
};

static struct thread_control_block queue[128];
static int t_running = 0;
static int num_thread = 0;
static int created_threads = 0;

/*
static void handler_test(int sig){
  printf("timer went off\n");
}
*/
void **retval;
static void schedule(){

  // set current running thread to ready
  if(!setjmp(queue[t_running].buf)){
    
    for (int i = t_running + 1; i < MAX_THREADS; i++){
     
      if(queue[i].t_stat == TS_READY){
	t_running = i;
	break;
      }if(queue[i].t_stat == TS_EXITED){
	pthread_join(i, retval);
      }

      if(i == MAX_THREADS-1) i = -1;
    }


    longjmp(queue[t_running].buf, 1);

  }
  /* 
     TODO: implement your round-robin scheduler, e.g., 
     - if whatever called us is not exiting 
       - mark preempted thread as runnable
       - save state of preempted thread
     - determin which thread should be running next
     - mark thread you are context switching to as running
     - restore registers of that thread
   */
}

// to supress compiler error saying these static functions may not be used...
static void schedule() __attribute__((unused));


static void scheduler_init()
{
  //set tcb for calling thread
  queue[0].tid = 0;
  queue[0].t_stat = TS_READY;
  queue[0].s_ptr = NULL;
  setjmp(queue[0].buf);

  for(int i = 1; i < MAX_THREADS; i++){
    queue[i].t_stat = TS_FREE;
  }

  num_thread ++;
  // struct sigaction handler = {{0}};
  
  //setup sighandler to call schedule when timer goes off
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = SA_NODEFER;
  handler.sa_handler = schedule;
  sigaction(SIGALRM, &handler, NULL);
  ualarm(QUANTUM, QUANTUM);
  /* 
     TODO: do everything that is needed to initialize your scheduler.
     For example:
     - allocate/initialize global threading data structures
     - create a TCB for the main thread. so your scheduler will be able to schedule it
     - set up your timers to call scheduler...
  */
  
}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
  // Create the timer and handler for the scheduler. Create thread 0.
  static bool is_first_call = true;
  if (is_first_call) {
    is_first_call = false;
    scheduler_init();
  }
  int queue_ind = 1;

  //---------------Initialize TCB----------------------------

  while (queue_ind < MAX_THREADS && queue[queue_ind].t_stat != TS_FREE) {   //loop through queue until free index
    queue_ind++;
  }
 
  if(queue_ind == MAX_THREADS){   //Return error if no free index
    fprintf(stderr, "MAX THREADS IN QUEUE\n");
    return -1;
  }

 
  created_threads++;
  *thread = created_threads;

  unsigned long int *s_ptr  = (unsigned long int *)malloc(THREAD_STACK_SIZE);
  queue[queue_ind].s_ptr = s_ptr;
  s_ptr = s_ptr+(THREAD_STACK_SIZE/sizeof(unsigned long int))-1;
  *s_ptr = (unsigned long int)pthread_exit;
  
  setjmp(queue[queue_ind].buf);
  queue[queue_ind].buf->__jmpbuf[JBL_R12] = (unsigned long int) start_routine;
  queue[queue_ind].buf->__jmpbuf[JBL_R13] = (unsigned long int) arg;
  queue[queue_ind].buf->__jmpbuf[JBL_PC] = _ptr_mangle((unsigned long int)start_thunk);
  queue[queue_ind].buf->__jmpbuf[JBL_RSP] = _ptr_mangle((unsigned long int)s_ptr);

  queue[queue_ind].tid = created_threads;
  queue[queue_ind].t_stat = TS_READY;

  num_thread++;
  schedule();
  
  
  /* TODO: Return 0 on successful thread creation, non-zero for an error.
   *       Be sure to set *thread on success.
   *
   * You need to create and initialize a TCB (thread control block) including:
   * - Allocate a stack for the thread
   * - Set up the registers for the functions, including:
   *   - Assign the stack pointer in the thread's registers to point to its stack. 
   *   - Assign the program counter in the thread's registers.
   *   - figure out how to have pthread_exit invoked if thread returns
   * - After you are done, mark your new thread as READY
   * Hint: Easiest to use setjmp to save a set of registers that you then modify, 
   *       and look at notes on reading/writing registers saved by setjmp using 
   * Hint: Be careful where the stackpointer is set to it may help to draw
   *       an empty stack diagram to answer that question.
   * Hint: Read over the comment in header file on start_thunk before 
   *       setting the PC.
   * Hint: Don't forget that the thread is expected to "return" to pthread_exit after it is done
   * 
   * Don't forget to assign RSP too! Functions know where to
   * return after they finish based on the calling convention (AMD64 in
   * our case). The address to return to after finishing start_routine
   * should be the first thing you push on your stack.
   */
  return 0;
}

void pthread_exit(void *value_ptr)
{
  /* TODO: Exit the current thread instead of exiting the entire process.
   * Hints:
   * - Release all resources for the current thread. CAREFUL though.
   *   If you free() the currently-in-use stack then do something like
   *   call a function or add/remove variables from the stack, bad things
   *   can happen.
   * - Update the thread's status to indicate that it has exited
   * What would you do after this?
   */
  //printf("exiting thread %d\n", t_running);
  queue[t_running].t_stat = TS_EXITED;
  queue[t_running].retval = value_ptr;
  free(queue[t_running].s_ptr);
  num_thread --;
  if(num_thread == 0) exit(1);
  else{
    schedule();
  }
  exit(0);
}

pthread_t pthread_self(void)
{
  /* 
   * TODO: Return the current thread instead of -1, note it is up to you what ptread_t refers to
   */
  return t_running;
}

int pthread_join(pthread_t thread, void **retval)
{
  //

  if(retval == NULL) return 0;
  int queue_ind = 0;
  for(int i = 0; i < MAX_THREADS; i ++){
    if(queue[i].tid == thread){
      queue_ind = i;
      break;
    }
  }
  while(queue[queue_ind].t_stat != TS_EXITED) schedule();
  queue[queue_ind].t_stat = TS_FREE;
  *retval = queue[queue_ind].retval;
	
  return 0;
}

/* 
 * Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */

static void lock()
{
  printf("locking thread\n");
  sigset_t sig;
  sigemptyset(&sig);
  sigaddset(&sig, SIGALRM);
  sigprocmask(SIG_BLOCK, &sig, NULL);

}

static void unlock()
{
  printf("unlocking thread\n");
  sigset_t sig;
  sigemptyset(&sig);
  sigaddset(&sig, SIGALRM);
  sigprocmask(SIG_UNBLOCK, &sig, NULL);

}

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
  struct my_mutex_t *my_mutex = (struct my_mutex_t *) mutex;
  assert(sizeof(struct my_mutex_t) <= sizeof(pthread_mutex_t));
  my_mutex->head = NULL;
  my_mutex->tail = NULL;
  my_mutex->locked = MTX_OPEN;
  my_mutex->holding_thread = -1;
  
  return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  lock();
  struct my_mutex_t *my_mutex = (struct my_mutex_t *) mutex;
  assert(sizeof(struct my_mutex_t) <= sizeof(pthread_mutex_t));
  if (my_mutex->locked == MTX_LOCKED){
    printf("Mutex is still Locked! Failed to Destroy\n");
    return -1;
  }

  my_mutex->locked = MTX_KILL;
  unlock();
  
  return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  lock();
  struct my_mutex_t *my_mutex = (struct my_mutex_t *) mutex;
  assert(sizeof(struct my_mutex_t) <= sizeof(pthread_mutex_t));

  if (my_mutex->locked == MTX_KILL){ 
    printf("Mutex was destroyed. Lock Failed\n");
    return -1;
  }else if (my_mutex->locked == MTX_OPEN){  // if lock is free lock it and assign thread
    my_mutex->locked = MTX_LOCKED;
    my_mutex->holding_thread = pthread_self();
  } else {                           // if lock is claimed add calling thread to waiting list
    if (my_mutex->head == NULL){
      my_mutex->head = &queue[t_running];
      my_mutex->tail = &queue[t_running];
    } else {
      my_mutex->tail->next_blocked = &queue[t_running];
      my_mutex->tail = &queue[t_running];
    }

    queue[t_running].t_stat = TS_BLOCKED;
    unlock();
    schedule();
  }
  
  return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  struct my_mutex_t *my_mutex = (struct my_mutex_t *) mutex;
 

  assert(sizeof(struct my_mutex_t) <= sizeof(pthread_mutex_t));

  if(my_mutex->holding_thread != pthread_self()){
    printf("Can't unlock thread held by other process\n");
    unlock();
    return -1;
  }
  
  if (my_mutex->locked == MTX_LOCKED){
    my_mutex->locked = MTX_OPEN;
    if(my_mutex->head == NULL){
      unlock();
      return 0;
    }
    my_mutex->head = my_mutex->head->next_blocked;
    if ( my_mutex->head == NULL){
      my_mutex->tail = NULL;
    }

    unlock();
    return 0;
  }
  
  return -1;
}

int pthread_barrier_init(pthread_barrier_t *restrict barrier,
			 const pthread_barrierattr_t *restrict attr,
			 unsigned count){
  lock();
  struct my_barrier_t *my_barrier = (struct my_barrier_t *)barrier;
  my_barrier->count = count;
  my_barrier->w_threads = 0;
  my_barrier->w_list = (struct thread_control_block **)malloc(count * sizeof(struct thread_control_block *));
  my_barrier->status = PB_FREE;

  unlock();
  return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
  lock();
  struct my_barrier_t *my_barrier = (struct my_barrier_t *)barrier;
  my_barrier->w_threads = 0;
  my_barrier->w_list = NULL;
  my_barrier->count = -1;
  my_barrier->status = PB_KILL;
  unlock();
  return -1;
}

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
  lock();
  struct my_barrier_t *my_barrier = (struct my_barrier_t *)barrier;

  // max number of blocked threads is reached
  if (my_barrier->w_threads == my_barrier->count-1){
    my_barrier->w_threads = 0;
    for (int i = 0; i < my_barrier->count; i++){
      my_barrier->w_list[i]->t_stat = TS_READY;
    }  
  }
  // max blocked threads not reached, add thread to blocked list
  else{
    my_barrier->w_threads++;
    my_barrier->w_list[my_barrier->w_threads-1] = &queue[t_running];
    unlock();
    schedule();
  }
  return 0;
}
