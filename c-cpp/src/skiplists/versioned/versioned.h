#ifndef VERSIONED_H
#define VERSIONED_H

#include "skiplist-ver.h"

enum STATE {OK, ABORT, TERMINATE};

int set_contains(sl_intset_t *set, val_t val);
int set_insert(sl_intset_t *set, val_t val);
int set_delete(sl_intset_t *set, val_t val);

#endif