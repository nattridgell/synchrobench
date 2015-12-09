#include "versioned.h"

/** Functions for version locking on a single node **/

inline int exists(sl_node_t *node) {
  return (node->target_height == 0 ? 0 : 1);
}

/**
 * Level locking functions
 * Need to lock a level of a node to:
 *   - to change the next pointer for a given level
 *
 **/
inline int get_level_version(sl_node_t *node, height_t height) {
  vlock_t val = atomic_load(&node->vlock[height]);
  return val & ~((vlock_t)1);
}

inline int try_lock_level_version(sl_node_t *node, vlock_t ver, height_t height) {
  return atomic_compare_exchange_strong(&node->vlock[height], &ver, ver + 1);
}

void lock_level(sl_node_t *node, height_t height) {
  vlock_t ver;
  do {
    ver = get_level_version(node, height);
  } while (!try_lock_level_version(node, ver, height));
}

void unlock_level(sl_node_t *node, height_t height) {
  atomic_fetch_add(&node->vlock[height], 1);
}

/**
 * Height locking functions
 * Need to lock the height of a node to:
 *   - update a level (i.e. next pointer for a level)
 *     - always lock the level, then the height
 *   - change the target height of a node
 *     - to logically delete, or logically re-insert the same node
 *     - target height 0 (logically deleted)
 *
 * Note: the target height of a node is also used for logical deletion.
 **/
inline int get_height_version(sl_node_t *node) {
  vlock_t val = atomic_load(&node->hlock);
  return val & ~((vlock_t)1);  
}

inline int try_lock_height_version(sl_node_t *node, vlock_t ver) {
  return atomic_compare_exchange_strong(&node->hlock, &ver, ver + 1);
}

// TODO: Use spinlock here?
static inline void lock_height(sl_node_t *node) {
  vlock_t ver;
  do {
    ver = get_height_version(node);
  } while (!try_lock_height_version(node, ver));
}

void unlock_height(sl_node_t *node) {
  atomic_fetch_add(&node->hlock, 1);
}


/**
 * Versioned Skip List Methods.
 * 
 */

int validate(sl_node_t *prev, sl_node_t *curr, int level, enum STATE *status) {
  int level_version = get_level_version(prev, level);
  if (prev->current_height <= level || prev->next[level] != curr) {
    *status = ABORT;
    return 0;
  }
  *status = OK;
  return level_version;
}

// Changes the state to ABORT, TERMINATE, or OK
void try_lock_level_and_height(sl_node_t *prev, sl_node_t *curr, sl_node_t *node_update_height, int level, int *height_ver, int *level_version, enum STATE *status) {
  if (*height_ver != get_height_version(node_update_height)) {
    *status = TERMINATE;
    return;
  }

  *level_version = validate(prev, curr, level, status);
  if (*status == ABORT)
    return;

  if (!try_lock_level_version(prev, *level_version, level)) {
    *status = ABORT;
    return;
  }

  if (!try_lock_height_version(node_update_height, *height_ver)) {
    unlock_level(prev, level);
    *status = TERMINATE;
  } else {
    ++(*height_ver);
  }
}

void traverse(val_t val, int from, int to, sl_node_t **prevs, sl_node_t **currs, sl_node_t *head) {
  sl_node_t *prev, *curr;
  prevs[TOP] = head;
  prev = prevs[from];

  while (prev->current_height <= from) {
    prev = prevs[++from];
  }
    

  for (int level = from; level >= to; --level) {
    curr = prev->next[level];
    while (curr->val < val) {
      prev = curr;
      curr = curr->next[level];
    }
    prevs[level] = prev;
    currs[level] = curr;
  }
}

void try_insert_at_level(sl_node_t *prev, sl_node_t *curr, sl_node_t *new_node, int level, int *height_ver, enum STATE *status) {
  int level_version = 0;
  // printf("Try insert val %d at lvl %d\n", new_node->val, level);
  try_lock_level_and_height(prev, curr, new_node, level, height_ver, &level_version, status);
  if (*status == ABORT || *status == TERMINATE)
    return;

  new_node->next[level] = curr;
  new_node->current_height++;
  prev->next[level] = new_node;

  // printf("New node current_height %d after insert at lvl %d\n",new_node->current_height, level );

  unlock_height(new_node);
  ++(*height_ver);
  unlock_level(prev, level);
  *status = OK;
}

void try_remove_at_level(sl_node_t *prev, sl_node_t *curr, int level, int *height_ver, enum STATE *status) {
  int level_version = 0;

  try_lock_level_and_height(prev, curr, curr, level, height_ver, &level_version, status);
  if (*status == ABORT || *status == TERMINATE)
    return;

  // Need to edit from here!!!
  lock_level(curr, level);
  curr->current_height--;
  prev->next[level] = curr->next[level];

  unlock_height(curr);
  ++(*height_ver);
  unlock_level(curr, level);
  unlock_level(prev, level);
  *status = OK;
}

int set_insert(sl_intset_t *set, val_t val) {
  sl_node_t *prevs[MAX_H];
  sl_node_t *currs[MAX_H];
  sl_node_t *prev, *curr, *new_node, *node_to_insert;
  new_node = NULL;
  node_to_insert = NULL;
  height_t height_ver;
  int target_height = get_rand_level();
  enum STATE status;
  int traverse_from = TOP;

  do {
    status = OK;
    traverse(val, traverse_from, 0, prevs, currs, set->head);
    prev = prevs[0];
    curr = currs[0];

    if (curr->val == val) {
      // printf("Found value %d\n", val);
      // Linearise insert
      do {
        height_ver = get_height_version(curr);
        // If it already logically exists, return 0.
        if (exists(curr)) {
          sl_delete_node(new_node);
          return 0; // Exists
        } else if (curr->current_height == 0) {
          // Logically and physically removed
          status = ABORT;
          break;
        } else if (try_lock_height_version(curr, height_ver)) {
          curr->target_height = target_height;
          unlock_height(curr);
          height_ver++;
          node_to_insert = curr;
          break;
        }
      } while (1);
    } else {
      if (!new_node)
        new_node = sl_new_simple_node(val, target_height);
      // printf("New node %d, with target_height %d\n", new_node->val, new_node->target_height);
      node_to_insert = new_node;
      height_ver = get_height_version(new_node);
      try_insert_at_level(prev, curr, node_to_insert, 0, &height_ver, &status);
    }

    traverse_from = 0;
  } while (status != OK);  

  for (int level = node_to_insert->current_height; level < node_to_insert->target_height; ) {
    prev = prevs[level];
    curr = currs[level];
    try_insert_at_level(prev, curr, node_to_insert, level, &height_ver, &status);
    if (status == ABORT)
      traverse(val, level, level, prevs, currs, set->head);
    else if (status == TERMINATE) {
      // printf("Status Terminate\n");
      return 1;
    } else {
      // printf("Successful insert at %d\n", level);
      level++;
    }
  }

  return 1;
}

int set_delete(sl_intset_t *set, val_t val) {
  sl_node_t *prevs[MAX_H];
  sl_node_t *currs[MAX_H];
  sl_node_t *prev, *curr;
  height_t height_ver;
  enum STATE status = OK;

  traverse(val, TOP, 0, prevs, currs, set->head);

  prev = prevs[0];
  curr = currs[0];

  if (curr->val != val)
    return 0;

  // Linearise remove
  do {
    height_ver = get_height_version(curr);
    // If it does not logically exist, return 0.
    if (!exists(curr))
      return 0;
    else if (try_lock_height_version(curr, height_ver)) {
      curr->target_height = 0;
      unlock_height(curr);
      height_ver += 2;
      break;
    }
  } while (1);

  int level = curr->current_height - 1;

  while (level >= curr->target_height && level >= 0) {
    prev = prevs[level];
    try_remove_at_level(prev, curr, level, &height_ver, &status);
    if (status == ABORT)
      traverse(val, level, level, prevs, currs, set->head);
    else if (status == TERMINATE)
      break;
    else
      --level;
  }

  return 1;
}

/**
 * Wait-free contains.
 */
int set_contains(sl_intset_t *set, val_t val) {
  sl_node_t *prev, *curr;
  prev = set->head;

  for (int level = TOP; level >= 0; level--) {
    curr = prev->next[level];
    while (curr->val < val) {
      prev = curr;
      curr = curr->next[level];
    }
    if (curr->val == val)
      return exists(curr);
  }
  return 0;
}
