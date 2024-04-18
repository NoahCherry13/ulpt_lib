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

pthread_mutex_t mutex;
int main()
{
  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_destroy(&mutex);
  
  return 0;
}
