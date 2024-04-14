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
  handler.sa_handler = handle_function;
  sigaction(SIGALRM, &handler, NULL); 
  ualarm(QUANTUM, QUANTUM);
  return 0;
}

void  disable_handler()
{
  sigset_tsigset;

  //Initialize set to 0
  sigemptyset(&handler.sa_mask);
  //Add the signal to the set
  sigaddset(&handler.sa_mask, signal);
  //Add the signals in the set to the process' blocked signals
  sigprocmask(SIG_BLOCK, &handler.sa_mask, NULL);
  if (signal == SIGALRM)
    printf("Sig Blocked");
}

void  enable_handler()
{
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = SA_NODEFER;
  handler.sa_handler = handle_function;
  sigaction(SIGALRM, &handler, NULL);
  ualarm(QUANTUM, QUANTUM);
}

int main()
{
  printf("Setting up handler\n");
  set_handler();

  printf("counting\n");
  int cnt = 0;
  for (int i = 0; i < 0xFFFFFFFF; i++){
    cnt++;
  }

  printf("Disabling handler\n");
  disable_handler();

  printf("counting\n");
  int count = 0;
  for (int i = 0; i < 0xFFFFFFFF; i++){
    cnt++;
  }
  
  return 0;
}
