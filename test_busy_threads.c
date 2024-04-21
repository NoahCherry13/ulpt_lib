#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

/* How many threads (aside from main) to create */
#define THREAD_CNT 10


/* Each counter goes up to a multiple of this value. If your test is too fast
 * use a bigger number. Too slow? Use a smaller number. See the comment about
 * sleeping in count() to avoid this size-tuning issue.
 */
#define COUNTER_FACTOR 1000000

// locations for  return values
int some_value[THREAD_CNT];
char test_lock = 1;
char test_barrier = 0;
/* Waste some time by counting to a big number.
 *
 * Alternatively, introduce your own sleep function to waste a specific amount
 * of time. But make sure it plays nice with your scheduler's interrupts (HINT:
 * see the man page in section 2 for nanosleep, and its possible ERROR codes).
 */
pthread_mutex_t mutex;
pthread_barrier_t barrier;
void *
count(void *arg)
{
  if (test_barrier && (long int)arg < 4) pthread_barrier_wait(&barrier);
  if (test_lock) pthread_mutex_lock(&mutex);
  int my_num = (long int)arg;
  int c = (my_num + 1) * COUNTER_FACTOR;
  int i;
  for (i = 0; i < c; i++) {
    if ((i % 20000) == 0) {
      printf("id: 0x%lx num %d counted to %d of %d\n", pthread_self(), my_num, i, c);
    }
  }
  some_value[my_num]=my_num;
  if (test_lock) pthread_mutex_unlock(&mutex);
  pthread_exit(&some_value[my_num]);
  return NULL;
}

/*
 * Expected behavior: THREAD_CNT number of threads print increasing numbers
 * in a round-robin fashion. The first thread finishes soonest, and the last
 * thread finishes latest. All threads are expected to reach their maximum
 * count.
 *
 * Tests faile when value returned by join is wrong. 
 *
 * Consider adding disable() and enable() to write more extensive tests. 
 * See man pages for sigprocmask, sigemptyset, and sigaddset.
 */
int main(int argc, char **argv) {
  pthread_t threads[THREAD_CNT];
  pthread_mutex_init(&mutex, NULL);
  pthread_barrier_init(&barrier, NULL, 4);
  unsigned long int i;
  printf("making threads\n");
  for(i = 0; i < THREAD_CNT; i++) {
    printf("creating thread %ld\n", i);
    pthread_create(&threads[i], NULL, count, (void *)i);
  }
  printf("Created all the threads\n");
  /* Collect statuses of the other threads, waiting for them to finish */
  for(i = 0; i < THREAD_CNT; i++) {
    void *pret;
    int ret;
    pthread_join(threads[i], &pret);
    ret = *(int *)pret;
    assert(ret == i);
  }
  pthread_mutex_destroy(&mutex);
  printf("done\n");
  return 0;
}
