#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#define QUEUESIZE 10
#define LOOP 1000
#define N_PRODUCERS 512
/* Uncomment the following line if not using the -D N_CONSUMERS=X preprocessing option
#define N_CONSUMERS 16
*/

typedef struct
{
  void * (*work)(void *);
  void * arg;
} workFunction;

void *fun(void *a)
{
  printf("General Kenobi! You are a bold one.\n");
  return (NULL);
}

void *producer (void *args);
void *consumer (void *args);

typedef struct
{
  struct timeval total_times[LOOP * N_PRODUCERS];
  long current_element;
  struct timeval current_times[QUEUESIZE];
  workFunction *buf[QUEUESIZE];
  long head, tail;
  int full, empty, done;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction *in);
void queueDel (queue *q, workFunction **out);

int main ()
{
  queue *fifo;
  pthread_t pro[N_PRODUCERS], con[N_CONSUMERS];

  fifo = queueInit ();
  if (fifo ==  NULL)
  {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  int i;

  for (i = 0; i < N_PRODUCERS; ++i)
  {
    pthread_create (&pro[i], NULL, producer, fifo);
  }
  for (i = 0; i < N_CONSUMERS; ++i)
  {
    pthread_create (&con[i], NULL, consumer, fifo);
  }

  for (i = 0; i < N_PRODUCERS; ++i)
  {
    pthread_join (pro[i], NULL);
  }

  // Release all blocked consumer threads
  pthread_mutex_lock(fifo->mut);
  fifo->done = 1;
  pthread_cond_broadcast(fifo->notEmpty);
  pthread_mutex_unlock(fifo->mut);

  for (i = 0; i < N_CONSUMERS; ++i)
  {
    pthread_join (con[i], NULL);
  }

  queueDelete (fifo);

  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  int i;
  
  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++)
  {
    workFunction *temp_fun;
    temp_fun = (workFunction *)malloc(sizeof(workFunction));
    if (temp_fun == NULL)
    {
      fprintf(stderr, "Memory allocation error.\n");
      exit(1);
    }
    pthread_mutex_lock (fifo->mut);
    while (fifo->full)
    {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    temp_fun->work = &fun;
    queueAdd (fifo, temp_fun);
    pthread_cond_broadcast (fifo->notEmpty);
    pthread_mutex_unlock (fifo->mut);
  }
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  workFunction **temp_fun;
  temp_fun = (workFunction**)malloc(sizeof(workFunction**));
  fifo = (queue *)q;

  while(1)
  {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty)
    {
      if (fifo->done) goto break_nested; // Gods, please have mercy (this usage of goto is in fact encouraged)
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo, temp_fun);
    (*temp_fun)->work(NULL);
    free(*temp_fun);
    pthread_cond_broadcast (fifo->notFull);
    pthread_mutex_unlock (fifo->mut);
    printf ("consumer: played message\n");
  }
  break_nested:
  pthread_cond_broadcast(fifo->notEmpty);
  pthread_mutex_unlock (fifo->mut);
  free(temp_fun);
  return (NULL);
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->current_element = 0;
  q->empty = 1;
  q->full = 0;
  q->done = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);

  FILE *times_file;
  times_file = fopen("times", "a");
  if (times_file == NULL)
  {
    fprintf(stderr, "Error opening file\n");
    exit(1);
  }
  printf("Writing to file...\n");
  fprintf(times_file, "QUEUESIZE %ld\n", (long)QUEUESIZE);
  fprintf(times_file, "LOOP %ld\n", (long)LOOP);
  fprintf(times_file, "N_PRODUCERS %ld\n", (long)N_PRODUCERS);
  fprintf(times_file, "N_CONSUMERS %ld\n", (long)N_CONSUMERS);
  int i;
  for (i = 0; i < N_PRODUCERS * LOOP; ++i)
  {
    fprintf(times_file, "%ld ", q->total_times[i].tv_sec * 1000000 + q->total_times[i].tv_usec);
  }
  fprintf(times_file, "\n-----DONE-----\n");
  fclose(times_file);

  free (q);
}

void queueAdd (queue *q, workFunction *in)
{
  q->buf[q->tail] = in;

  gettimeofday(&(q->current_times[q->tail]), NULL);

  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction **out)
{
  *out = q->buf[q->head];

  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  q->total_times[q->current_element].tv_sec = current_time.tv_sec - q->current_times[q->head].tv_sec;
  q->total_times[q->current_element].tv_usec = current_time.tv_usec - q->current_times[q->head].tv_usec;
  q->current_element++;

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
