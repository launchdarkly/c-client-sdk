#include "assertion.h"
#include "reference_count.h"

bool
LDi_rc_initialize(struct ld_rc_t *const rc,
    void *const value, void (*const destructor)(void *value))
{
    LD_ASSERT(rc);
    LD_ASSERT(value);
    LD_ASSERT(destructor);

    return false;
}

void *
LDi_rc_aquire(struct ld_rc_t *const rc)
{
    LD_ASSERT(rc);

    return NULL;
}

void
LDi_rc_release(struct ld_rc_t *const rc)
{
    LD_ASSERT(rc);
}

void
LDi_rc_destroy(struct ld_rc_t *const rc)
{
    if (rc) {
        
    }
}
