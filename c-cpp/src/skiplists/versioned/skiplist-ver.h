#ifndef SKIPLIST_VERSIONED
#define SKIPLIST_VERSIONED

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

// #include <atomic_ops.h>
#include <stdatomic.h>

#include "tm.h"

#define DEFAULT_DURATION                10000
#define DEFAULT_INITIAL                 256
#define DEFAULT_NB_THREADS              1
#define DEFAULT_RANGE                   0x7FFFFFFF
#define DEFAULT_SEED                    0
#define DEFAULT_UPDATE                  20
#define DEFAULT_ELASTICITY		4
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE 		1

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

/*
#define ATOMIC_CAS_MB(a, e, v)          (atomic_compare_exchange_strong((volatile AO_t *)(a), (AO_t)(e), (AO_t)(v)))
#define ATOMIC_FETCH_AND_INC_FULL(a)    (atomic_fetch_add((volatile atomic_uint *)(a), 1))
*/
// static volatile AO_t stop;

// #define TRANSACTIONAL                   d->unit_tx

extern unsigned int global_seed;

typedef uint32_t height_t;
typedef int val_t;
typedef uint32_t vlock_t;

#define VAL_MIN                         INT_MIN
#define VAL_MAX                         INT_MAX
#define MAX_H                           32
#define TOP                             (MAX_H - 1)


typedef struct sl_node {
  val_t val;
  volatile height_t target_height;
  volatile height_t current_height;
  _Atomic(vlock_t) hlock;
  struct sl_node *next[MAX_H];
  _Atomic(vlock_t) vlock[MAX_H];
} sl_node_t;

typedef struct sl_intset {
	sl_node_t *head;
} sl_intset_t;

inline void *check_loc(void *ptr);

int get_rand_level();
int floor_log_2(unsigned int n);

/* 
 * Create a new node without setting its next fields. 
 */
sl_node_t *sl_new_simple_node(val_t val, int target_height);

/* 
 * Create a new node with its next field. 
 * If next=NULL, then this create a tail node. 
 */
sl_node_t *sl_new_node(val_t val, sl_node_t *next, int target_height);
void sl_delete_node(sl_node_t *n);
sl_intset_t *sl_set_new();
void sl_set_delete(sl_intset_t *set);
int sl_set_size(sl_intset_t *set);

// NOTE: unsafe resetting of locks
void reset_locks(sl_node_t *node);


inline void *check_loc(void *ptr)
{
  if (!ptr) {
    perror("*alloc failed");
    exit(1);
  }
  return ptr;
}

/* 
 * Returns a pseudo-random value in [1;range).
 * Depending on the symbolic constant RAND_MAX>=32767 defined in stdlib.h,
 * the granularity of rand() could be lower-bounded by the 32767^th which might 
 * be too high for given values of range and initial.
 */
inline long rand_range(long r) {
  int m = RAND_MAX;
  long d, v = 0;
  
  do {
    d = (m > r ? r : m);    
    v += 1 + (long)(d * ((double)rand()/((double)(m)+1.0)));
    r -= m;
  } while (r > 0);
  return v;
}

#endif
