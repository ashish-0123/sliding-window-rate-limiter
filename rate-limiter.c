/***********************************************************************
 * FILENAME: rate-limiter.c
 *
 * DESCRIPTION:
 *   Sample sliding window based implementation of a rate limiter.
 *
 * NOTES:
 *   1. This implementation assumes an array of tenant specific queues
 *      for performance reasons as it is assumed that tenants are added
 *      / removed less frequently.
 *
 *      Alternate efficient dynamic data structures could be:
 *      a) Single queue with nodes containing tenant_id
 *      b) AVL / RB Trees based on tenant_id containing a single queue
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define SUCCESS 0
#define FAILURE 1

#define MAX_TENANTS 100   /* Active tenants       */
#define WINDOW_SIZE 10000 /* Miliseconds (10s)    */
#define MAX_REQ 10        /* 10ms service rate    */

#define TEST_NUM_TENANTS 3
#define TEST_MAX_REQUESTS 200
#define TEST_REQ_DELAY 200000

/* Dequeue implementation (Ideally should be separate files) */

/* Linked list node */
typedef struct node
{
  long data; /* timestamp */
  struct node* next;
} qnode_t;

/* Dequeue */
typedef struct
{
  qnode_t* head;
  qnode_t* tail;
  unsigned int size;
} queue_t;

qnode_t*
allocate_node(long data)
{
  qnode_t* temp = (qnode_t*)malloc(sizeof(qnode_t));
  if (NULL == temp)
    return NULL;

  temp->data = data;
  temp->next = NULL;
  return temp;
}

unsigned int
initialize_queue(queue_t** q, long data)
{
  *q = (queue_t*)malloc(sizeof(queue_t));
  if (NULL == *q)
    return FAILURE;

  (*q)->head = (*q)->tail = allocate_node(data);
  if (NULL == (*q)->head)
    return FAILURE;

  (*q)->size = 1;
  return SUCCESS;
}

void
destroy_queue(queue_t* q)
{
  qnode_t *temp = NULL, *next = NULL;
  if (NULL == q)
    return;

  temp = q->head;
  next = NULL;

  while (NULL != temp) {
    next = temp->next;
    free(temp);
    temp = next;
  }
  q->head = q->tail = NULL;
  free(q);
}

unsigned int
enqueue(queue_t** q, long data)
{
  qnode_t* temp = NULL;
  if (NULL == *q)
    return initialize_queue(q, data);
  else if (NULL == (temp = allocate_node(data)))
    return FAILURE;

  if ((*q)->size) {
    (*q)->tail->next = temp;
  } else {
    (*q)->head = temp;
  }

  (*q)->tail = temp;
  (*q)->size++;

  return SUCCESS;
}

long
dequeue(queue_t** q)
{
  long data;
  qnode_t* p = NULL;

  /* uninitialized queue */
  if (NULL == *q || NULL == (*q)->head)
    return -1;

  p = (*q)->head;
  (*q)->head = (*q)->head->next;

  /* if last node */
  if (NULL == (*q)->head)
    (*q)->tail = NULL;

  (*q)->size--;

  data = p->data;
  free(p);

  return data;
}

/* Rate limiter functionality and helper functions */

long
get_current_time_ms()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000L) + (tv.tv_usec / 1000);
}

int
check_tenant_allowed(queue_t** q, long timestamp)
{
  while (*q && (*q)->head && (timestamp - (*q)->head->data >= WINDOW_SIZE)) {
    printf("removed expired node (timestamp = %lu)\n", dequeue(q));
  }
  if (!(*q) || (*q)->size < MAX_REQ) {
    enqueue(q, get_current_time_ms());
    return SUCCESS;
  } else {
    return FAILURE;
  }
}

int
main(void)
{
  /* Sample usage.
   * - 200 requests distributed over 3 tenants randomly
   * - 0.2 seconds between requests (5 req/sec)
   * - max rps (per config) 1 req / sec / tenant
   */

  queue_t *tenant_queues[MAX_TENANTS] = { NULL }, *q = NULL;
  long curr_time_ms = 0;
  int tenant_id = 0;

  srand(time(NULL));

  for (int i = 0; i <= TEST_MAX_REQUESTS; i++) {
    curr_time_ms = get_current_time_ms();
#ifdef RANDOM
    tenant_id = rand() % TEST_NUM_TENANTS;
#else
    tenant_id = ++tenant_id % TEST_NUM_TENANTS;
#endif
    if (SUCCESS ==
        check_tenant_allowed(&tenant_queues[tenant_id], curr_time_ms)) {
      printf("Tenant %d - Request allowed: %d\n", tenant_id, i);
    } else {
      printf("Tenant %d - Request denied: %d\n", tenant_id, i);
    }
#if DEBUG
    q = tenant_queues[tenant_id];
    printf("\t(curr_time: %lu, q-size: %d, q-head: %lu, q-tail: %lu)\n",
           curr_time_ms,
           q->size,
           q->head->data,
           q->tail->data);
#endif
    usleep(TEST_REQ_DELAY);
  }

  for (int i = 0; i < MAX_TENANTS; i++) {
    destroy_queue(tenant_queues[i]);
  }

  return 0;
}
