#include "reference_count.h"
#include "assertion.h"

LDBoolean
LDi_rc_initialize(
    struct ld_rc_t *const rc,
    void *const           value,
    void (*const destructor)(void *value))
{
    LD_ASSERT(rc);
    LD_ASSERT(value);
    LD_ASSERT(destructor);

    if (!LDi_mutex_init(&rc->lock)) {
        return LDBooleanFalse;
    }

    rc->count      = 1;
    rc->value      = value;
    rc->destructor = destructor;

    return LDBooleanTrue;
}

void
LDi_rc_increment(struct ld_rc_t *const rc)
{
    LD_ASSERT(rc);

    LDi_mutex_lock(&rc->lock);
    rc->count++;
    LDi_mutex_unlock(&rc->lock);
}

void
LDi_rc_decrement(struct ld_rc_t *const rc)
{
    unsigned int count;

    LD_ASSERT(rc);

    LDi_mutex_lock(&rc->lock);
    rc->count--;
    count = rc->count;
    LDi_mutex_unlock(&rc->lock);

    if (count == 0) {
        rc->destructor(rc->value);
    }
}

void
LDi_rc_destroy(struct ld_rc_t *const rc)
{
    if (rc) {
        LDi_mutex_destroy(&rc->lock);
    }
}
