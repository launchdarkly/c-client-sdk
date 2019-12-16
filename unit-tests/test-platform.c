#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include "ldapi.h"
#include "ldinternal.h"

static void
testDeviceId()
{
    char *const deviceid = LDi_deviceid();

    if (deviceid) {
        printf("Device ID is: %s", deviceid);
    } else {
        printf("ERROR: failed to get device ID");
    }

    LDFree(deviceid);
}

static void
testConditionWaitTimeout()
{
    int start, end;
    ld_cond_t condition;
    ld_mutex_t lock;

    LDi_condinit(&condition);
    LDi_mtxinit(&lock);

    start = time(NULL);
    LDi_condwait(&condition, &lock, 2000);
    end = time(NULL);
    LD_ASSERT(end - start > 1 && start - end < 3);

    LDi_conddestroy(&condition);
    LDi_mtxdestroy(&lock);
}

int
main(int argc, char **argv)
{
    testDeviceId();
    testConditionWaitTimeout();
}
