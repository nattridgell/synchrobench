/*
 * File:
 *   skiplist-lock.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Skip list implementation of an integer set
 *
 * Copyright (c) 2009-2010.
 *
 * skiplist-lock.c is part of Synchrobench
 * 
 * Synchrobench is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include "skiplist-ver.h"


int get_rand_level() {
  int i, level = 1;
  for (i = 0; i < MAX_H - 1; i++) {
    if ((rand_range(100)-1) < 50)
      level++;
    else
      break;
  }
  /* 1 <= level <= MAX_H */
  return level;
}

int floor_log_2(unsigned int n) {
  int pos = 0;
  if (n >= 1<<16) { n >>= 16; pos += 16; }
  if (n >= 1<< 8) { n >>=  8; pos +=  8; }
  if (n >= 1<< 4) { n >>=  4; pos +=  4; }
  if (n >= 1<< 2) { n >>=  2; pos +=  2; }
  if (n >= 1<< 1) {           pos +=  1; }
  return ((n == 0) ? (-1) : pos);
}

/* 
 * Create a new node without setting its next fields. 
 */
sl_node_t *sl_new_simple_node(val_t val, int target_height)
{
  sl_node_t *node;
  node = (sl_node_t *)check_loc(malloc(sizeof(sl_node_t)));
  node->val = val;
  node->target_height = target_height;
  node->current_height = 0;
  node->next = (sl_node_t **)check_loc(malloc(MAX_H*sizeof(sl_node_t *)));
  node->vlock = (_Atomic(vlock_t) *)check_loc(malloc(MAX_H*sizeof(vlock_t *)));
  // Initialise atomics
  for (int i = 0; i < MAX_H; ++i)
    node->vlock[i] = ATOMIC_VAR_INIT(0);
  node->hlock = ATOMIC_VAR_INIT(0);
  return node;
}

/* 
 * Create a new node with its next field. 
 * If next=NULL, then this create a tail node. 
 */
sl_node_t *sl_new_node(val_t val, sl_node_t *next, int target_height)
{
  sl_node_t *node = sl_new_simple_node(val, target_height);
  
  for (int i = 0; i < MAX_H; i++)
    node->next[i] = next;
  
  return node;
}

void sl_delete_node(sl_node_t *node)
{
  if (!node)
    return;
  free(node->next);
  free(node->vlock);
  free(node);
}

sl_intset_t *sl_set_new()
{
  sl_intset_t *set;
  sl_node_t *head, *tail;
  
  set = (sl_intset_t *)check_loc(malloc(sizeof(sl_intset_t)));
  tail = sl_new_node(VAL_MAX, NULL, MAX_H);
  head = sl_new_node(VAL_MIN, tail, MAX_H);

  head->current_height = MAX_H;
  tail->current_height = MAX_H;

  set->head = head;
  return set;
}

void sl_set_delete(sl_intset_t *set)
{
  sl_node_t *node, *next;
  
  node = set->head;
  while (node != NULL) {
    next = node->next[0];
    sl_delete_node(node);
    node = next;
  }
  free(set);
}

int sl_set_size(sl_intset_t *set)
{
  int size = 0;
  sl_node_t *node;
  
  /* We have at least 2 elements */
  node = set->head->next[0];
  while (node->next[0] != NULL) {
    if (node->target_height > 0)
      size++;
    node = node->next[0];
  }
  
  return size;
}
