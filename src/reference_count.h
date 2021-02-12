#pragma once

#include <stdbool.h>

#include "concurrency.h"

struct ld_rc_t
{
    void *       value;
    unsigned int count;
    void (*destructor)(void *value);
    ld_mutex_t lock;
};

bool
LDi_rc_initialize(
    struct ld_rc_t *const rc,
    void *const           value,
    void (*const destructor)(void *value));

void
LDi_rc_increment(struct ld_rc_t *const rc);

void
LDi_rc_decrement(struct ld_rc_t *const rc);

void
LDi_rc_destroy(struct ld_rc_t *const rc);
