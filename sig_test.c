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

#define QUANTUM (50 * 1000)

static struct sigaction handler;

int handle_function()
{
  printf("Handler Triggered!!!\n");
  return 0;
}

int set_handler()
{
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = SA_NODEFER;
  handler.sa_handler = (void*)handle_function;
  sigaction(SIGALRM, &handler, NULL); 
  ualarm(QUANTUM, QUANTUM);
  return 0;
}

void  disable_handler()
{
  sigset_t sig;
  sigemptyset(&sig);
  sigaddset(&sig, SIGALRM);
  sigprocmask(SIG_BLOCK, &sig, NULL);
}

void  enable_handler()
{
  sigset_t sig;
  sigemptyset(&sig);
  sigaddset(&sig, SIGALRM);
  sigprocmask(SIG_UNBLOCK, &sig, NULL);
}

int main()
{
  printf("Setting up handler\n");
  set_handler();

  printf("counting\n");
  int cnt = 0;
  for (int i = 0; i < 0x0FFFFFFF; i++){
    cnt++;
  }

  printf("Disabling handler\n");
  disable_handler();

  printf("counting\n");
  for (int i = 0; i < 0x0FFFFFFF; i++){
    cnt++;
  }

  printf("Re-enabling handler\n");
  enable_handler();

  printf("counting\n");

  for (int i = 0; i < 0x0FFFFFFF; i++){
    cnt++;
  }
  
  return 0;
}
