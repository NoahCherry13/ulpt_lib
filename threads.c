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
//static struct sigaction handler;

/* 
   Thread_status identifies the current state of a thread. What states could a thread be in?
   Values below are just examples you can use or not use. 
 */
enum thread_status
{
 TS_EXITED,
 TS_RUNNING,
 TS_READY,
 TS_FREE
};

/* The thread control block stores information about a thread. You will
 * need one of this per thread. What information do you need in it? 
 * Hint, remember what information Linux maintains for each task?
 */
struct thread_control_block {
  pthread_t tid;
  enum thread_status t_stat;
  jmp_buf buf;
  unsigned long int *s_ptr;
  void *retval;
};

static struct thread_control_block queue[128];
static int t_running = 0;
static int num_thread = 0;
static int created_threads = 0;

static void handler_test(){
  printf("timer went off\n");
}

static void schedule(){
  printf("scheduler called\n");
  // set current running thread to ready
  if(!setjmp(queue[t_running].buf)){

    /*
    if(queue[t_running].t_stat == TS_RUNNING){
      printf("%d was running\n", t_running);
      queue[t_running].t_stat = TS_READY;
    }
    */
    
    //int prev_thread_id = t_running;
    for (int i = t_running + 1; i < MAX_THREADS; i++){
     
      if(queue[i].t_stat == TS_READY){
	t_running = i;
	//queue[i].t_stat = TS_RUNNING;
	break;
      }
      
      if(i == MAX_THREADS-1) i = -1;
    }

    printf("switching to: %d\n", t_running);
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
  struct sigaction handler = {{0}};
  
  //setup sighandler to call schedule when timer goes off
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = SA_NODEFER;
  sigaction(SIGALRM, &handler, NULL);
  handler.sa_handler = &handler_test;
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
  printf("exiting thread %d\n", t_running);
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
  printf("joining %ld\n", thread);
  if(retval == NULL) return 0;
  int queue_ind = 0;
  for(int i = 0; i < MAX_THREADS; i ++){
    if(queue[i].tid == thread){
      queue_ind = i;
      break;
    }
  }
  while(queue[queue_ind].t_stat != TS_EXITED);
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
